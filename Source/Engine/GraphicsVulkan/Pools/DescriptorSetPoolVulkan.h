#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <Foundation/Algorithm/HashingUtils.h>

template <>
struct xiiHashHelper<vk::DescriptorType>
{
  static xiiUInt32 Hash(vk::DescriptorType value);
  static bool      Equal(vk::DescriptorType a, vk::DescriptorType b);
};

class XII_GRAPHICSVULKAN_DLL xiiGALDescriptorSetPoolVulkan
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALDescriptorSetPoolVulkan);

public:
  static float GetDescriptorTypeWeight(vk::DescriptorType vkDescriptorType);

  vk::DescriptorSet RequestDescriptorSet(vk::DescriptorSetLayout vkDescriptorSetLayout);

  void ReclaimDescriptorPool(vk::DescriptorPool vkDescriptorPool);

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;

  xiiGALDescriptorSetPoolVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, xiiUInt32 uiBaseSize);
  ~xiiGALDescriptorSetPoolVulkan();

  vk::DescriptorPool CreateVulkanDescriptorPool();

  xiiGALDeviceVulkan* m_pDeviceVulkan;

  xiiMutex                            m_PoolMutex;
  xiiDynamicArray<vk::DescriptorPool> m_DescriptorPools;
  xiiDeque<vk::DescriptorPool>        m_QueuedDescriptorPools;
  xiiUInt32                           m_uiBaseSize = 0U;
  vk::DescriptorPool                  m_vkCurrentDescriptorPool;
};
