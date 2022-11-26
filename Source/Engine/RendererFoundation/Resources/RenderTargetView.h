
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class XII_RENDERERFOUNDATION_DLL xiiGALRenderTargetView : public xiiGALObject<xiiGALRenderTargetViewCreationDescription>
{
public:
  XII_ALWAYS_INLINE xiiGALTexture* GetTexture() const { return m_pTexture; }

protected:
  friend class xiiGALDevice;

  xiiGALRenderTargetView(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& description);

  virtual ~xiiGALRenderTargetView();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;

  xiiGALTexture* m_pTexture;
};
