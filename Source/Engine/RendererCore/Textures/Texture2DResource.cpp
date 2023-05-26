#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/xiiTexFormat/xiiTexFormat.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTexture2DResource, 1, xiiRTTIDefaultAllocator<xiiTexture2DResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCVarInt cvar_RenderingOffscreenTargetResolution1("Rendering.Offscreen.TargetResolution1", 256, xiiCVarFlags::Default, "Configurable render target resolution");
xiiCVarInt cvar_RenderingOffscreenTargetResolution2("Rendering.Offscreen.TargetResolution2", 512, xiiCVarFlags::Default, "Configurable render target resolution");

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiTexture2DResource);

xiiTexture2DResource::xiiTexture2DResource() :
  xiiResource(DoUpdate::OnAnyThread, xiiTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

xiiTexture2DResource::xiiTexture2DResource(xiiResource::DoUpdate ResourceUpdateThread) :
  xiiResource(ResourceUpdateThread, xiiTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

xiiResourceLoadDesc xiiTexture2DResource::UnloadData(Unload WhatToUnload)
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
    if (!m_hSamplerState.IsInvalidated())
    {
      xiiGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
      m_hSamplerState.Invalidate();
    }
  }

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable    = 2 - m_uiLoadedTextures;
  res.m_State                      = m_uiLoadedTextures == 0 ? xiiResourceState::Unloaded : xiiResourceState::Loaded;
  return res;
}

void xiiTexture2DResource::FillOutDescriptor(xiiTexture2DResourceDescriptor& td, const xiiImage* pImage, bool bSRGB, xiiUInt32 uiNumMipLevels, xiiUInt32& out_MemoryUsed, xiiHybridArray<xiiGALSystemMemoryDescription, 32>& initData)
{
  const xiiUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  const xiiGALResourceFormat::Enum format = xiiTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), bSRGB);

  td.m_DescGAL.m_Format          = format;
  td.m_DescGAL.m_uiWidth         = pImage->GetWidth(uiHighestMipLevel);
  td.m_DescGAL.m_uiHeight        = pImage->GetHeight(uiHighestMipLevel);
  td.m_DescGAL.m_uiDepth         = pImage->GetDepth(uiHighestMipLevel);
  td.m_DescGAL.m_uiMipLevelCount = uiNumMipLevels;
  td.m_DescGAL.m_uiArraySize     = pImage->GetNumArrayIndices();

  if (xiiImageFormat::GetType(pImage->GetImageFormat()) == xiiImageFormatType::BLOCK_COMPRESSED)
  {
    td.m_DescGAL.m_uiWidth  = xiiMath::RoundUp(td.m_DescGAL.m_uiWidth, 4);
    td.m_DescGAL.m_uiHeight = xiiMath::RoundUp(td.m_DescGAL.m_uiHeight, 4);
  }

  if (td.m_DescGAL.m_uiDepth > 1)
    td.m_DescGAL.m_Type = xiiGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    td.m_DescGAL.m_Type = xiiGALTextureType::TextureCube;

  XII_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces");

  out_MemoryUsed = 0;

  initData.Clear();

  for (xiiUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (xiiUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (xiiUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        xiiGALSystemMemoryDescription& id = initData.ExpandAndGetRef();

        id.m_pData = const_cast<xiiUInt8*>(pImage->GetPixelPointer<xiiUInt8>(mip, face, array_index));

        if (xiiImageFormat::GetType(pImage->GetImageFormat()) == xiiImageFormatType::BLOCK_COMPRESSED)
        {
          const xiiUInt32 uiMemPitchFactor = xiiGALResourceFormat::GetBitsPerElement(format) * 4 / 8;

          id.m_uiRowPitch = xiiMath::RoundUp(pImage->GetWidth(mip), 4) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiRowPitch = static_cast<xiiUInt32>(pImage->GetRowPitch(mip));
        }

        XII_ASSERT_DEV(pImage->GetDepthPitch(mip) < xiiMath::MaxValue<xiiUInt32>(), "Depth pitch exceeds xiiGAL limits.");
        id.m_uiSlicePitch = static_cast<xiiUInt32>(pImage->GetDepthPitch(mip));

        out_MemoryUsed += id.m_uiSlicePitch;
      }
    }
  }

  const xiiArrayPtr<xiiGALSystemMemoryDescription> InitDataPtr(initData);

  td.m_InitialContent = InitDataPtr;
}


