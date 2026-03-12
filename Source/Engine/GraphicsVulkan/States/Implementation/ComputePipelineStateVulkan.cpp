#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>
#include <GraphicsVulkan/States/ComputePipelineStateVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALComputePipelineStateVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALComputePipelineStateVulkan::xiiGALComputePipelineStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALComputePipelineStateCreationDescription& creationDescription) :
  xiiGALComputePipelineState(std::move(pDeviceVulkan), creationDescription), m_vkPipelineBindPoint(vk::PipelineBindPoint::eCompute)
{
}

xiiGALComputePipelineStateVulkan::~xiiGALComputePipelineStateVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipelineCache));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipeline));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipelineLayout));
}

xiiResult xiiGALComputePipelineStateVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan>                    pDeviceVulkan                    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                                          vkLogicalDevice                  = pDeviceVulkan->GetVulkanLogicalDevice();
  xiiSharedPtr<xiiGALPipelineResourceSignatureVulkan> pPipelineResourceSignatureVulkan = m_Description.m_pPipelineResourceSignature.Downcast<xiiGALPipelineResourceSignatureVulkan>();

  vk::ComputePipelineCreateInfo vkComputePipelineCreateInfo = {};
  vkComputePipelineCreateInfo.pNext                         = nullptr;
  vkComputePipelineCreateInfo.flags                         = {};
  vkComputePipelineCreateInfo.basePipelineHandle            = nullptr; // A pipeline to derive from.
  vkComputePipelineCreateInfo.basePipelineIndex             = -1;      // An index into the pCreateInfos parameter to use as a pipeline to derive from.

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  vkComputePipelineCreateInfo.flags |= vk::PipelineCreateFlagBits::eDisableOptimization;
#endif

  xiiHybridArray<vk::PipelineShaderStageCreateInfo, 1U> vkShaderStages(pDeviceVulkan->GetAllocator());
  {
#define DEFINE_VULKAN_SHADER_IF_EXISTS(shaderType, shaderStageFlagBits)                                              \
  if (xiiSharedPtr<xiiGALShaderVulkan> pShaderVulkan = m_Description.m_p##shaderType.Downcast<xiiGALShaderVulkan>()) \
  {                                                                                                                  \
    vk::PipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};                                          \
    vkPipelineShaderStageCreateInfo.pNext                             = nullptr;                                     \
    vkPipelineShaderStageCreateInfo.flags                             = {};                                          \
    vkPipelineShaderStageCreateInfo.stage                             = shaderStageFlagBits;                         \
    vkPipelineShaderStageCreateInfo.pName                             = "main";                                      \
    vkPipelineShaderStageCreateInfo.module                            = pShaderVulkan->GetVulkanShaderModule();      \
    vkPipelineShaderStageCreateInfo.pSpecializationInfo               = nullptr;                                     \
                                                                                                                     \
    vkShaderStages.PushBack(vkPipelineShaderStageCreateInfo);                                                        \
  }

    DEFINE_VULKAN_SHADER_IF_EXISTS(ComputeShader, vk::ShaderStageFlagBits::eCompute);

#undef DEFINE_VULKAN_SHADER_IF_EXISTS

    XII_ASSERT_DEV(!vkShaderStages.IsEmpty(), "");
  }
  vkComputePipelineCreateInfo.stage = vkShaderStages.PeekBack();

  vk::PipelineLayoutCreateInfo vkPipelineLayoutCreateInfo = {};
  {
    auto pDescriptorSetLayouts = pPipelineResourceSignatureVulkan->GetVulkanDescriptorSetLayouts();

    vkPipelineLayoutCreateInfo.pNext          = nullptr;
    vkPipelineLayoutCreateInfo.flags          = {};
    vkPipelineLayoutCreateInfo.setLayoutCount = pDescriptorSetLayouts.GetCount();
    vkPipelineLayoutCreateInfo.pSetLayouts    = pDescriptorSetLayouts.GetPtr();

    // Build push constant ranges from the pipeline resource signature description.
    const auto&                            pushConstantRanges = pPipelineResourceSignatureVulkan->GetDescription().m_PushConstantRanges;
    xiiDynamicArray<vk::PushConstantRange> vkPushRanges(pDeviceVulkan->GetAllocator());

    if (!pushConstantRanges.IsEmpty())
    {
      vkPushRanges.SetCount(pushConstantRanges.GetCount());

      for (xiiUInt32 i = 0; i < pushConstantRanges.GetCount(); ++i)
      {
        const xiiGALPushConstantRange& range = pushConstantRanges[i];
        vkPushRanges[i].stageFlags           = xiiVulkanTypeConversions::GetShaderStageFlags(range.m_ShaderStages);
        vkPushRanges[i].offset               = range.m_uiOffset;
        vkPushRanges[i].size                 = range.m_uiSize;
      }

      vkPipelineLayoutCreateInfo.pushConstantRangeCount = vkPushRanges.GetCount();
      vkPipelineLayoutCreateInfo.pPushConstantRanges    = vkPushRanges.GetData();
    }
    else
    {
      vkPipelineLayoutCreateInfo.pushConstantRangeCount = 0U;
      vkPipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;
    }

    VK_ASSERT_DEV(vkLogicalDevice.createPipelineLayout(&vkPipelineLayoutCreateInfo, nullptr, &m_vkPipelineLayout, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }
  vkComputePipelineCreateInfo.layout = m_vkPipelineLayout;

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createComputePipelines(m_vkPipelineCache, 1U, &vkComputePipelineCreateInfo, nullptr, &m_vkPipeline, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

void xiiGALComputePipelineStateVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  if (m_vkPipeline == VK_NULL_HANDLE)
    return;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkPipeline, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_ComputePipelineStateVulkan);
