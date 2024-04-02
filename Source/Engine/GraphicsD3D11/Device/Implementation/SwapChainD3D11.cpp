#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Device/SwapChainD3D11.h>
#include <GraphicsD3D11/Resources/TextureD3D11.h>

#include <GraphicsD3D11/Utilities/D3D11TypeConversions.h>

#include <VersionHelpers.h>
#include <dxgi1_2.h>

xiiGALSwapChainD3D11::xiiGALSwapChainD3D11(const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(creationDescription)
{
}

xiiGALSwapChainD3D11::~xiiGALSwapChainD3D11() = default;

xiiResult xiiGALSwapChainD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  if (CreateDXGISwapChain().Failed())
    return XII_FAILURE;

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_Description.m_pWindow->AddReference();

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  DestroyBackBufferInternal(pDeviceD3D11);

  if (m_pSwapChain)
  {
    // Full screen swap chains must be switched to windowed mode before destruction.
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
    BOOL bIsFullScreen = FALSE;
    if (SUCCEEDED(m_pSwapChain->GetFullscreenState(&bIsFullScreen, nullptr)))
    {
      m_pSwapChain->SetFullscreenState(FALSE, nullptr);
    }
    else
    {
      xiiLog::Error("Failed to query swap chain full screen state.");
    }

    XII_GAL_D3D11_RELEASE(m_pSwapChain);

    m_Description.m_pWindow->RemoveReference();
  }

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainD3D11::CreateDXGISwapChain()
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

  if (m_Description.m_PreTransform != xiiGALSurfaceTransform::Optimal && m_Description.m_PreTransform != xiiGALSurfaceTransform::Identity)
  {
    xiiLog::Warning("The current pre-transform is unsupported by Direct3D swap chains. Use xiiGALSurfaceTransform::Optimal (recommended) or xiiGALSurfaceTransform::Identity.");
  }
  m_DesiredSurfaceTransform    = xiiGALSurfaceTransform::Optimal;
  m_Description.m_PreTransform = xiiGALSurfaceTransform::Identity;

  HWND hNativeWindow = xiiMinWindows::ToNative(m_Description.m_pWindow->GetNativeWindowHandle());

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  if (!m_Description.m_Resolution.HasNonZeroArea())
  {
    RECT rect;
    if (m_FullScreenMode.m_bIsFullScreen)
    {
      const HWND hDesktop = GetDesktopWindow();
      GetWindowRect(hDesktop, &rect);
    }
    else
    {
      GetClientRect(hNativeWindow, &rect);
    }
    m_Description.m_Resolution = xiiSizeU32(rect.right - rect.left, rect.bottom - rect.top);
  }
#endif

  DXGI_FORMAT dxgiColorBufferFormat = pDeviceD3D11->GetFormatLookupTable().GetFormatInfo(m_Description.m_ColorBufferFormat).m_eRenderTarget;

  DXGI_SWAP_CHAIN_DESC1 swapChainDescription = {};
  swapChainDescription.Width                 = m_Description.m_Resolution.width;
  swapChainDescription.Height                = m_Description.m_Resolution.height;
  swapChainDescription.Stereo                = FALSE;

  // Multi-sampled swap chains are not supported anymore. CreateSwapChainForHwnd() fails when sample count is not 1 for any swap effect.
  swapChainDescription.SampleDesc = {.Count = 1U, .Quality = 0U};

  // Flip model swapchains (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL and DXGI_SWAP_EFFECT_FLIP_DISCARD) are more efficient, and only support the following Formats:
  // - DXGI_FORMAT_R16G16B16A16_FLOAT
  // - DXGI_FORMAT_B8G8R8A8_UNORM
  // - DXGI_FORMAT_R8G8B8A8_UNORM
  // - DXGI_FORMAT_R10G10B10A2_UNORM
  // So, we can't have sRGB swap chain formats (we can have non-srgb swap chain with srgb texture view).
  switch (dxgiColorBufferFormat)
  {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      swapChainDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      break;

    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      swapChainDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
      break;

    default:
      swapChainDescription.Format = dxgiColorBufferFormat;
  }

  XII_ASSERT_DEV(!m_Description.m_Usage.IsNoFlagSet(), "No swap chain usage flags are set!");

  if (m_Description.m_Usage.IsSet(xiiGALSwapChainUsageFlags::RenderTarget))
  {
    swapChainDescription.BufferUsage |= DXGI_USAGE_RENDER_TARGET_OUTPUT;
  }
  if (m_Description.m_Usage.IsAnySet(xiiGALSwapChainUsageFlags::ShaderResource | xiiGALSwapChainUsageFlags::InputAttachment))
  {
    swapChainDescription.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
  }

  swapChainDescription.BufferCount = m_Description.m_uiBufferCount;
  swapChainDescription.Scaling     = DXGI_SCALING_NONE;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
  // DXGI_SCALING_NONE is supported starting with Windows 8
  if (!IsWindows8OrGreater())
    swapChainDescription.Scaling = DXGI_SCALING_STRETCH;
#endif

  // DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL is the flip presentation model, where the contents of the back
  // buffer is preserved after the call to Present. This flag cannot be used with multisampling.
  // The only swap effect that supports multisampling is DXGI_SWAP_EFFECT_DISCARD.
  // Windows Store apps must use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL or DXGI_SWAP_EFFECT_FLIP_DISCARD.
  swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  swapChainDescription.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; //  Transparency behavior is not specified

  // DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH enables an application to switch modes by calling
  // IDXGISwapChain::ResizeTarget(). When switching from windowed to fullscreen mode, the display
  // mode (or monitor resolution) will be changed to match the dimensions of the application window.
  swapChainDescription.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  // Create DXGI Factory.
  IDXGIFactory2* pDXGIFactory = nullptr;
  if (FAILED(CreateDXGIFactory1(__uuidof(pDXGIFactory), reinterpret_cast<void**>(static_cast<IDXGIFactory2**>(&pDXGIFactory)))))
  {
    xiiLog::Error("Failed to create DXGI factory.");
    return XII_FAILURE;
  }

  IDXGISwapChain1* pSwapChain1 = nullptr;

  XII_SCOPE_EXIT(XII_GAL_D3D11_RELEASE(pSwapChain1); XII_GAL_D3D11_RELEASE(pDXGIFactory););

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDescription = {};

  fullScreenDescription.Windowed                = D3D11_BOOL(m_FullScreenMode.m_bIsFullScreen);
  fullScreenDescription.RefreshRate.Numerator   = m_FullScreenMode.m_uiRefreshRateNumerator;
  fullScreenDescription.RefreshRate.Denominator = m_FullScreenMode.m_uiRefreshRateDenominator;
  fullScreenDescription.Scaling                 = xiiD3D11TypeConversions::GetScalingMode(m_FullScreenMode.m_ScalingMode);
  fullScreenDescription.ScanlineOrdering        = xiiD3D11TypeConversions::GetScanLineOrder(m_FullScreenMode.m_ScanLineOrder);

  if (FAILED(pDXGIFactory->CreateSwapChainForHwnd(pDeviceD3D11->GetD3D11Device(), hNativeWindow, &swapChainDescription, &fullScreenDescription, nullptr, &pSwapChain1)))
  {
    xiiLog::Error("Failed to create the DXGI Swap Chain.");
    return XII_FAILURE;
  }

  {
    // This is silly, but IDXGIFactory used for MakeWindowAssociation must be retrieved via
    // calling IDXGISwapchain::GetParent first, otherwise it won't work
    // https://www.gamedev.net/forums/topic/634235-dxgidisabling-altenter/?do=findComment&comment=4999990
    IDXGIFactory1* pFactoryFromSC;
    if (SUCCEEDED(pSwapChain1->GetParent(__uuidof(pFactoryFromSC), (void**)&pFactoryFromSC)))
    {
      // Do not allow the swap chain to handle Alt+Enter.
      pFactoryFromSC->MakeWindowAssociation(hNativeWindow, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
    }
    XII_GAL_D3D11_RELEASE(pFactoryFromSC);
  }
#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  if (m_FullScreenMode.m_bIsFullScreen)
  {
    xiiLog::Warning("UWP applications do not support full screen mode.");
  }

  if (FAILED(pDXGIFactory->CreateSwapChainForCoreWindow(pDeviceD3D11->GetD3D11Device(), reinterpret_cast<IUnknown*>(m_Description.m_pWindow->GetNativeWindowHandle()), &swapChainDescription, nullptr, &pSwapChain1)))
  {
    xiiLog::Error("Failed to create the DXGI Swap Chain.");
    return XII_FAILURE;
  }
#endif

  if (FAILED(pSwapChain1->QueryInterface(&m_pSwapChain)))
  {
    xiiLog::Error("Failed to query the required swap chain interface.");
    return XII_FAILURE;
  }
  return CreateBackBufferInternal(pDeviceD3D11);
}

xiiResult xiiGALSwapChainD3D11::UpdateSwapChain(bool bCreateNew)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

  // When switching to full screen mode, WM_SIZE is send to the window
  // and Resize() is called before the new swap chain is created
  if (!m_pSwapChain)
    return XII_SUCCESS;

  if (ID3D11DeviceContext* pContextD3D11 = pDeviceD3D11->GetImmediateContext())
  {
    DestroyBackBufferInternal(pDeviceD3D11);

    // Need to flush pending deletion or ResizeBuffers will fail as the backbuffer is still referenced.
    pDeviceD3D11->FlushPendingObjects();

    if (bCreateNew)
    {
      XII_GAL_D3D11_RELEASE(m_pSwapChain);

      // Only one flip presentation model swap chain can be associated with an HWND.
      // We must make sure that the swap chain is actually released by D3D11 before creating a new one.
      // To force the destruction, we need to ensure no views are bound to pipeline state, and then call Flush
      // on the immediate context. Destruction must be forced before calling IDXGIFactory2::CreateSwapChainForHwnd(), or
      // IDXGIFactory2::CreateSwapChainForCoreWindow() again to create a new swap chain.
      // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476425(v=vs.85).aspx#Defer_Issues_with_Flip
      pContextD3D11->Flush();

      CreateDXGISwapChain().AssertSuccess();
    }
    else
    {
      DXGI_SWAP_CHAIN_DESC swapChainDescription;
      memset(&swapChainDescription, 0, sizeof(swapChainDescription));
      m_pSwapChain->GetDesc(&swapChainDescription);

      if (FAILED(m_pSwapChain->ResizeBuffers(swapChainDescription.BufferCount, m_Description.m_Resolution.width, m_Description.m_Resolution.height, swapChainDescription.BufferDesc.Format, swapChainDescription.Flags)))
      {
        xiiLog::Error("Failed to resize the DXGI swap chain.");
        return XII_FAILURE;
      }

      // Call flush to release resources.
      pContextD3D11->Flush();
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainD3D11::CreateBackBufferInternal(xiiGALDeviceD3D11* pDeviceD3D11)
{
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

  if (newTransform != xiiGALSurfaceTransform::Optimal && newTransform != xiiGALSurfaceTransform::Identity)
  {
    xiiLog::Warning("The current pre-transform is unsupported by Direct3D swap chains. Use xiiGALSurfaceTransform::Optimal (recommended) or xiiGALSurfaceTransform::Identity.");
  }
  newTransform = xiiGALSurfaceTransform::Optimal;

  if (newSize.HasNonZeroArea() && (newSize.width != m_Description.m_Resolution.width || newSize.height != m_Description.m_Resolution.height || m_DesiredSurfaceTransform != newTransform))
  {
    if (UpdateSwapChain(false).Succeeded())
    {
      xiiLog::Info("Resized swapchain to {}x{}.", m_Description.m_Resolution.width, m_Description.m_Resolution.height);
    }
  }
  return XII_SUCCESS;
}

void xiiGALSwapChainD3D11::SetMaximumFrameLatency(xiiUInt32 uiMaxLatency)
{
  m_uiMaximumFrameLatency = uiMaxLatency;

  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

  IDXGIDevice1* pDXGIDevice = nullptr;
  if (SUCCEEDED(pDeviceD3D11->GetD3D11Device()->QueryInterface(__uuidof(pDXGIDevice), reinterpret_cast<void**>(static_cast<IDXGIDevice1**>(&pDXGIDevice)))))
  {
    if (FAILED(pDXGIDevice->SetMaximumFrameLatency(m_uiMaximumFrameLatency)))
    {
      xiiLog::Error("Failed to set the maximum frame latency for DXGI device.");
    }
  }
  else
  {
    xiiLog::Error("Failed to query IDXGIDevice1 interface from Direct3D11 device.");
  }
  XII_GAL_D3D11_RELEASE(pDXGIDevice);
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Device_Implementation_SwapChainD3D11);
