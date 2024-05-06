#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Device/SwapChain.h>

class XII_GRAPHICSD3D12_DLL xiiGALSwapChainD3D12 final : public xiiGALSwapChain
{
public:
  virtual void AcquireNextRenderTarget(xiiGALDevice* pDevice) override final;

  virtual void Present(xiiGALDevice* pDevice) override final;

  virtual xiiResult Resize(xiiGALDevice* pDevice, xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) override final;

  virtual void SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode) override final;

  virtual void SetWindowedMode() override final;

  virtual void SetMaximumFrameLatency(xiiUInt32 uiMaxLatency) override final;

  Diligent::ISwapChain* GetSwapChain() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChainD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  xiiResult CreateBackBufferInternal(xiiGALDeviceD3D12* pDeviceD3D12);

  void DestroyBackBufferInternal(xiiGALDeviceD3D12* pDeviceD3D12);

protected:
  struct RenderTargetInfo
  {
    XII_DECLARE_POD_TYPE();

    Diligent::ITextureView* m_pTextureView = nullptr;
    xiiGALTextureHandle     m_hRenderTargetHandle;
  };

  Diligent::ISwapChain*                m_pSwapChain = nullptr;
  xiiHybridArray<RenderTargetInfo, 2U> m_BackbufferTextures;
};

#include <GraphicsD3D12/Device/Implementation/SwapChainD3D12_inl.h>
