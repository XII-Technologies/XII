/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

/// This class provides functionality to capture images from the GPU, typically used for screenshots, debugging or analysis purposes.
class XII_GRAPHICSFOUNDATION_DLL xiiGALImageCapture
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALImageCapture);

public:
  struct CaptureDescription
  {
    xiiSharedPtr<xiiGALTexture> m_pTexture;
    xiiUInt32                   m_uiTextureID = 0U;

    XII_ALWAYS_INLINE explicit operator bool() const { return m_pTexture != nullptr; }
  };

  xiiGALImageCapture(xiiSharedPtr<xiiGALDevice> pDevice);
  ~xiiGALImageCapture();

  /// Retrieves the device associated with this image capture instance.
  [[nodiscard]] xiiSharedPtr<xiiGALDevice> GetDevice() const { return m_pDevice; }

  /// Retrieves the oldest available capture texture, if any.
  [[nodiscard]] CaptureDescription GetCapture();

  /// Checks if there is a texture available for capture.
  [[nodiscard]] bool HasCapture();

  /// Returns the number of textures that are currently available for capture.
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetPendingCaptureCount() const
  {
    XII_LOCK(m_PendingTexturesMutex);

    return m_PendingTextures.GetCount();
  }

  /// Captures the current frame from the specified swap chain and command list.
  void Capture(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSharedPtr<xiiGALCommandList> pCommandList, xiiUInt32 uiFrameIndex);

  /// Waits on the capture fence until the completed value is reached.
  void WaitForCompletedValue();

  /// Recycles a staging texture that is no longer needed.
  void RecycleStagingTexture(xiiSharedPtr<xiiGALTexture>&& pStagingTexture);

private:
  struct PendingTextureDescription
  {
    explicit PendingTextureDescription(xiiSharedPtr<xiiGALTexture> pStagingTexture, xiiUInt32 uiTextureID, xiiUInt64 uiFenceValue) :
      m_pStagingTexture(std::move(pStagingTexture)), m_uiTextureID(uiTextureID), m_uiFenceValue(uiFenceValue)
    {
    }

    xiiSharedPtr<xiiGALTexture> m_pStagingTexture;
    const xiiUInt32             m_uiTextureID;
    const xiiUInt64             m_uiFenceValue;

    bool operator==(const PendingTextureDescription& rhs) const
    {
      return m_pStagingTexture == rhs.m_pStagingTexture && m_uiTextureID == rhs.m_uiTextureID && m_uiFenceValue == rhs.m_uiFenceValue;
    }
  };

  xiiSharedPtr<xiiGALDevice> m_pDevice;
  xiiSharedPtr<xiiGALFence>  m_pFence;

  mutable xiiMutex                                m_AvailableTexturesMutex;
  xiiHybridArray<xiiSharedPtr<xiiGALTexture>, 2U> m_AvailableTextures;

  mutable xiiMutex                    m_PendingTexturesMutex;
  xiiDeque<PendingTextureDescription> m_PendingTextures;

  xiiUInt64 m_uiCurrentFenceValue;
};
