#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Device/SwapChain.h>

class XII_GRAPHICSVULKAN_DLL xiiGALSwapChainVulkan final : public xiiGALSwapChain
{
public:
  virtual void AcquireNextRenderTarget() override final;

  virtual void Present() override final;

  virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) override final;

  virtual void SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode) override final;

  virtual void SetWindowedMode() override final;

  virtual void SetMaximumFrameLatency(xiiUInt32 uiMaxLatency) override final;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChainVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  xiiResult CreateBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan);

  void DestroyBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan);

protected:
};

#include <GraphicsVulkan/Device/Implementation/SwapChainVulkan_inl.h>
