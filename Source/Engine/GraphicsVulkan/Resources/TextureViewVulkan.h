#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/TextureView.h>

namespace vk
{
  class ImageView;
}

class XII_GRAPHICSVULKAN_DLL xiiGALTextureViewVulkan final : public xiiGALTextureView
{
public:
  XII_ALWAYS_INLINE vk::ImageView GetVulkanImageView() const { return m_vkImageView; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureViewVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription);

  virtual ~xiiGALTextureViewVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  vk::ImageView m_vkImageView;
};
