
#pragma once

#include <Foundation/Types/UniquePtr.h>

class xiiGALDeviceVulkan;
class xiiPipelineBarrierVulkan;
class xiiCommandBufferPoolVulkan;
class xiiStagingBufferPoolVulkan;

/// \brief Thread-safe context for initializing resources. Records a command buffer that transitions all newly created resources into their initial state.
class xiiInitContextVulkan
{
public:
  xiiInitContextVulkan(xiiGALDeviceVulkan* pDevice);
  ~xiiInitContextVulkan();

  /// \brief Returns a finished command buffer of all background loading up to this point.
  ///    The command buffer is already ended and marked to be reclaimed so the only thing done on it should be to submit it.
  vk::CommandBuffer GetFinishedCommandBuffer();

  /// \brief Initializes a texture and moves it into its default state.
  /// \param pTexture The texture to initialize.
  /// \param createInfo The image creation info for the texture. Needed for initial state information.
  /// \param pInitialData The initial data of the texture. If not set, the initial content will be undefined.
  void InitTexture(const xiiGALTextureVulkan* pTexture, vk::ImageCreateInfo& createInfo, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData);

  /// \brief Needs to be called by the xiiGALDeviceVulkan just before a texture is destroyed to clean up stale barriers.
  void TextureDestroyed(const xiiGALTextureVulkan* pTexture);

private:
  void EnsureCommandBufferExists();

  xiiGALDeviceVulkan* m_pDevice = nullptr;

  xiiMutex                                 m_Lock;
  vk::CommandBuffer                        m_currentCommandBuffer;
  xiiUniquePtr<xiiPipelineBarrierVulkan>   m_pPipelineBarrier;
  xiiUniquePtr<xiiCommandBufferPoolVulkan> m_pCommandBufferPool;
  xiiUniquePtr<xiiStagingBufferPoolVulkan> m_pStagingBufferPool;
};
