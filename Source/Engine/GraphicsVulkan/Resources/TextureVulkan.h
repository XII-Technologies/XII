#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

namespace vk
{
  class Image;
}

class XII_GRAPHICSVULKAN_DLL xiiGALTextureVulkan final : public xiiGALTexture
{
public:
  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final;

  XII_ALWAYS_INLINE vk::Image GetVulkanImage() const { return m_vkImage; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureVulkan();

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData) override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
  vk::Image m_vkImage;
};
