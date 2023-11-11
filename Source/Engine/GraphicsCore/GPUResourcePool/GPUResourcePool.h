#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsFoundation/Resources/ResourceFormats.h>

struct xiiGALDeviceEvent;

/// \brief This class serves as a pool for GPU related resources (e.g. buffers and textures required for rendering).
/// Note that the functions creating and returning render targets are thread safe (by using a mutex).
class XII_RENDERERCORE_DLL xiiGPUResourcePool
{
public:
  xiiGPUResourcePool();
  ~xiiGPUResourcePool();

  /// \brief Returns a render target handle for the given texture description
  /// Note that you should return the handle to the pool and never destroy it directly with the device.
  xiiGALTextureHandle GetRenderTarget(const xiiGALTextureCreationDescription& textureDesc);

  /// \brief Convenience functions which creates a texture description fit for a 2d render target without a mip chains.
  xiiGALTextureHandle GetRenderTarget(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiGALResourceFormat::Enum format, xiiGALMSAASampleCount::Enum sampleCount = xiiGALMSAASampleCount::None, xiiUInt32 uiSliceColunt = 1);

  /// \brief Returns a render target to the pool so other consumers can use it.
  /// Note that targets which are returned to the pool are susceptible to destruction due to garbage collection.
  void ReturnRenderTarget(xiiGALTextureHandle hRenderTarget);


  /// \brief Returns a buffer handle for the given buffer description
  xiiGALBufferHandle GetBuffer(const xiiGALBufferCreationDescription& bufferDesc);

  /// \brief Returns a buffer to the pool so other consumers can use it.
  void ReturnBuffer(xiiGALBufferHandle hBuffer);


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
    xiiGALTextureHandle m_hTexture;
    xiiUInt64           m_uiLastUsed = 0;
  };

  struct BufferHandleWithAge
  {
    xiiGALBufferHandle m_hBuffer;
    xiiUInt64          m_uiLastUsed = 0;
  };

  xiiEventSubscriptionID m_GALDeviceEventSubscriptionID   = 0;
  xiiUInt64              m_uiMemoryThresholdForGC         = 256 * 1024 * 1024;
  xiiUInt64              m_uiCurrentlyAllocatedMemory     = 0;
  xiiUInt16              m_uiNumAllocationsThresholdForGC = 128;
  xiiUInt16              m_uiNumAllocationsSinceLastGC    = 0;
  xiiUInt16              m_uiFramesThresholdSinceLastGC   = 60; ///< Every 60 frames resources unused for more than 10 frames in a row are GCed.
  xiiUInt16              m_uiFramesSinceLastGC            = 0;

  xiiMap<xiiUInt32, xiiDynamicArray<TextureHandleWithAge>> m_AvailableTextures;
  xiiSet<xiiGALTextureHandle>                              m_TexturesInUse;

  xiiMap<xiiUInt32, xiiDynamicArray<BufferHandleWithAge>> m_AvailableBuffers;
  xiiSet<xiiGALBufferHandle>                              m_BuffersInUse;

  xiiMutex m_Lock;

  xiiGALDevice* m_pDevice;

private:
  static xiiGPUResourcePool* s_pDefaultInstance;
};
