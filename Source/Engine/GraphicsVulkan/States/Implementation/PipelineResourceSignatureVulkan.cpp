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

  xiiGALDescriporTypeVulkan vkDescriptorType = xiiVulkanTypeConversions::GetDescriptorType(resourceDescription);

  if (vkDescriptorType != xiiGALDescriporTypeVulkan::CombinedImageSampler && vkDescriptorType != xiiGALDescriporTypeVulkan::Sampler)
  {
    xiiLog::Error("Immutable sampler can only be assigned to a sampled image or separate sampler.");
    return xiiInvalidIndex;
  }

  xiiStringBuilder sb;

  for (xiiUInt32 i = 0; i < pipelineDescription.m_ImmutableSamplers.GetCount(); ++i)
  {
    const auto& immutableSampler = pipelineDescription.m_ImmutableSamplers[i];

    sb.SetFormat("{}", immutableSampler.m_SamplerOrTextureName);

    if (immutableSampler.m_ShaderStages.AreAllSet(resourceDescription.m_ShaderStages) && sb.IsEqual(resourceDescription.m_sName.GetView()))
    {
      return i;
    }
  }

  return xiiInvalidIndex;
}

xiiGALPipelineResourceSignatureVulkan::xiiGALPipelineResourceSignatureVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription) :
  xiiGALPipelineResourceSignature(pDeviceVulkan, creationDescription), m_DescriptorSetLayouts(pDeviceVulkan->GetAllocator()), m_ImmutableSamplers(pDeviceVulkan->GetAllocator())
{
}

xiiGALPipelineResourceSignatureVulkan::~xiiGALPipelineResourceSignatureVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  m_PipelineResourceSetLayouts.Clear();
  m_PipelineResourceSetLayouts.Compact();

  for (xiiUInt32 i = 0; i < m_DescriptorSetLayouts.GetCount(); ++i)
  {
    if (m_DescriptorSetLayouts[i] != VK_NULL_HANDLE)
    {
      pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_DescriptorSetLayouts[i]));

      m_DescriptorSetLayouts[i] = VK_NULL_HANDLE;
    }
  }
  m_DescriptorSetLayouts.Clear();
  m_DescriptorSetLayouts.Compact();

  for (xiiUInt32 i = 0; i < m_ImmutableSamplers.GetCount(); ++i)
  {
    if (m_ImmutableSamplers[i])
    {
      m_ImmutableSamplers[i].DeInitialize(pDeviceVulkan);
    }
  }
}

