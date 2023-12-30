#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Device/PassD3D12.h>
#include <GraphicsD3D12/Device/SwapChainD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>

#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

#include <Diligent/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>

xiiGALSwapChainD3D12::xiiGALSwapChainD3D12(const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(creationDescription)
{
}

xiiGALSwapChainD3D12::~xiiGALSwapChainD3D12() = default;

xiiResult xiiGALSwapChainD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

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

  auto* pFactoryD3D12 = static_cast<Diligent::IEngineFactoryD3D12*>(pDeviceD3D12->GetFactory());

  Diligent::FullScreenModeDesc fullScreenModeDescription;
  pFactoryD3D12->CreateSwapChainD3D12(pDeviceD3D12->GetDevice(), pDeviceD3D12->GetImmediateContext(), swapChainDescription, fullScreenModeDescription, nativeWindow, &m_pSwapChain);

  if (m_pSwapChain == nullptr)
  {
    return XII_FAILURE;
  }

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_Description.m_pWindow->AddReference();

  return CreateBackBufferInternal(pDeviceD3D12);
}

xiiResult xiiGALSwapChainD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  DestroyBackBufferInternal(pDeviceD3D12);

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

xiiResult xiiGALSwapChainD3D12::CreateBackBufferInternal(xiiGALDeviceD3D12* pDeviceD3D12)
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

  xiiGALTextureHandle hBackbufferTexture = pDeviceD3D12->CreateTexture(textureDescription);
  XII_ASSERT_RELEASE(!hBackbufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

  RenderTargetInfo renderTargetInfo{pRTV, hBackbufferTexture};

  m_BackbufferTextures.PushBack(renderTargetInfo);

  m_RenderTargets.m_hRTs[0] = hBackbufferTexture;

  m_CurrentSize = textureDescription.m_Size;

  return XII_SUCCESS;
}

void xiiGALSwapChainD3D12::DestroyBackBufferInternal(xiiGALDeviceD3D12* pDeviceD3D12)
{
  for (auto& iter : m_BackbufferTextures)
  {
    pDeviceD3D12->DestroyTexture(iter.m_hRenderTargetHandle);

    iter.m_hRenderTargetHandle.Invalidate();
  }
  m_RenderTargets.m_hRTs[0].Invalidate();
  m_BackbufferTextures.Clear();
}

void xiiGALSwapChainD3D12::AcquireNextRenderTarget(xiiGALDevice* pDevice)
{
  XII_PROFILE_SCOPE("AcquireNextRenderTarget");

  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

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
    CreateBackBufferInternal(pDeviceD3D12).AssertSuccess();
  }
}

void xiiGALSwapChainD3D12::Present(xiiGALDevice* pDevice)
{
  XII_PROFILE_SCOPE("PresentRenderTarget");

  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  XII_ASSERT_DEV(m_pSwapChain->GetCurrentBackBufferRTV()->GetTexture() == static_cast<xiiGALTextureD3D12*>(pDeviceD3D12->GetTexture(m_RenderTargets.m_hRTs[0]))->GetTexture(), "Invalid Swapchain texture. Did you forget to call xiiGALSwapChain::AcquireNextRenderTarget?");

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

xiiResult xiiGALSwapChainD3D12::Resize(xiiGALDevice* pDevice, xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  DestroyBackBufferInternal(pDeviceD3D12);

  // Need to flush dead objects or ResizeBuffers will fail as the backbuffer is still referenced.
  pDeviceD3D12->GetDefaultPass()->ReleaseCachedRenderPassesAndFramebuffers();
  pDeviceD3D12->FlushPendingObjects();

  m_pSwapChain->Resize(newSize.width, newSize.height, xiiDiligentTypeConversions::GetSurfaceTransform(newTransform));

  return CreateBackBufferInternal(pDeviceD3D12);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Device_Implementation_SwapChainD3D12);
