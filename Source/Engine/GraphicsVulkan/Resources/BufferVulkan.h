#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

namespace vk
{
  class Buffer;
}

class XII_GRAPHICSVULKAN_DLL xiiGALBufferVulkan final : public xiiGALBuffer
{
public:
  virtual xiiGALMemoryProperties GetMemoryProperties() const override final;

  virtual void FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual void InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual xiiGALSparseBufferProperties GetSparseProperties() const override final;

  XII_ALWAYS_INLINE vk::Buffer GetVulkanBuffer() const { return m_vkBuffer; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferVulkan();

  virtual xiiResult InitPlatform(const xiiGALBufferData* pInitialData) override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
  vk::Buffer m_vkBuffer;
};
