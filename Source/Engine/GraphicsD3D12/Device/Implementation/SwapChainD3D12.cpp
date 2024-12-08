#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Device/SwapChainD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>

#include <VersionHelpers.h>
#include <dxgi1_4.h>

xiiGALSwapChainD3D12::xiiGALSwapChainD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(pDeviceD3D12, creationDescription)
{
}

xiiGALSwapChainD3D12::~xiiGALSwapChainD3D12() = default;

xiiResult xiiGALSwapChainD3D12::InitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  if (CreateDXGISwapChain().Failed())
    return XII_FAILURE;

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_Description.m_pWindow->AddReference();

  return CreateBackBufferInternal(pDeviceD3D12);
}

xiiResult xiiGALSwapChainD3D12::DeInitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  DestroyBackBufferInternal(pDeviceD3D12);

  if (m_pDXGISwapChain3)
  {
    // Full screen swap chains must be switched to windowed mode before destruction.
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
    BOOL bIsFullScreen = FALSE;
    if (FAILED(m_pDXGISwapChain3->GetFullscreenState(&bIsFullScreen, nullptr)))
    {
      xiiLog::Error("Failed to query swap chain full screen state.");
    }
    if (bIsFullScreen == TRUE)
    {
      m_pDXGISwapChain3->SetFullscreenState(FALSE, nullptr);
    }

    XII_GAL_D3D12_RELEASE(m_pDXGISwapChain3);

    m_Description.m_pWindow->RemoveReference();
  }

  return XII_SUCCESS;
}

void xiiGALSwapChainD3D12::SetDebugNamePlatform(xiiStringView sName)
{
  if (m_pDXGISwapChain3 != nullptr)
  {
    xiiStringBuilder sb;
    if (FAILED(m_pDXGISwapChain3->SetPrivateData(WKPDID_D3DDebugObjectName, sName.GetElementCount(), sName.GetData(sb))))
    {
      xiiLog::Error("Failed to set the Direct3D11 swap chain debug name.");
    }
  }
}

