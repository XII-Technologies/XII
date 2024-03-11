#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>

#include <Diligent/Graphics/GraphicsEngine/interface/Fence.h>

xiiGALCommandQueueVulkan::xiiGALCommandQueueVulkan(xiiGALDeviceVulkan& deviceVulkan, Diligent::IDeviceContext* pDeviceContext) :
  xiiGALCommandQueue(), m_DeviceVulkan(deviceVulkan), m_pContext(pDeviceContext)
{
  {
    Diligent::FenceDesc fenceDescription = {};
    fenceDescription.Name                = m_pContext->GetDesc().Name;
    fenceDescription.Type                = Diligent::FENCE_TYPE_GENERAL;

    m_DeviceVulkan.GetDevice()->CreateFence(fenceDescription, &m_pFence);

    m_uiCompletedFenceValue = 0;
  }
}

xiiGALCommandQueueVulkan::~xiiGALCommandQueueVulkan()
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pFence);
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandQueueVulkan);
