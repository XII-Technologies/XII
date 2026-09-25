/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// Frame-scoped GPU resource pool dedicated to the render graph's transient resource needs.
///
/// Transient resources are GPU textures and buffers that are created for a single frame, used by one
/// or more render graph passes, and then returned to the pool for reuse in subsequent frames. The cache
/// uses the resource creation description hash as a key, so resources with identical descriptions are
/// reused without re-allocation.
///
/// ## Frame lifecycle
/// \code{.cpp}
/// cache.BeginFrame(uiFrameIndex); // Prepare pool for this frame.
/// // ... graph executes, passes call AcquireTexture / ReturnTexture ...
/// cache.EndFrame();               // Return all still-active resources back to pool.
/// \endcode
///
/// ## Thread safety
/// AcquireTexture, ReturnTexture, AcquireBuffer, ReturnBuffer are all protected by an internal
/// mutex and are safe to call concurrently from parallel pass execution threads.
///
/// ## Stale eviction
/// Call ReleaseStaleResources() periodically (e.g. once per frame) to destroy pooled resources
/// that have not been used for more than the configured number of frames.
class XII_GRAPHICSCORE_DLL xiiRenderGraphResourceCache
{
public:
  xiiRenderGraphResourceCache();
  ~xiiRenderGraphResourceCache();

  /// Initializes the cache with the GAL device used to create resources.
  void Initialize(xiiSharedPtr<xiiGALDevice> pDevice);

  /// Releases all pooled and active resources and nulls the device reference.
  void Shutdown();

  /// Called once at the start of a frame before Execute(). Uses a conservative three-frame
  /// retirement window for callers that do not expose an exact completed-frame index.
  void BeginFrame(xiiUInt64 uiFrameIndex);

  /// Starts a frame and promotes only resources whose last GPU frame is known to be complete.
  /// This overload is preferred by render loops with explicit frame fences.
  void BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrameIndex);

  /// Called once at the end of a frame after Execute(). Returns all active resources to the pool.
  void EndFrame();

  // Texture

  /// Returns a texture matching the given description, creating one if no pool entry exists.
  [[nodiscard]] xiiSharedPtr<xiiGALTexture> AcquireTexture(const xiiGALTextureCreationDescription& description);

  /// Returns a texture to the pool for potential reuse in subsequent frames.
  void ReturnTexture(xiiSharedPtr<xiiGALTexture> pTexture);

  // Buffer

  /// Returns a buffer matching the given description, creating one if no pool entry exists.
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> AcquireBuffer(const xiiGALBufferCreationDescription& description);

  /// Returns a buffer to the pool for potential reuse in subsequent frames.
  void ReturnBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer);

  // Maintenance

  /// Destroys pooled resources that have not been acquired for more than uiMinAgeFrames frames.
  ///
  /// \param uiMinAgeFrames Resources unused for at least this many frames are released. Default is 4.
  void ReleaseStaleResources(xiiUInt32 uiMinAgeFrames = 4U);

  /// Returns the number of resources currently acquired (not yet returned this frame).
  [[nodiscard]] xiiUInt32 GetActiveResourceCount() const;

  /// Returns the number of resources sitting idle in the pool.
  [[nodiscard]] xiiUInt32 GetPooledResourceCount() const;

private:
  struct PooledTexture
  {
    xiiSharedPtr<xiiGALTexture> m_pTexture;
    xiiUInt64                   m_uiLastUsedFrame = 0ULL;
  };

  struct PooledBuffer
  {
    xiiSharedPtr<xiiGALBuffer> m_pBuffer;
    xiiUInt64                  m_uiLastUsedFrame = 0ULL;
  };

private:
  xiiSharedPtr<xiiGALDevice> m_pDevice;
  xiiUInt64                  m_uiCurrentFrame   = 0ULL;
  xiiUInt64                  m_uiCompletedFrame = 0ULL;

  /// Idle textures keyed by creation-description hash.
  xiiHashTable<xiiUInt32, xiiDynamicArray<PooledTexture>> m_TexturePool;

  /// Idle buffers keyed by creation-description hash.
  xiiHashTable<xiiUInt32, xiiDynamicArray<PooledBuffer>> m_BufferPool;

  /// Resources leave the active set after recording, but remain unavailable until the GPU frame
  /// that referenced them has completed.
  xiiDynamicArray<PooledTexture> m_RetiredTextures;
  xiiDynamicArray<PooledBuffer>  m_RetiredBuffers;

  /// Textures that have been acquired and are in flight this frame.
  xiiDynamicArray<xiiSharedPtr<xiiGALTexture>> m_ActiveTextures;

  /// Buffers that have been acquired and are in flight this frame.
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> m_ActiveBuffers;

  mutable xiiMutex m_Mutex;
};
