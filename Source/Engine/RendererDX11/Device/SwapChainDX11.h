
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

struct IDXGISwapChain;

class xiiGALSwapChainDX11 : public xiiGALWindowSwapChain
{
public:
  virtual void      AcquireNextRenderTarget(xiiGALDevice* pDevice) override;
  virtual void      PresentRenderTarget(xiiGALDevice* pDevice) override;
  virtual xiiResult UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode) override;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALSwapChainDX11(const xiiGALWindowSwapChainCreationDescription& Description);

  virtual ~xiiGALSwapChainDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  xiiResult         CreateBackBufferInternal(xiiGALDeviceDX11* pDXDevice);
  void              DestroyBackBufferInternal(xiiGALDeviceDX11* pDXDevice);
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;


  IDXGISwapChain* m_pDXSwapChain;

  xiiGALTextureHandle m_hBackBufferTexture;

  xiiEnum<xiiGALPresentMode> m_CurrentPresentMode;
  bool                       m_bCanMakeDirectScreenshots = true;
  // We can't do screenshots if we're using any of the FLIP swap effects.
  // If the user requests screenshots anyways, we need to put another buffer in between.
  // For ease of use, this is m_hBackBufferTexture and the actual "OS backbuffer" is this texture.
  // In any other case this handle is unused.
  xiiGALTextureHandle m_hActualBackBufferTexture;
};

#include <RendererDX11/Device/Implementation/SwapChainDX11_inl.h>
