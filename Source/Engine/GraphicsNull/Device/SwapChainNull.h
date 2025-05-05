#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Device/SwapChain.h>

class XII_GRAPHICSNULL_DLL xiiGALSwapChainNull final : public xiiGALSwapChain
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALSwapChainNull, xiiGALSwapChain);

public:
  virtual void Present() override final;

  virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) override final;

  virtual void SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode) override final;

  virtual void SetWindowedMode() override final;

  virtual void SetMaximumFrameLatency(xiiUInt32 uiMaxLatency) override final;

protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChainNull();

  virtual xiiResult InitPlatform() override final;

  xiiResult CreateBackBufferInternal();
};
