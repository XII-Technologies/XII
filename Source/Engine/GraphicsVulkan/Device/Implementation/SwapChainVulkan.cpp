#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/PassVulkan.h>
#include <GraphicsVulkan/Device/SwapChainVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

#include <Diligent/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>

xiiGALSwapChainVulkan::xiiGALSwapChainVulkan(const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(creationDescription)
{
}

xiiGALSwapChainVulkan::~xiiGALSwapChainVulkan() = default;

xiiResult xiiGALSwapChainVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::NativeWindow nativeWindow{xiiMinWindows::ToNative(m_Description.m_pWindow->GetNativeWindowHandle())};

  Diligent::SwapChainDesc swapChainDescription;
  swapChainDescription.Width               = m_Description.m_Resolution.width;
  swapChainDescription.Height              = m_Description.m_Resolution.height;
  swapChainDescription.ColorBufferFormat   = xiiDiligentTypeConversions::GetTextureFormat(m_Description.m_ColorBufferFormat);
  swapChainDescription.DepthBufferFormat   = Diligent::TEX_FORMAT_UNKNOWN; // Do not create a default depth buffer.
  swapChainDescription.Usage               = xiiDiligentTypeConversions::GetSwapChainUsageFlags(m_Description.m_Usage);
  swapChainDescription.PreTransform        = xiiDiligentTypeConversions::GetSurfaceTransform(m_Description.m_PreTransform);
  swapChainDescription.BufferCount         = m_Description.m_uiBufferCount;
  swapChainDescription.DefaultDepthValue   = m_Description.m_fDefaultDepthValue;
  swapChainDescription.DefaultStencilValue = m_Description.m_uiDefaultStencilValue;
  swapChainDescription.IsPrimary           = m_Description.m_bIsPrimary;

  auto* pFactoryVulkan = static_cast<Diligent::IEngineFactoryVk*>(pDeviceVulkan->GetFactory());

  pFactoryVulkan->CreateSwapChainVk(pDeviceVulkan->GetDevice(), pDeviceVulkan->GetImmediateContext(), swapChainDescription, nativeWindow, &m_pSwapChain);

  if (m_pSwapChain == nullptr)
  {
    return XII_FAILURE;
  }

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_Description.m_pWindow->AddReference();

  return CreateBackBufferInternal(pDeviceVulkan);
}

xiiResult xiiGALSwapChainVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  DestroyBackBufferInternal(pDeviceVulkan);

  if (m_pSwapChain)
  {
    // Full screen swap chains must be switched to windowed mode before destruction.
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
    m_pSwapChain->SetWindowedMode();

    XII_GAL_DILIGENT_PTR_RELEASE(m_pSwapChain);

    m_Description.m_pWindow->RemoveReference();
  }

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainVulkan::CreateBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan)
{
  Diligent::ITextureView* pRTV     = m_pSwapChain->GetCurrentBackBufferRTV();
  Diligent::ITexture*     pTexture = pRTV->GetTexture();

  if (pRTV == nullptr)
  {
    xiiLog::Error("Couldn't access backbuffer texture of swapchain");

    return XII_FAILURE;
  }

  const Diligent::TextureDesc& textureDesc = pTexture->GetDesc();

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_sName              = textureDesc.Name;
  textureDescription.m_Type               = xiiGALResourceDimension::Texture2D;
  textureDescription.m_Size.width         = textureDesc.Width;
  textureDescription.m_Size.height        = textureDesc.Height;
  textureDescription.m_uiArraySizeOrDepth = textureDesc.ArraySize;
  textureDescription.m_Format             = xiiDiligentTypeConversions::GetGALTextureFormat(textureDesc.Format);
  textureDescription.m_uiMipLevels        = textureDesc.MipLevels;
  textureDescription.m_uiSampleCount      = textureDesc.SampleCount;
  textureDescription.m_BindFlags          = xiiDiligentTypeConversions::GetGALBindFlags(textureDesc.BindFlags);
  textureDescription.m_Usage              = xiiDiligentTypeConversions::GetGALUsage(textureDesc.Usage);
  textureDescription.m_CPUAccessFlags     = xiiDiligentTypeConversions::GetGALCPUAccessFlags(textureDesc.CPUAccessFlags);
  textureDescription.m_MiscFlags          = xiiDiligentTypeConversions::GetGALMiscTextureFlags(textureDesc.MiscFlags);

  textureDescription.m_ClearValue.m_TextureFormat            = xiiDiligentTypeConversions::GetGALTextureFormat(textureDesc.ClearValue.Format);
  textureDescription.m_ClearValue.m_ClearColor.r             = textureDesc.ClearValue.Color[0];
  textureDescription.m_ClearValue.m_ClearColor.g             = textureDesc.ClearValue.Color[1];
  textureDescription.m_ClearValue.m_ClearColor.b             = textureDesc.ClearValue.Color[2];
  textureDescription.m_ClearValue.m_ClearColor.a             = textureDesc.ClearValue.Color[3];
  textureDescription.m_ClearValue.m_DepthStencil.m_fDepth    = textureDesc.ClearValue.DepthStencil.Depth;
  textureDescription.m_ClearValue.m_DepthStencil.m_uiStencil = textureDesc.ClearValue.DepthStencil.Stencil;
  textureDescription.m_uiImmediateContextMask                = textureDesc.ImmediateContextMask;

  textureDescription.m_pExisitingNativeObject = pTexture;

  xiiGALTextureHandle hBackbufferTexture = pDeviceVulkan->CreateTexture(textureDescription);
  XII_ASSERT_RELEASE(!hBackbufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

  RenderTargetInfo renderTargetInfo{pRTV, hBackbufferTexture};

  m_BackbufferTextures.PushBack(renderTargetInfo);

  m_RenderTargets.m_hRTs[0] = hBackbufferTexture;

  m_CurrentSize = textureDescription.m_Size;

  return XII_SUCCESS;
}

void xiiGALSwapChainVulkan::DestroyBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan)
{
  for (auto& iter : m_BackbufferTextures)
  {
    pDeviceVulkan->DestroyTexture(iter.m_hRenderTargetHandle);

    iter.m_hRenderTargetHandle.Invalidate();
  }
  m_RenderTargets.m_hRTs[0].Invalidate();
  m_BackbufferTextures.Clear();
}