xiiResult xiiGALPipelineResourceSignatureVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  // First build set layout and resource binding description.
  for (xiiUInt32 uiResource = 0; uiResource < m_Description.m_Resources.GetCount(); ++uiResource)
  {
    const auto& resource = m_Description.m_Resources[uiResource];

    m_PipelineResourceSetLayouts.EnsureCount(resource.m_uiBindSet + 1);

    auto& set = m_PipelineResourceSetLayouts[resource.m_uiBindSet];

    xiiGALPipelineResourceDescriptionVulkan& pipelineResource = set.ExpandAndGetRef();
    pipelineResource.m_sName                                  = resource.m_sName;
    pipelineResource.m_DescriptorType                         = xiiVulkanTypeConversions::GetDescriptorType(resource);
    pipelineResource.m_uiBindingSet                           = resource.m_uiBindSet;
    pipelineResource.m_uiBindingIndex                         = resource.m_uiBindSlot;
    pipelineResource.m_uiSamplerIndex                         = xiiInvalidIndex;
    pipelineResource.m_uiArraySize                            = resource.m_uiArraySize;
    pipelineResource.m_ShaderStages                           = resource.m_ShaderStages;
    pipelineResource.m_bHasImmutableSampler                   = false;

    if (resource.m_ResourceType == xiiGALShaderResourceType::TextureAndSampler)
    {
      // TextureAndSampler shader resources will inherit the same bind slot.
      pipelineResource.m_uiSamplerIndex = resource.m_uiBindSlot;
    }
    else
    {
      if ((pipelineResource.m_DescriptorType == xiiGALDescriporTypeVulkan::CombinedImageSampler || pipelineResource.m_DescriptorType == xiiGALDescriporTypeVulkan::Sampler) && pipelineResource.m_uiSamplerIndex == xiiInvalidIndex)
      {
        xiiUInt32 uiImmutableSamplerIndex       = FindImmutableSampler(m_Description, resource);
        pipelineResource.m_bHasImmutableSampler = uiImmutableSamplerIndex != xiiInvalidIndex;

        if (pipelineResource.m_bHasImmutableSampler)
        {
          pipelineResource.m_uiSamplerIndex = uiImmutableSamplerIndex;
        }
      }
    }
  }

  m_ImmutableSamplers.SetCount(m_Description.m_ImmutableSamplers.GetCount());

  vk::DescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo = {};
  vkDescriptorSetLayoutCreateInfo.pNext                             = nullptr;
  vkDescriptorSetLayoutCreateInfo.flags                             = {};

  xiiDynamicArray<vk::DescriptorSetLayoutBinding> vkDescriptorSetLayoutBindings(pDeviceVulkan->GetAllocator());
  xiiDynamicArray<xiiDynamicArray<vk::Sampler>>   vkTempSamplerArrayAssignment(pDeviceVulkan->GetAllocator());

  for (xiiUInt32 uiSet = 0; uiSet < m_PipelineResourceSetLayouts.GetCount(); ++uiSet)
  {
    const auto& setLayout = m_PipelineResourceSetLayouts[uiSet];

    XII_SCOPE_EXIT(vkDescriptorSetLayoutBindings.Clear(); vkTempSamplerArrayAssignment.Clear(););

    for (xiiUInt32 uiResourceIndex = 0; uiResourceIndex < setLayout.GetCount(); ++uiResourceIndex)
    {
      const auto& resourceLayout = setLayout[uiResourceIndex];

      vk::DescriptorSetLayoutBinding& vkDescriptorLayoutBinding = vkDescriptorSetLayoutBindings.ExpandAndGetRef();
      vkDescriptorLayoutBinding.binding                         = resourceLayout.m_uiBindingIndex;
      vkDescriptorLayoutBinding.descriptorType                  = xiiVulkanTypeConversions::GetDescriptorType(resourceLayout.m_DescriptorType);
      vkDescriptorLayoutBinding.descriptorCount                 = resourceLayout.m_uiArraySize;
      vkDescriptorLayoutBinding.stageFlags                      = xiiVulkanTypeConversions::GetShaderStageFlags(resourceLayout.m_ShaderStages);

      vk::Sampler* pVkImmutableSamplers = nullptr;
      if (resourceLayout.m_bHasImmutableSampler)
      {
        XII_ASSERT_DEV(vkDescriptorLayoutBinding.descriptorType == vk::DescriptorType::eCombinedImageSampler || vkDescriptorLayoutBinding.descriptorType == vk::DescriptorType::eSampler, "");
        XII_ASSERT_DEV(resourceLayout.m_uiSamplerIndex != xiiInvalidIndex, "");

        if (!m_ImmutableSamplers[resourceLayout.m_uiSamplerIndex])
        {
          const auto& immutableSamplerDescription = m_Description.m_ImmutableSamplers[resourceLayout.m_uiSamplerIndex].m_SamplerDescription;

          m_ImmutableSamplers[resourceLayout.m_uiSamplerIndex].Initialize(pDeviceVulkan, immutableSamplerDescription);
        }

        vkTempSamplerArrayAssignment.PushBack(xiiDynamicArray<vk::Sampler>(pDeviceVulkan->GetAllocator()));
        vkTempSamplerArrayAssignment.PeekBack().SetCount(resourceLayout.m_uiArraySize, m_ImmutableSamplers[resourceLayout.m_uiSamplerIndex].GetVulkanSampler());

        pVkImmutableSamplers = vkTempSamplerArrayAssignment.PeekBack().GetData();
      }

      vkDescriptorLayoutBinding.pImmutableSamplers = pVkImmutableSamplers;
    }

    vkDescriptorSetLayoutCreateInfo.pBindings    = !vkDescriptorSetLayoutBindings.IsEmpty() ? vkDescriptorSetLayoutBindings.GetData() : nullptr;
    vkDescriptorSetLayoutCreateInfo.bindingCount = vkDescriptorSetLayoutBindings.GetCount();

    vk::DescriptorSetLayout& vkDescriptorSetLayout = m_DescriptorSetLayouts.ExpandAndGetRef();
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createDescriptorSetLayout(&vkDescriptorSetLayoutCreateInfo, nullptr, &vkDescriptorSetLayout, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }
  return XII_SUCCESS;
}

void xiiGALPipelineResourceSignatureVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp(sName);

  for (xiiUInt32 i = 0; i < m_DescriptorSetLayouts.GetCount(); ++i)
  {
    XII_ASSERT_DEBUG(m_DescriptorSetLayouts[i] != VK_NULL_HANDLE, "Invalid Vulkan descriptor set layout.");

    pDeviceVulkan->SetVulkanObjectDebugName(m_DescriptorSetLayouts[i], tmp.GetData());
  }
}

void xiiGALPipelineResourceSignatureVulkan::ImmutableSamplerStorage::Initialize(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALSamplerCreationDescription& samplerDescription)
{
  XII_ASSERT_DEV(pDeviceVulkan != nullptr, "");

  if (m_pSamplerVulkan == nullptr)
  {
    m_pSamplerVulkan = pDeviceVulkan->CreateSampler(samplerDescription).Downcast<xiiGALSamplerVulkan>();
  }
}

void xiiGALPipelineResourceSignatureVulkan::ImmutableSamplerStorage::DeInitialize(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan)
{
  XII_ASSERT_DEV(pDeviceVulkan != nullptr, "");

  m_pSamplerVulkan.Clear();
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_PipelineResourceSignatureVulkan);
