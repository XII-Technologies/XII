
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

#include <vulkan/vulkan.hpp>

class XII_RENDERERVULKAN_DLL xiiGALBufferVulkan : public xiiGALBuffer
{
public:
  void              DiscardBuffer() const;
  XII_ALWAYS_INLINE vk::Buffer    GetVkBuffer() const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;

  XII_ALWAYS_INLINE vk::IndexType       GetIndexType() const;
  XII_ALWAYS_INLINE xiiVulkanAllocation GetAllocation() const;
  XII_ALWAYS_INLINE const xiiVulkanAllocationInfo& GetAllocationInfo() const;
  XII_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  XII_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;
  static vk::DeviceSize             GetAlignment(const xiiGALDeviceVulkan* pDevice, vk::BufferUsageFlags usage);

protected:
  struct BufferVulkan
  {
    vk::Buffer          m_buffer;
    xiiVulkanAllocation m_alloc;
    mutable xiiUInt64   m_currentFrame = 0;
  };

  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferVulkan(const xiiGALBufferCreationDescription& Description, bool bCPU = false);

  virtual ~xiiGALBufferVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<const xiiUInt8> pInitialData) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;
  virtual void      SetDebugNamePlatform(const char* szName) const override;
  void              CreateBuffer() const;

  mutable BufferVulkan             m_currentBuffer;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  mutable xiiDeque<BufferVulkan>   m_usedBuffers;
  mutable xiiVulkanAllocationInfo  m_allocInfo;

  // Data for memory barriers and access
  vk::PipelineStageFlags m_stages    = {};
  vk::AccessFlags        m_access    = {};
  vk::IndexType          m_indexType = vk::IndexType::eUint16; // Only applicable for index buffers
  vk::BufferUsageFlags   m_usage     = {};
  vk::DeviceSize         m_size      = 0;

  xiiGALDeviceVulkan* m_pDeviceVulkan = nullptr;
  vk::Device          m_device;

  bool              m_bCPU = false;
  mutable xiiString m_sDebugName;
};

#include <RendererVulkan/Resources/Implementation/BufferVulkan_inl.h>
