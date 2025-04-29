#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

namespace vk
{
  class Image;
  class Buffer;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALTextureVulkan final : public xiiGALTexture
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTextureVulkan, xiiGALTexture);

public:
  XII_ALWAYS_INLINE virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final { return m_SparseTextureProperties; }

  XII_ALWAYS_INLINE vk::Image     GetVulkanImage() const { return m_vkImage; }
  XII_ALWAYS_INLINE VmaAllocation GetAllocationDescription() const { return m_ImageMemoryAllocation; }

  XII_ALWAYS_INLINE vk::Buffer    GetVulkanStagingBuffer() const { return m_vkStagingBuffer; }
  XII_ALWAYS_INLINE VmaAllocation GetStagingBufferAllocationDescription() const { return m_StagingBufferMemoryAllocation; }

  XII_ALWAYS_INLINE bool IsNativeObjectWrapper() const { return m_Description.m_pExisitingNativeObject != nullptr; }

  vk::ImageLayout GetVulkanImageLayout() const;
  void            SetVulkanImageLayout(vk::ImageLayout vkImageLayout);

  // For non-compressed color format buffer, the offset must be a multiple of the format's texel block size.
  // For compressed format buffer, the offset must be a multiple of the compressed texel block size in bytes.
  // For depth-stencil format buffer, the offset must be a multiple of 4.
  // If command buffer does not support graphics or compute commands, then the buffer offset must be a multiple of 4.
  // ("Copying Data Between Buffers and Images")
  static constexpr xiiUInt32 s_uiStagingBufferOffsetAlignment = 16U; // max texel size - 16 bytes (RGBA32F), max texel block size - 16 bytes.

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureVulkan();

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData) override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  vk::Result CreateVulkanStagingBuffer(const xiiGALTextureData* pInitialData, const xiiGALResourceFormatDescription& formatProperties);

  void InitializeImageContent(const vk::ImageCreateInfo& vkImageCreateInfo, const xiiGALResourceFormatDescription& formatProperties, const xiiGALTextureData* pInitialData);
  void InitializeSparseTextureProperties();

  static void ComputeVkImageCreateInfo(const xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription, vk::ImageCreateInfo& ref_vkImageCreateInfo);

  vk::Image     m_vkImage               = VK_NULL_HANDLE;
  VmaAllocation m_ImageMemoryAllocation = {};

  vk::Buffer    m_vkStagingBuffer               = VK_NULL_HANDLE;
  VmaAllocation m_StagingBufferMemoryAllocation = {};

  xiiGALSparseTextureProperties m_SparseTextureProperties;
};
