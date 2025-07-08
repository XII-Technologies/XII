#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Threading/Mutex.h>

struct xiiGALDeviceEvent;

/// \brief Manages a pool of reusable GPU resources such as buffers, textures, and samplers.
///
/// The GPU resource pool helps reduce allocation overhead and memory fragmentation by reusing previously created GPU resources based on matching creation parameters. It is thread-safe
/// and supports garbage collection of unused resources based on usage age and memory thresholds.
///
/// Resources can be retrieved using GetBuffer(), GetTexture(), GetSampler(), or GetRenderTarget(), and must be returned to the pool using the corresponding Return*() methods. The pool will
/// automatically release stale resources that have not been used for a defined number of frames, or when memory or allocation count thresholds are exceeded.
///
/// A global default instance can be accessed and configured using GetDefaultInstance() and SetDefaultInstance().
///
/// \see xiiGALBufferCreationDescription, xiiGALTextureCreationDescription, xiiGALSamplerCreationDescription
class XII_GRAPHICSCORE_DLL xiiGPUResourcePool
{
public:
  /// \brief Constructs a GPU resource pool with default garbage collection thresholds.
  xiiGPUResourcePool();

  /// \brief Destroys the resource pool and releases all remaining resources.
  ~xiiGPUResourcePool();


  /// \brief Retrieves a buffer from the pool or allocates a new one if no suitable match exists.
  xiiSharedPtr<xiiGALBuffer> GetBuffer(const xiiGALBufferCreationDescription& description);

  /// \brief Returns a buffer to the pool for potential reuse.
  void ReturnBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer);


  /// \brief Retrieves a render target texture with simplified input parameters.
  ///
  /// Internally constructs a full texture creation description and either reuses or allocates a new texture.
  xiiSharedPtr<xiiGALTexture> GetRenderTarget(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiEnum<xiiGALResourceFormat> format, xiiEnum<xiiGALMSAASampleCount> sampleCount, xiiUInt32 uiSliceColunt, bool bIsArray);

  /// \brief Retrieves a texture from the pool or creates a new one with the provided description.
  xiiSharedPtr<xiiGALTexture> GetTexture(const xiiGALTextureCreationDescription& description);

  /// \brief Returns a texture to the pool for reuse.
  void ReturnTexture(xiiSharedPtr<xiiGALTexture> pTexture);


  /// \brief Retrieves a GPU sampler from the pool or allocates one if needed.
  xiiSharedPtr<xiiGALSampler> GetSampler(const xiiGALSamplerCreationDescription& description);

  /// \brief Returns a sampler to the pool for reuse.
  void ReturnSampler(xiiSharedPtr<xiiGALSampler> pSampler);


  /// \brief Releases resources that have not been used for at least \a uiMinimumAge frames.
  ///
  /// This can be triggered automatically based on thresholds or manually, for example after editor window resizes or level transitions.
  ///
  /// \param uiMinimumAge How many frames at least the resource needs to have been unused before it will be stale.
  void ReleaseStaleResources(xiiUInt32 uiMinimumAge);


  /// \brief Returns the global default GPU resource pool instance, if any.
  static xiiGPUResourcePool* GetDefaultInstance();

  /// \brief Sets the global default GPU resource pool instance.
  static void SetDefaultInstance(xiiGPUResourcePool* pDefaultInstance);

protected:
  /// \brief Internal method that checks thresholds and releases stale resources if needed.
  void CheckAndPotentiallyReleaseStaleResources();

  /// \brief Updates internal memory statistics for debugging or visualization.
  void UpdateMemoryStats() const;

  /// \brief Handles events triggered by the underlying GAL device (e.g. EndFrame).
  void GALDeviceEventHandler(const xiiGALDeviceEvent& e);

  struct TextureHandleWithAge
  {
    xiiSharedPtr<xiiGALTexture> m_pTexture;
    xiiUInt64                   m_uiLastUsed = 0ULL;
  };

  struct BufferHandleWithAge
  {
    xiiSharedPtr<xiiGALBuffer> m_pBuffer;
    xiiUInt64                  m_uiLastUsed = 0ULL;
  };

  struct SamplerHandleWithAge
  {
    xiiSharedPtr<xiiGALSampler> m_pSampler;
    xiiUInt64                   m_uiLastUsed = 0ULL;
  };

  xiiEventSubscriptionID m_GALDeviceEventSubscriptionID;
  xiiUInt64              m_uiMemoryThresholdForGC         = 256 * 1024 * 1024; ///< Memory limit (in bytes) that triggers resource garbage collection.
  xiiUInt64              m_uiCurrentlyAllocatedMemory     = 0;                 ///< Total memory currently allocated through this pool.
  xiiUInt16              m_uiNumAllocationsThresholdForGC = 128;               ///< Allocation count threshold that triggers garbage collection.
  xiiUInt16              m_uiNumAllocationsSinceLastGC    = 0;                 ///< Tracks how many allocations have occurred since the last GC.
  xiiUInt16              m_uiFramesThresholdSinceLastGC   = 60;                ///< Number of frames to wait before rechecking unused resources for GC.
  xiiUInt16              m_uiFramesSinceLastGC            = 0;                 ///< Number of frames that have passed since the last GC.

  // Resource storage maps indexed by hashed creation description.
  xiiMap<xiiUInt32, xiiDynamicArray<TextureHandleWithAge>> m_AvailableTextures;
  xiiSet<xiiSharedPtr<xiiGALTexture>>                      m_TexturesInUse;

  xiiMap<xiiUInt32, xiiDynamicArray<BufferHandleWithAge>> m_AvailableBuffers;
  xiiSet<xiiSharedPtr<xiiGALBuffer>>                      m_BuffersInUse;

  xiiMap<xiiUInt32, xiiDynamicArray<SamplerHandleWithAge>> m_AvailableSamplers;
  xiiSet<xiiSharedPtr<xiiGALSampler>>                      m_SamplersInUse;

  xiiMutex m_Lock;

  xiiSharedPtr<xiiGALDevice> m_pDevice;

private:
  static xiiGPUResourcePool* s_pDefaultInstance;
};
