#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>

xiiGALCommandQueueVulkan::xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceVulkan, creationDescription)
{
  xiiGALCommandListCreationDescription commandListDescription = {.m_QueueType = m_Description.m_QueueType};
  m_pDefaultCommandList                                       = XII_DEFAULT_NEW(xiiGALCommandListVulkan, pDeviceVulkan, this, commandListDescription);
}

xiiGALCommandQueueVulkan::~xiiGALCommandQueueVulkan()
{
  m_pDefaultCommandList.Clear();
}

xiiResult xiiGALCommandQueueVulkan::InitPlatform()
{
  return XII_FAILURE;
}

xiiResult xiiGALCommandQueueVulkan::DeInitPlatform()
{
  return XII_FAILURE;
}

void xiiGALCommandQueueVulkan::SetDebugNamePlatform(xiiStringView sName)
{
}

xiiGALCommandList* xiiGALCommandQueueVulkan::BeginCommandList()
{
  return m_pDefaultCommandList.Borrow();
}

void xiiGALCommandQueueVulkan::UnbindTextureFromFramebuffer(xiiGALTextureVulkan* pTextureVulkan)
{
}

xiiUInt64 xiiGALCommandQueueVulkan::Submit(xiiGALCommandList* pCommandList, bool bReset)
{
  return 0U;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandQueueVulkan);
