
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

#include <vulkan/vulkan.hpp>

class xiiGALBufferVulkan;
class xiiGALTextureVulkan;

class xiiGALResourceViewVulkan : public xiiGALResourceView
{
public:
  const vk::DescriptorImageInfo&  GetImageInfo(bool bIsArray) const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  vk::ImageSubresourceRange       GetRange() const;
  const vk::BufferView&           GetBufferView() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALResourceViewVulkan(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description);
  ~xiiGALResourceViewVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  vk::ImageSubresourceRange        m_range;
  mutable vk::DescriptorImageInfo  m_resourceImageInfo;
  mutable vk::DescriptorImageInfo  m_resourceImageInfoArray;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::BufferView                   m_bufferView;
};

#include <RendererVulkan/Resources/Implementation/ResourceViewVulkan_inl.h>
