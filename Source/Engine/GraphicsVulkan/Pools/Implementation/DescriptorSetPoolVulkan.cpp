/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Pools/DescriptorSetPoolVulkan.h>

xiiUInt32 xiiHashHelper<vk::DescriptorType>::Hash(vk::DescriptorType value)
{
  return xiiHashHelper<xiiUInt32>::Hash(xiiUInt32(value));
}

bool xiiHashHelper<vk::DescriptorType>::Equal(vk::DescriptorType a, vk::DescriptorType b)
{
  return a == b;
}

xiiGALDescriptorSetPoolVulkan::xiiGALDescriptorSetPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiBaseSize, xiiUInt32 uiMaxSets) :
  m_pDeviceVulkan(pDeviceVulkan), m_DescriptorPools(pDeviceVulkan->GetAllocator()), m_QueuedDescriptorPools(pDeviceVulkan->GetAllocator()), m_uiBaseSize(uiBaseSize), m_uiMaxSets(uiMaxSets)
{
}

xiiGALDescriptorSetPoolVulkan::~xiiGALDescriptorSetPoolVulkan()
{
  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  for (xiiUInt32 i = 0; i < m_DescriptorPools.GetCount(); ++i)
  {
    vk::DescriptorPool& vkDescriptorPool = m_DescriptorPools[i];

    vkLogicalDevice.destroyDescriptorPool(vkDescriptorPool, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }

  m_QueuedDescriptorPools.Clear();
  m_QueuedDescriptorPools.Compact();

  m_DescriptorPools.Clear();
  m_DescriptorPools.Compact();
}

vk::DescriptorSet xiiGALDescriptorSetPoolVulkan::RequestDescriptorSet(vk::DescriptorSetLayout vkDescriptorSetLayout, xiiArrayPtr<const xiiGALPipelineResourceDescriptionVulkan> resources)
{
  xiiHashTable<vk::DescriptorType, xiiUInt32> required;
  for (const auto& resource : resources)
  {
    auto type = xiiVulkanTypeConversions::GetDescriptorType(resource.m_DescriptorType);
    required[type] += resource.m_uiArraySize;
  }
  bool fits = m_uiRemainingSets != 0U;
  for (auto it = required.GetIterator(); it.IsValid(); ++it)
    fits &= m_Remaining[it.Key()] >= it.Value();
  if (!fits)
    m_vkCurrentDescriptorPool = VK_NULL_HANDLE;
  if (!m_vkCurrentDescriptorPool)
  {
    m_vkCurrentDescriptorPool = CreateVulkanDescriptorPool(required);
  }

  vk::DescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
  vkDescriptorSetAllocateInfo.pNext                         = nullptr;
  vkDescriptorSetAllocateInfo.descriptorPool                = m_vkCurrentDescriptorPool;
  vkDescriptorSetAllocateInfo.pSetLayouts                   = &vkDescriptorSetLayout;
  vkDescriptorSetAllocateInfo.descriptorSetCount            = 1U;

  vk::Device        vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();
  vk::DescriptorSet vkDescriptorSet = VK_NULL_HANDLE;
  vk::Result        vkResult        = vkLogicalDevice.allocateDescriptorSets(&vkDescriptorSetAllocateInfo, &vkDescriptorSet, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  bool bPoolExhausted = false;
  switch (vkResult)
  {
    case vk::Result::eSuccess:
      break;

    case vk::Result::eErrorFragmentedPool:
    case vk::Result::eErrorOutOfPoolMemory:
      bPoolExhausted = true;
      break;

    default:
      VK_ASSERT_DEV(vkResult);
      break;
  }

  if (bPoolExhausted)
  {
    // We now create a descriptor set allocator per command list. Thus, we can't necessarily 'reclaim' without more complex tracking.
    // m_pDeviceVulkan->ReclaimLater(m_vkCurrentDescriptorPool);

    m_vkCurrentDescriptorPool = CreateVulkanDescriptorPool(required);

    vkDescriptorSetAllocateInfo.descriptorPool = m_vkCurrentDescriptorPool;

    VK_ASSERT_DEV(vkLogicalDevice.allocateDescriptorSets(&vkDescriptorSetAllocateInfo, &vkDescriptorSet, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }

  if (vkResult == vk::Result::eSuccess || bPoolExhausted)
  {
    for (auto it = required.GetIterator(); it.IsValid(); ++it)
      m_Remaining[it.Key()] -= it.Value();
    --m_uiRemainingSets;
  }
  return vkDescriptorSet;
}

void xiiGALDescriptorSetPoolVulkan::ReclaimDescriptorPool(vk::DescriptorPool&& vkDescriptorPool)
{
  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  VK_ASSERT_DEV(vkLogicalDevice.resetDescriptorPool(vkDescriptorPool, vk::DescriptorPoolResetFlagBits{}, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  m_QueuedDescriptorPools.PushBack(vkDescriptorPool);
}

vk::DescriptorPool xiiGALDescriptorSetPoolVulkan::CreateVulkanDescriptorPool(const xiiHashTable<vk::DescriptorType, xiiUInt32>& required)
{
  vk::DescriptorPool vkDescriptorPool = VK_NULL_HANDLE;

  // Reclaimed pools have the standard capacities. Oversized layouts need a pool
  // constructed for their actual descriptor counts.
  bool oversized = false;
  for (auto it = required.GetIterator(); it.IsValid(); ++it)
    oversized |= it.Value() > static_cast<xiiUInt32>(GetDescriptorTypeWeight(it.Key()) * m_uiBaseSize);
  m_Remaining.Clear();
  m_uiRemainingSets = m_uiMaxSets;
  if (m_QueuedDescriptorPools.IsEmpty() || oversized)
  {
    vk::Device  vkLogicalDevice   = m_pDeviceVulkan->GetVulkanLogicalDevice();
    const auto& extensionFeatures = m_pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();

    xiiTemporaryHybridArray<vk::DescriptorPoolSize, 16U> descriptorPoolSizes;
    vk::DescriptorType                                   descriptorTypes[] = {vk::DescriptorType::eSampler, vk::DescriptorType::eCombinedImageSampler, vk::DescriptorType::eSampledImage, vk::DescriptorType::eStorageImage, vk::DescriptorType::eUniformTexelBuffer, vk::DescriptorType::eStorageTexelBuffer, vk::DescriptorType::eUniformBuffer,
                                                                              vk::DescriptorType::eStorageBuffer, vk::DescriptorType::eUniformBufferDynamic, vk::DescriptorType::eStorageBufferDynamic, vk::DescriptorType::eInputAttachment, vk::DescriptorType::eInlineUniformBlock, vk::DescriptorType::eAccelerationStructureKHR};

    for (const vk::DescriptorType& vkDescriptorType : descriptorTypes)
    {
      xiiUInt32 uiDescriptorCount = static_cast<xiiUInt32>(GetDescriptorTypeWeight(vkDescriptorType) * m_uiBaseSize);

      const auto* needed = required.GetValue(vkDescriptorType);
      if (needed)
        uiDescriptorCount = xiiMath::Max(uiDescriptorCount, *needed);
      m_Remaining.Insert(vkDescriptorType, uiDescriptorCount);
      if (uiDescriptorCount > 0)
      {
        if (vkDescriptorType == vk::DescriptorType::eAccelerationStructureKHR && extensionFeatures.m_AccelerationStructure.accelerationStructure == vk::False)
          continue;

        descriptorPoolSizes.PushBack({vkDescriptorType, uiDescriptorCount});
      }
    }

    vk::DescriptorPoolCreateInfo vkDescriptorPoolCreateInfo = {};
    vkDescriptorPoolCreateInfo.pNext                        = nullptr;
    // Pools are compatible with ordinary and descriptor-indexing layouts. This is required for
    // sets whose runtime arrays are updated while already referenced by submitted command buffers.
    vkDescriptorPoolCreateInfo.flags         = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
    vkDescriptorPoolCreateInfo.maxSets       = m_uiMaxSets;
    vkDescriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizes.GetCount();
    vkDescriptorPoolCreateInfo.pPoolSizes    = descriptorPoolSizes.GetData();

    VK_ASSERT_DEV(vkLogicalDevice.createDescriptorPool(&vkDescriptorPoolCreateInfo, nullptr, &vkDescriptorPool, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    m_DescriptorPools.PushBack(vkDescriptorPool);
  }
  else
  {
    vkDescriptorPool = m_QueuedDescriptorPools.PeekFront();

    m_QueuedDescriptorPools.PopFront();
    for (auto it = required.GetIterator(); it.IsValid(); ++it)
      m_Remaining.Insert(it.Key(), static_cast<xiiUInt32>(GetDescriptorTypeWeight(it.Key()) * m_uiBaseSize));
  }

  XII_ASSERT_DEBUG(vkDescriptorPool != VK_NULL_HANDLE, "Invalid descriptor pool.");

  return vkDescriptorPool;
}

float xiiGALDescriptorSetPoolVulkan::GetDescriptorTypeWeight(vk::DescriptorType vkDescriptorType)
{
  switch (vkDescriptorType)
  {
    case vk::DescriptorType::eSampler:
      return 0.5f;
    case vk::DescriptorType::eCombinedImageSampler:
      return 2.0f;
    case vk::DescriptorType::eSampledImage:
      return 2.0f;
    case vk::DescriptorType::eStorageImage:
      return 1.0f;
    case vk::DescriptorType::eUniformTexelBuffer:
      return 1.0f;
    case vk::DescriptorType::eStorageTexelBuffer:
      return 1.0f;
    case vk::DescriptorType::eUniformBuffer:
      return 1.0f;
    case vk::DescriptorType::eStorageBuffer:
      return 1.0f;
    case vk::DescriptorType::eUniformBufferDynamic:
      return 1.0f;
    case vk::DescriptorType::eStorageBufferDynamic:
      return 1.0f;
    case vk::DescriptorType::eInputAttachment:
      return 0.5f;
    case vk::DescriptorType::eInlineUniformBlock: // Unused at the moment.
      return 0.0;
    case vk::DescriptorType::eAccelerationStructureKHR:
      return 1.0f;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return 0.0f;
}
