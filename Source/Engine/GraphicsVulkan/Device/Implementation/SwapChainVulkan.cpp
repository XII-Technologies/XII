/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Core/System/Window.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>
#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/SwapChainVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Pools/SemaphorePoolVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

#if XII_ENABLED(XII_SUPPORTS_SDL)
#  include <SDL3/SDL_init.h>
#  include <SDL3/SDL_video.h>
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#  include <wayland-client.h>
#endif

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALSwapChainVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALSwapChainVulkan::xiiGALSwapChainVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(std::move(pDeviceVulkan), creationDescription), m_ImageAcquiredSemaphores(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator()), m_DrawCompleteSemaphores(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator()), m_SwapChainTextures(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator()), m_SwapChainImagesInitialized(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator())
{
}

xiiGALSwapChainVulkan::~xiiGALSwapChainVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Instance                     vkInstance    = pDeviceVulkan->GetVulkanInstance();

  if (m_vkSwapChain != VK_NULL_HANDLE)
  {
    ReleaseSwapChainResources(true);

    XII_ASSERT_DEV(m_vkSwapChain == VK_NULL_HANDLE, "The Vulkan swap chain has not yet been released!");

    m_Description.m_pWindow->RemoveReference();
  }

  if (m_vkSurface != VK_NULL_HANDLE)
  {
    vkInstance.destroySurfaceKHR(m_vkSurface, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

xiiResult xiiGALSwapChainVulkan::InitPlatform()
{
  XII_LOG_BLOCK("xiiGALSwapChainVulkan::InitPlatform");

#if XII_ENABLED(XII_SUPPORTS_SDL)
  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    xiiLog::Error("Unable to initialize SDL Video: {}", SDL_GetError());
    return XII_FAILURE;
  }
#endif

  XII_SUCCEED_OR_RETURN(CreateVulkanSurface());
  XII_SUCCEED_OR_RETURN(CreateVulkanSwapChain());
  XII_SUCCEED_OR_RETURN(CreateBackBufferInternal());

  {
    xiiGALFenceCreationDescription fenceDescription;
    fenceDescription.m_Type = xiiGALFenceType::CpuWaitOnly;

    m_pFrameCompleteFence = m_pDevice->CreateFence(fenceDescription);

    if (!m_pFrameCompleteFence)
    {
      xiiLog::Error("Failed to create Vulkan SwapChain frame complete fence.");
      return XII_FAILURE;
    }

    m_pFrameCompleteFence->SetDebugName("SwapChain frame complete fence.");
  }

  // Note that the image may be immediately out of date.
  XII_IGNORE_UNUSED(AcquireNextImage());

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_Description.m_pWindow->AddReference();

  return XII_SUCCESS;
}

void xiiGALSwapChainVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkSwapChain, sName.GetData(tmp));
}

