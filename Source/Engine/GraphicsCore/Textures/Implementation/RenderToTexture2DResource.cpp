/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Textures/RenderToTexture2DResource.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/xiiTexFormat/xiiTexFormat.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderToTexture2DResource, 1, xiiRTTIDefaultAllocator<xiiRenderToTexture2DResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, Texture2D)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiResourceManager::RegisterResourceOverrideType(xiiGetStaticRTTI<xiiRenderToTexture2DResource>(), [](const xiiStringBuilder& sResourceID) -> bool {
      return sResourceID.HasExtension(".xiiBinRenderTarget");
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

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  m_Type     = xiiGALResourceDimension::Texture2D;
  m_Format   = descriptor.m_Format;
  m_uiWidth  = descriptor.m_uiWidth;
  m_uiHeight = descriptor.m_uiHeight;

  xiiGALTextureCreationDescription descGAL;
  descGAL.m_Type               = xiiGALResourceDimension::Texture2D;
  descGAL.m_Size.width         = m_uiWidth;
  descGAL.m_Size.height        = m_uiHeight;
  descGAL.m_uiArraySizeOrDepth = 1U;
  descGAL.m_uiMipLevels        = 1U;
  descGAL.m_uiSampleCount      = descriptor.m_SampleCount;
  descGAL.m_Format             = m_Format;
  descGAL.m_BindFlags          = xiiGALBindFlags::ShaderResource | (!xiiGALResourceFormat::IsDepthFormat(m_Format) ? xiiGALBindFlags::RenderTarget : xiiGALBindFlags::DepthStencil);

  xiiGALTextureData textureData(descriptor.m_InitialContent);
  m_pGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descGAL, &textureData);

  XII_ASSERT_DEV(m_pGALTexture[m_uiLoadedTextures] != nullptr, "Texture data could not be uploaded to the GPU.");

  m_pGALTexture[m_uiLoadedTextures]->SetDebugName(GetResourceDescription());

  m_pSampler.Clear();
  m_pSampler = pDevice->CreateSampler(descriptor.m_SamplerDesc);

  XII_ASSERT_DEV(m_pSampler != nullptr, "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

xiiResourceLoadDesc xiiRenderToTexture2DResource::UnloadData(Unload WhatToUnload)
{
  for (xiiInt32 r = 0; r < 2; ++r)
  {
    m_pGALTexture[r].Clear();
  }

  m_uiLoadedTextures = 0;

  m_pSampler.Clear();

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable    = 2 - m_uiLoadedTextures;
  res.m_State                      = xiiResourceState::Unloaded;
  return res;
}

xiiSharedPtr<xiiGALTextureView> xiiRenderToTexture2DResource::GetRenderTargetView() const
{
  return m_pGALTexture[0]->GetDefaultView(xiiGALTextureViewType::RenderTarget);
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

static xiiUInt16 GetNextBestResolution(float fRes)
{
  fRes = xiiMath::Clamp(fRes, 8.0f, 4096.0f);

  xiiInt32 mulEight = (xiiInt32)xiiMath::Floor((fRes + 7.9f) / 8.0f);

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

  XII_ASSERT_DEV(bIsRenderTarget, "Trying to create a RenderToTexture resource from data that is not set up as a render-target");

  {
    XII_ASSERT_DEV(m_uiLoadedTextures == 0, "not implemented");

    if (texFormat.m_iRenderTargetResolutionX == -1)
    {
      if (texFormat.m_iRenderTargetResolutionY == 1)
      {
        xiiCVarInt* pRenderingOffscreenTargetResolution1 = (xiiCVarInt*)xiiCVar::FindCVarByName("Rendering.Offscreen.TargetResolution1");
        texFormat.m_iRenderTargetResolutionX             = GetNextBestResolution(pRenderingOffscreenTargetResolution1->GetValue() * texFormat.m_fResolutionScale);
        texFormat.m_iRenderTargetResolutionY             = texFormat.m_iRenderTargetResolutionX;
      }
      else if (texFormat.m_iRenderTargetResolutionY == 2)
      {
        xiiCVarInt* pRenderingOffscreenTargetResolution2 = (xiiCVarInt*)xiiCVar::FindCVarByName("Rendering.Offscreen.TargetResolution2");
        texFormat.m_iRenderTargetResolutionX             = GetNextBestResolution(pRenderingOffscreenTargetResolution2->GetValue() * texFormat.m_fResolutionScale);
        texFormat.m_iRenderTargetResolutionY             = texFormat.m_iRenderTargetResolutionX;
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Textures_RenderToTexture2DResource);
