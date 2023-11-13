#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/RenderTargetSetup.h>

/// \brief Interface that defines methods to manipulate a swap chain object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALSwapChain : public xiiGALObject<xiiGALSwapChainCreationDescription>
{
public:
  /// \brief Acquires the next render target for presenting.
  virtual void AcquireNextRenderTarget(xiiGALDevice* pDevice) = 0;

  /// \brief This presents a rendered image to the screen.
  virtual void Present(xiiGALDevice* pDevice, xiiUInt32 uiSyncInterval) = 0;

  /// \brief This changes the swap chain size.
  ///
  /// \param newSize      - The new logical swap chain width and height (not accounting for the pre-transform), in pixels.
  /// \param newTransform - newTransform The new surface transform.
  ///
  /// \note When resizing non-primary swap chains, the engine unbinds the swap chain buffers from the output.
  virtual xiiResult Resize(xiiGALDevice* pDevice, xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) = 0;

  /// \brief This sets the swap chain to full screen mode. Note that this is only supported on the Windows platform.
  virtual void SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode) = 0;

  /// \brief This sets the swap chain to windowed mode. Note that this is only supported on the Windows platform.
  virtual void SetWindowedMode() = 0;

  /// \brief This sets the maximum number of frames that the swap chain is allowed to queue for rendering.
  ///
  /// This value is only relevant for DirectX11 and DirectX12 backends and ignored for others. By default it matches the number of buffers in the swap chain. For example, for a 2-buffer
  /// swap chain, the CPU can enqueue frames 0 and 1, but Present command of frame 2 will block until frame 0 is presented. If in the example above the maximum frame latency is set
  /// to 1, then Present command of frame 1 will block until Present of frame 0 is complete.
  virtual void SetMaximumFrameLatency(xiiUInt32 uiMaxLatency) = 0;

  const xiiGALRenderTargets& GetRenderTargets() const;

  xiiGALTextureHandle GetBackBufferTexture() const;

  xiiSizeU32 GetCurrentSize() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALSwapChain(const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChain();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;

  xiiGALRenderTargets m_RenderTargets;
  xiiSizeU32          m_CurrentSize = {};
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALSwapChain);

#include <GraphicsFoundation/Device/Implementation/SwapChain_inl.h>
