#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

namespace vk
{
  class Buffer;
}

class XII_GRAPHICSVULKAN_DLL xiiGALBufferVulkan final : public xiiGALBuffer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBufferVulkan, xiiGALBuffer);

public:
  virtual void                         FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;
  virtual void                         InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;
  virtual xiiGALSparseBufferProperties GetSparseProperties() const override final;

  XII_ALWAYS_INLINE vk::Buffer GetVulkanBuffer() const { return m_vkBuffer; }
  vk::DeviceAddress            GetVulkanBufferDeviceAddress() const;

  void                   SetAccessFlags(vk::AccessFlags accessFlags);
  vk::AccessFlags        GetAccessFlags() const;
  XII_ALWAYS_INLINE bool CheckAccessFlags(vk::AccessFlags accessFlags) const { return (GetAccessFlags() & accessFlags) == accessFlags; }

  XII_ALWAYS_INLINE xiiEnum<xiiGALValueType> GetIndexFormat() const { return m_IndexFormat; };

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferVulkan();

  virtual xiiResult InitPlatform(const xiiGALBufferData* pInitialData) override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  vk::Buffer    m_vkBuffer               = VK_NULL_HANDLE;
  VmaAllocation m_BufferMemoryAllocation = {};

  xiiEnum<xiiGALValueType> m_IndexFormat = xiiGALValueType::Undefined; // Strictly index buffers.
};
