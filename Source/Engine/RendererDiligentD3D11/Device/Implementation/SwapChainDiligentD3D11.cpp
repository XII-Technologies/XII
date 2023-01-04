#include <RendererDiligentD3D11/RendererDiligentD3D11PCH.h>

#include <RendererDiligentD3D11/Device/DeviceDiligentD3D11.h>
#include <RendererDiligentD3D11/Device/SwapChainDiligentD3D11.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>


void xiiGALSwapChainDiligentD3D11::AcquireNextRenderTarget(xiiGALDevice* pDevice)
{
}

void xiiGALSwapChainDiligentD3D11::PresentRenderTarget(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligentD3D11* pDeviceDiligent = static_cast<xiiGALDeviceDiligentD3D11*>(pDevice);

  m_pSwapChain->Present(m_CurrentPresentMode == xiiGALPresentMode::VSync ? 1 : 0);
}

xiiResult xiiGALSwapChainDiligentD3D11::UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode)
{
  xiiGALDeviceDiligentD3D11* pDeviceDiligent = static_cast<xiiGALDeviceDiligentD3D11*>(pDevice);

  m_CurrentPresentMode = newPresentMode;

  // Need to flush dead objects or ResizeBuffers will fail as the backbuffer is still referenced.
  pDeviceDiligent->FlushDeadObjects();

  m_pSwapChain->Resize(m_WindowDesc.m_pWindow->GetClientAreaSize().width, m_WindowDesc.m_pWindow->GetClientAreaSize().height);

  return CreateBackBufferInternal(pDeviceDiligent);
}

xiiGALSwapChainDiligentD3D11::xiiGALSwapChainDiligentD3D11(const xiiGALWindowSwapChainCreationDescription& Description) :
  xiiGALSwapChainDiligent(Description)
{
}

xiiGALSwapChainDiligentD3D11::~xiiGALSwapChainDiligentD3D11() {}


xiiResult xiiGALSwapChainDiligentD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligentD3D11* pDeviceDiligent = static_cast<xiiGALDeviceDiligentD3D11*>(pDevice);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  Diligent::Win32NativeWindow Window{xiiMinWindows::ToNative(m_WindowDesc.m_pWindow->GetNativeWindowHandle())};
#else
#  error Not Implemented on platform!
#endif

  Diligent::SwapChainDesc SCDesc;
  SCDesc.IsPrimary         = m_WindowDesc.m_bIsPrimarySwapchain;
  SCDesc.Width             = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  SCDesc.Height            = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  SCDesc.ColorBufferFormat = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget;
  SCDesc.DepthBufferFormat = Diligent::TEX_FORMAT_D32_FLOAT;

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  // Enforce double buffering on the UWP Platform
  SCDesc.BufferCount = 2;
#else
  SCDesc.BufferCount = m_WindowDesc.m_bDoubleBuffered ? 2 : 1;
#endif

  const Diligent::RENDER_DEVICE_TYPE& deviceType = pDeviceDiligent->GetDeviceType();

  switch (deviceType)
  {
#if D3D11_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D11:
    {
      Diligent::FullScreenModeDesc FSMDesc;

      auto* pFactoryD3D11 = static_cast<Diligent::IEngineFactoryD3D11*>(pDeviceDiligent->GetFactory().RawPtr());
      pFactoryD3D11->CreateSwapChainD3D11(pDeviceDiligent->GetDevice(), pDeviceDiligent->GetImmediateContext(), SCDesc, FSMDesc, Window, &m_pSwapChain);
    }
    break;
#endif

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (!m_pSwapChain)
  {
    xiiLog::Error("Failed to create device SwapChain");
    return XII_FAILURE;
  }

  return CreateBackBufferInternal(pDeviceDiligent);
}

xiiResult xiiGALSwapChainDiligentD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  return xiiGALSwapChainDiligent::DeInitPlatform(pDevice);
}



XII_STATICLINK_FILE(RendererDiligentD3D11, RendererDiligentD3D11_Device_Implementation_SwapChainDiligentD3D11);
