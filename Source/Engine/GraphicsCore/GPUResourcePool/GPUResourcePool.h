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

  /// \brief Returns a render target handle for the given texture description
  /// Note that you should return the handle to the pool and never destroy it directly with the device.
  xiiSharedPtr<xiiGALTexture> GetRenderTarget(const xiiGALTextureCreationDescription& textureDesc);

  /// \brief Convenience functions which creates a texture description fit for a 2d render target without a mip chains.
  xiiSharedPtr<xiiGALTexture> GetRenderTarget(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiEnum<xiiGALResourceFormat> format, xiiEnum<xiiGALMSAASampleCount> sampleCount = xiiGALMSAASampleCount::OneSample, xiiUInt32 uiSliceColunt = 1, bool bIsArray = false);

  /// \brief Returns a render target to the pool so other consumers can use it.
  /// Note that targets which are returned to the pool are susceptible to destruction due to garbage collection.
  void ReturnRenderTarget(xiiSharedPtr<xiiGALTexture> hRenderTarget);


  /// \brief Returns a buffer handle for the given buffer description
  xiiSharedPtr<xiiGALBuffer> GetBuffer(const xiiGALBufferCreationDescription& bufferDesc);

  /// \brief Returns a buffer to the pool so other consumers can use it.
  void ReturnBuffer(xiiSharedPtr<xiiGALBuffer> hBuffer);


  /// \brief Tries to free resources which are currently in the pool.
  /// Triggered automatically due to allocation number / size thresholds but can be triggered manually (e.g. after editor window resize)
  ///
  /// \param uiMinimumAge How many frames at least the resource needs to have been unused before it will be GCed.
  void RunGC(xiiUInt32 uiMinimumAge);


  static xiiGPUResourcePool* GetDefaultInstance();
  static void                SetDefaultInstance(xiiGPUResourcePool* pDefaultInstance);

protected:
  void CheckAndPotentiallyRunGC();
  void UpdateMemoryStats() const;
  void GALDeviceEventHandler(const xiiGALDeviceEvent& e);

  struct TextureHandleWithAge
  {
    xiiSharedPtr<xiiGALTexture> m_pTexture;
    xiiUInt64                   m_uiLastUsed = 0;
  };

  struct BufferHandleWithAge
  {
    xiiSharedPtr<xiiGALBuffer> m_pBuffer;
    xiiUInt64                  m_uiLastUsed = 0;
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

  xiiMutex m_Lock;

  xiiSharedPtr<xiiGALDevice> m_pDevice;

private:
  static xiiGPUResourcePool* s_pDefaultInstance;
};
