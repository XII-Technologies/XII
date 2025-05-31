#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/TextureView.h>

namespace vk
{
  class ImageView;
}

class XII_GRAPHICSVULKAN_DLL xiiGALTextureViewVulkan final : public xiiGALTextureView
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTextureViewVulkan, xiiGALTextureView);

public:
  [[nodiscard]] XII_ALWAYS_INLINE vk::ImageView GetVulkanImageView() const { return m_vkImageView; }
  [[nodiscard]] XII_ALWAYS_INLINE const vk::DescriptorImageInfo* GetVulkanDescriptorImageInfo() const { return &m_vkDescriptorImageInfo; }

protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;
  friend class xiiGALTextureVulkan;

  xiiGALTextureViewVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureViewCreationDescription& creationDescription);

  virtual ~xiiGALTextureViewVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::ImageView m_vkImageView;

  vk::DescriptorImageInfo m_vkDescriptorImageInfo = {};
};
