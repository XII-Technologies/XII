#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

/// \brief Interface that defines methods to manipulate a swap chain object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALSwapChain : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALSwapChain, xiiGALDeviceObject);

public:
  /// \brief This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALSwapChainCreationDescription& GetDescription() const { return m_Description; };

  /// \brief This retrieves the current back buffer texture.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTexture> GetBackBufferTexture() const { return m_pBackBufferTexture; };

  /// \brief This retrieves the current swap chain size.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSizeU32 GetCurrentSize() const { return m_Description.m_Resolution; };

  /// \brief This sets the present mode.
  XII_ALWAYS_INLINE void SetPresentMode(xiiEnum<xiiGALPresentMode> presentMode) { m_PresentMode = presentMode; };

  /// \brief This retrieves the current present mode.
  [[nodiscard]] XII_ALWAYS_INLINE xiiEnum<xiiGALPresentMode> GetPresentMode() const { return m_PresentMode; };

  /// \brief This presents a rendered image to the screen. Additionally, acquires the next render target for presenting on where supported.
  virtual void Present() = 0;

  /// \brief This changes the swap chain size.
  ///
  /// \param newSize      - The new logical swap chain width and height (not accounting for the pre-transform), in pixels.
  /// \param newTransform - The new surface transform.
  ///
  /// \note When resizing non-primary swap chains, the engine unbinds the swap chain buffers from the output.
  virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) = 0;

  /// \brief This sets the swap chain to full screen mode. Note that this is only supported on the Windows platform.
  virtual void SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode) = 0;

  /// \brief This sets the swap chain to windowed mode. Note that this is only supported on the Windows platform.
  virtual void SetWindowedMode() = 0;

  /// \brief This sets the maximum number of frames that the swap chain is allowed to queue for rendering.
  ///
  /// This value is only relevant for DirectX11 and DirectX12 backends and ignored for others. By default it matches the number of buffers in the swap chain. For example, for a 2-buffer
  /// swap chain, the CPU can enqueue frames 0 and 1, but Present command of frame 2 will block until frame 0 is presented. If in the example above the maximum frame latency is set
  /// to 1, then Present command of frame 1 will block until Present of frame 0 is complete.
  virtual void SetMaximumFrameLatency(xiiUInt32 uiMaxLatency) { XII_IGNORE_UNUSED(uiMaxLatency); };

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALSwapChain(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChain();

  virtual xiiResult InitPlatform() = 0;

  xiiSharedPtr<xiiGALTexture> m_pBackBufferTexture;

  xiiEnum<xiiGALPresentMode> m_PresentMode = xiiGALPresentMode::VSync;

  xiiGALSwapChainCreationDescription m_Description;

  xiiEnum<xiiGALSurfaceTransform> m_DesiredSurfaceTransform = xiiGALSurfaceTransform::Optimal;
};
