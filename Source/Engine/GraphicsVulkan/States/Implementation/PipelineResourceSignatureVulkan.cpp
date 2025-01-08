#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALPipelineResourceSignatureVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiUInt32 FindImmutableSampler(const xiiGALPipelineResourceSignatureCreationDescription& pipelineDescription, const xiiGALPipelineResourceDescription& resourceDescription)
{
  XII_ASSERT_DEV(!resourceDescription.m_sName.IsEmpty(), "");

  vk::DescriptorType vkDescriptorType = xiiVulkanTypeConversions::GetDescriptorType(resourceDescription);

  if (vkDescriptorType != vk::DescriptorType::eCombinedImageSampler && vkDescriptorType != vk::DescriptorType::eSampler)
  {
    xiiLog::Error("Immutable sampler can only be assigned to a sampled image or separate sampler.");
    return xiiInvalidIndex;
  }

  const bool       bPermitSuffix = vkDescriptorType == vk::DescriptorType::eSampler;
  xiiStringBuilder sb;

  for (xiiUInt32 i = 0; i < pipelineDescription.m_ImmutableSamplers.GetCount(); ++i)
  {
    const auto& immutableSampler = pipelineDescription.m_ImmutableSamplers[i];

    if (bPermitSuffix)
    {
      sb.SetFormat("{}{}", immutableSampler.m_SamplerOrTextureName, bPermitSuffix ? pipelineDescription.m_sCombinedSamplerSuffix : "");
    }

    if (immutableSampler.m_ShaderStages.AreAllSet(resourceDescription.m_ShaderStages) && sb.IsEqual(resourceDescription.m_sName.GetView()))
    {
      return i;
    }
  }

  return xiiInvalidIndex;
}

xiiGALPipelineResourceSignatureVulkan::xiiGALPipelineResourceSignatureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription) :
  xiiGALPipelineResourceSignature(pDeviceVulkan, creationDescription), m_DescriptorSetLayouts(pDeviceVulkan->GetAllocator()), m_ImmutableSamplers(pDeviceVulkan->GetAllocator())
{
}

xiiGALPipelineResourceSignatureVulkan::~xiiGALPipelineResourceSignatureVulkan() = default;

