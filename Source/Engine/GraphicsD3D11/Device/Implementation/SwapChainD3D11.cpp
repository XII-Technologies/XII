#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Device/SwapChainD3D11.h>
#include <GraphicsD3D11/Resources/TextureD3D11.h>

#include <GraphicsD3D11/Utilities/D3D11TypeConversions.h>

#include <Diligent/Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>

xiiGALSwapChainD3D11::xiiGALSwapChainD3D11(const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(creationDescription)
{
}

xiiGALSwapChainD3D11::~xiiGALSwapChainD3D11() = default;

xiiResult xiiGALSwapChainD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

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

  auto* pFactoryD3D11 = static_cast<Diligent::IEngineFactoryD3D11*>(pDeviceD3D11->GetFactory());

  Diligent::FullScreenModeDesc fullScreenModeDescription;
  pFactoryD3D11->CreateSwapChainD3D11(pDeviceD3D11->GetDevice(), pDeviceD3D11->GetImmediateContext(), swapChainDescription, fullScreenModeDescription, nativeWindow, &m_pSwapChain);

  if (m_pSwapChain == nullptr)
  {
    return XII_FAILURE;
  }

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_Description.m_pWindow->AddReference();

  return CreateBackBufferInternal(pDeviceD3D11);
}

xiiResult xiiGALSwapChainD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  DestroyBackBufferInternal(pDeviceD3D11);

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

xiiResult xiiGALSwapChainD3D11::CreateBackBufferInternal(xiiGALDeviceD3D11* pDeviceD3D11)
{
  Diligent::ITextureView* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();

  if (pRTV == nullptr)
  {
    xiiLog::Error("Couldn't access backbuffer texture of swapchain");

    return XII_FAILURE;
  }

  Diligent::ITexture*          pTexture    = pRTV->GetTexture();
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

  xiiGALTextureHandle hBackbufferTexture = pDeviceD3D11->CreateTexture(textureDescription);
  XII_ASSERT_RELEASE(!hBackbufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

  RenderTargetInfo renderTargetInfo{pRTV, hBackbufferTexture};

  m_BackbufferTextures.PushBack(renderTargetInfo);

  m_hBackBufferTexture = hBackbufferTexture;

  m_CurrentSize = textureDescription.m_Size;

  return XII_SUCCESS;
}

void xiiGALSwapChainD3D11::DestroyBackBufferInternal(xiiGALDeviceD3D11* pDeviceD3D11)
{
  for (auto& iter : m_BackbufferTextures)
  {
    pDeviceD3D11->DestroyTexture(iter.m_hRenderTargetHandle);

    iter.m_hRenderTargetHandle.Invalidate();
  }
  m_hBackBufferTexture.Invalidate();
  m_BackbufferTextures.Clear();
}

void xiiGALSwapChainD3D11::AcquireNextRenderTarget(xiiGALDevice* pDevice)
{
  XII_PROFILE_SCOPE("AcquireNextRenderTarget");

  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  Diligent::ITextureView* pCurrentTextureView = m_pSwapChain->GetCurrentBackBufferRTV();

  bool bBackBufferFound = false;
  for (auto& backBufferInfo : m_BackbufferTextures)
  {
    if (backBufferInfo.m_pTextureView == pCurrentTextureView)
    {
      bBackBufferFound     = true;
      m_hBackBufferTexture = backBufferInfo.m_hRenderTargetHandle;

      break;
    }
  }

  if (!bBackBufferFound)
  {
    CreateBackBufferInternal(pDeviceD3D11).AssertSuccess();
  }
}

void xiiGALSwapChainD3D11::Present(xiiGALDevice* pDevice)
{
  XII_PROFILE_SCOPE("PresentRenderTarget");

  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  XII_ASSERT_DEV(m_pSwapChain->GetCurrentBackBufferRTV()->GetTexture() == static_cast<xiiGALTextureD3D11*>(pDeviceD3D11->GetTexture(m_hBackBufferTexture))->GetTexture(), "Invalid Swapchain texture. Did you forget to call xiiGALSwapChain::AcquireNextRenderTarget?");

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

xiiResult xiiGALSwapChainD3D11::Resize(xiiGALDevice* pDevice, xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  DestroyBackBufferInternal(pDeviceD3D11);

  // Need to flush dead objects or ResizeBuffers will fail as the backbuffer is still referenced.
  pDeviceD3D11->FlushPendingObjects();

  m_pSwapChain->Resize(newSize.width, newSize.height, xiiDiligentTypeConversions::GetSurfaceTransform(newTransform));

  return CreateBackBufferInternal(pDeviceD3D11);
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Device_Implementation_SwapChainD3D11);
