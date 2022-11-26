#pragma once

#include <RendererFoundation/Resources/Texture.h>


struct ID3D11Resource;

class xiiGALTextureDX11 : public xiiGALTexture
{
public:
  XII_ALWAYS_INLINE ID3D11Resource* GetDXTexture() const;

  XII_ALWAYS_INLINE ID3D11Resource* GetDXStagingTexture() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALTextureDX11(const xiiGALTextureCreationDescription& Description);

  ~xiiGALTextureDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  xiiResult CreateStagingTexture(xiiGALDeviceDX11* pDevice);

  ID3D11Resource* m_pDXTexture;

  ID3D11Resource* m_pDXStagingTexture;

  void* m_pExisitingNativeObject = nullptr;
};

#include <RendererDX11/Resources/Implementation/TextureDX11_inl.h>
