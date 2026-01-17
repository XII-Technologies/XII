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
  [[nodiscard]] XII_ALWAYS_INLINE virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final { return m_SparseTextureProperties; }

  [[nodiscard]] XII_ALWAYS_INLINE vk::Image           GetVulkanImage() const { return m_vkImage; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiVulkanAllocation GetAllocationDescription() const { return m_ImageMemoryAllocation; }

  [[nodiscard]] XII_ALWAYS_INLINE vk::Buffer          GetVulkanStagingBuffer() const { return m_vkStagingBuffer; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiVulkanAllocation GetStagingBufferAllocationDescription() const { return m_StagingBufferMemoryAllocation; }

  [[nodiscard]] XII_ALWAYS_INLINE bool IsNativeObjectWrapper() const { return m_Description.m_pExistingNativeObject != nullptr; }

  [[nodiscard]] vk::ImageLayout GetVulkanImageLayout() const;
  void                          SetVulkanImageLayout(vk::ImageLayout vkImageLayout);

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

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind) override final;

  virtual xiiInternal::NewInstance<xiiGALTextureView> CreateViewPlatform(const xiiGALTextureViewCreationDescription& description) override;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::Result CreateVulkanStagingBuffer(const xiiGALTextureData* pInitialData, const xiiGALResourceFormatDescription& formatProperties);

  void InitializeImageContent(const vk::ImageCreateInfo& vkImageCreateInfo, const xiiGALResourceFormatDescription& formatProperties, const xiiGALTextureData* pInitialData);
  void InitializeSparseTextureProperties();
  void InitializeExternalMemoryProperties(xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind);

  static void ComputeVkImageCreateInfo(const xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription, vk::ImageCreateInfo& ref_vkImageCreateInfo);

  vk::Image           m_vkImage;
  xiiVulkanAllocation m_ImageMemoryAllocation;

  vk::Buffer          m_vkStagingBuffer;
  xiiVulkanAllocation m_StagingBufferMemoryAllocation;

  xiiGALSparseTextureProperties m_SparseTextureProperties;

  xiiGALExternalMemoryDescription m_ExternalMemoryDescription;
  vk::Semaphore                   m_vkExternalMemorySemaphore;
};
