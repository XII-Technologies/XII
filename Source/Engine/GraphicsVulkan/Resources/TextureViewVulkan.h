#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/TextureView.h>

class XII_GRAPHICSVULKAN_DLL xiiGALTextureViewVulkan final : public xiiGALTextureView
{
public:
  Diligent::ITextureView* GetTextureView() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureViewVulkan(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription);

  virtual ~xiiGALTextureViewVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::ITextureView* m_pTextureView = nullptr;
};

#include <GraphicsVulkan/Resources/Implementation/TextureViewVulkan_inl.h>
