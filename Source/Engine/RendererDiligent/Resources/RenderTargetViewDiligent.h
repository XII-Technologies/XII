#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

class xiiGALRenderTargetViewDiligent : public xiiGALRenderTargetView
{
public:
  XII_ALWAYS_INLINE Diligent::ITextureView* GetRenderTargetView();

  XII_ALWAYS_INLINE Diligent::ITextureView* GetDepthStencilView();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALRenderTargetViewDiligent(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description);

  virtual ~xiiGALRenderTargetViewDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::RefCntAutoPtr<Diligent::ITextureView> m_pRenderTargetView;

  Diligent::RefCntAutoPtr<Diligent::ITextureView> m_pDepthStencilView;
};

#include <RendererDiligent/Resources/Implementation/RenderTargetViewDiligent_inl.h>
