
#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <RendererDiligent/RendererDiligentDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

class xiiGALDeviceDiligent;

struct RenderTargetInfo
{
  XII_DECLARE_POD_TYPE();

  Diligent::ITexture*     m_pTexture;
  Diligent::ITextureView* m_pTextureView;
};

template <>
struct xiiHashHelper<RenderTargetInfo>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const RenderTargetInfo& value)
  {
    const xiiUInt32 hashA = xiiHashHelper<const void*>::Hash(value.m_pTexture);
    const xiiUInt32 hashB = xiiHashHelper<const void*>::Hash(value.m_pTextureView);
    return xiiHashingUtils::CombineHashValues32(hashA, hashB);
  }

  XII_ALWAYS_INLINE static bool Equal(const RenderTargetInfo& a, const RenderTargetInfo& b)
  {
    return a.m_pTexture == b.m_pTexture && a.m_pTextureView == b.m_pTextureView;
  }
};

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

  Diligent::RefCntAutoPtr<Diligent::ISwapChain> m_pSwapChain;

  xiiHashTable<RenderTargetInfo, xiiGALTextureHandle> m_BackbufferTextures;

  xiiEnum<xiiGALPresentMode> m_CurrentPresentMode;
};

#include <RendererDiligent/Device/Implementation/SwapChainDiligent_inl.h>
