#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

XII_DEFINE_AS_POD_TYPE(vk::DescriptorType);

template <>
struct xiiHashHelper<vk::DescriptorType>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(vk::DescriptorType value) { return xiiHashHelper<xiiUInt32>::Hash(xiiUInt32(value)); }
  XII_ALWAYS_INLINE static bool      Equal(vk::DescriptorType a, vk::DescriptorType b) { return a == b; }
};

class XII_RENDERERVULKAN_DLL xiiDescriptorSetPoolVulkan
{
public:
  static void                                     Initialize(vk::Device device);
  static void                                     DeInitialize();
  static xiiHashTable<vk::DescriptorType, float>& AccessDescriptorPoolWeights();

  static vk::DescriptorSet CreateDescriptorSet(vk::DescriptorSetLayout layout);
  static void              UpdateDescriptorSet(vk::DescriptorSet descriptorSet, xiiArrayPtr<vk::WriteDescriptorSet> update);
  static void              ReclaimPool(vk::DescriptorPool& descriptorPool);

private:
  static constexpr xiiUInt32 s_uiPoolBaseSize = 1024;

  static vk::DescriptorPool GetNewPool();

  static vk::DescriptorPool                      s_currentPool;
  static xiiHybridArray<vk::DescriptorPool, 4>   s_freePools;
  static vk::Device                              s_device;
  static xiiHashTable<vk::DescriptorType, float> s_descriptorWeights;
};
