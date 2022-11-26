
#pragma once

#include <RendererFoundation/Resources/Texture.h>

class XII_RENDERERFOUNDATION_DLL xiiGALProxyTexture : public xiiGALTexture
{
public:
  virtual ~xiiGALProxyTexture();

  virtual const xiiGALResourceBase* GetParentResource() const override;

protected:
  friend class xiiGALDevice;

  xiiGALProxyTexture(const xiiGALTexture& parentTexture);

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  const xiiGALTexture* m_pParentTexture;
};
