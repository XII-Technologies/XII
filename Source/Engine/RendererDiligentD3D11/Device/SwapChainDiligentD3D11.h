
#pragma once

#include <RendererDiligent/Device/SwapChainDiligent.h>
#include <RendererDiligentD3D11/RendererDiligentD3D11DLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

class XII_RENDERERDILIGENTD3D11_DLL xiiGALSwapChainDiligentD3D11 : public xiiGALSwapChainDiligent
{
public:
  virtual void      AcquireNextRenderTarget(xiiGALDevice* pDevice) override;
  virtual void      PresentRenderTarget(xiiGALDevice* pDevice) override;
  virtual xiiResult UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode) override;

protected:
  friend class xiiGALDeviceDiligentD3D11;
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainDiligentD3D11(const xiiGALWindowSwapChainCreationDescription& Description);

  virtual ~xiiGALSwapChainDiligentD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;
};

#include <RendererDiligentD3D11/Device/Implementation/SwapChainDiligentD3D11_inl.h>
