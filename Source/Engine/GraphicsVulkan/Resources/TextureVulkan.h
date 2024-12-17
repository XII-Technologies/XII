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
  XII_ALWAYS_INLINE virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final { return m_SparseTextureProperties; }

  XII_ALWAYS_INLINE vk::Image GetVulkanImage() const { return m_vkImage; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureVulkan();

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData) override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

  void InitializeSparseTextureProperties();

protected:
  vk::Image m_vkImage;

  xiiGALSparseTextureProperties m_SparseTextureProperties;
};
