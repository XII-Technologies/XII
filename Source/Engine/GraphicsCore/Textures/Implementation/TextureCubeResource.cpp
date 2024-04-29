#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/xiiTexFormat/xiiTexFormat.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeResource, 1, xiiRTTIDefaultAllocator<xiiTextureCubeResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiTextureCubeResource);
// clang-format on

xiiTextureCubeResource::xiiTextureCubeResource() :
  xiiResource(DoUpdate::OnAnyThread, xiiTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
  m_uiLoadedTextures = 0;
  m_uiMemoryGPU[0]   = 0;
  m_uiMemoryGPU[1]   = 0;
  m_Format           = xiiGALTextureFormat::Unknown;
  m_uiWidthAndHeight = 0;
}

xiiResourceLoadDesc xiiTextureCubeResource::UnloadData(Unload WhatToUnload)
{
  if (m_uiLoadedTextures > 0)
  {
    for (xiiInt32 r = 0; r < 2; ++r)
    {
      --m_uiLoadedTextures;

      if (!m_hGALTexture[m_uiLoadedTextures].IsInvalidated())
      {
        xiiGALDevice::GetDefaultDevice()->DestroyTexture(m_hGALTexture[m_uiLoadedTextures]);
        m_hGALTexture[m_uiLoadedTextures].Invalidate();
      }

      m_uiMemoryGPU[m_uiLoadedTextures] = 0;

      if (WhatToUnload == Unload::OneQualityLevel || m_uiLoadedTextures == 0)
        break;
    }
  }

  if (WhatToUnload == Unload::AllQualityLevels)
  {
    if (!m_hSampler.IsInvalidated())
    {
      xiiGALDevice::GetDefaultDevice()->DestroySampler(m_hSampler);
      m_hSampler.Invalidate();
    }
  }

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable    = 2 - m_uiLoadedTextures;
  res.m_State                      = m_uiLoadedTextures == 0 ? xiiResourceState::Unloaded : xiiResourceState::Loaded;

  return res;
}

