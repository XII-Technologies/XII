
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class xiiGALDeviceVulkan;

class xiiGALSwapChainVulkan : public xiiGALWindowSwapChain
{
public:
  virtual void      AcquireNextRenderTarget(xiiGALDevice* pDevice) override;
  virtual void      PresentRenderTarget(xiiGALDevice* pDevice) override;
  virtual xiiResult UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode) override;

  XII_ALWAYS_INLINE vk::SwapchainKHR GetVulkanSwapChain() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALSwapChainVulkan(const xiiGALWindowSwapChainCreationDescription& Description);

  virtual ~xiiGALSwapChainVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  xiiResult         CreateSwapChainInternal();
  void              DestroySwapChainInternal(xiiGALDeviceVulkan* pVulkanDevice);
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  xiiGALDeviceVulkan*        m_pVulkanDevice = nullptr;
  xiiEnum<xiiGALPresentMode> m_currentPresentMode;

  vk::SurfaceKHR                         m_vulkanSurface;
  vk::SwapchainKHR                       m_vulkanSwapChain;
  xiiHybridArray<vk::Image, 3>           m_swapChainImages;
  xiiHybridArray<xiiGALTextureHandle, 3> m_swapChainTextures;
  xiiHybridArray<vk::Fence, 3>           m_swapChainImageInUseFences;
  xiiUInt32                              m_uiCurrentSwapChainImage = 0;

  vk::Semaphore m_currentPipelineImageAvailableSemaphore;
};

#include <RendererVulkan/Device/Implementation/SwapChainVulkan_inl.h>
