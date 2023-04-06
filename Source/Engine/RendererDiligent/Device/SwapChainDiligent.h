
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

struct RenderTargetInfo
{
  XII_DECLARE_POD_TYPE();

  Diligent::ITexture*     m_pTexture;
  Diligent::ITextureView* m_pTextureView;
};

template <>
struct xiiHashHelper<RenderTargetInfo>;

class XII_RENDERERDILIGENT_DLL xiiGALSwapChainDiligent : public xiiGALWindowSwapChain
{
public:
  virtual void      AcquireNextRenderTarget(xiiGALDevice* pDevice) override;
  virtual void      PresentRenderTarget(xiiGALDevice* pDevice) override;
  virtual xiiResult UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode) override;

  XII_ALWAYS_INLINE Diligent::ISwapChain* GetSwapChain();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainDiligent(const xiiGALWindowSwapChainCreationDescription& Description);

  virtual ~xiiGALSwapChainDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  xiiResult CreateBackBufferInternal(xiiGALDeviceDiligent* pDeviceDiligent, bool bInitPlatform);
  void      DestroyBackBufferInternal(xiiGALDeviceDiligent* pDeviceDiligent);

  Diligent::ISwapChain* m_pSwapChain = nullptr;

  xiiHashTable<RenderTargetInfo, xiiGALTextureHandle> m_BackbufferTextures;

  xiiEnum<xiiGALPresentMode> m_CurrentPresentMode;
};

#include <RendererDiligent/Device/Implementation/SwapChainDiligent_inl.h>