xiiResourceLoadDesc xiiTextureCubeResource::UpdateContent(xiiStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    xiiResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::LoadedResourceMissing;

    return res;
  }

  xiiImage* pImage = nullptr;
  Stream->ReadBytes(&pImage, sizeof(xiiImage*));

  bool bIsFallback = false;
  *Stream >> bIsFallback;

  xiiTexFormat texFormat;
  texFormat.ReadHeader(*Stream);

  const xiiUInt32 uiNumMipmapsLowRes = xiiTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : 6;

  const xiiUInt32 uiNumMipLevels    = xiiMath::Min(m_uiLoadedTextures == 0 ? uiNumMipmapsLowRes : pImage->GetNumMipLevels(), pImage->GetNumMipLevels());
  const xiiUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  if (pImage->GetWidth(uiHighestMipLevel) != pImage->GetHeight(uiHighestMipLevel))
  {
    xiiLog::Error("Cubemap width '{0}' is not identical to height '{1}'", pImage->GetWidth(uiHighestMipLevel), pImage->GetHeight(uiHighestMipLevel));

    xiiResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::LoadedResourceMissing;

    return res;
  }

  m_Format           = xiiTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), texFormat.m_bSRGB);
  m_uiWidthAndHeight = pImage->GetWidth(uiHighestMipLevel);

  xiiGALTextureCreationDescription texDesc;
  texDesc.m_Format      = m_Format;
  texDesc.m_Size.width  = m_uiWidthAndHeight;
  texDesc.m_Size.height = m_uiWidthAndHeight;
  texDesc.m_uiMipLevels = uiNumMipLevels;
  texDesc.m_BindFlags   = xiiGALBindFlags::ShaderResource;
  texDesc.m_Usage       = xiiGALResourceUsage::Immutable;

  xiiUInt32 uiDepth = pImage->GetDepth(uiHighestMipLevel);
  if (uiDepth > 1)
  {
    texDesc.m_Type               = xiiGALResourceDimension::Texture3D;
    texDesc.m_uiArraySizeOrDepth = uiDepth;
  }
  else
  {
    texDesc.m_uiArraySizeOrDepth = pImage->GetNumArrayIndices();
    texDesc.m_Type               = (texDesc.m_uiArraySizeOrDepth > 1) ? xiiGALResourceDimension::Texture2DArray : xiiGALResourceDimension::Texture2D;

    if (pImage->GetNumFaces() == 6)
    {
      texDesc.m_Type               = xiiGALResourceDimension::TextureCube;
      texDesc.m_uiArraySizeOrDepth = 6;
    }
  }

  XII_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '{0}')", GetResourceID());

  m_uiMemoryGPU[m_uiLoadedTextures] = 0;

  const auto& formatProperties = xiiGALGraphicsUtilities::GetTextureFormatProperties(m_Format);

  xiiHybridArray<xiiGALTextureSubResourceData, 32> InitData;

  for (xiiUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (xiiUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (xiiUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        xiiGALTextureSubResourceData& id = InitData.ExpandAndGetRef();

        id.m_pData = pImage->GetPixelPointer<xiiUInt8>(mip, face, array_index);

        XII_ASSERT_DEV(pImage->GetDepthPitch(mip) < xiiMath::MaxValue<xiiUInt32>(), "Depth pitch exceeds xiiGAL limits.");

        if (xiiImageFormat::GetType(pImage->GetImageFormat()) == xiiImageFormatType::BLOCK_COMPRESSED)
        {
          const xiiUInt32 uiMemPitchFactor = formatProperties.GetElementSize() * 2 / 8;

          id.m_uiStride = xiiMath::Max<xiiUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiStride = static_cast<xiiUInt32>(pImage->GetRowPitch(mip));
        }

        id.m_uiDepthStride = static_cast<xiiUInt32>(pImage->GetDepthPitch(mip));

        m_uiMemoryGPU[m_uiLoadedTextures] += id.m_uiDepthStride;
      }
    }
  }

  const xiiArrayPtr<xiiGALTextureSubResourceData> InitDataPtr(InitData);

  xiiTextureCubeResourceDescriptor td;
  td.m_DescGAL                = texDesc;
  td.m_SamplerDesc.m_AddressU = xiiTextureUtils::GALTextureAddressMode(texFormat.m_AddressModeU);
  td.m_SamplerDesc.m_AddressV = xiiTextureUtils::GALTextureAddressMode(texFormat.m_AddressModeV);
  td.m_SamplerDesc.m_AddressW = xiiTextureUtils::GALTextureAddressMode(texFormat.m_AddressModeW);
  td.m_InitialContent         = InitDataPtr;

  xiiTextureUtils::ConfigureSampler(static_cast<xiiTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

  // ignore its return value here, we build our own
  CreateResource(std::move(td));

  {
    xiiResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;

    if (uiHighestMipLevel == 0)
      res.m_uiQualityLevelsLoadable = 0;
    else
      res.m_uiQualityLevelsLoadable = 1;

    res.m_State = xiiResourceState::Loaded;

    return res;
  }
}

void xiiTextureCubeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiTextureCubeResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiTextureCubeResource, xiiTextureCubeResourceDescriptor)
{
  xiiResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable    = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State                      = xiiResourceState::Loaded;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  XII_ASSERT_DEV(descriptor.m_DescGAL.m_Size.width == descriptor.m_DescGAL.m_Size.height, "Cubemap width and height must be identical");

  m_Format           = descriptor.m_DescGAL.m_Format;
  m_uiWidthAndHeight = descriptor.m_DescGAL.m_Size.width;

  xiiGALTextureData textureData;
  textureData.m_SubResources   = descriptor.m_InitialContent;
  descriptor.m_DescGAL.m_sName = GetResourceDescription();
  descriptor.m_DescGAL.m_BindFlags.Add(xiiGALBindFlags::ShaderResource);
  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_DescGAL, &textureData);

  XII_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  pDevice->GetTexture(m_hGALTexture[m_uiLoadedTextures])->SetDebugName(GetResourceDescription());

  if (!m_hSampler.IsInvalidated())
  {
    pDevice->DestroySampler(m_hSampler);
  }

  m_hSampler = pDevice->CreateSampler(descriptor.m_SamplerDesc);

  XII_ASSERT_DEV(!m_hSampler.IsInvalidated(), "Sampler error");

  ++m_uiLoadedTextures;

  return ret;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Textures_TextureCubeResource);
