#pragma once

#include <RendererFoundation/Resources/Texture.h>

class xiiGALTextureDiligent : public xiiGALTexture
{
public:
  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::ITexture>& GetTexture();

  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::ITexture>& GetStagingTexture();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALTextureDiligent(const xiiGALTextureCreationDescription& Description);

  ~xiiGALTextureDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  xiiResult CreateStagingTexture(xiiGALDeviceDiligent* pDevice);

  Diligent::RefCntAutoPtr<Diligent::ITexture> m_pTexture;

  Diligent::RefCntAutoPtr<Diligent::ITexture> m_pStagingTexture;

  void* m_pExisitingNativeObject = nullptr;
};

#include <RendererDiligent/Resources/Implementation/TextureDiligent_inl.h>
