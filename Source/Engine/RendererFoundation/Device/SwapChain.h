
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Size.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class XII_RENDERERFOUNDATION_DLL xiiGALSwapChain : public xiiGALObject<xiiGALSwapChainCreationDescription>
{
public:
  const xiiGALRenderTargets& GetRenderTargets() const { return m_RenderTargets; }
  xiiGALTextureHandle        GetBackBufferTexture() const { return m_RenderTargets.m_hRTs[0]; }
  xiiSizeU32                 GetCurrentSize() const { return m_CurrentSize; }

  virtual void      AcquireNextRenderTarget(xiiGALDevice* pDevice)                                    = 0;
  virtual void      PresentRenderTarget(xiiGALDevice* pDevice)                                        = 0;
  virtual xiiResult UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode) = 0;

  virtual ~xiiGALSwapChain();

protected:
  friend class xiiGALDevice;

  xiiGALSwapChain(const xiiRTTI* pSwapChainType);

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice)   = 0;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;

  xiiGALRenderTargets m_RenderTargets;
  xiiSizeU32          m_CurrentSize = {};
};
XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALSwapChain);


class XII_RENDERERFOUNDATION_DLL xiiGALWindowSwapChain : public xiiGALSwapChain
{
public:
  using Functor = xiiDelegate<xiiGALSwapChainHandle(const xiiGALWindowSwapChainCreationDescription&)>;
  static void SetFactoryMethod(Functor factory);

  static xiiGALSwapChainHandle Create(const xiiGALWindowSwapChainCreationDescription& desc);

public:
  const xiiGALWindowSwapChainCreationDescription& GetWindowDescription() const { return m_WindowDesc; }

protected:
  xiiGALWindowSwapChain(const xiiGALWindowSwapChainCreationDescription& Description);

protected:
  static Functor s_Factory;

protected:
  xiiGALWindowSwapChainCreationDescription m_WindowDesc;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALWindowSwapChain);

#include <RendererFoundation/Device/Implementation/SwapChain_inl.h>