void xiiGALSwapChainVulkan::AcquireNextRenderTarget(xiiGALDevice* pDevice)
{
  XII_PROFILE_SCOPE("AcquireNextRenderTarget");

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::ITextureView* pCurrentTextureView = m_pSwapChain->GetCurrentBackBufferRTV();

  bool bBackBufferFound = false;
  for (auto& backBufferInfo : m_BackbufferTextures)
  {
    if (backBufferInfo.m_pTextureView == pCurrentTextureView)
    {
      bBackBufferFound          = true;
      m_RenderTargets.m_hRTs[0] = backBufferInfo.m_hRenderTargetHandle;

      break;
    }
  }

  if (!bBackBufferFound)
  {
    CreateBackBufferInternal(pDeviceVulkan).AssertSuccess();
  }
}

void xiiGALSwapChainVulkan::Present(xiiGALDevice* pDevice)
{
  XII_PROFILE_SCOPE("PresentRenderTarget");

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  XII_ASSERT_DEV(m_pSwapChain->GetCurrentBackBufferRTV()->GetTexture() == static_cast<xiiGALTextureVulkan*>(pDeviceVulkan->GetTexture(m_RenderTargets.m_hRTs[0]))->GetTexture(), "Invalid Swapchain texture. Did you forget to call xiiGALSwapChain::AcquireNextRenderTarget?");

  xiiUInt32 uiSyncInterval = 1U;
  switch (m_PresentMode)
  {
    case xiiGALPresentMode::Immediate:
      uiSyncInterval = 0U;
      break;
    case xiiGALPresentMode::VSync:
      uiSyncInterval = 1U;
      break;
  }
  m_pSwapChain->Present(uiSyncInterval);
}

xiiResult xiiGALSwapChainVulkan::Resize(xiiGALDevice* pDevice, xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  DestroyBackBufferInternal(pDeviceVulkan);

  // Need to flush dead objects or ResizeBuffers will fail as the backbuffer is still referenced.
  pDeviceVulkan->GetDefaultPass()->ReleaseCachedRenderPassesAndFramebuffers();
  pDeviceVulkan->FlushPendingObjects();

  m_pSwapChain->Resize(newSize.width, newSize.height, xiiDiligentTypeConversions::GetSurfaceTransform(newTransform));

  return CreateBackBufferInternal(pDeviceVulkan);
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_SwapChainVulkan);
