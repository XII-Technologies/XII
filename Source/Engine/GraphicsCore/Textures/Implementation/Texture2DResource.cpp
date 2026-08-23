/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTexture2DResource, 1, xiiRTTIDefaultAllocator<xiiTexture2DResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiTexture2DResource);

xiiTexture2DResource::xiiTexture2DResource() :
  xiiResource(DoUpdate::OnAnyThread, 2U)
{
}

xiiTexture2DResource::xiiTexture2DResource(xiiResource::DoUpdate ResourceUpdateThread) :
  xiiResource(ResourceUpdateThread, 2U)
{
}

xiiResourceLoadDescription xiiTexture2DResource::UnloadData(Unload WhatToUnload)
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

void xiiTexture2DResource::FillOutDescriptor(xiiTexture2DResourceDescriptor& ref_td, const xiiImage* pImage, xiiUInt32 uiNumMipLevels, xiiUInt32& out_uiMemoryUsed, xiiHybridArray<xiiGALTextureSubResourceData, 32>& ref_initData)
{
  const xiiUInt32 uiHighestMipLevel = pImage->GetMipLevelCount() - uiNumMipLevels;

  const xiiEnum<xiiGALResourceFormat>    format           = pImage->GetImageFormat();
  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(format);

  ref_td.m_TextureDescription.m_Format      = format;
  ref_td.m_TextureDescription.m_Size.width  = pImage->GetWidth(uiHighestMipLevel);
  ref_td.m_TextureDescription.m_Size.height = pImage->GetHeight(uiHighestMipLevel);
  ref_td.m_TextureDescription.m_uiMipLevels = uiNumMipLevels;
  ref_td.m_TextureDescription.m_Usage       = xiiGALResourceUsage::Immutable;
  ref_td.m_TextureDescription.m_BindFlags.Add(xiiGALBindFlags::ShaderResource);

  xiiUInt32 uiDepth = pImage->GetDepth(uiHighestMipLevel);
  if (uiDepth > 1)
  {
    ref_td.m_TextureDescription.m_Type               = xiiGALResourceDimension::Texture3D;
    ref_td.m_TextureDescription.m_uiArraySizeOrDepth = uiDepth;
  }
  else
  {
    ref_td.m_TextureDescription.m_uiArraySizeOrDepth = pImage->GetNumArrayIndices();
    ref_td.m_TextureDescription.m_Type               = (ref_td.m_TextureDescription.m_uiArraySizeOrDepth > 1) ? xiiGALResourceDimension::Texture2DArray : xiiGALResourceDimension::Texture2D;

    if (pImage->GetNumFaces() == 6)
      ref_td.m_TextureDescription.m_Type = xiiGALResourceDimension::TextureCube;
  }

  if (formatProperties.IsCompressed())
  {
    ref_td.m_TextureDescription.m_Size.width  = xiiMath::RoundUp(ref_td.m_TextureDescription.m_Size.width, 4);
    ref_td.m_TextureDescription.m_Size.height = xiiMath::RoundUp(ref_td.m_TextureDescription.m_Size.height, 4);
  }

  XII_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces");

  out_uiMemoryUsed = 0;

  ref_initData.Clear();

  for (xiiUInt32 arrayIndex = 0; arrayIndex < pImage->GetNumArrayIndices(); ++arrayIndex)
  {
    for (xiiUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (xiiUInt32 mip = uiHighestMipLevel; mip < pImage->GetMipLevelCount(); ++mip)
      {
        xiiGALTextureSubResourceData& id = ref_initData.ExpandAndGetRef();
        id.m_pData                       = pImage->GetSubImageView(mip, face, arrayIndex).GetByteBlobPtr();

        XII_ASSERT_DEV(pImage->GetDepthPitch(mip) < xiiMath::MaxValue<xiiUInt64>(), "Depth pitch exceeds xiiGAL limits.");

        if (formatProperties.IsCompressed())
        {
          const xiiUInt32 uiMemPitchFactor = formatProperties.GetElementSize() * 2 / 8;

          id.m_uiStride = xiiMath::RoundUp(pImage->GetWidth(mip), 4) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiStride = pImage->GetRowPitch(mip);
        }

        id.m_uiDepthStride = pImage->GetDepthPitch(mip);

        out_uiMemoryUsed += static_cast<xiiUInt32>(id.m_uiDepthStride);
      }
    }
  }

  const xiiArrayPtr<xiiGALTextureSubResourceData> InitDataPtr(ref_initData);

  ref_td.m_InitialContent = InitDataPtr;
}

xiiResourceLoadDescription xiiTexture2DResource::UpdateContent(xiiStreamReader* pStream)
{
  if (pStream == nullptr)
  {
    xiiResourceLoadDescription res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::LoadedResourceMissing;

    return res;
  }

  xiiTexture2DResourceDescriptor td;
  xiiImage*                      pImage      = nullptr;
  bool                           bIsFallback = false;

  {
    *pStream >> td.m_TextureDescription;
    *pStream >> td.m_SamplerDescription;
    *pStream >> bIsFallback;

    pStream->ReadBytes(&pImage, sizeof(xiiImage*));
  }

  {
    const xiiUInt32 uiNumMipmapsLowRes   = xiiMath::Min(pImage->GetMipLevelCount(), 6U);
    xiiUInt32       uiUploadNumMipLevels = 0;
    bool            bCouldLoadMore       = false;

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
        // ignore this texture entirely, if we already have low res data but assume we could load a higher resolution version
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
        bCouldLoadMore       = uiNumMipmapsLowRes < pImage->GetMipLevelCount();
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        uiUploadNumMipLevels = pImage->GetMipLevelCount();
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

      xiiTemporaryHybridArray<xiiGALTextureSubResourceData, 32> initData;
      FillOutDescriptor(td, pImage, uiUploadNumMipLevels, m_uiMemoryGPU[m_uiLoadedTextures], initData);

      // ignore its return value here, we build our own
      CreateResource(std::move(td));
    }

    {
      xiiResourceLoadDescription res;
      res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
      res.m_uiQualityLevelsLoadable    = bCouldLoadMore ? 1 : 0;
      res.m_State                      = xiiResourceState::Loaded;

      return res;
    }
  }
}

void xiiTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiTexture2DResource, xiiTexture2DResourceDescriptor)
{
  xiiResourceLoadDescription ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable    = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State                      = xiiResourceState::Loaded;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  m_Type     = descriptor.m_TextureDescription.m_Type;
  m_Format   = descriptor.m_TextureDescription.m_Format;
  m_uiWidth  = descriptor.m_TextureDescription.m_Size.width;
  m_uiHeight = descriptor.m_TextureDescription.m_Size.height;

  descriptor.m_TextureDescription.m_BindFlags.Add(xiiGALBindFlags::ShaderResource);

  xiiGALTextureData textureData(descriptor.m_InitialContent);
  m_pGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_TextureDescription, &textureData);

  XII_ASSERT_DEV(m_pGALTexture[m_uiLoadedTextures] != nullptr, "Texture Data could not be uploaded to the GPU");

  xiiStringBuilder sDebugName;
  sDebugName.SetFormat("{} ([{}] - {}x{})", GetResourceIdOrDescription(), m_uiLoadedTextures, m_uiWidth, m_uiHeight);
  m_pGALTexture[m_uiLoadedTextures]->SetDebugName(sDebugName);

  m_pSampler.Clear();
  m_pSampler = pDevice->CreateSampler(descriptor.m_SamplerDescription);

  XII_ASSERT_DEV(m_pSampler != nullptr, "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Textures_Texture2DResource);
