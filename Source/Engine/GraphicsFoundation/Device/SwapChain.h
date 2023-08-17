#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/Descriptors.h>

/// \brief Interface that defines methods to manipulate a swap chain object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALSwapChain : public xiiGALObject<xiiGALSwapChainCreationDescription>
{
public:
  /// \brief This presents a rendered image to the screen.
  virtual void Present(xiiUInt32 uiSyncInterval) = 0;

  /// \brief This changes the swap chain size.
  /// 
  /// \param newSize      - The new logical swap chain width and height (not accounting for the pre-transform), in pixels.
  /// \param newTransform - newTransform The new surface transform.
  ///
  /// \note When resizing non-primary swap chains, the engine unbinds the swap chain buffers from the output.
  virtual void Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) = 0;

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

  /// \brief This returns the render target view handle of the current back buffer in the swap chain.
  ///
  /// \note For the Direct3D12 and Vulkan backends, the function returns a different handle for every offscreen buffer in the swap chain
  /// (flipped by every call to xiiGALSwapChain::Present()). For the Direct3D11 backend it always returns the same handle.
  ///
  /// The method does *NOT* increment the reference counter of the returned object, so ReleaseRef() must not be called.
  virtual xiiGALTextureViewHandle GetCurrentBackBufferRTV() = 0;

  /// \brief This returns the depth-stencil view handle of the depth buffer.
  ///
  /// The method does *NOT* increment the reference counter of the returned object, so ReleaseRef() must not be called.
  virtual xiiGALTextureViewHandle GetDepthBufferDSV() = 0;

protected:
  friend class xiiGALDevice;

  xiiGALSwapChain(const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChain();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

#include <GraphicsFoundation/Device/Implementation/SwapChain_inl.h>
