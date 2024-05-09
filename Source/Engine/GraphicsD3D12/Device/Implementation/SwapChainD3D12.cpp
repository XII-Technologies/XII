#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Device/SwapChainD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>

xiiGALSwapChainD3D12::xiiGALSwapChainD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(pDeviceD3D12, creationDescription)
{
}

xiiGALSwapChainD3D12::~xiiGALSwapChainD3D12() = default;

xiiResult xiiGALSwapChainD3D12::InitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainD3D12::DeInitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainD3D12::CreateBackBufferInternal(xiiGALDeviceD3D12* pDeviceD3D12)
{
  return XII_SUCCESS;
}

void xiiGALSwapChainD3D12::DestroyBackBufferInternal(xiiGALDeviceD3D12* pDeviceD3D12)
{
}

void xiiGALSwapChainD3D12::AcquireNextRenderTarget(xiiGALDevice* pDevice)
{
}

void xiiGALSwapChainD3D12::Present(xiiGALDevice* pDevice)
{
}

xiiResult xiiGALSwapChainD3D12::Resize(xiiGALDevice* pDevice, xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Device_Implementation_SwapChainD3D12);
