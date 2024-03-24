#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>

#include <Diligent/Graphics/GraphicsEngine/interface/Fence.h>

xiiGALCommandQueueD3D12::xiiGALCommandQueueD3D12(xiiGALDeviceD3D12& deviceD3D12, Diligent::IDeviceContext* pDeviceContext) :
  xiiGALCommandQueue(), m_DeviceD3D12(deviceD3D12), m_pContext(pDeviceContext)
{
  {
    Diligent::FenceDesc fenceDescription = {};
    fenceDescription.Name                = m_pContext->GetDesc().Name;
    fenceDescription.Type                = Diligent::FENCE_TYPE_GENERAL;

    m_DeviceD3D12.GetDevice()->CreateFence(fenceDescription, &m_pFence);

    m_uiCompletedFenceValue = 0;
  }
}

xiiGALCommandQueueD3D12::~xiiGALCommandQueueD3D12()
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pFence);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandQueueD3D12);