xiiResult xiiGALSwapChainVulkan::CreateVulkanSurface()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Instance                     vkInstance    = pDeviceVulkan->GetVulkanInstance();

  if (m_vkSurface != VK_NULL_HANDLE)
  {
    vkInstance.destroySurfaceKHR(m_vkSurface, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    m_vkSurface = VK_NULL_HANDLE;
  }

  // Create OS-Specific surface.
#if defined(VK_USE_PLATFORM_WIN32_KHR)
  vk::Win32SurfaceCreateInfoKHR vkSurfaceCreateInfo = {};
  vkSurfaceCreateInfo.pNext                         = nullptr;
  vkSurfaceCreateInfo.flags                         = {};
  vkSurfaceCreateInfo.hinstance                     = GetModuleHandle(NULL);
  vkSurfaceCreateInfo.hwnd                          = xiiMinWindows::ToNative(m_Description.m_pWindow->GetNativeWindowHandle());

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkInstance.createWin32SurfaceKHR(&vkSurfaceCreateInfo, nullptr, &m_vkSurface, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
  vk::MacOSSurfaceCreateInfoMVK vkSurfaceCreateInfo = {};
  vkSurfaceCreateInfo.pNext                         = nullptr;
  vkSurfaceCreateInfo.flags                         = {};
  vkSurfaceCreateInfo.pView                         = m_Description.m_pWindow->GetNativeWindowHandle();

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkInstance.createMacOSSurfaceMVK(&vkSurfaceCreateInfo, nullptr, &m_vkSurface, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
  vk::WaylandSurfaceCreateInfoKHR vkSurfaceCreateInfo = {};
  vkSurfaceCreateInfo.pNext                           = nullptr;
  vkSurfaceCreateInfo.flags                           = {};
  vkSurfaceCreateInfo.display                         = static_cast<wl_display*>(SDL_GetPointerProperty(SDL_GetWindowProperties(m_Description.m_pWindow->GetNativeWindowHandle()), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr));
  vkSurfaceCreateInfo.surface                         = static_cast<wl_surface*>(SDL_GetPointerProperty(SDL_GetWindowProperties(m_Description.m_pWindow->GetNativeWindowHandle()), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkInstance.createWaylandSurfaceKHR(&vkSurfaceCreateInfo, nullptr, &m_vkSurface, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
#else
#  error "Unsupported platform."
#endif

  // Check present support on the graphics queue.
  {
    vk::PhysicalDevice           vkPhysicalDevice         = pDeviceVulkan->GetVulkanPhysicalDevice();
    xiiGALQueueInformationVulkan graphicsQueueInformation = pDeviceVulkan->GetCommandQueueInformation(xiiGALCommandQueueFlags::Graphics);
    vk::Bool32                   bHasPresentSupport       = vk::False;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceSupportKHR(graphicsQueueInformation.m_uiQueueIndex, m_vkSurface, &bHasPresentSupport, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    if (bHasPresentSupport == vk::False)
    {
      xiiLog::Error("Selected physical device does not support present capability.");
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainVulkan::CreateVulkanSwapChain()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::PhysicalDevice               vkPhysicalDevice = pDeviceVulkan->GetVulkanPhysicalDevice();
  vk::Device                       vkLogicalDevice  = pDeviceVulkan->GetVulkanLogicalDevice();

  // Retrieve the list of vk::Formats that are supported.
  xiiUInt32 uiFormatCount = 0U;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceFormatsKHR(m_vkSurface, &uiFormatCount, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiFormatCount > 0U, "");

  xiiTemporaryHybridArray<vk::SurfaceFormatKHR, 4U> supportedFormats;
  supportedFormats.SetCountUninitialized(uiFormatCount);
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceFormatsKHR(m_vkSurface, &uiFormatCount, supportedFormats.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiFormatCount == supportedFormats.GetCount(), "");

  m_vkColorFormat = xiiVulkanTypeConversions::GetFormat(m_Description.m_ColorBufferFormat);

  vk::ColorSpaceKHR vkColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
  if (uiFormatCount == 1 && supportedFormats.PeekBack().format == vk::Format::eUndefined)
  {
    // If the format list includes just one entry of vk::Format::eUndefined, the surface has no preferred format. Otherwise, at least one supported format will be returned.

    // Nothing else to do.
  }
  else
  {
    bool bFormatFound = false;
    for (const vk::SurfaceFormatKHR& surfaceFormat : supportedFormats)
    {
      if (surfaceFormat.format == m_vkColorFormat)
      {
        bFormatFound = true;
        vkColorSpace = surfaceFormat.colorSpace;
        break;
      }
    }

    if (!bFormatFound)
    {
      vk::Format vkReplacementColorFormat = vk::Format::eUndefined;
      switch (m_vkColorFormat)
      {
        case vk::Format::eR8G8B8A8Unorm: vkReplacementColorFormat = vk::Format::eB8G8R8A8Unorm; break;
        case vk::Format::eB8G8R8A8Unorm: vkReplacementColorFormat = vk::Format::eR8G8B8A8Unorm; break;
        case vk::Format::eB8G8R8A8Srgb: vkReplacementColorFormat = vk::Format::eR8G8B8A8Srgb; break;
        case vk::Format::eR8G8B8A8Srgb: vkReplacementColorFormat = vk::Format::eB8G8R8A8Srgb; break;

        default: vkReplacementColorFormat = vk::Format::eUndefined; break;
      }

      bool bReplacementFormatFound = false;
      for (const vk::SurfaceFormatKHR& surfaceFormat : supportedFormats)
      {
        if (surfaceFormat.format == vkReplacementColorFormat)
        {
          bReplacementFormatFound = true;
          vkColorSpace            = surfaceFormat.colorSpace;
          break;
        }
      }

      if (bReplacementFormatFound)
      {
        xiiLog::Dev("Requested color buffer format '{}' is not supported by the surface and will be replaced with '{}'.", vk::to_string(m_vkColorFormat).data(), vk::to_string(vkReplacementColorFormat).data());

        m_vkColorFormat                   = vkReplacementColorFormat;
        m_Description.m_ColorBufferFormat = xiiVulkanTypeConversions::GetGALResourceFormat(vkReplacementColorFormat);
      }
      else
      {
        // Neither the requested format nor the common replacement are supported.
        // Fall back to the first supported surface format to guarantee a valid swapchain format.
        const vk::SurfaceFormatKHR& vkFallbackFormat = supportedFormats[0];
        xiiLog::Dev("Requested color buffer format '{}' is not supported by the surface. Falling back to supported format '{}'.", vk::to_string(m_vkColorFormat).data(), vk::to_string(vkFallbackFormat.format).data());

        vkColorSpace                      = vkFallbackFormat.colorSpace;
        m_vkColorFormat                   = vkFallbackFormat.format;
        m_Description.m_ColorBufferFormat = xiiVulkanTypeConversions::GetGALResourceFormat(m_vkColorFormat);
      }
    }
  }

  vk::SurfaceCapabilitiesKHR surfaceCapabilities;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceCapabilitiesKHR(m_vkSurface, &surfaceCapabilities, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  xiiUInt32 uiPresentModeCount = 0U;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfacePresentModesKHR(m_vkSurface, &uiPresentModeCount, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiPresentModeCount > 0, "");

  xiiTemporaryHybridArray<vk::PresentModeKHR, 4U> presentModes;
  presentModes.SetCountUninitialized(uiPresentModeCount);
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfacePresentModesKHR(m_vkSurface, &uiPresentModeCount, presentModes.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiPresentModeCount == presentModes.GetCount(), "");

  vk::SurfaceTransformFlagsKHR vkPreTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  if (m_DesiredSurfaceTransform != xiiGALSurfaceTransform::Optimal)
  {
    vkPreTransform = xiiVulkanTypeConversions::GetSurfaceTransform(m_DesiredSurfaceTransform);

    if (surfaceCapabilities.supportedTransforms & vkPreTransform)
    {
      m_Description.m_PreTransform = m_DesiredSurfaceTransform;
    }
    else
    {
      xiiLog::Warning("{} is not supported engine. Optimal surface transform will be used instead. Query the swap chain description to get the actual surface transform.", vk::to_string(vkPreTransform).data());

      m_DesiredSurfaceTransform = xiiGALSurfaceTransform::Optimal;
    }
  }

  if (m_DesiredSurfaceTransform == xiiGALSurfaceTransform::Optimal)
  {
    // Use current surface transform to avoid extra cost of presenting the image.
    // If preTransform does not match the currentTransform value returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR, the presentation engine will transform the image content as part of the presentation operation.
    // https://community.arm.com/developer/tools-software/graphics/b/blog/posts/appropriate-use-of-surface-rotation

    vkPreTransform               = surfaceCapabilities.currentTransform;
    m_Description.m_PreTransform = xiiVulkanTypeConversions::GetGALSurfaceTransform(vkPreTransform);

    xiiLog::Dev("Using {} swap chain pre-transform.", vk::to_string(vkPreTransform).data());
  }

  xiiSizeU32   windowSize        = m_Description.m_pWindow->GetClientAreaSize();
  vk::Extent2D vkSwapchainExtent = {};
  // The width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
  if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF && windowSize.HasNonZeroArea())
  {
    // If the surface size is undefined, the size is set to the size of the images requested.
    vkSwapchainExtent.width  = xiiMath::Min(xiiMath::Max(windowSize.width, surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
    vkSwapchainExtent.height = xiiMath::Min(xiiMath::Max(windowSize.height, surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);
  }
  else
  {
    // If the surface size is defined, the swap chain size must match.
    vkSwapchainExtent = surfaceCapabilities.currentExtent;
  }

  m_CurrentSize.width  = vkSwapchainExtent.width;
  m_CurrentSize.height = vkSwapchainExtent.height;

  // If the computed swap chain extent has zero area, the window is effectively minimized.
  // Do not create a swap chain with zero extent as that causes issues when restoring the window.
  if (m_CurrentSize.width == 0 || m_CurrentSize.height == 0)
  {
    m_bIsMinimized = true;
    xiiLog::Dev("Swap chain creation skipped because surface extent is 0x0 (minimized).");
    return XII_SUCCESS;
  }

  // The FIFO present mode is guaranteed by the spec to always be supported.
  vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
  {
    xiiTemporaryHybridArray<vk::PresentModeKHR, 4U> preferredPresentModes;

    if (m_PresentMode == xiiGALPresentMode::VSync)
    {
      // FIFO relaxed waits for the next VSync, but if the frame is late,
      // it still shows it even if VSync has already passed, which may
      // result in tearing.
      preferredPresentModes.PushBack(vk::PresentModeKHR::eFifoRelaxed);
      preferredPresentModes.PushBack(vk::PresentModeKHR::eFifo);
    }
    else
    {
      // Mailbox is the lowest latency non-tearing presentation mode.
      preferredPresentModes.PushBack(vk::PresentModeKHR::eMailbox);
      preferredPresentModes.PushBack(vk::PresentModeKHR::eImmediate);
      preferredPresentModes.PushBack(vk::PresentModeKHR::eFifo);
    }

    for (const vk::PresentModeKHR& preferredMode : preferredPresentModes)
    {
      if (presentModes.Contains(preferredMode))
      {
        presentMode = preferredMode;
        break;
      }
    }

    xiiLog::Dev("Using {} swap chain present mode.", vk::to_string(presentMode).data());
  }

  // Determine the number of VkImage's to use in the swap chain.
  // We need to acquire only 1 presentable image at at time.
  // Asking for minImageCount images ensures that we can acquire 1 presentable image as long as we present it before attempting to acquire another.
  m_uiDesiredBufferCount = m_Description.m_uiBufferCount;
  if (m_uiDesiredBufferCount < surfaceCapabilities.minImageCount)
  {
    xiiLog::Dev("Desired back buffer count ({}) is smaller than the minimal image count supported for this surface ({}). Resetting to {}.", m_uiDesiredBufferCount, surfaceCapabilities.minImageCount, surfaceCapabilities.minImageCount);

    m_uiDesiredBufferCount = surfaceCapabilities.minImageCount;
  }
  if (surfaceCapabilities.maxImageCount != 0 && m_uiDesiredBufferCount > surfaceCapabilities.maxImageCount)
  {
    xiiLog::Dev("Desired back buffer count ({}) is greater than the maximal image count supported for this surface ({}). Resetting to {}.", m_uiDesiredBufferCount, surfaceCapabilities.maxImageCount, surfaceCapabilities.maxImageCount);

    m_uiDesiredBufferCount = surfaceCapabilities.maxImageCount;
  }

  // We must use m_DesiredBufferCount instead of m_SwapChainDesc.BufferCount, because Vulkan may decide to always add extra buffers, causing infinite growth of the swap chain when it is recreated:
  //                          m_Description.m_uiBufferCount
  // CreateVulkanSwapChain()          2 -> 4
  // CreateVulkanSwapChain()          4 -> 6
  // CreateVulkanSwapChain()          6 -> 8
  xiiUInt32 uiDesiredSwapChainImageCount = m_uiDesiredBufferCount;

  // Find a supported composite alpha mode - one of these is guaranteed to be set.
  vk::CompositeAlphaFlagBitsKHR compositeAlpha         = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  vk::CompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::CompositeAlphaFlagBitsKHR::ePreMultiplied, vk::CompositeAlphaFlagBitsKHR::ePostMultiplied, vk::CompositeAlphaFlagBitsKHR::eInherit};
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(compositeAlphaFlags); ++i)
  {
    if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i])
    {
      compositeAlpha = compositeAlphaFlags[i];
      break;
    }
  }

  vk::SwapchainKHR vkOldSwapChain = m_vkSwapChain;
  m_vkSwapChain                   = VK_NULL_HANDLE;

  vk::SwapchainCreateInfoKHR swapChainCreateInfo = {};
  swapChainCreateInfo.flags                      = {};
  swapChainCreateInfo.pNext                      = nullptr;
  swapChainCreateInfo.surface                    = m_vkSurface;
  swapChainCreateInfo.minImageCount              = uiDesiredSwapChainImageCount;
  swapChainCreateInfo.imageFormat                = m_vkColorFormat;
  swapChainCreateInfo.imageExtent.width          = vkSwapchainExtent.width;
  swapChainCreateInfo.imageExtent.height         = vkSwapchainExtent.height;
  swapChainCreateInfo.preTransform               = static_cast<vk::SurfaceTransformFlagBitsKHR>(xiiVulkanTypeConversions::GetUnderlyingFlagsValue(vkPreTransform));
  swapChainCreateInfo.compositeAlpha             = compositeAlpha;
  swapChainCreateInfo.imageArrayLayers           = 1U;
  swapChainCreateInfo.presentMode                = presentMode;
  swapChainCreateInfo.oldSwapchain               = vkOldSwapChain;
  swapChainCreateInfo.clipped                    = vk::True;
  swapChainCreateInfo.imageColorSpace            = vkColorSpace;

  XII_ASSERT_DEV(m_Description.m_UsageFlags != xiiGALSwapChainUsageFlags::None, "No swap chain flags are defined.");
  if (m_Description.m_UsageFlags.IsSet(xiiGALSwapChainUsageFlags::RenderTarget))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  if (m_Description.m_UsageFlags.IsSet(xiiGALSwapChainUsageFlags::ShaderResource))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eSampled;
  if (m_Description.m_UsageFlags.IsSet(xiiGALSwapChainUsageFlags::InputAttachment))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eInputAttachment;
  if (m_Description.m_UsageFlags.IsSet(xiiGALSwapChainUsageFlags::CopySource))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
  swapChainCreateInfo.imageUsage &= surfaceCapabilities.supportedUsageFlags; // Clamp to supported usage flags.

  swapChainCreateInfo.imageSharingMode      = vk::SharingMode::eExclusive;
  swapChainCreateInfo.queueFamilyIndexCount = 0U;
  swapChainCreateInfo.pQueueFamilyIndices   = nullptr;

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createSwapchainKHR(&swapChainCreateInfo, nullptr, &m_vkSwapChain, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  if (vkOldSwapChain != VK_NULL_HANDLE)
  {
    vkLogicalDevice.destroySwapchainKHR(vkOldSwapChain, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    vkOldSwapChain = VK_NULL_HANDLE;
  }

  xiiUInt32 uiSwapChainImageCount = 0U;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.getSwapchainImagesKHR(m_vkSwapChain, &uiSwapChainImageCount, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiSwapChainImageCount > 0, "");

  if (uiSwapChainImageCount != m_Description.m_uiBufferCount)
  {
    xiiLog::Dev("Created swap chain with {} images vs {} requested.", uiSwapChainImageCount, m_Description.m_uiBufferCount);

    m_Description.m_uiBufferCount = uiSwapChainImageCount;
  }

  m_ImageAcquiredSemaphores.SetCountUninitialized(uiSwapChainImageCount);
  m_DrawCompleteSemaphores.SetCountUninitialized(uiSwapChainImageCount);

  xiiGALSemaphorePoolVulkan* pSemaphorePool = pDeviceVulkan->GetVulkanSemaphorePool();

  for (xiiUInt32 i = 0; i < uiSwapChainImageCount; ++i)
  {
    m_ImageAcquiredSemaphores[i] = pSemaphorePool->RequestSemaphore();
    m_DrawCompleteSemaphores[i]  = pSemaphorePool->RequestSemaphore();
  }

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainVulkan::RecreateVulkanSwapChain()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::PhysicalDevice               vkPhysicalDevice = pDeviceVulkan->GetVulkanPhysicalDevice();
  vk::Device                       vkLogicalDevice  = pDeviceVulkan->GetVulkanLogicalDevice();

  // Do not release the Vulkan swap chain as we will use use it as old-SwapChain parameter.
  ReleaseSwapChainResources(false);

  // Check if the surface is lost.
  {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = {};
    if (vkPhysicalDevice.getSurfaceCapabilitiesKHR(m_vkSurface, &surfaceCapabilities, pDeviceVulkan->GetVulkanDynamicDispatchLoader()) == vk::Result::eErrorSurfaceLostKHR)
    {
      // Destroy the swap chain associated with the surface.
      if (m_vkSwapChain != VK_NULL_HANDLE)
      {
        vkLogicalDevice.destroySwapchainKHR(m_vkSwapChain, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

        m_vkSwapChain = VK_NULL_HANDLE;
      }

      // Recreate the surface.
      XII_SUCCEED_OR_RETURN(CreateVulkanSurface());
    }
  }

  XII_SUCCEED_OR_RETURN(CreateVulkanSwapChain());
  XII_SUCCEED_OR_RETURN(CreateBackBufferInternal());

  return XII_SUCCESS;
}

void xiiGALSwapChainVulkan::ReleaseSwapChainResources(bool bReleaseSwapChain)
{
  if (m_vkSwapChain == VK_NULL_HANDLE)
    return;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  // The frame-complete fence is signaled by the rendering submission that precedes
  // vkQueuePresentKHR. It therefore cannot prove that the presentation operation has
  // released the swapchain image or its wait semaphore. Synchronize the presentation
  // queue before releasing either object or destroying/retiring the swapchain.
  WaitForPresentQueueIdle();

  // All references to the swap chain must be released before it can be destroyed.
  m_pBackBufferTexture.Clear();
  m_SwapChainTextures.Clear();
  m_SwapChainImagesInitialized.Clear();

  // We must wait until GPU is idled before destroying the fences as they are destroyed immediately.
  // The semaphores are managed and will be kept alive by the command queue they are submitted to.
  m_uiSemaphoreIndex = 0U;

  xiiGALSemaphorePoolVulkan* pSemaphorePool = pDeviceVulkan->GetVulkanSemaphorePool();

  for (xiiUInt32 i = 0; i < m_DrawCompleteSemaphores.GetCount(); ++i)
  {
    pSemaphorePool->ReclaimSemaphore(std::move(m_DrawCompleteSemaphores[i]));
  }
  m_DrawCompleteSemaphores.Clear();

  for (xiiUInt32 i = 0; i < m_ImageAcquiredSemaphores.GetCount(); ++i)
  {
    pSemaphorePool->ReclaimSemaphore(std::move(m_ImageAcquiredSemaphores[i]));
  }
  m_ImageAcquiredSemaphores.Clear();

  if (bReleaseSwapChain)
  {
    vk::Device vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

    vkLogicalDevice.destroySwapchainKHR(m_vkSwapChain, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    m_vkSwapChain = VK_NULL_HANDLE;
  }
}

void xiiGALSwapChainVulkan::WaitForPresentQueueIdle()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->LockCommandQueueAndRun(xiiGALCommandQueueFlags::Graphics, [&pDeviceVulkan](const vk::Queue& vkQueue) -> void {
    VK_ASSERT_DEV(vkQueue.waitIdle(pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  });
}

void xiiGALSwapChainVulkan::ThrottleFrameSubmission()
{
  if (m_uiFrameIndex > m_Description.m_uiBufferCount)
  {
    m_pFrameCompleteFence->Wait(m_uiFrameIndex - m_Description.m_uiBufferCount);
  }
}

xiiResult xiiGALSwapChainVulkan::CreateBackBufferInternal()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  // If there is no Vulkan swap chain (e.g. because the window is minimized), nothing to do.
  if (m_vkSwapChain == VK_NULL_HANDLE || !m_CurrentSize.HasNonZeroArea())
  {
    m_pBackBufferTexture.Clear();
    m_SwapChainTextures.Clear();
    m_SwapChainImagesInitialized.Clear();
    return XII_SUCCESS;
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  {
    xiiUInt32  uiSwapChainImageCount = 0U;
    vk::Result result                = vkLogicalDevice.getSwapchainImagesKHR(m_vkSwapChain, &uiSwapChainImageCount, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    XII_ASSERT_DEBUG(result == vk::Result::eSuccess, "");
    XII_ASSERT_DEBUG(uiSwapChainImageCount == m_Description.m_uiBufferCount, "Unexpected swap chain buffer count.");
  }
#endif

  xiiTemporaryHybridArray<vk::Image, 2U> swapChainImages;
  swapChainImages.SetCountUninitialized(m_Description.m_uiBufferCount);

  m_SwapChainTextures.SetCount(m_Description.m_uiBufferCount);
  m_SwapChainImagesInitialized.SetCount(m_Description.m_uiBufferCount, false);

  xiiUInt32 uiSwapChainImageCount = m_Description.m_uiBufferCount;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.getSwapchainImagesKHR(m_vkSwapChain, &uiSwapChainImageCount, swapChainImages.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiSwapChainImageCount == swapChainImages.GetCount(), "");

  xiiStringBuilder sb;
  for (xiiUInt32 i = 0; i < uiSwapChainImageCount; ++i)
  {
    xiiGALTextureCreationDescription textureCreationDescription;
    textureCreationDescription.m_Type                  = xiiGALResourceDimension::Texture2D;
    textureCreationDescription.m_Size.width            = m_CurrentSize.width;
    textureCreationDescription.m_Size.height           = m_CurrentSize.height;
    textureCreationDescription.m_Format                = m_Description.m_ColorBufferFormat;
    textureCreationDescription.m_uiArraySizeOrDepth    = 1U;
    textureCreationDescription.m_uiMipLevels           = 1U;
    textureCreationDescription.m_uiSampleCount         = 1U;
    textureCreationDescription.m_BindFlags             = xiiGALGraphicsUtilities::SwapChainUsageFlagsToBindFlags(m_Description.m_UsageFlags);
    textureCreationDescription.m_Usage                 = xiiGALResourceUsage::Mutable;
    textureCreationDescription.m_CPUAccessFlags        = xiiGALCPUAccessFlag::None;
    textureCreationDescription.m_MiscFlags             = xiiGALMiscTextureFlags::None;
    textureCreationDescription.m_pExistingNativeObject = swapChainImages[i];

    m_SwapChainTextures[i] = pDeviceVulkan->CreateTexture(textureCreationDescription);
    XII_ASSERT_RELEASE(m_SwapChainTextures[i] != nullptr, "Failed to create native backbuffer texture object!");

    sb.SetFormat("Main Back Buffer ({})", m_SwapChainTextures.GetCount());

    m_SwapChainTextures[i]->SetDebugName(sb);
  }
  return XII_SUCCESS;
}

vk::Result xiiGALSwapChainVulkan::AcquireNextImage()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  // Applications should not rely on vkAcquireNextImageKHR blocking in order to meter their rendering speed.
  // The implementation may return from this function immediately regardless of how many presentation requests are queued,
  // and regardless of when queued presentation requests will complete relative to the call. Instead, applications can use fences
  // to meter their frame generation work to match the presentation rate.

  // vkAcquireNextImageKHR requires that the semaphore is not in use, so we must wait for the frame (FrameIndex - BufferCount) to complete.
  // This also ensures that there are no more than BufferCount frames in flight at any time.
  ThrottleFrameSubmission();

  // If there is no swap chain (window minimized) nothing to acquire.
  if (m_vkSwapChain == VK_NULL_HANDLE || m_bIsMinimized || !m_CurrentSize.HasNonZeroArea())
  {
    m_bIsImageAcquired = false;
    m_pBackBufferTexture.Clear();
    return vk::Result::eSuccess;
  }

  // Guard against the case where the semaphore pool hasn't been populated yet (e.g. after rapid minimize/restore).
  if (m_ImageAcquiredSemaphores.GetCount() == 0 || m_uiSemaphoreIndex >= m_ImageAcquiredSemaphores.GetCount())
  {
    m_bIsImageAcquired = false;
    m_pBackBufferTexture.Clear();
    return vk::Result::eSuccess;
  }

  const vk::Semaphore& vkImageAcquiredSemaphore = m_ImageAcquiredSemaphores[m_uiSemaphoreIndex];

  vk::Result result  = vkLogicalDevice.acquireNextImageKHR(m_vkSwapChain, xiiMath::MaxValue<xiiUInt64>(), vkImageAcquiredSemaphore, VK_NULL_HANDLE, &m_uiBackBufferIndex, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  m_bIsImageAcquired = (result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR);

#if XII_ENABLED(XII_PLATFORM_OSX)
  if (result == vk::Result::eSuboptimalKHR)
  {
    // https://github.com/KhronosGroup/MoltenVK/issues/2542
    m_bIsImageAcquired = false;
  }
#endif
  if (m_bIsImageAcquired)
  {
    // Next command in the device context must wait for the next image to be acquired.
    // Unlike fences or events, the act of waiting for a semaphore also un-signals that semaphore.
    // SwapChain image may be used as render target or as destination for copy command.

    if (auto pCommandListVulkan = pDeviceVulkan->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics}).Downcast<xiiGALCommandListVulkan>())
    {
      pCommandListVulkan->Begin();
      {
        xiiGALScopedDebugGroup debugGroup(pCommandListVulkan, "Add Swap Chain Wait Semaphore");

        pCommandListVulkan->AddWaitSemaphore(vkImageAcquiredSemaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer);

        // Vulkan validation layers do not like uninitialized memory. Clear back buffer the first time we acquire it.
        if (!m_SwapChainImagesInitialized[m_uiBackBufferIndex])
        {
          pCommandListVulkan->ClearRenderTargetView(m_SwapChainTextures[m_uiBackBufferIndex]->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black);

          m_SwapChainImagesInitialized[m_uiBackBufferIndex] = true;
        }
      }
      pCommandListVulkan->End();

      auto pCommandQueue = pDeviceVulkan->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);

      pCommandQueue->Submit(pCommandListVulkan);
    }
  }

  if (m_bIsImageAcquired)
  {
    m_pBackBufferTexture = m_SwapChainTextures[m_uiBackBufferIndex];
  }
  else
  {
    m_pBackBufferTexture.Clear();
  }

  return result;
}

void xiiGALSwapChainVulkan::Present()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  // Do not read m_pBackBufferTexture here because we may recreate the swap chain below
  // and m_pBackBufferTexture may change. Resolve the current backbuffer after recreation.

  // If there is no swap chain (e.g. window minimized) or the surface has zero area, try to detect an unminimize and recreate.
  if (m_vkSwapChain == VK_NULL_HANDLE || m_bIsMinimized || !m_CurrentSize.HasNonZeroArea())
  {
    // Check if the window was restored without a resize event. If the client area is now non-zero, recreate the swap chain.
    xiiSizeU32 windowSize = m_Description.m_pWindow->GetClientAreaSize();
    if (windowSize.HasNonZeroArea())
    {
      xiiLog::Dev("Window restored to non-zero size ({}x{}). Recreating swap chain.", windowSize.width, windowSize.height);

      // Attempt to recreate the swap chain now that the window has a valid size.
      RecreateVulkanSwapChain().IgnoreResult();
      m_bIsMinimized = false;

      m_uiSemaphoreIndex = m_Description.m_uiBufferCount > 0 ? m_Description.m_uiBufferCount - 1 : 0;

      // Try to acquire the first image for rendering.
      AcquireNextImage();
    }
    else
    {
      ThrottleFrameSubmission();
      return;
    }
  }

  // Ensure draw-complete semaphores are available and the back buffer index is valid.
  if (m_DrawCompleteSemaphores.GetCount() == 0 || m_uiBackBufferIndex >= m_DrawCompleteSemaphores.GetCount())
  {
    xiiLog::Dev("Draw-complete semaphore unavailable (count={} index={}). Attempting to recreate swap chain.", m_DrawCompleteSemaphores.GetCount(), m_uiBackBufferIndex);

    // Try to recreate and acquire a valid image. If that fails, throttle and skip present this frame.
    RecreateVulkanSwapChain().IgnoreResult();
    AcquireNextImage();
    ThrottleFrameSubmission();
    return;
  }

  const vk::Semaphore& vkDrawCompleteSemaphore = m_DrawCompleteSemaphores[m_uiBackBufferIndex];

  if (auto pCommandListVulkan = pDeviceVulkan->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics}).Downcast<xiiGALCommandListVulkan>())
  {
    pCommandListVulkan->Begin();
    {
      // To properly handle the case where vkAcquireNextImageKHR returns the same index twice in a row, use
      // a separate semaphore per swap chain image and index these semaphores using the index of the acquired image.
      if (m_bIsImageAcquired && !m_bIsMinimized)
      {
        xiiSharedPtr<xiiGALTextureVulkan> pCurrentBackbufferVulkan = m_pBackBufferTexture.Downcast<xiiGALTextureVulkan>();

        if (pCurrentBackbufferVulkan)
        {
          pCommandListVulkan->TransitionImageLayout(pCurrentBackbufferVulkan, vk::ImageLayout::ePresentSrcKHR);
          pCommandListVulkan->AddSignalSemaphore(vkDrawCompleteSemaphore);
        }
      }

      pCommandListVulkan->EnqueueSignal(m_pFrameCompleteFence, m_uiFrameIndex++);
    }
    pCommandListVulkan->End();

    auto pCommandQueue = pDeviceVulkan->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);

    pCommandQueue->Submit(pCommandListVulkan);
  }

  if (!m_bIsMinimized)
  {
    vk::Result result = vk::Result::eSuccess;

    // Only present if the image was acquired successfully.
    if (m_bIsImageAcquired)
    {
      // Unlike fences or events, the act of waiting for a semaphore also un-signals that semaphore.

      vk::PresentInfoKHR vkPresentInformation = {};
      vkPresentInformation.pNext              = nullptr;
      vkPresentInformation.pResults           = &result;
      vkPresentInformation.pSwapchains        = &m_vkSwapChain;
      vkPresentInformation.pImageIndices      = &m_uiBackBufferIndex;
      vkPresentInformation.swapchainCount     = 1U;
      vkPresentInformation.pWaitSemaphores    = &vkDrawCompleteSemaphore;
      vkPresentInformation.waitSemaphoreCount = 1U;

      pDeviceVulkan->LockCommandQueueAndRun(xiiGALCommandQueueFlags::Graphics, [&pDeviceVulkan, &vkPresentInformation](const vk::Queue& vkQueue) -> void {
        XII_IGNORE_UNUSED(vkQueue.presentKHR(&vkPresentInformation, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
      });
    }

    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
    {
      RecreateVulkanSwapChain().AssertSuccess();

      m_uiSemaphoreIndex = m_Description.m_uiBufferCount - 1; // To start with 0 index when acquire next image.
    }
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    else if (m_bIsImageAcquired)
    {
      XII_ASSERT_DEBUG(result == vk::Result::eSuccess, "Swap Chain presentation failed.");
    }
#endif
  }

  if (!m_bIsMinimized)
  {
    ++m_uiSemaphoreIndex;
    if (m_uiSemaphoreIndex >= m_Description.m_uiBufferCount)
    {
      m_uiSemaphoreIndex = 0U;
    }

    bool       bEnableVSync = m_PresentMode == xiiGALPresentMode::VSync;
    vk::Result result       = (m_bIsVSyncEnabled == bEnableVSync) ? AcquireNextImage() : vk::Result::eErrorOutOfDateKHR;
    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
    {
      m_bIsVSyncEnabled = bEnableVSync;

      RecreateVulkanSwapChain().AssertSuccess();

      m_uiSemaphoreIndex = m_Description.m_uiBufferCount - 1; // To start with 0 index when acquire next image.

      result = AcquireNextImage();

#if XII_ENABLED(XII_PLATFORM_OSX)
      // For some reason, on MoltenVk we may get VK_SUBOPTIMAL_KHR first time we acquire the image after the swap chain has been recreated.
      // Recreating it yet again seems to fix the problem.
      if (result == vk::Result::eSuboptimalKHR)
      {
        RecreateVulkanSwapChain().AssertSuccess();

        result = AcquireNextImage();
      }
#endif
    }

    // The image may still be out of date if the window keeps changing size.
  }
  else
  {
    ThrottleFrameSubmission();
  }
}

xiiResult xiiGALSwapChainVulkan::Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform)
{
  bool bRecreateSwapChain = false;

  // Set minimized flag based on new size early so creation logic can react accordingly.
  m_bIsMinimized = (newSize.width == 0 && newSize.height == 0);

  if (newSize.HasNonZeroArea() && (newSize != m_CurrentSize || m_DesiredSurfaceTransform != newTransform))
  {
    m_DesiredSurfaceTransform = newTransform;
    bRecreateSwapChain        = true;

    xiiLog::Dev("Resizing swap chain to {}x{}.", newSize.width, newSize.height);
  }

  if (bRecreateSwapChain)
  {
    if (RecreateVulkanSwapChain().Succeeded())
    {
      if (AcquireNextImage() != vk::Result::eSuccess)
      {
        xiiLog::Error("Failed to acquire next image for the just resized swap chain.");
      }
    }
    else
    {
      xiiLog::Error("Failed to resize the swap chain.");
    }
  }

  m_bIsMinimized = (newSize.width == 0 && newSize.height == 0);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_SwapChainVulkan);