xiiResult xiiGALSwapChainD3D12::CreateDXGISwapChain()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  if (m_Description.m_PreTransform != xiiGALSurfaceTransform::Optimal && m_Description.m_PreTransform != xiiGALSurfaceTransform::Identity)
  {
    xiiLog::Warning("The current pre-transform is unsupported by Direct3D swap chains. Use xiiGALSurfaceTransform::Optimal (recommended) or xiiGALSurfaceTransform::Identity.");
  }
  m_DesiredSurfaceTransform    = xiiGALSurfaceTransform::Optimal;
  m_Description.m_PreTransform = xiiGALSurfaceTransform::Identity;

  HWND hNativeWindow = xiiMinWindows::ToNative(m_Description.m_pWindow->GetNativeWindowHandle());

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

  DXGI_FORMAT dxgiColorBufferFormat = xiiD3D12TypeConversions::GetFormat(m_Description.m_ColorBufferFormat);

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
      break;
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

  // Query IDXGIFactory2.
  IDXGIFactory2* pDXGIFactory2 = nullptr;
  XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pDXGIFactory2));

  if (FAILED(pDeviceD3D12->GetDXGIFactory()->GetParent(__uuidof(pDXGIFactory2), reinterpret_cast<void**>(static_cast<IDXGIFactory2**>(&pDXGIFactory2)))))
  {
    xiiLog::Error("Failed to query IDXGIFactory2 factor to create swap chain.");
    return XII_FAILURE;
  }

  // DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT enables querying a waitable object that can be
  // used to synchronize presentation with CPU timeline.
  if (!m_FullScreenMode.m_bIsFullScreen)
  {
    // We do not need pDXGIFactory3 itself, however DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT flag
    // is only supported starting with Windows 8.1, and so is IDXGIFactory3 interface. We query this
    // interface to check Windows 8.1.
    // Note that we can't use IsWindows8Point1OrGreater because unlike IsWindows8OrGreater, it returns
    // false if an application is not manifested for Windows 8.1 or Windows 10, even if the current
    // operating system version is Windows 8.1 or Windows 10.
    IDXGIFactory3* pDXGIFactory3 = nullptr;
    XII_GAL_D3D12_RELEASE(pDXGIFactory3);

    if (SUCCEEDED(pDXGIFactory2->QueryInterface(__uuidof(pDXGIFactory3), reinterpret_cast<void**>(static_cast<IDXGIFactory3**>(&pDXGIFactory3)))))
    {
      swapChainDescription.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    }
  }

  xiiGALCommandQueueD3D12* pCommandQueueD3D12 = static_cast<xiiGALCommandQueueD3D12*>(pDeviceD3D12->GetDefaultCommandQueue(xiiGALCommandQueueType::Graphics, false));
  IDXGISwapChain1*         pDXGISwapChain1    = nullptr;
  XII_GAL_D3D12_RELEASE(pDXGISwapChain1);

  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDescription = {};

  fullScreenDescription.Windowed                = D3D12_BOOL(!m_FullScreenMode.m_bIsFullScreen);
  fullScreenDescription.RefreshRate.Numerator   = m_FullScreenMode.m_uiRefreshRateNumerator;
  fullScreenDescription.RefreshRate.Denominator = m_FullScreenMode.m_uiRefreshRateDenominator;
  fullScreenDescription.Scaling                 = xiiD3D12TypeConversions::GetScalingMode(m_FullScreenMode.m_ScalingMode);
  fullScreenDescription.ScanlineOrdering        = xiiD3D12TypeConversions::GetScanLineOrder(m_FullScreenMode.m_ScanLineOrder);

  HRESULT hResult = pDXGIFactory2->CreateSwapChainForHwnd(pCommandQueueD3D12->GetD3D12CommandQueue(), hNativeWindow, &swapChainDescription, &fullScreenDescription, nullptr, &pDXGISwapChain1);
  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to create the DXGI Swap Chain: {}", xiiHRESULTtoString(hResult));
    return XII_FAILURE;
  }

  {
    // This is silly, but IDXGIFactory used for MakeWindowAssociation must be retrieved via
    // calling IDXGISwapchain::GetParent first, otherwise it won't work
    // https://www.gamedev.net/forums/topic/634235-dxgidisabling-altenter/?do=findComment&comment=4999990
    IDXGIFactory1* pFactoryFromSC = nullptr;
    XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pFactoryFromSC));

    if (SUCCEEDED(pDXGISwapChain1->GetParent(__uuidof(pFactoryFromSC), (void**)&pFactoryFromSC)))
    {
      // Do not allow the swap chain to handle Alt+Enter.
      pFactoryFromSC->MakeWindowAssociation(hNativeWindow, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
    }
  }

  if (FAILED(pDXGISwapChain1->QueryInterface(__uuidof(m_pDXGISwapChain3), reinterpret_cast<void**>(static_cast<IDXGISwapChain3**>(&m_pDXGISwapChain3)))))
  {
    xiiLog::Error("Failed to query the required swap chain interface IDXGISwapChain3.");
    return XII_FAILURE;
  }

  if ((swapChainDescription.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) != 0)
  {
    IDXGISwapChain2* pDXGISwapChain2 = nullptr;
    XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pDXGISwapChain2));

    if (SUCCEEDED(pDXGISwapChain1->QueryInterface(__uuidof(pDXGISwapChain2), reinterpret_cast<void**>(static_cast<IDXGISwapChain2**>(&pDXGISwapChain2)))))
    {
      // IMPORTANT: SetMaximumFrameLatency must be called BEFORE GetFrameLatencyWaitableObject!
      pDXGISwapChain2->SetMaximumFrameLatency(m_uiMaximumFrameLatency);

      m_FrameLatencyWaitableObject = pDXGISwapChain2->GetFrameLatencyWaitableObject();
      XII_ASSERT_RELEASE(m_FrameLatencyWaitableObject != NULL, "Swap Chain Waitable object must not be null.");
    }
  }
  else
  {
    m_FrameLatencyWaitableObject = NULL;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainD3D12::UpdateSwapChain(bool bCreateNew)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  // When switching to full screen mode, WM_SIZE is send to the window
  // and Resize() is called before the new swap chain is created
  if (!m_pDXGISwapChain3)
    return XII_SUCCESS;

  xiiGALCommandQueueD3D12* pCommandQueueD3D12 = static_cast<xiiGALCommandQueueD3D12*>(pDeviceD3D12->GetDefaultCommandQueue(xiiGALCommandQueueType::Graphics, false));
  // Flush command queue?

  {
    // Reset command list swapchain references or ResizeBuffers will fail as the backbuffer is still referenced.
    for (xiiUInt32 i = 0; i < m_BackBufferTextures.GetCount(); ++i)
    {
      xiiGALTextureD3D12* pTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pDeviceD3D12->GetTexture(m_BackBufferTextures[i]));

      pCommandQueueD3D12->UnbindTextureFromFramebuffer(pTextureD3D12);
    }

    DestroyBackBufferInternal(pDeviceD3D12);

    // Need to flush pending deletion or ResizeBuffers will fail as the backbuffer is still referenced.
    pDeviceD3D12->WaitIdle();

    if (bCreateNew)
    {
      XII_GAL_D3D12_RELEASE(m_pDXGISwapChain3);

      CreateDXGISwapChain().AssertSuccess();
    }
    else
    {
      DXGI_SWAP_CHAIN_DESC swapChainDescription;
      memset(&swapChainDescription, 0, sizeof(swapChainDescription));
      m_pDXGISwapChain3->GetDesc(&swapChainDescription);

      HRESULT hResult = m_pDXGISwapChain3->ResizeBuffers(swapChainDescription.BufferCount, m_Description.m_Resolution.width, m_Description.m_Resolution.height, swapChainDescription.BufferDesc.Format, swapChainDescription.Flags);
      if (FAILED(hResult))
      {
        xiiLog::Error("Failed to resize the DXGI swap chain: {}", xiiHRESULTtoString(hResult));
        return XII_FAILURE;
      }
    }
  }
  return CreateBackBufferInternal(pDeviceD3D12);
}

