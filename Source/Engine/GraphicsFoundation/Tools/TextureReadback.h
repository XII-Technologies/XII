#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

/// \brief Helper to read back GPU compute outputs from textures into CPU-readable staging textures.
class XII_GRAPHICSFOUNDATION_DLL xiiGALTextureReadback
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALTextureReadback);

public:
  struct ReadbackRequest
  {
    xiiSharedPtr<xiiGALTexture> m_pTexture;         // Source GPU texture (UAV, SRV, etc.)
    xiiUInt32                   m_uiTextureID = 0U; // Optional user tag (frame or job id)

    // Subresource addressing
    xiiUInt32 m_uiMipLevel   = 0U;
    xiiUInt32 m_uiArraySlice = 0U;

    // Optional region; if zeroed -> full subresource
    xiiUInt32 m_uiRegionX = 0U;
    xiiUInt32 m_uiRegionY = 0U;
    xiiUInt32 m_uiRegionW = 0U;
    xiiUInt32 m_uiRegionH = 0U;

    // Optional CPU repack settings (applied after map)
    bool m_bRepackTightRows   = false; // Copies rows to width*Bpp
    bool m_bConvertBGRAtoRGBA = false; // Simple swizzle for common 4-channel formats

    XII_ALWAYS_INLINE explicit operator bool() const { return m_pTexture != nullptr; }
  };

  struct ReadbackCapture
  {
    xiiSharedPtr<xiiGALTexture> m_pStagingTexture; // CPU-readable texture with copied data
    xiiUInt32                   m_uiTextureID = 0U;

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

    // Repack hints (consumer may call RepackTightRows / BGRAtoRGBA on mapped data)
    bool m_bRepackTightRows   = false;
    bool m_bConvertBGRAtoRGBA = false;

    XII_ALWAYS_INLINE explicit operator bool() const { return m_pStagingTexture != nullptr; }
  };

public:
  xiiGALTextureReadback(xiiSharedPtr<xiiGALDevice> pDevice);
  ~xiiGALTextureReadback();

  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALDevice> GetDevice() const { return m_pDevice; }

  /// \brief Enqueue a readback from a texture into an internal staging texture.
  /// Records copy commands on the provided command list and signals an internal fence.
  void Enqueue(xiiSharedPtr<xiiGALCommandList> pCommandList, const ReadbackRequest& request);

  /// \brief Returns true if the oldest pending readback has completed on the GPU.
  [[nodiscard]] bool HasCompleted() const;

  /// \brief Retrieves the oldest completed readback capture (if any). Non-blocking.
  [[nodiscard]] ReadbackCapture GetCompleted();

  /// \brief Blocks until at least one pending readback completes.
  void WaitForNextCompleted();

  /// \brief Recycle a staging texture back into the internal pool for reuse.
  void RecycleStagingTexture(xiiSharedPtr<xiiGALTexture>&& pStagingTexture);

private:
  struct Pending
  {
    xiiSharedPtr<xiiGALTexture> m_pStagingTexture;
    xiiUInt64                   m_uiFenceValue = 0U;
    ReadbackCapture             m_CaptureMeta;

    XII_ALWAYS_INLINE bool operator==(const Pending& rhs) const
    {
      return m_pStagingTexture == rhs.m_pStagingTexture && m_uiFenceValue == rhs.m_uiFenceValue && m_CaptureMeta.m_uiTextureID == rhs.m_CaptureMeta.m_uiTextureID;
    }
  };

  xiiSharedPtr<xiiGALDevice> m_pDevice;
  xiiSharedPtr<xiiGALFence>  m_pFence;
  xiiUInt64                  m_uiNextFenceValue = 1U;

  mutable xiiMutex                                m_PoolMutex;
  xiiHybridArray<xiiSharedPtr<xiiGALTexture>, 4U> m_StagingPool;

  mutable xiiMutex  m_PendingMutex;
  xiiDeque<Pending> m_Pending;

  // Internal: create or fetch a staging texture matching region size + format.
  xiiSharedPtr<xiiGALTexture> AcquireStagingTexture(const xiiGALTextureCreationDescription& description);
};
