#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Device/SwapChain.h>

class XII_GRAPHICSVULKAN_DLL xiiGALSwapChainVulkan final : public xiiGALSwapChain
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
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainVulkan(const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChainVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

  xiiResult CreateBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan);

  void DestroyBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan);

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

#include <GraphicsVulkan/Device/Implementation/SwapChainVulkan_inl.h>
