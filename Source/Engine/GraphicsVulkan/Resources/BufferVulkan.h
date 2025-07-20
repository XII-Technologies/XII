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

  [[nodiscard]] XII_ALWAYS_INLINE vk::Buffer          GetVulkanBuffer() const { return m_vkBuffer; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiVulkanAllocation GetAllocationDescription() const { return m_BufferMemoryAllocation; }

  [[nodiscard]] vk::DeviceAddress GetVulkanBufferDeviceAddress() const;

  void                                 SetAccessFlags(vk::AccessFlags accessFlags);
  [[nodiscard]] vk::AccessFlags        GetAccessFlags() const;
  [[nodiscard]] XII_ALWAYS_INLINE bool CheckAccessFlags(vk::AccessFlags accessFlags) const { return (GetAccessFlags() & accessFlags) == accessFlags; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferVulkan();

  virtual xiiResult InitPlatform(const xiiGALBufferData* pInitialData) override final;

  virtual xiiInternal::NewInstance<xiiGALBufferView> CreateViewPlatform(const xiiGALBufferViewCreationDescription& description) override;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::Buffer          m_vkBuffer               = VK_NULL_HANDLE;
  xiiVulkanAllocation m_BufferMemoryAllocation = {};
};
