#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

class XII_GRAPHICSVULKAN_DLL xiiGALTextureVulkan final : public xiiGALTexture
{
public:
  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureVulkan();

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData) override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsVulkan/Resources/Implementation/TextureVulkan_inl.h>
