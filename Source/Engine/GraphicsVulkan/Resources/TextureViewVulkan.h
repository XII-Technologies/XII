#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/TextureView.h>

class XII_GRAPHICSVULKAN_DLL xiiGALTextureViewVulkan final : public xiiGALTextureView
{
public:

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureViewVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription);

  virtual ~xiiGALTextureViewVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsVulkan/Resources/Implementation/TextureViewVulkan_inl.h>
