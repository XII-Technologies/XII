#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>
#include <GraphicsVulkan/States/RayTracingPipelineStateVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALRayTracingPipelineStateVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALRayTracingPipelineStateVulkan::xiiGALRayTracingPipelineStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALRayTracingPipelineStateCreationDescription& creationDescription) :
  xiiGALRayTracingPipelineState(std::move(pDeviceVulkan), creationDescription), m_vkPipelineBindPoint(vk::PipelineBindPoint::eRayTracingKHR)
{
}

xiiGALRayTracingPipelineStateVulkan::~xiiGALRayTracingPipelineStateVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipelineCache));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipeline));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipelineLayout));
}

xiiResult xiiGALRayTracingPipelineStateVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan>                    pDeviceVulkan                    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                                          vkLogicalDevice                  = pDeviceVulkan->GetVulkanLogicalDevice();
  xiiSharedPtr<xiiGALPipelineResourceSignatureVulkan> pPipelineResourceSignatureVulkan = m_Description.m_pPipelineResourceSignature.Downcast<xiiGALPipelineResourceSignatureVulkan>();

  vk::RayTracingPipelineCreateInfoKHR vkRayTracingPipelineCreateInfo = {};
  vkRayTracingPipelineCreateInfo.pNext                               = nullptr;
  vkRayTracingPipelineCreateInfo.flags                               = {};
  vkRayTracingPipelineCreateInfo.basePipelineHandle                  = nullptr; // A pipeline to derive from.
  vkRayTracingPipelineCreateInfo.basePipelineIndex                   = -1;      // An index into the pCreateInfos parameter to use as a pipeline to derive from.

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  vkRayTracingPipelineCreateInfo.flags |= vk::PipelineCreateFlagBits::eDisableOptimization;
#endif

  XII_ASSERT_NOT_IMPLEMENTED;

  // VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createRayTracingPipelinesKHR(VK_NULL_HANDLE, m_vkPipelineCache, 1U, &vkRayTracingPipelineCreateInfo, nullptr, &m_vkPipeline, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_FAILURE;
}

void xiiGALRayTracingPipelineStateVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  if (m_vkPipeline == VK_NULL_HANDLE)
    return;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkPipeline, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_PipelineStateVulkan);
