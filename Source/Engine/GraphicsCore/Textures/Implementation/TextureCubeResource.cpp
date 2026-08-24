/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Textures/TextureCubeResource.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeResource, 1, xiiRTTIDefaultAllocator<xiiTextureCubeResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiTextureCubeResource);

xiiTextureCubeResource::xiiTextureCubeResource() :
  xiiResource(DoUpdate::OnAnyThread, 2U)
{
  m_uiLoadedTextures = 0;
  m_uiMemoryGPU[0]   = 0;
  m_uiMemoryGPU[1]   = 0;
  m_Format           = xiiGALResourceFormat::Unknown;
  m_uiWidthAndHeight = 0;
}

xiiResourceLoadDescription xiiTextureCubeResource::UnloadData(Unload WhatToUnload)
{
  if (m_uiLoadedTextures > 0)
  {
    for (xiiInt32 r = 0; r < 2; ++r)
    {
      --m_uiLoadedTextures;

      m_pGALTexture[m_uiLoadedTextures].Clear();

      if (WhatToUnload == Unload::OneQualityLevel || m_uiLoadedTextures == 0)
        break;
    }
  }

  if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_pSampler.Clear();
  }

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable    = 2 - m_uiLoadedTextures;
  res.m_State                      = m_uiLoadedTextures == 0 ? xiiResourceState::Unloaded : xiiResourceState::Loaded;

  return res;
}

xiiResourceLoadDescription xiiTextureCubeResource::UpdateContent(xiiStreamReader* pStream)
{
  if (pStream == nullptr)
  {
    xiiResourceLoadDescription res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::LoadedResourceMissing;

    return res;
  }

  xiiTextureCubeResourceDescriptor td;
  xiiImage*                        pImage      = nullptr;
  bool                             bIsFallback = false;

  {
    *pStream >> td.m_TextureDescription;
    *pStream >> td.m_SamplerDescription;
    *pStream >> bIsFallback;

    pStream->ReadBytes(&pImage, sizeof(xiiImage*));
  }

  const xiiUInt32 uiNumMipmapsLowRes = 6U;
  const xiiUInt32 uiNumMipLevels     = xiiMath::Min(m_uiLoadedTextures == 0 ? uiNumMipmapsLowRes : pImage->GetMipLevelCount(), pImage->GetMipLevelCount());
  const xiiUInt32 uiHighestMipLevel  = pImage->GetMipLevelCount() - uiNumMipLevels;

  if (pImage->GetWidth(uiHighestMipLevel) != pImage->GetHeight(uiHighestMipLevel))
  {
    xiiLog::Error("Cubemap width '{0}' is not identical to height '{1}'", pImage->GetWidth(uiHighestMipLevel), pImage->GetHeight(uiHighestMipLevel));

    xiiResourceLoadDescription res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::LoadedResourceMissing;

    return res;
  }

  m_Format           = pImage->GetImageFormat();
  m_uiWidthAndHeight = pImage->GetWidth(uiHighestMipLevel);

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Format      = m_Format;
  textureDescription.m_Size.width  = m_uiWidthAndHeight;
  textureDescription.m_Size.height = m_uiWidthAndHeight;
  textureDescription.m_uiMipLevels = uiNumMipLevels;
  textureDescription.m_BindFlags   = xiiGALBindFlags::ShaderResource;
  textureDescription.m_Usage       = xiiGALResourceUsage::Immutable;

  xiiUInt32 uiDepth = pImage->GetDepth(uiHighestMipLevel);
  if (uiDepth > 1)
  {
    textureDescription.m_Type               = xiiGALResourceDimension::Texture3D;
    textureDescription.m_uiArraySizeOrDepth = uiDepth;
  }
  else
  {
    textureDescription.m_uiArraySizeOrDepth = pImage->GetNumArrayIndices();
    textureDescription.m_Type               = (textureDescription.m_uiArraySizeOrDepth > 1) ? xiiGALResourceDimension::Texture2DArray : xiiGALResourceDimension::Texture2D;

    if (pImage->GetNumFaces() == 6)
    {
      textureDescription.m_Type               = xiiGALResourceDimension::TextureCube;
      textureDescription.m_uiArraySizeOrDepth = 6;
    }
  }

  XII_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '{0}')", GetResourceID());

  m_uiMemoryGPU[m_uiLoadedTextures] = 0;

  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(m_Format);

  xiiHybridArray<xiiGALTextureSubResourceData, 32> InitData;

  for (xiiUInt32 arrayIndex = 0; arrayIndex < pImage->GetNumArrayIndices(); ++arrayIndex)
  {
    for (xiiUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (xiiUInt32 mip = uiHighestMipLevel; mip < pImage->GetMipLevelCount(); ++mip)
      {
        xiiGALTextureSubResourceData& id = InitData.ExpandAndGetRef();
        id.m_pData                       = pImage->GetSubImageView(mip, face, arrayIndex).GetByteBlobPtr();

        XII_ASSERT_DEV(pImage->GetDepthPitch(mip) < xiiMath::MaxValue<xiiUInt64>(), "Depth pitch exceeds xiiGAL limits.");

        if (formatProperties.IsCompressed())
        {
          const xiiUInt64 uiMemPitchFactor = formatProperties.GetElementSize() * 2 / 8;

          id.m_uiStride = xiiMath::Max<xiiUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiStride = pImage->GetRowPitch(mip);
        }

        id.m_uiDepthStride = pImage->GetDepthPitch(mip);

        m_uiMemoryGPU[m_uiLoadedTextures] += static_cast<xiiUInt32>(id.m_uiDepthStride);
      }
    }
  }

  const xiiArrayPtr<xiiGALTextureSubResourceData> InitDataPtr(InitData);

  td.m_TextureDescription = textureDescription;
  td.m_InitialContent     = InitDataPtr;

  // ignore its return value here, we build our own
  CreateResource(std::move(td));

  {
    xiiResourceLoadDescription res;
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
  xiiResourceLoadDescription ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable    = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State                      = xiiResourceState::Loaded;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  XII_ASSERT_DEV(descriptor.m_TextureDescription.m_Size.width == descriptor.m_TextureDescription.m_Size.height, "Cubemap width and height must be identical");

  m_Format           = descriptor.m_TextureDescription.m_Format;
  m_uiWidthAndHeight = descriptor.m_TextureDescription.m_Size.width;

  xiiGALTextureData textureData(descriptor.m_InitialContent);
  descriptor.m_TextureDescription.m_BindFlags.Add(xiiGALBindFlags::ShaderResource);
  m_pGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_TextureDescription, &textureData);

  XII_ASSERT_DEV(m_pGALTexture[m_uiLoadedTextures] != nullptr, "Texture Data could not be uploaded to the GPU");

  m_pGALTexture[m_uiLoadedTextures]->SetDebugName(GetResourceDescription());

  m_pSampler.Clear();
  m_pSampler = pDevice->CreateSampler(descriptor.m_SamplerDescription);

  XII_ASSERT_DEV(m_pSampler != nullptr, "Sampler error");

  ++m_uiLoadedTextures;

  return ret;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Textures_TextureCubeResource);
