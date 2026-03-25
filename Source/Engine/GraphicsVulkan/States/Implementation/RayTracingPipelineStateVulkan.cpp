#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>
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

  if (pPipelineResourceSignatureVulkan == nullptr)
  {
    return XII_FAILURE;
  }

  xiiDynamicArray<vk::PipelineShaderStageCreateInfo> vkShaderStages(pDeviceVulkan->GetAllocator());
  xiiDynamicArray<vk::RayTracingShaderGroupCreateInfoKHR> vkShaderGroups(pDeviceVulkan->GetAllocator());

  auto AddShaderStage = [&](xiiSharedPtr<xiiGALShader> pShader, xiiBitflags<xiiGALShaderType> expectedShaderType) -> xiiUInt32
  {
    if (pShader == nullptr)
    {
      return VK_SHADER_UNUSED_KHR;
    }

    xiiSharedPtr<xiiGALShaderVulkan> pShaderVulkan = pShader.Downcast<xiiGALShaderVulkan>();
    if (pShaderVulkan == nullptr)
    {
      return VK_SHADER_UNUSED_KHR;
    }

    if (!pShaderVulkan->GetDescription().m_ShaderType.IsSet(expectedShaderType))
    {
      return VK_SHADER_UNUSED_KHR;
    }

    vk::PipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};
    vkPipelineShaderStageCreateInfo.pNext                             = nullptr;
    vkPipelineShaderStageCreateInfo.flags                             = {};
    vkPipelineShaderStageCreateInfo.stage                             = xiiVulkanTypeConversions::GetShaderStageFlags(expectedShaderType);
    vkPipelineShaderStageCreateInfo.pName                             = "main";
    vkPipelineShaderStageCreateInfo.module                            = pShaderVulkan->GetVulkanShaderModule();
    vkPipelineShaderStageCreateInfo.pSpecializationInfo               = nullptr;

    const xiiUInt32 uiShaderStageIndex = vkShaderStages.GetCount();
    vkShaderStages.PushBack(vkPipelineShaderStageCreateInfo);
    return uiShaderStageIndex;
  };

  for (const xiiGALRayTracingGeneralShaderGroupDescription& groupDescription : m_Description.m_GeneralShaders)
  {
    const xiiUInt32 uiRayGenIndex   = AddShaderStage(groupDescription.m_pShader, xiiGALShaderType::RayGeneration);
    const xiiUInt32 uiRayMissIndex  = AddShaderStage(groupDescription.m_pShader, xiiGALShaderType::RayMiss);
    const xiiUInt32 uiCallableIndex = AddShaderStage(groupDescription.m_pShader, xiiGALShaderType::Callable);

    const bool bHasValidShader = uiRayGenIndex != VK_SHADER_UNUSED_KHR || uiRayMissIndex != VK_SHADER_UNUSED_KHR || uiCallableIndex != VK_SHADER_UNUSED_KHR;
    if (!bHasValidShader)
    {
      return XII_FAILURE;
    }

    vk::RayTracingShaderGroupCreateInfoKHR vkShaderGroupCreateInfo = {};
    vkShaderGroupCreateInfo.pNext                                  = nullptr;
    vkShaderGroupCreateInfo.type                                   = vk::RayTracingShaderGroupTypeKHR::eGeneral;
    vkShaderGroupCreateInfo.generalShader                          = uiRayGenIndex != VK_SHADER_UNUSED_KHR ? uiRayGenIndex : (uiRayMissIndex != VK_SHADER_UNUSED_KHR ? uiRayMissIndex : uiCallableIndex);
    vkShaderGroupCreateInfo.closestHitShader                       = VK_SHADER_UNUSED_KHR;
    vkShaderGroupCreateInfo.anyHitShader                           = VK_SHADER_UNUSED_KHR;
    vkShaderGroupCreateInfo.intersectionShader                     = VK_SHADER_UNUSED_KHR;

    vkShaderGroups.PushBack(vkShaderGroupCreateInfo);
  }

  for (const xiiGALRayTracingTriangleHitShaderGroupDescription& groupDescription : m_Description.m_TriangleHitShaders)
  {
    const xiiUInt32 uiClosestHitIndex = AddShaderStage(groupDescription.m_pClosestHitShader, xiiGALShaderType::RayClosestHit);
    const xiiUInt32 uiAnyHitIndex     = AddShaderStage(groupDescription.m_pAnyHitShader, xiiGALShaderType::RayAnyHit);

    if (uiClosestHitIndex == VK_SHADER_UNUSED_KHR)
    {
      return XII_FAILURE;
    }

    vk::RayTracingShaderGroupCreateInfoKHR vkShaderGroupCreateInfo = {};
    vkShaderGroupCreateInfo.pNext                                  = nullptr;
    vkShaderGroupCreateInfo.type                                   = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
    vkShaderGroupCreateInfo.generalShader                          = VK_SHADER_UNUSED_KHR;
    vkShaderGroupCreateInfo.closestHitShader                       = uiClosestHitIndex;
    vkShaderGroupCreateInfo.anyHitShader                           = uiAnyHitIndex;
    vkShaderGroupCreateInfo.intersectionShader                     = VK_SHADER_UNUSED_KHR;

    vkShaderGroups.PushBack(vkShaderGroupCreateInfo);
  }

  for (const xiiGALRayTracingProceduralHitShaderGroupDescription& groupDescription : m_Description.m_ProceduralHitShaders)
  {
    const xiiUInt32 uiIntersectionIndex = AddShaderStage(groupDescription.m_pIntersectionShader, xiiGALShaderType::RayIntersection);
    const xiiUInt32 uiClosestHitIndex   = AddShaderStage(groupDescription.m_pClosestHitShader, xiiGALShaderType::RayClosestHit);
    const xiiUInt32 uiAnyHitIndex       = AddShaderStage(groupDescription.m_pAnyHitShader, xiiGALShaderType::RayAnyHit);

    if (uiIntersectionIndex == VK_SHADER_UNUSED_KHR)
    {
      return XII_FAILURE;
    }

    vk::RayTracingShaderGroupCreateInfoKHR vkShaderGroupCreateInfo = {};
    vkShaderGroupCreateInfo.pNext                                  = nullptr;
    vkShaderGroupCreateInfo.type                                   = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup;
    vkShaderGroupCreateInfo.generalShader                          = VK_SHADER_UNUSED_KHR;
    vkShaderGroupCreateInfo.closestHitShader                       = uiClosestHitIndex;
    vkShaderGroupCreateInfo.anyHitShader                           = uiAnyHitIndex;
    vkShaderGroupCreateInfo.intersectionShader                     = uiIntersectionIndex;

    vkShaderGroups.PushBack(vkShaderGroupCreateInfo);
  }

  if (vkShaderStages.IsEmpty() || vkShaderGroups.IsEmpty())
  {
    return XII_FAILURE;
  }

  vk::PipelineLayoutCreateInfo vkPipelineLayoutCreateInfo = {};
  {
    auto pDescriptorSetLayouts = pPipelineResourceSignatureVulkan->GetVulkanDescriptorSetLayouts();

    vkPipelineLayoutCreateInfo.pNext          = nullptr;
    vkPipelineLayoutCreateInfo.flags          = {};
    vkPipelineLayoutCreateInfo.setLayoutCount = pDescriptorSetLayouts.GetCount();
    vkPipelineLayoutCreateInfo.pSetLayouts    = pDescriptorSetLayouts.GetPtr();

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

  vk::RayTracingPipelineCreateInfoKHR vkRayTracingPipelineCreateInfo = {};
  vkRayTracingPipelineCreateInfo.pNext                               = nullptr;
  vkRayTracingPipelineCreateInfo.flags                               = {};
  vkRayTracingPipelineCreateInfo.stageCount                          = vkShaderStages.GetCount();
  vkRayTracingPipelineCreateInfo.pStages                             = vkShaderStages.GetData();
  vkRayTracingPipelineCreateInfo.groupCount                          = vkShaderGroups.GetCount();
  vkRayTracingPipelineCreateInfo.pGroups                             = vkShaderGroups.GetData();
  vkRayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth        = xiiMath::Max(1U, static_cast<xiiUInt32>(m_Description.m_RayTracingPipeline.m_uiMaxRecursionDepth));
  vkRayTracingPipelineCreateInfo.layout                              = m_vkPipelineLayout;
  vkRayTracingPipelineCreateInfo.pLibraryInfo                        = nullptr;
  vkRayTracingPipelineCreateInfo.pLibraryInterface                   = nullptr;
  vkRayTracingPipelineCreateInfo.pDynamicState                       = nullptr;
  vkRayTracingPipelineCreateInfo.basePipelineHandle                  = nullptr; // A pipeline to derive from.
  vkRayTracingPipelineCreateInfo.basePipelineIndex                   = -1;      // An index into the pCreateInfos parameter to use as a pipeline to derive from.

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  vkRayTracingPipelineCreateInfo.flags |= vk::PipelineCreateFlagBits::eDisableOptimization;
#endif

  const xiiUInt32 uiMaxRecursionDepth = pDeviceVulkan->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiMaxRecursionDepth;
  if (uiMaxRecursionDepth > 0U)
  {
    vkRayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = xiiMath::Min(vkRayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth, uiMaxRecursionDepth);
  }

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createRayTracingPipelinesKHR(VK_NULL_HANDLE, m_vkPipelineCache, 1U, &vkRayTracingPipelineCreateInfo, nullptr, &m_vkPipeline, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

void xiiGALRayTracingPipelineStateVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  if (m_vkPipeline == VK_NULL_HANDLE)
    return;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkPipeline, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_RayTracingPipelineStateVulkan);
