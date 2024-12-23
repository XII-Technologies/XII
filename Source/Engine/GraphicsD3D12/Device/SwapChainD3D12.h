#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Device/SwapChain.h>

struct IDXGISwapChain3;

class XII_GRAPHICSD3D12_DLL xiiGALSwapChainD3D12 final : public xiiGALSwapChain
{
public:
  virtual void Present() override final;

  virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) override final;

  virtual void SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode) override final;

  virtual void SetWindowedMode() override final;

  virtual void SetMaximumFrameLatency(xiiUInt32 uiMaxLatency) override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChainD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

  xiiResult CreateDXGISwapChain();

  xiiResult UpdateSwapChain(bool bCreateNew);

  xiiResult CreateBackBufferInternal(xiiGALDeviceD3D12* pDeviceD3D11);

  void DestroyBackBufferInternal(xiiGALDeviceD3D12* pDeviceD3D11);

  void WaitForFrame();

protected:
  IDXGISwapChain3* m_pDXGISwapChain3 = nullptr;

  xiiHybridArray<xiiGALTextureHandle, 2U> m_BackBufferTextures;

  xiiGALFullScreenModeDescription m_FullScreenMode;

  HANDLE m_FrameLatencyWaitableObject = NULL;

  xiiUInt32 m_uiMaximumFrameLatency = 0U;
};