xiiResult xiiGALSwapChainD3D12::CreateBackBufferInternal(xiiGALDeviceD3D12* pDeviceD3D12)
{
  return XII_SUCCESS;
}

void xiiGALSwapChainD3D12::DestroyBackBufferInternal(xiiGALDeviceD3D12* pDeviceD3D12)
{
  for (xiiUInt32 i = 0; i < m_BackBufferTextures.GetCount(); ++i)
  {
    pDeviceD3D12->DestroyTexture(m_BackBufferTextures[i]);
    m_BackBufferTextures[i].Invalidate();
  }
  m_hBackBufferTexture.Invalidate();
}

void xiiGALSwapChainD3D12::WaitForFrame()
{
  // https://docs.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains#step-4-wait-before-rendering-each-frame
  if (m_FrameLatencyWaitableObject != NULL)
  {
    // 0.5 second timeout (shouldn't ever occur)
    DWORD uiResult = WaitForSingleObjectEx(m_FrameLatencyWaitableObject, 500, true);

    if (uiResult != WAIT_OBJECT_0)
    {
      if (uiResult == WAIT_TIMEOUT)
      {
        xiiLog::Error("Timeout elapsed while waiting for the frame waitable object. This is a strong indication of a synchronization error.");
      }
      else
      {
        xiiLog::Error("Waiting for the frame waitable object failed. This is a strong indication of a synchronization error.");
      }
    }
  }
}

void xiiGALSwapChainD3D12::AcquireNextRenderTarget()
{
}