xiiResourceLoadDesc xiiTexture2DResource::UpdateContent(xiiStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    xiiResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::LoadedResourceMissing;

    return res;
  }

  xiiTexture2DResourceDescriptor td;
  xiiImage*                      pImage      = nullptr;
  bool                           bIsFallback = false;
  xiiTexFormat                   texFormat;

  // load image data
  {
    Stream->ReadBytes(&pImage, sizeof(xiiImage*));
    *Stream >> bIsFallback;
    texFormat.ReadHeader(*Stream);

    td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
    td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
    td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  }

  const bool bIsRenderTarget = texFormat.m_iRenderTargetResolutionX != 0;
  XII_ASSERT_DEV(!bIsRenderTarget, "Render targets are not supported by regular 2D texture resources");

  {

    const xiiUInt32 uiNumMipmapsLowRes   = xiiTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : xiiMath::Min(pImage->GetNumMipLevels(), 6U);
    xiiUInt32       uiUploadNumMipLevels = 0;
    bool            bCouldLoadMore       = false;

    if (bIsFallback)
    {
      if (m_uiLoadedTextures == 0)
      {
        // Upload fallback textures if we do not yet have any texture data.
        bCouldLoadMore       = true;
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        // Ignore this texture if we already have a low resolution data, assume we could load a higher resolution version.
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
        // Ignore the texture if we have fully loaded the data.
        xiiLog::Debug("Ignoring texture data, resource is already fully loaded.");
      }
    }

    if (uiUploadNumMipLevels > 0)
    {
      XII_ASSERT_DEBUG(m_uiLoadedTextures < 2, "Invalid texture upload");

      xiiHybridArray<xiiGALSystemMemoryDescription, 32> initData;
      FillOutDescriptor(td, pImage, texFormat.m_bSRGB, uiUploadNumMipLevels, m_uiMemoryGPU[m_uiLoadedTextures], initData);

      xiiTextureUtils::ConfigureSampler(static_cast<xiiTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

      // Ignore its return value here, we build our own.
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

void xiiTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiTexture2DResource, xiiTexture2DResourceDescriptor)
{
  xiiResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable    = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State                      = xiiResourceState::Loaded;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  m_Type     = descriptor.m_DescGAL.m_Type;
  m_Format   = descriptor.m_DescGAL.m_Format;
  m_uiWidth  = descriptor.m_DescGAL.m_uiWidth;
  m_uiHeight = descriptor.m_DescGAL.m_uiHeight;

  descriptor.m_DescGAL.m_szName = GetResourceDescription();

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_DescGAL, descriptor.m_InitialContent);
  XII_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  if (!m_hSamplerState.IsInvalidated())
  {
    pDevice->DestroySamplerState(m_hSamplerState);
  }

  m_hSamplerState = pDevice->CreateSamplerState(descriptor.m_SamplerDesc);
  XII_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// TODO (resources): move into separate file

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderToTexture2DResource, 1, xiiRTTIDefaultAllocator<xiiRenderToTexture2DResource>);
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, Texture2D)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    xiiResourceManager::RegisterResourceOverrideType(xiiGetStaticRTTI<xiiRenderToTexture2DResource>(), [](const xiiStringBuilder& sResourceID) -> bool  {
      return sResourceID.HasExtension(".xiiRenderTarget");
    });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::UnregisterResourceOverrideType(xiiGetStaticRTTI<xiiRenderToTexture2DResource>());
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiRenderToTexture2DResource);

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiRenderToTexture2DResource, xiiRenderToTexture2DResourceDescriptor)
{
  xiiResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable    = 0;
  ret.m_State                      = xiiResourceState::Loaded;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  m_Type     = xiiGALTextureType::Texture2D;
  m_Format   = descriptor.m_Format;
  m_uiWidth  = descriptor.m_uiWidth;
  m_uiHeight = descriptor.m_uiHeight;

  xiiGALTextureCreationDescription descGAL;
  descGAL.SetAsRenderTarget(m_uiWidth, m_uiHeight, m_Format, descriptor.m_SampleCount);
  descGAL.m_szName = GetResourceDescription();

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descGAL, descriptor.m_InitialContent);
  XII_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture data could not be uploaded to the GPU");

  if (!m_hSamplerState.IsInvalidated())
  {
    pDevice->DestroySamplerState(m_hSamplerState);
  }

  m_hSamplerState = pDevice->CreateSamplerState(descriptor.m_SamplerDesc);
  XII_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

