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
public:
  XII_ALWAYS_INLINE virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final { return m_SparseTextureProperties; }

  XII_ALWAYS_INLINE vk::Image GetVulkanImage() const { return m_vkImage; }
  XII_ALWAYS_INLINE vk::Buffer GetVulkanStagingBuffer() const { return m_vkStagingBuffer; }

  // For non-compressed color format buffer, the offset must be a multiple of the format's texel block size.
  // For compressed format buffer, the offset must be a multiple of the compressed texel block size in bytes.
  // For depth-stencil format buffer, the offset must be a multiple of 4.
  // If command buffer does not support graphics or compute commands, then the buffer offset must be a multiple of 4.
  // ("Copying Data Between Buffers and Images")
  static constexpr xiiUInt32 s_uiStagingBufferOffsetAlignment = 16U; // max texel size - 16 bytes (RGBA32F), max texel block size - 16 bytes.

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureVulkan();

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData) override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

  vk::Result CreateVulkanStagingBuffer(const xiiGALTextureData* pInitialData, const xiiGALResourceFormatDescription& formatProperties);

  void InitializeSparseTextureProperties();

  static void ComputeVkImageCreateInfo(const xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription, vk::ImageCreateInfo& ref_vkImageCreateInfo);
  static void InitializeImageContent(const xiiGALDeviceVulkan* pDeviceVulkan, const vk::ImageCreateInfo& vkImageCreateInfo, const vk::Image& vkImage, const xiiGALTextureData* pInitialData);

protected:
  vk::Image     m_vkImage;
  vk::Buffer    m_vkStagingBuffer;
  VmaAllocation m_MemoryAllocation;

  xiiGALSparseTextureProperties m_SparseTextureProperties;
};
