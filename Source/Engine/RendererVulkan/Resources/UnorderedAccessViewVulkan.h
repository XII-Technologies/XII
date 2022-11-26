
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

#include <vulkan/vulkan.hpp>

class xiiGALBufferVulkan;

class xiiGALUnorderedAccessViewVulkan : public xiiGALUnorderedAccessView
{
public:
  XII_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;
  const vk::DescriptorBufferInfo&                  GetBufferInfo() const;
  vk::ImageSubresourceRange                        GetRange() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALUnorderedAccessViewVulkan(xiiGALResourceBase* pResource, const xiiGALUnorderedAccessViewCreationDescription& Description);
  ~xiiGALUnorderedAccessViewVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

private:
  mutable vk::DescriptorImageInfo  m_resourceImageInfo;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::ImageSubresourceRange        m_range;
};

#include <RendererVulkan/Resources/Implementation/UnorderedAccessViewVulkan_inl.h>
