#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/BufferView.h>

namespace vk
{
  class BufferView;
}

class XII_GRAPHICSVULKAN_DLL xiiGALBufferViewVulkan final : public xiiGALBufferView
{
public:
  XII_ALWAYS_INLINE const vk::BufferView GetVulkanBufferView() const { return m_vkBufferView; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferViewVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferViewVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  vk::BufferView m_vkBufferView;
};
