#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Device/SwapChain.h>

class XII_GRAPHICSVULKAN_DLL xiiGALSwapChainVulkan final : public xiiGALSwapChain
{
public:
  virtual void AcquireNextRenderTarget() override final;

  virtual void Present() override final;

  virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) override final;

  virtual void SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode) override final;

  virtual void SetWindowedMode() override final;

  virtual void SetMaximumFrameLatency(xiiUInt32 uiMaxLatency) override final;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChainVulkan();

  virtual xiiResult InitPlatform() override final;
  virtual xiiResult DeInitPlatform() override final;

  xiiResult CreateVulkanSurface();
  xiiResult CreateVulkanSwapChain();

  xiiResult CreateBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan);
  void DestroyBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan);

protected:
  vk::SurfaceKHR   m_vkSurface;
  vk::SwapchainKHR m_vkSwapChain;
  vk::Format       m_vkColorFormat = vk::Format::eUndefined;

#if XII_ENABLED(XII_PLATFORM_ANDROID)
  // Surface extent corresponding to identity transform. We have to store this value,
  // because on Android vkGetPhysicalDeviceSurfaceCapabilitiesKHR is not reliable and
  // starts reporting incorrect dimensions after few rotations.
  vk::Extent2D m_vkSurfaceIdentityExtent;

  // Keep track of current surface transform to detect orientation changes.
  vk::SurfaceTransformFlagsKHR m_vkCurrentSurfaceTransform = {};
#endif

  xiiUInt32 m_uiDesiredBufferCount = 0U;

  xiiUInt32 m_uiBackBufferIndex = 0U;
  bool      m_bIsMinimized      = false;
};

#include <GraphicsVulkan/Device/Implementation/SwapChainVulkan_inl.h>
