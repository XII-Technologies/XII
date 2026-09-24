/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Device/SwapChain.h>

namespace vk
{
  enum class Format;

  struct Extent2D;

  class SurfaceKHR;
  class SwapchainKHR;
  class Semaphore;
  class Fence;
  class Image;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALSwapChainVulkan final : public xiiGALSwapChain
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALSwapChainVulkan, xiiGALSwapChain);

public:
  virtual void Present() override final;

  virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) override final;

private:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChainVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

  xiiResult CreateVulkanSurface();
  xiiResult CreateVulkanSwapChain();
  xiiResult RecreateVulkanSwapChain();
  void      ReleaseSwapChainResources(bool bReleaseSwapChain);
  void      WaitForPresentQueueIdle();
  void      ThrottleFrameSubmission();

  xiiResult CreateBackBufferInternal();

  vk::Result AcquireNextImage();

private:
  vk::SurfaceKHR   m_vkSurface;
  vk::SwapchainKHR m_vkSwapChain;
  vk::Format       m_vkColorFormat        = vk::Format::eUndefined;
  xiiUInt32        m_uiDesiredBufferCount = 0U;

  xiiDynamicArray<vk::Semaphore> m_ImageAcquiredSemaphores;
  xiiDynamicArray<vk::Semaphore> m_DrawCompleteSemaphores;

  xiiDynamicArray<xiiSharedPtr<xiiGALTexture>> m_SwapChainTextures;
  xiiDynamicArray<bool>                        m_SwapChainImagesInitialized;
  xiiUInt32                                    m_uiBackBufferIndex = 0U;
  xiiUInt32                                    m_uiSemaphoreIndex  = 0U;

  xiiSharedPtr<xiiGALFence> m_pFrameCompleteFence;
  xiiUInt64                 m_uiFrameIndex = 1ULL;

  bool m_bIsImageAcquired = false;
  bool m_bIsMinimized     = false;
  bool m_bIsVSyncEnabled  = false;
};
