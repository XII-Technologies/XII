#pragma once

#include <RendererFoundation/Resources/Texture.h>

#include <vulkan/vulkan.hpp>

class xiiGALBufferVulkan;
class xiiGALDeviceVulkan;

class xiiGALTextureVulkan : public xiiGALTexture
{
public:
  enum class StagingMode : xiiUInt8
  {
    None,
    Buffer,          ///< We can use vkCopyImageToBuffer to a CPU buffer.
    Texture,         ///< Formats differ and we need to render to a linear CPU texture to do the conversion.
    TextureAndBuffer ///< Formats differ and linear texture can't be rendered to. Render to optimal layout GPU texture and then use vkCopyImageToBuffer to CPU buffer.
  };
  struct SubResourceOffset
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt32 m_uiOffset;
    xiiUInt32 m_uiSize;
    xiiUInt32 m_uiRowLength;
    xiiUInt32 m_uiImageHeight;
  };

  XII_ALWAYS_INLINE vk::Image GetImage() const;
  XII_ALWAYS_INLINE vk::Format GetImageFormat() const { return m_imageFormat; }
  XII_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout() const;
  XII_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout(vk::ImageLayout targetLayout) const;
  XII_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  XII_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;

  XII_ALWAYS_INLINE xiiVulkanAllocation GetAllocation() const;
  XII_ALWAYS_INLINE const xiiVulkanAllocationInfo& GetAllocationInfo() const;

  XII_ALWAYS_INLINE bool GetFormatOverrideEnabled() const;
  XII_ALWAYS_INLINE bool IsLinearLayout() const;

  vk::Extent3D              GetMipLevelSize(xiiUInt32 uiMipLevel) const;
  vk::ImageSubresourceRange GetFullRange() const;
  vk::ImageAspectFlags      GetAspectMask() const;

  // Read-back staging resources
  XII_ALWAYS_INLINE StagingMode         GetStagingMode() const;
  XII_ALWAYS_INLINE xiiGALTextureHandle GetStagingTexture() const;
  XII_ALWAYS_INLINE xiiGALBufferHandle  GetStagingBuffer() const;
  xiiUInt32                             ComputeSubResourceOffsets(xiiDynamicArray<SubResourceOffset>& out_subResourceOffsets) const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureVulkan(const xiiGALTextureCreationDescription& Description);
  xiiGALTextureVulkan(const xiiGALTextureCreationDescription& Description, vk::Format OverrideFormat, bool bLinearCPU);

  ~xiiGALTextureVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  StagingMode ComputeStagingMode(const vk::ImageCreateInfo& createInfo) const;
  xiiResult   CreateStagingBuffer(const vk::ImageCreateInfo& createInfo);

  vk::Image              m_image;
  vk::Format             m_imageFormat     = vk::Format::eUndefined;
  vk::ImageLayout        m_preferredLayout = vk::ImageLayout::eUndefined;
  vk::PipelineStageFlags m_stages          = {};
  vk::AccessFlags        m_access          = {};

  xiiVulkanAllocation     m_alloc = nullptr;
  xiiVulkanAllocationInfo m_allocInfo;

  xiiGALDeviceVulkan* m_pDevice                = nullptr;
  void*               m_pExisitingNativeObject = nullptr;

  bool m_formatOverride = false;
  bool m_bLinearCPU     = false;

  StagingMode         m_stagingMode = StagingMode::None;
  xiiGALTextureHandle m_hStagingTexture;
  xiiGALBufferHandle  m_hStagingBuffer;
};

#include <RendererVulkan/Resources/Implementation/TextureVulkan_inl.h>
