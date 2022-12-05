
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

class xiiGALSwapChainDiligent : public xiiGALWindowSwapChain
{
public:
  virtual void      AcquireNextRenderTarget(xiiGALDevice* pDevice) override;
  virtual void      PresentRenderTarget(xiiGALDevice* pDevice) override;
  virtual xiiResult UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode) override;

  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::ISwapChain>& GetSwapChain();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainDiligent(const xiiGALWindowSwapChainCreationDescription& Description);

  virtual ~xiiGALSwapChainDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;
  xiiResult         CreateBackBufferInternal(xiiGALDeviceDiligent* pDeviceDiligent);

  xiiGALDeviceDiligent* m_pDeviceDiligent = nullptr;

  Diligent::RefCntAutoPtr<Diligent::ISwapChain> m_pSwapChain;

  xiiEnum<xiiGALPresentMode> m_CurrentPresentMode;

  xiiGALTextureHandle          m_hBackbufferTexture;
  xiiGALRenderTargetViewHandle m_hBackbufferTextureView;
};

#include <RendererDiligent/Device/Implementation/SwapChainDiligent_inl.h>
