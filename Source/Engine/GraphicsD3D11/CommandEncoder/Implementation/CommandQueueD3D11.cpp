#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/CommandEncoder/CommandQueueD3D11.h>
#include <GraphicsD3D11/Device/DeviceD3D11.h>

#include <Diligent/Graphics/GraphicsEngine/interface/Fence.h>

xiiGALCommandQueueD3D11::xiiGALCommandQueueD3D11(xiiGALDeviceD3D11& deviceD3D11, Diligent::IDeviceContext* pDeviceContext) :
  xiiGALCommandQueue(), m_DeviceD3D11(deviceD3D11), m_pContext(pDeviceContext)
{
  {
    Diligent::FenceDesc fenceDescription = {};
    fenceDescription.Name                = m_pContext->GetDesc().Name;
    fenceDescription.Type                = Diligent::FENCE_TYPE_GENERAL;

    m_DeviceD3D11.GetDevice()->CreateFence(fenceDescription, &m_pFence);

    m_uiCompletedFenceValue = 0;
  }
}

xiiGALCommandQueueD3D11::~xiiGALCommandQueueD3D11()
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pFence);
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_CommandEncoder_Implementation_CommandQueueD3D11);
