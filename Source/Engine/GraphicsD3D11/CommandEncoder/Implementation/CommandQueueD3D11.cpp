#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/CommandEncoder/CommandListD3D11.h>
#include <GraphicsD3D11/CommandEncoder/CommandQueueD3D11.h>
#include <GraphicsD3D11/Device/DeviceD3D11.h>

xiiGALCommandQueueD3D11::xiiGALCommandQueueD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceD3D11, creationDescription)
{
  xiiGALCommandListCreationDescription commandListDescription = {.m_QueueType = creationDescription.m_QueueType};
  m_pCommandList                                              = XII_DEFAULT_NEW(xiiGALCommandListD3D11, pDeviceD3D11, commandListDescription);
}

xiiGALCommandQueueD3D11::~xiiGALCommandQueueD3D11()
{
}

xiiGALCommandList* xiiGALCommandQueueD3D11::BeginCommandList()
{
  return m_pCommandList.Borrow();
}

void xiiGALCommandQueueD3D11::SubmitPlatform(xiiGALCommandList* pCommandList)
{
  if (pCommandList == nullptr)
    return;

  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);
  xiiGALCommandListD3D11* pCommandListD3D11 = static_cast<xiiGALCommandListD3D11*>(pCommandList);

  ID3D11DeviceContext* pD3D11CommandList = pCommandListD3D11->GetD3D11CommandList();
  pDeviceD3D11->GetImmediateContext()->FinishCommandList(FALSE, &pD3D11CommandList);
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_CommandEncoder_Implementation_CommandQueueD3D11);