xiiResult xiiGALPipelineResourceSignatureVulkan::InitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_ImmutableSamplers.SetCount(m_Description.m_ImmutableSamplers.GetCount());

  vk::DescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo = {};
  vkDescriptorSetLayoutCreateInfo.pNext                             = nullptr;
  vkDescriptorSetLayoutCreateInfo.flags                             = {};

  xiiDynamicArray<vk::DescriptorSetLayoutBinding> vkDescriptorSetLayoutBindings(pDeviceVulkan->GetAllocator());
  xiiDynamicArray<xiiDynamicArray<vk::Sampler>>   vkTempSamplerArrayAssignment(pDeviceVulkan->GetAllocator());

  for (xiiUInt32 uiResource = 0; uiResource < m_Description.m_Resources.GetCount(); ++uiResource)
  {
    const auto& resource                  = m_Description.m_Resources[uiResource];
    auto&       vkDescriptorLayoutBinding = vkDescriptorSetLayoutBindings.ExpandAndGetRef();

    vk::Sampler* pVkImmutableSamplers = nullptr;
    if (vkDescriptorLayoutBinding.descriptorType == vk::DescriptorType::eCombinedImageSampler || vkDescriptorLayoutBinding.descriptorType == vk::DescriptorType::eSampler)
    {
      xiiUInt32 uiSourceImmutableSamplerIndex = FindImmutableSampler(m_Description, resource);

      if (!m_ImmutableSamplers[uiSourceImmutableSamplerIndex])
      {
        const auto& immutableSamplerDescription = m_Description.m_ImmutableSamplers[uiSourceImmutableSamplerIndex].m_SamplerDescription;

        m_ImmutableSamplers[uiSourceImmutableSamplerIndex].Initialize(pDeviceVulkan, immutableSamplerDescription);
      }

      vkTempSamplerArrayAssignment.PushBack(xiiDynamicArray<vk::Sampler>(pDeviceVulkan->GetAllocator()));
      vkTempSamplerArrayAssignment.PeekBack().SetCount(resource.m_uiArraySize, m_ImmutableSamplers[uiSourceImmutableSamplerIndex].GetVulkanSampler());

      pVkImmutableSamplers = vkTempSamplerArrayAssignment.PeekBack().GetData();
    }

    vkDescriptorLayoutBinding.binding            = resource.m_uiBindSlot;
    vkDescriptorLayoutBinding.descriptorType     = xiiVulkanTypeConversions::GetDescriptorType(resource);
    vkDescriptorLayoutBinding.descriptorCount    = resource.m_uiArraySize;
    vkDescriptorLayoutBinding.stageFlags         = xiiVulkanTypeConversions::GetShaderStageFlags(resource.m_ShaderStages);
    vkDescriptorLayoutBinding.pImmutableSamplers = pVkImmutableSamplers;
  }

  vkDescriptorSetLayoutCreateInfo.pBindings    = vkDescriptorSetLayoutBindings.GetData();
  vkDescriptorSetLayoutCreateInfo.bindingCount = vkDescriptorSetLayoutBindings.GetCount();

  vk::Device               vkLogicalDevice       = pDeviceVulkan->GetVulkanLogicalDevice();
  vk::DescriptorSetLayout& vkDescriptorSetLayout = m_DescriptorSetLayouts.ExpandAndGetRef();
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createDescriptorSetLayout(&vkDescriptorSetLayoutCreateInfo, nullptr, &vkDescriptorSetLayout, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

xiiResult xiiGALPipelineResourceSignatureVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  for (xiiUInt32 i = 0; i < m_DescriptorSetLayouts.GetCount(); ++i)
  {
    if (m_DescriptorSetLayouts[i] != VK_NULL_HANDLE)
    {
      pDeviceVulkan->SafeReleaseDeviceObject(m_DescriptorSetLayouts[i]);

      m_DescriptorSetLayouts[i] = VK_NULL_HANDLE;
    }
  }
  m_DescriptorSetLayouts.Clear();

  for (xiiUInt32 i = 0; i < m_ImmutableSamplers.GetCount(); ++i)
  {
    if (m_ImmutableSamplers[i])
    {
      m_ImmutableSamplers[i].DeInitialize(pDeviceVulkan);
    }
  }

  return XII_SUCCESS;
}

void xiiGALPipelineResourceSignatureVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp(sName);

  for (xiiUInt32 i = 0; i < m_DescriptorSetLayouts.GetCount(); ++i)
  {
    XII_ASSERT_DEBUG(m_DescriptorSetLayouts[i] != VK_NULL_HANDLE, "Invalid Vulkan descriptor set layout.");

    pDeviceVulkan->SetVulkanObjectDebugName(m_DescriptorSetLayouts[i], tmp.GetData());
  }
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

void xiiGALPipelineResourceSignatureVulkan::ImmutableSamplerStorage::Initialize(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSamplerCreationDescription& samplerDescription)
{
  XII_ASSERT_DEV(pDeviceVulkan != nullptr, "");

  if (m_pSamplerVulkan == nullptr)
  {
    m_pSamplerVulkan = pDeviceVulkan->CreateSamplerInternal(samplerDescription);
  }
}

void xiiGALPipelineResourceSignatureVulkan::ImmutableSamplerStorage::DeInitialize(xiiGALDeviceVulkan* pDeviceVulkan)
{
  XII_ASSERT_DEV(pDeviceVulkan != nullptr, "");

  if (m_pSamplerVulkan != nullptr)
  {
    pDeviceVulkan->DestroySamplerInternal(m_pSamplerVulkan);
  }
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_PipelineResourceSignatureVulkan);
