#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/TextureView.h>

class XII_GRAPHICSVULKAN_DLL xiiGALTextureViewVulkan final : public xiiGALTextureView
{
public:
  XII_ALWAYS_INLINE const Diligent::ITextureView* GetTextureView() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureViewVulkan(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription);

  virtual ~xiiGALTextureViewVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::ITextureView> m_pTextureView;
};

#include <GraphicsVulkan/Resources/Implementation/TextureViewVulkan_inl.h>
