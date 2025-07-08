#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Threading/Mutex.h>

struct xiiGALDeviceEvent;

/// \brief This class serves as a pool for GPU related resources (e.g. buffers and textures required for rendering).
/// Note that the functions creating and returning render targets are thread safe (by using a mutex).
class XII_GRAPHICSCORE_DLL xiiGPUResourcePool
{
public:
  xiiGPUResourcePool();

  ~xiiGPUResourcePool();


  xiiSharedPtr<xiiGALBuffer> GetBuffer(const xiiGALBufferCreationDescription& description);

  void ReturnBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer);


  xiiSharedPtr<xiiGALTexture> GetTexture(const xiiGALTextureCreationDescription& description);

  void ReturnTexture(xiiSharedPtr<xiiGALTexture> pTexture);


  xiiSharedPtr<xiiGALSampler> GetSampler(const xiiGALSamplerCreationDescription& description);

  void ReturnSampler(xiiSharedPtr<xiiGALSampler> pSampler);


  /// \brief Tries to free resources which are currently in the pool.
  /// Triggered automatically due to allocation number / size thresholds but can be triggered manually (e.g. after editor window resize)
  ///
  /// \param uiMinimumAge How many frames at least the resource needs to have been unused before it will be GCed.
  void ReleaseStaleResources(xiiUInt32 uiMinimumAge);


  static xiiGPUResourcePool* GetDefaultInstance();

  static void SetDefaultInstance(xiiGPUResourcePool* pDefaultInstance);

protected:
  void CheckAndPotentiallyReleaseStaleResources();
  void UpdateMemoryStats() const;
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
  xiiUInt64              m_uiMemoryThresholdForGC         = 256 * 1024 * 1024;
  xiiUInt64              m_uiCurrentlyAllocatedMemory     = 0;
  xiiUInt16              m_uiNumAllocationsThresholdForGC = 128;
  xiiUInt16              m_uiNumAllocationsSinceLastGC    = 0;
  xiiUInt16              m_uiFramesThresholdSinceLastGC   = 60; ///< Every 60 frames resources unused for more than 10 frames in a row are GCed.
  xiiUInt16              m_uiFramesSinceLastGC            = 0;

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