xiiResourceLoadDesc xiiRenderToTexture2DResource::UnloadData(Unload WhatToUnload)
{
  for (xiiInt32 r = 0; r < 2; ++r)
  {
    if (!m_hGALTexture[r].IsInvalidated())
    {
      xiiGALDevice::GetDefaultDevice()->DestroyTexture(m_hGALTexture[r]);
      m_hGALTexture[r].Invalidate();
    }

    m_uiMemoryGPU[r] = 0;
  }

  m_uiLoadedTextures = 0;

  if (!m_hSamplerState.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
    m_hSamplerState.Invalidate();
  }

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable    = 2 - m_uiLoadedTextures;
  res.m_State                      = xiiResourceState::Unloaded;
  return res;
}

xiiGALRenderTargetViewHandle xiiRenderToTexture2DResource::GetRenderTargetView() const
{
  return xiiGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hGALTexture[0]);
}

void xiiRenderToTexture2DResource::AddRenderView(xiiViewHandle hView)
{
  m_RenderViews.PushBack(hView);
}

void xiiRenderToTexture2DResource::RemoveRenderView(xiiViewHandle hView)
{
  m_RenderViews.RemoveAndSwap(hView);
}

const xiiDynamicArray<xiiViewHandle>& xiiRenderToTexture2DResource::GetAllRenderViews() const
{
  return m_RenderViews;
}

static xiiUInt16 GetNextBestResolution(float res)
{
  res = xiiMath::Clamp(res, 8.0f, 4096.0f);

  xiiInt32 mulEight = (xiiInt32)xiiMath::Floor((res + 7.9f) / 8.0f);

  return static_cast<xiiUInt16>(mulEight * 8);
}

xiiResourceLoadDesc xiiRenderToTexture2DResource::UpdateContent(xiiStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    xiiResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::LoadedResourceMissing;

    return res;
  }

  xiiRenderToTexture2DResourceDescriptor td;
  xiiImage*                              pImage      = nullptr;
  bool                                   bIsFallback = false;
  xiiTexFormat                           texFormat;

  // Load Image Data
  {
    Stream->ReadBytes(&pImage, sizeof(xiiImage*));
    *Stream >> bIsFallback;
    texFormat.ReadHeader(*Stream);

    td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
    td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
    td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  }

  const bool bIsRenderTarget = texFormat.m_iRenderTargetResolutionX != 0;

  XII_ASSERT_DEV(bIsRenderTarget, "Trying to create a RenderToTexture resource from data that is not set up as a render-target");

  {
    XII_ASSERT_DEV(m_uiLoadedTextures == 0, "not implemented");

    if (texFormat.m_iRenderTargetResolutionX == -1)
    {
      if (texFormat.m_iRenderTargetResolutionY == 1)
      {
        texFormat.m_iRenderTargetResolutionX = GetNextBestResolution(cvar_RenderingOffscreenTargetResolution1 * texFormat.m_fResolutionScale);
        texFormat.m_iRenderTargetResolutionY = texFormat.m_iRenderTargetResolutionX;
      }
      else if (texFormat.m_iRenderTargetResolutionY == 2)
      {
        texFormat.m_iRenderTargetResolutionX = GetNextBestResolution(cvar_RenderingOffscreenTargetResolution2 * texFormat.m_fResolutionScale);
        texFormat.m_iRenderTargetResolutionY = texFormat.m_iRenderTargetResolutionX;
      }
      else
      {
        XII_REPORT_FAILURE("Invalid render target configuration: {0} x {1}", texFormat.m_iRenderTargetResolutionX, texFormat.m_iRenderTargetResolutionY);
      }
    }

    td.m_Format   = static_cast<xiiGALResourceFormat::Enum>(texFormat.m_GalRenderTargetFormat);
    td.m_uiWidth  = texFormat.m_iRenderTargetResolutionX;
    td.m_uiHeight = texFormat.m_iRenderTargetResolutionY;

    xiiTextureUtils::ConfigureSampler(static_cast<xiiTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

    m_uiLoadedTextures = 0;

    CreateResource(std::move(td));
  }

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;
  return res;
}

void xiiRenderToTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiRenderToTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Textures_Texture2DResource);
