/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/BufferView.h>

namespace vk
{
  class BufferView;
}

class XII_GRAPHICSVULKAN_DLL xiiGALBufferViewVulkan final : public xiiGALBufferView
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBufferViewVulkan, xiiGALBufferView);

public:
  XII_ALWAYS_INLINE const vk::BufferView GetVulkanBufferView() const { return m_vkBufferView; }
  XII_ALWAYS_INLINE const vk::DescriptorBufferInfo* GetVulkanDescriptorBufferInfo() const { return &m_vkDescriptorBufferInfo; }

protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;
  friend class xiiGALBufferVulkan;

  xiiGALBufferViewVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, xiiSharedPtr<xiiGALBuffer> pBuffer, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferViewVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::BufferView m_vkBufferView;

  vk::DescriptorBufferInfo m_vkDescriptorBufferInfo = {};
};
