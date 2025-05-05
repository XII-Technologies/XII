#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/States/TilePipelineStateVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTilePipelineStateVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALTilePipelineStateVulkan::xiiGALTilePipelineStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTilePipelineStateCreationDescription& creationDescription) :
  xiiGALTilePipelineState(pDeviceVulkan, creationDescription)
{
}

xiiGALTilePipelineStateVulkan::~xiiGALTilePipelineStateVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipelineCache));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipeline));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipelineLayout));
}

xiiResult xiiGALTilePipelineStateVulkan::InitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_FAILURE;
}

void xiiGALTilePipelineStateVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  if (m_vkPipeline == VK_NULL_HANDLE)
    return;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkPipeline, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_TilePipelineStateVulkan);
