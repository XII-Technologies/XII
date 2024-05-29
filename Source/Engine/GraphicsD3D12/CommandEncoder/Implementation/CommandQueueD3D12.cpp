#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>

xiiGALCommandQueueD3D12::xiiGALCommandQueueD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceD3D12, creationDescription)
{
  xiiGALCommandListCreationDescription commandListDescription = {.m_QueueType = m_Description.m_QueueType};
  m_pDefaultCommandList                                       = XII_DEFAULT_NEW(xiiGALCommandListD3D12, pDeviceD3D12, commandListDescription);
}

xiiGALCommandQueueD3D12::~xiiGALCommandQueueD3D12()
{
  m_pDefaultCommandList.Clear();
}

xiiResult xiiGALCommandQueueD3D12::InitPlatform()
{
  return XII_FAILURE;
}

xiiResult xiiGALCommandQueueD3D12::DeInitPlatform()
{
  return XII_FAILURE;
}

void xiiGALCommandQueueD3D12::SetDebugNamePlatform(xiiStringView sName)
{
}

xiiGALCommandList* xiiGALCommandQueueD3D12::BeginCommandList(xiiStringView sScopeName)
{
  return m_pDefaultCommandList.Borrow();
}

void xiiGALCommandQueueD3D12::UnbindTextureFromFramebuffer(xiiGALTextureD3D12* pTextureD3D12)
{
}

xiiUInt64 xiiGALCommandQueueD3D12::SubmitPlatform(xiiGALCommandList* pCommandList, bool bReset)
{
  return 0U;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandQueueD3D12);
