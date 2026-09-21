/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

/// Interface that defines methods to manipulate a swap chain object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALSwapChain : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALSwapChain, xiiGALDeviceObject);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALSwapChainCreationDescription& GetDescription() const { return m_Description; };

  /// This retrieves the current back buffer texture.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTexture> GetBackBufferTexture() const { return m_pBackBufferTexture; };

  /// This retrieves the current swap chain size.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSizeU32 GetCurrentSize() const { return m_CurrentSize; };

  /// This sets the present mode.
  XII_ALWAYS_INLINE void SetPresentMode(xiiEnum<xiiGALPresentMode> presentMode) { m_PresentMode = presentMode; };

  /// This retrieves the current present mode.
  [[nodiscard]] XII_ALWAYS_INLINE xiiEnum<xiiGALPresentMode> GetPresentMode() const { return m_PresentMode; };

  /// This presents a rendered image to the screen. Additionally, acquires the next render target for presenting on where supported.
  virtual void Present() = 0;

  /// This changes the swap chain size.
  ///
  /// \param newSize      - The new logical swap chain width and height (not accounting for the pre-transform), in pixels.
  /// \param newTransform - The new surface transform.
  ///
  /// \note When resizing non-primary swap chains, the engine unbinds the swap chain buffers from the output.
  virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) = 0;

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

  xiiSizeU32 m_CurrentSize = xiiSizeU32::MakeZero();
};
