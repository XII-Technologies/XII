#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALPipelineResourceSignatureVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALPipelineResourceSignatureVulkan::xiiGALPipelineResourceSignatureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription) :
  xiiGALPipelineResourceSignature(pDeviceVulkan, creationDescription)
{
}

xiiGALPipelineResourceSignatureVulkan::~xiiGALPipelineResourceSignatureVulkan() = default;

xiiResult xiiGALPipelineResourceSignatureVulkan::InitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  vk::DescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo = {};
  vkDescriptorSetLayoutCreateInfo.pNext                             = nullptr;
  vkDescriptorSetLayoutCreateInfo.flags                             = {};

  xiiDynamicArray<vk::DescriptorSetLayoutBinding> vkDescriptorSetLayoutBindings(pDeviceVulkan->GetAllocator());

  for (xiiUInt32 uiResource = 0; uiResource < m_Description.m_Resources.GetCount(); ++uiResource)
  {
    const auto& resource                  = m_Description.m_Resources[uiResource];
    auto&       vkDescriptorLayoutBinding = vkDescriptorSetLayoutBindings.ExpandAndGetRef();

    vkDescriptorLayoutBinding.binding            = resource.m_uiBindSlot;
    vkDescriptorLayoutBinding.descriptorType     = xiiVulkanTypeConversions::GetDescriptorType(resource);
    vkDescriptorLayoutBinding.descriptorCount    = resource.m_uiArraySize;
    vkDescriptorLayoutBinding.stageFlags         = xiiVulkanTypeConversions::GetShaderStageFlags(resource.m_ShaderStages);
    vkDescriptorLayoutBinding.pImmutableSamplers = {};

    // \todo GraphicsVulkan: Implement immutable samplers.
  }

  vkDescriptorSetLayoutCreateInfo.pBindings    = vkDescriptorSetLayoutBindings.GetData();
  vkDescriptorSetLayoutCreateInfo.bindingCount = vkDescriptorSetLayoutBindings.GetCount();

  vk::Device vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createDescriptorSetLayout(&vkDescriptorSetLayoutCreateInfo, nullptr, &m_vkDescriptorSetLayout, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

xiiResult xiiGALPipelineResourceSignatureVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  if (m_vkDescriptorSetLayout != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(m_vkDescriptorSetLayout);

    m_vkDescriptorSetLayout = VK_NULL_HANDLE;
  }

  return XII_SUCCESS;
}

void xiiGALPipelineResourceSignatureVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  if (m_vkDescriptorSetLayout == VK_NULL_HANDLE)
    return;

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkDescriptorSetLayout, sName.GetData(tmp));
}

bool xiiGALPipelineResourceSignatureVulkan::IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const
{
  const xiiGALPipelineResourceSignatureVulkan* pPipelineResourceSignatureVulkan = static_cast<const xiiGALPipelineResourceSignatureVulkan*>(pPipelineResourceSignature);

  if (pPipelineResourceSignature == this)
    return true;

  const auto& sourceDescription  = GetDescription();
  const auto& compareDescription = pPipelineResourceSignature->GetDescription();

  if (sourceDescription.m_bUseCombinedTextureSamplers != compareDescription.m_bUseCombinedTextureSamplers)
    return false;

  for (const auto& resource : compareDescription.m_Resources)
  {
    if (!sourceDescription.m_Resources.Contains(resource))
      return false;
  }

  for (const auto& immutableSampler : compareDescription.m_ImmutableSamplers)
  {
    if (!sourceDescription.m_ImmutableSamplers.Contains(immutableSampler))
      return false;
  }

  return false;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_PipelineResourceSignatureVulkan);
