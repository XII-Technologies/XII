#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>

xiiGALCommandQueueD3D12::xiiGALCommandQueueD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceD3D12, creationDescription)
{
}

xiiGALCommandQueueD3D12::~xiiGALCommandQueueD3D12()
{
}

void xiiGALCommandQueueD3D12::InitializePlatform()
{
}

void xiiGALCommandQueueD3D12::DeInitializePlatform()
{
}

void xiiGALCommandQueueD3D12::SetDebugNamePlatform(xiiStringView sName)
{
}

xiiUInt64 xiiGALCommandQueueD3D12::WaitForIdle()
{
  return xiiUInt64();
}

xiiGALCommandList* xiiGALCommandQueueD3D12::BeginCommandList()
{
  return nullptr;
}

void xiiGALCommandQueueD3D12::UnbindTextureFromFramebuffer(xiiGALTextureD3D12* pTextureD3D12)
{
}

xiiUInt64 xiiGALCommandQueueD3D12::Submit(xiiGALCommandList* pCommandList, bool bReset)
{
  return 0U;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandQueueD3D12);
