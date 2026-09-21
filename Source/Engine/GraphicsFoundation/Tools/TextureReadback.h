/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

/// Helper to read back GPU compute outputs from textures into CPU-readable staging textures.
class XII_GRAPHICSFOUNDATION_DLL xiiGALTextureReadback
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALTextureReadback);

public:
  struct ReadbackRequest
  {
    xiiGALTexture* m_pTexture    = nullptr; // Source GPU texture (UAV, SRV, etc.)
    xiiUInt32      m_uiTextureID = 0U;      // Optional user tag (frame or job id)

    // Subresource addressing
    xiiUInt32 m_uiMipLevel   = 0U;
    xiiUInt32 m_uiArraySlice = 0U;

    // Optional region; if zeroed -> full subresource
    xiiUInt32 m_uiRegionX = 0U;
    xiiUInt32 m_uiRegionY = 0U;
    xiiUInt32 m_uiRegionW = 0U;
    xiiUInt32 m_uiRegionH = 0U;

    XII_ALWAYS_INLINE explicit operator bool() const { return m_pTexture != nullptr; }
  };

  struct ReadbackCapture
  {
    xiiGALTexture* m_pStagingTexture = nullptr; // CPU-readable texture with copied data
    xiiUInt32      m_uiTextureID     = 0U;

    // Metadata for consumer-side mapping
    xiiUInt32 m_uiWidth          = 0U;
    xiiUInt32 m_uiHeight         = 0U;
    xiiUInt32 m_uiBytesPerPixel  = 0U;
    xiiUInt32 m_uiRowStrideBytes = 0U; // Row stride of staging subresource

    // Subresource mirrored from request
    xiiUInt32 m_uiMipLevel   = 0U;
    xiiUInt32 m_uiArraySlice = 0U;

    // Region mirrored from request; if zeroed => full subresource
    xiiUInt32 m_uiRegionX = 0U;
    xiiUInt32 m_uiRegionY = 0U;
    xiiUInt32 m_uiRegionW = 0U;
    xiiUInt32 m_uiRegionH = 0U;

    XII_ALWAYS_INLINE explicit operator bool() const { return m_pStagingTexture != nullptr; }
  };

public:
  xiiGALTextureReadback(xiiGALDevice* pDevice);
  ~xiiGALTextureReadback();

  /// Enqueue a readback from a texture into an internal staging texture.
  ///
  /// Records copy commands on the provided command list and signals an internal fence.
  void Enqueue(xiiGALCommandList* pCommandList, const ReadbackRequest& request);

  /// Returns true if the oldest pending readback has completed on the GPU.
  [[nodiscard]] bool HasCompleted() const;

  /// Retrieves the oldest completed readback capture (if any). Non-blocking.
  [[nodiscard]] ReadbackCapture GetCompleted();

  /// Blocks until at least one pending readback completes.
  void WaitForNextCompleted();

  /// Recycle a staging texture back into the internal pool for reuse.
  void RecycleStagingTexture(xiiGALTexture* pStagingTexture);

private:
  struct Pending
  {
    xiiGALTexture*  m_pStagingTexture = nullptr;
    xiiUInt64       m_uiFenceValue    = 0U;
    ReadbackCapture m_CaptureMeta;

    XII_ALWAYS_INLINE bool operator==(const Pending& rhs) const
    {
      return m_pStagingTexture == rhs.m_pStagingTexture && m_uiFenceValue == rhs.m_uiFenceValue && m_CaptureMeta.m_uiTextureID == rhs.m_CaptureMeta.m_uiTextureID;
    }
  };

  struct TextureResource
  {
    xiiSharedPtr<xiiGALTexture> m_pTexture;
    bool                        m_bInUse = false;

    XII_ALWAYS_INLINE bool operator==(const TextureResource& rhs) const
    {
      return m_pTexture == rhs.m_pTexture;
    }
  };

  xiiGALDevice*             m_pDevice;
  xiiSharedPtr<xiiGALFence> m_pFence;
  xiiUInt64                 m_uiNextFenceValue = 1U;

  mutable xiiMutex                    m_PoolMutex;
  xiiHybridArray<TextureResource, 4U> m_StagingPool;

  mutable xiiMutex  m_PendingMutex;
  xiiDeque<Pending> m_Pending;

  // Internal: create or fetch a staging texture matching region size + format.
  xiiGALTexture* AcquireStagingTexture(const xiiGALTextureCreationDescription& description);
};
