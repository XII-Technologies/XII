
#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>

#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <Texture/xiiTexFormat/xiiTexFormat.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTexture3DResource, 1, xiiRTTIDefaultAllocator<xiiTexture3DResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiTexture3DResource);

xiiTexture3DResource::xiiTexture3DResource() :
  xiiResource(DoUpdate::OnAnyThread, xiiTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

xiiTexture3DResource::xiiTexture3DResource(xiiResource::DoUpdate ResourceUpdateThread) :
  xiiResource(ResourceUpdateThread, xiiTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

xiiResourceLoadDesc xiiTexture3DResource::UnloadData(Unload WhatToUnload)
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

void xiiTexture3DResource::FillOutDescriptor(xiiTexture3DResourceDescriptor& ref_td, const xiiImage* pImage, bool bSRGB, xiiUInt32 uiNumMipLevels, xiiUInt32& out_uiMemoryUsed, xiiHybridArray<xiiGALTextureSubResourceData, 32>& ref_initData)
{
  const xiiUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  const xiiEnum<xiiGALTextureFormat> format = xiiTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), bSRGB);

  ref_td.m_DescGAL.m_Format      = format;
  ref_td.m_DescGAL.m_Size.width  = pImage->GetWidth(uiHighestMipLevel);
  ref_td.m_DescGAL.m_Size.height = pImage->GetHeight(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiMipLevels = uiNumMipLevels;
  ref_td.m_DescGAL.m_BindFlags.Add(xiiGALBindFlags::ShaderResource);

  xiiUInt32 uiDepth = pImage->GetDepth(uiHighestMipLevel);
  if (uiDepth > 1)
  {
    ref_td.m_DescGAL.m_Type               = xiiGALResourceDimension::Texture3D;
    ref_td.m_DescGAL.m_uiArraySizeOrDepth = uiDepth;
  }
  else
  {
    ref_td.m_DescGAL.m_uiArraySizeOrDepth = pImage->GetNumArrayIndices();
    ref_td.m_DescGAL.m_Type               = (ref_td.m_DescGAL.m_uiArraySizeOrDepth > 1) ? xiiGALResourceDimension::Texture2DArray : xiiGALResourceDimension::Texture2D;
  }

  out_uiMemoryUsed = 0;

  ref_initData.Clear();

  for (xiiUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (xiiUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (xiiUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        xiiGALTextureSubResourceData& id = ref_initData.ExpandAndGetRef();

        id.m_pData = const_cast<xiiUInt8*>(pImage->GetPixelPointer<xiiUInt8>(mip, face, array_index));

        if (xiiImageFormat::GetType(pImage->GetImageFormat()) == xiiImageFormatType::BLOCK_COMPRESSED)
        {
          const xiiUInt32 uiMemPitchFactor = xiiGALTextureFormat::GetBitsPerElement(format) * 4 / 8;

          id.m_uiStride = xiiMath::Max<xiiUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiStride = static_cast<xiiUInt32>(pImage->GetRowPitch(mip));
        }

        XII_ASSERT_DEV(pImage->GetDepthPitch(mip) < xiiMath::MaxValue<xiiUInt32>(), "Depth pitch exceeds xiiGAL limits.");
        id.m_uiDepthStride = static_cast<xiiUInt32>(pImage->GetDepthPitch(mip));

        out_uiMemoryUsed += id.m_uiDepthStride;
      }
    }
  }

  const xiiArrayPtr<xiiGALTextureSubResourceData> InitDataPtr(ref_initData);

  ref_td.m_InitialContent = InitDataPtr;
}


xiiResourceLoadDesc xiiTexture3DResource::UpdateContent(xiiStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    xiiResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::LoadedResourceMissing;

    return res;
  }

  xiiTexture3DResourceDescriptor td;
  xiiImage*                      pImage      = nullptr;
  bool                           bIsFallback = false;
  xiiTexFormat                   texFormat;

  // load image data
  {
    Stream->ReadBytes(&pImage, sizeof(xiiImage*));
    *Stream >> bIsFallback;
    texFormat.ReadHeader(*Stream);

    td.m_SamplerDesc.m_AddressU = xiiTextureUtils::GALTextureAddressMode(texFormat.m_AddressModeU);
    td.m_SamplerDesc.m_AddressV = xiiTextureUtils::GALTextureAddressMode(texFormat.m_AddressModeV);
    td.m_SamplerDesc.m_AddressW = xiiTextureUtils::GALTextureAddressMode(texFormat.m_AddressModeW);
  }

  const bool bIsRenderTarget = texFormat.m_iRenderTargetResolutionX != 0;
  XII_ASSERT_DEV(!bIsRenderTarget, "Render targets are not supported by regular 2D texture resources");

  {

    const xiiUInt32 uiNumMipmapsLowRes =
      xiiTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : xiiMath::Min(pImage->GetNumMipLevels(), 6U);
    xiiUInt32 uiUploadNumMipLevels = 0;
    bool      bCouldLoadMore       = false;

    if (bIsFallback)
    {
      if (m_uiLoadedTextures == 0)
      {
        // only upload fallback textures, if we don't have any texture data at all, yet
        bCouldLoadMore       = true;
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        // ignore this texture entirely, if we already have low res data
        // but assume we could load a higher resolution version
        bCouldLoadMore = true;
        xiiLog::Debug("Ignoring fallback texture data, low-res resource data is already loaded.");
      }
      else
      {
        xiiLog::Debug("Ignoring fallback texture data, resource is already fully loaded.");
      }
    }
    else
    {
      if (m_uiLoadedTextures == 0)
      {
        bCouldLoadMore       = uiNumMipmapsLowRes < pImage->GetNumMipLevels();
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        uiUploadNumMipLevels = pImage->GetNumMipLevels();
      }
      else
      {
        // ignore the texture, if we already have fully loaded data
        xiiLog::Debug("Ignoring texture data, resource is already fully loaded.");
      }
    }

    if (uiUploadNumMipLevels > 0)
    {
      XII_ASSERT_DEBUG(m_uiLoadedTextures < 2, "Invalid texture upload");

      xiiHybridArray<xiiGALTextureSubResourceData, 32> initData;
      FillOutDescriptor(td, pImage, texFormat.m_bSRGB, uiUploadNumMipLevels, m_uiMemoryGPU[m_uiLoadedTextures], initData);

      xiiTextureUtils::ConfigureSampler(static_cast<xiiTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

      // ignore its return value here, we build our own
      CreateResource(std::move(td));
    }

    {
      xiiResourceLoadDesc res;
      res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
      res.m_uiQualityLevelsLoadable    = bCouldLoadMore ? 1 : 0;
      res.m_State                      = xiiResourceState::Loaded;

      return res;
    }
  }
}

void xiiTexture3DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiTexture3DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiTexture3DResource, xiiTexture3DResourceDescriptor)
{
  xiiResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable    = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State                      = xiiResourceState::Loaded;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  m_Type     = descriptor.m_DescGAL.m_Type;
  m_Format   = descriptor.m_DescGAL.m_Format;
  m_uiWidth  = descriptor.m_DescGAL.m_Size.width;
  m_uiHeight = descriptor.m_DescGAL.m_Size.height;
  m_uiDepth  = descriptor.m_DescGAL.m_uiArraySizeOrDepth;

  xiiGALTextureData textureData;
  textureData.m_SubResources   = descriptor.m_InitialContent;
  descriptor.m_DescGAL.m_sName = GetResourceDescription();
  descriptor.m_DescGAL.m_BindFlags.Add(xiiGALBindFlags::ShaderResource);
  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_DescGAL, &textureData);

  XII_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  if (!m_hSampler.IsInvalidated())
  {
    pDevice->DestroySampler(m_hSampler);
  }

  m_hSampler = pDevice->CreateSampler(descriptor.m_SamplerDesc);

  XII_ASSERT_DEV(!m_hSampler.IsInvalidated(), "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Textures_Texture3DResource);