void xiiGALSwapChainD3D12::Present()
{
  XII_PROFILE_SCOPE("PresentRenderTarget");

  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

#if 0
  if (!m_hActualBackBufferTexture.IsInvalidated())
  {
    if (auto pQueue = pDeviceD3D12->GetGraphicsQueue())
    {
      auto pCommandList = pQueue->BeginCommandList();

      pCommandList->CopyTexture(m_hBackBufferTexture, m_hActualBackBufferTexture);

      pQueue->Submit(pCommandList);
    }
  }
#endif

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

  // In contrast to MSDN sample, we wait for the frame as late as possible - right
  // before presenting.
  // https://docs.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains#step-4-wait-before-rendering-each-frame
  WaitForFrame();

  HRESULT hResult = m_pDXGISwapChain3->Present(uiSyncInterval, 0);
  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to present to swap chain: {}", xiiHRESULTtoString(hResult));
  }
}

xiiResult xiiGALSwapChainD3D12::Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform)
{
  if (newTransform != xiiGALSurfaceTransform::Optimal && newTransform != xiiGALSurfaceTransform::Identity)
  {
    xiiLog::Warning("The current pre-transform is unsupported by Direct3D swap chains. Use xiiGALSurfaceTransform::Optimal (recommended) or xiiGALSurfaceTransform::Identity.");
  }
  newTransform = xiiGALSurfaceTransform::Optimal;

  if (newSize.HasNonZeroArea() && (newSize.width != m_Description.m_Resolution.width || newSize.height != m_Description.m_Resolution.height || m_DesiredSurfaceTransform != newTransform))
  {
    m_Description.m_Resolution = newSize;

    if (UpdateSwapChain(false).Succeeded())
    {
      xiiLog::Info("Resized swapchain to {}x{}.", m_Description.m_Resolution.width, m_Description.m_Resolution.height);
    }
  }
  return XII_SUCCESS;
}

void xiiGALSwapChainD3D12::SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode)
{
  if (m_pDXGISwapChain3)
  {
    // If we are already in fullscreen mode, we need to switch to windowed mode first,
    // because a swap chain must be in windowed mode when it is released.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
    if (m_FullScreenMode.m_bIsFullScreen)
    {
      m_pDXGISwapChain3->SetFullscreenState(FALSE, nullptr);
    }

    m_FullScreenMode.m_bIsFullScreen            = true;
    m_FullScreenMode.m_uiRefreshRateNumerator   = displayMode.m_uiRefreshRateNumerator;
    m_FullScreenMode.m_uiRefreshRateDenominator = displayMode.m_uiRefreshRateDenominator;
    m_FullScreenMode.m_ScalingMode              = displayMode.m_ScalingMode;
    m_FullScreenMode.m_ScanLineOrder            = displayMode.m_ScanLineOrder;

    m_Description.m_Resolution = displayMode.m_Resolution;
    if (displayMode.m_TextureFormat != xiiGALTextureFormat::Unknown)
    {
      m_Description.m_ColorBufferFormat = displayMode.m_TextureFormat;
    }

    UpdateSwapChain(true).AssertSuccess();
  }
}

void xiiGALSwapChainD3D12::SetWindowedMode()
{
  if (m_FullScreenMode.m_bIsFullScreen)
  {
    m_FullScreenMode.m_bIsFullScreen = false;

    m_pDXGISwapChain3->SetFullscreenState(FALSE, nullptr);
  }
}

void xiiGALSwapChainD3D12::SetMaximumFrameLatency(xiiUInt32 uiMaxLatency)
{
  if (m_uiMaximumFrameLatency == uiMaxLatency)
    return;

  m_uiMaximumFrameLatency = uiMaxLatency;

  if (m_FrameLatencyWaitableObject != NULL)
  {
    // A swap chain must be in windowed mode when it is released.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
    if (m_FullScreenMode.m_bIsFullScreen)
    {
      // SetFullscreenState(FALSE) calls Resize that initializes Width and Height
      // with the window size. We need to save current values and restore them.
      xiiSizeU32 size = m_Description.m_Resolution;

      m_pDXGISwapChain3->SetFullscreenState(FALSE, nullptr);

      m_Description.m_Resolution = size;
    }

    // Destroying the swap chain and creating a new one is the only reliable way to
    // change the maximum frame latency of a waitable swap chain.
    UpdateSwapChain(true).AssertSuccess();
  }
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Device_Implementation_SwapChainD3D12);
