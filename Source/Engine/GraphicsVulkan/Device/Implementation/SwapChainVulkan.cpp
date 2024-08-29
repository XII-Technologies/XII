#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Core/System/Window.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/SwapChainVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

xiiGALSwapChainVulkan::xiiGALSwapChainVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(pDeviceVulkan, creationDescription), m_ImageAcquiredSemaphores(pDeviceVulkan->GetAllocator()), m_DrawCompleteSemaphores(pDeviceVulkan->GetAllocator()), m_ImageAcquiredFences(pDeviceVulkan->GetAllocator()), m_SwapChainImages(pDeviceVulkan->GetAllocator()), m_SwapChainTextures(pDeviceVulkan->GetAllocator()), m_SwapChainImagesInitialized(pDeviceVulkan->GetAllocator()), m_ImageAcquiredFenceSubmitted(pDeviceVulkan->GetAllocator())
{
}

xiiGALSwapChainVulkan::~xiiGALSwapChainVulkan() = default;

xiiResult xiiGALSwapChainVulkan::InitPlatform()
{
  XII_LOG_BLOCK("xiiGALSwapChainVulkan::InitPlatform");

  XII_SUCCEED_OR_RETURN(CreateVulkanSurface());
  XII_SUCCEED_OR_RETURN(CreateVulkanSwapChain());

  return CreateBackBufferInternal();
}

xiiResult xiiGALSwapChainVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Instance        vkInstance    = pDeviceVulkan->GetVulkanInstance();

  if (m_vkSwapChain != VK_NULL_HANDLE)
  {
    // TODO
  }

  if (m_vkSurface != VK_NULL_HANDLE)
  {
    vkInstance.destroySurfaceKHR(m_vkSurface, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainVulkan::CreateVulkanSurface()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Instance        vkInstance    = pDeviceVulkan->GetVulkanInstance();

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
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
  vk::AndroidSurfaceCreateInfoKHR vkSurfaceCreateInfo = {};
  vkSurfaceCreateInfo.pNext                           = nullptr;
  vkSurfaceCreateInfo.flags                           = {};
  vkSurfaceCreateInfo.window                          = m_Description.m_pWindow->GetNativeWindowHandle();

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkInstance.createAndroidSurfaceKHR(&vkSurfaceCreateInfo, nullptr, &m_vkSurface, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
#elif defined(VK_USE_PLATFORM_IOS_MVK)
  vk::IOSSurfaceCreateInfoMVK vkSurfaceCreateInfo = {};
  vkSurfaceCreateInfo.pNext                       = nullptr;
  vkSurfaceCreateInfo.flags                       = {};
  vkSurfaceCreateInfo.pView                       = m_Description.m_pWindow->GetNativeWindowHandle();

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkInstance.createIOSSurfaceMVK(&vkSurfaceCreateInfo, nullptr, &m_vkSurface, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
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
  vkSurfaceCreateInfo.display                         = m_Description.m_pWindow->GetNativeWindowHandle();
  vkSurfaceCreateInfo.surface                         = nullptr;

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkInstance.createWaylandSurfaceKHR(&vkSurfaceCreateInfo, nullptr, &m_vkSurface, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
#elif defined(VK_USE_PLATFORM_XCB_KHR)
  xiiWindowHandle windowHandle = m_Description.m_pWindow->GetNativeWindowHandle();
  XII_ASSERT_DEV(windowHandle.xcbWindow.m_uiWindowID != 0 && windowHandle.xcbWindow.m_pConnection != nullptr, "");

  vk::XcbSurfaceCreateInfoKHR vkSurfaceCreateInfo = {};
  vkSurfaceCreateInfo.pNext                       = nullptr;
  vkSurfaceCreateInfo.flags                       = {};
  vkSurfaceCreateInfo.window                      = windowHandle.xcbWindow.m_uiWindowID;
  vkSurfaceCreateInfo.connection                  = windowHandle.xcbWindow.m_pConnection;

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkInstance.createXcbSurfaceKHR(&vkSurfaceCreateInfo, nullptr, &m_vkSurface, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
#else
#  error "Unsupported platform."
#endif

  // Check present support on the graphics queue.
  {
    vk::PhysicalDevice                   vkPhysicalDevice         = pDeviceVulkan->GetVulkanPhysicalDevice();
    xiiGALDeviceVulkan::QueueInformation graphicsQueueInformation = pDeviceVulkan->GetGraphicsQueueInformation();
    vk::Bool32                           bHasPresentSupport       = vk::False;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceSupportKHR(graphicsQueueInformation.m_uiQueueIndex, m_vkSurface, &bHasPresentSupport));

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
  xiiGALDeviceVulkan* pDeviceVulkan    = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::PhysicalDevice  vkPhysicalDevice = pDeviceVulkan->GetVulkanPhysicalDevice();
  vk::Device          vkLogicalDevice  = pDeviceVulkan->GetVulkanLogicalDevice();

  // Retrieve the list of vk::Formats that are supported.
  xiiUInt32 uiFormatCount = 0U;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceFormatsKHR(m_vkSurface, &uiFormatCount, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiFormatCount > 0U, "");

  xiiDynamicArray<vk::SurfaceFormatKHR> supportedFormats(pDeviceVulkan->GetAllocator());
  supportedFormats.SetCount(uiFormatCount);
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceFormatsKHR(m_vkSurface, &uiFormatCount, supportedFormats.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiFormatCount == supportedFormats.GetCount(), "");

  m_vkColorFormat = xiiVulkanTypeConversions::GetFormat(m_Description.m_ColorBufferFormat);

  vk::ColorSpaceKHR colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
  if (uiFormatCount == 1 && supportedFormats.PeekBack().format == vk::Format::eUndefined)
  {
    // If the format list includes just one entry of vk::Format::eUndefined, the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.

    // Do nothing.
  }
  else
  {
    bool bFormatFound = false;
    for (const vk::SurfaceFormatKHR& surfaceFormat : supportedFormats)
    {
      if (surfaceFormat.format == m_vkColorFormat)
      {
        bFormatFound = true;
        colorSpace   = surfaceFormat.colorSpace;
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
          colorSpace              = surfaceFormat.colorSpace;
        }
      }

      if (bReplacementFormatFound)
      {
        xiiLog::Info("Requested color buffer format '{}' is not supported by the surface and will be replaced with '{}'.", vk::to_string(m_vkColorFormat).data(), vk::to_string(vkReplacementColorFormat).data());

        m_vkColorFormat                   = vkReplacementColorFormat;
        m_Description.m_ColorBufferFormat = xiiVulkanTypeConversions::GetGALFormat(vkReplacementColorFormat);
      }
      else
      {
        xiiLog::Warning("Requested color buffer format '{}' is not supported by the surface.", vk::to_string(m_vkColorFormat).data());
      }
    }
  }

  vk::SurfaceCapabilitiesKHR surfaceCapabilities = {};
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceCapabilitiesKHR(m_vkSurface, &surfaceCapabilities, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  xiiUInt32 uiPresentModeCount = 0U;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfacePresentModesKHR(m_vkSurface, &uiPresentModeCount, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiPresentModeCount > 0, "");

  xiiDynamicArray<vk::PresentModeKHR> presentModes(pDeviceVulkan->GetAllocator());
  presentModes.SetCount(uiPresentModeCount);
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
    // https://android-developers.googleblog.com/2020/02/handling-device-orientation-efficiently.html
    // https://community.arm.com/developer/tools-software/graphics/b/blog/posts/appropriate-use-of-surface-rotation

    vkPreTransform               = surfaceCapabilities.currentTransform;
    m_Description.m_PreTransform = xiiVulkanTypeConversions::GetGALSurfaceTransform(vkPreTransform);

    xiiLog::Info("Using {} swap chain pre-transform.", vk::to_string(vkPreTransform).data());
  }

  vk::Extent2D swapchainExtent = {};
  // The width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
  if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF && m_Description.m_Resolution.width != 0 && m_Description.m_Resolution.height != 0)
  {
    // If the surface size is undefined, the size is set to the size of the images requested.
    swapchainExtent.width  = xiiMath::Min(xiiMath::Max(m_Description.m_Resolution.width, surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
    swapchainExtent.height = xiiMath::Min(xiiMath::Max(m_Description.m_Resolution.height, surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);
  }
  else
  {
    // If the surface size is defined, the swap chain size must match.
    swapchainExtent = surfaceCapabilities.currentExtent;
  }

#if XII_ENABLED(XII_PLATFORM_ANDROID)
  // On Android, vkGetPhysicalDeviceSurfaceCapabilitiesKHR is not reliable and starts reporting incorrect dimensions after few rotations.
  // To alleviate the problem, we store the surface extent corresponding to identity rotation.
  // https://android-developers.googleblog.com/2020/02/handling-device-orientation-efficiently.html
  if (m_vkSurfaceIdentityExtent.width == 0 || m_vkSurfaceIdentityExtent.height == 0)
  {
    m_vkSurfaceIdentityExtent = surfaceCapabilities.currentExtent;

    constexpr vk::SurfaceTransformFlagsKHR rotate90TransformFlags = vk::SurfaceTransformFlagBitsKHR::eRotate90 | vk::SurfaceTransformFlagBitsKHR::eRotate270 | vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90 | vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270;
    if (surfaceCapabilities.currentTransform & rotate90TransformFlags)
    {
      xiiMath::Swap(m_vkSurfaceIdentityExtent.width, m_vkSurfaceIdentityExtent.height);
    }
  }

  if (m_DesiredSurfaceTransform == xiiGALSurfaceTransform::Optimal)
  {
    swapchainExtent = m_vkSurfaceIdentityExtent;
  }
  m_vkCurrentSurfaceTransform = surfaceCapabilities.currentTransform;
#endif

  swapchainExtent.width             = xiiMath::Max(swapchainExtent.width, 1U);
  swapchainExtent.height            = xiiMath::Max(swapchainExtent.height, 1U);
  m_Description.m_Resolution.width  = swapchainExtent.width;
  m_Description.m_Resolution.height = swapchainExtent.height;

  // The FIFO present mode is guaranteed by the spec to always be supported.
  vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
  {
    xiiDynamicArray<vk::PresentModeKHR> preferredPresentModes(pDeviceVulkan->GetAllocator());

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
      if (preferredPresentModes.Contains(preferredMode))
      {
        presentMode = preferredMode;
        break;
      }
    }

    xiiLog::Info("Using {} swap chain present mode.", vk::to_string(presentMode).data());
  }

  // Determine the number of VkImage's to use in the swap chain.
  // We need to acquire only 1 presentable image at at time.
  // Asking for minImageCount images ensures that we can acquire 1 presentable image as long as we present it before attempting to acquire another.
  m_uiDesiredBufferCount = m_Description.m_uiBufferCount;
  if (m_uiDesiredBufferCount < surfaceCapabilities.minImageCount)
  {
    xiiLog::Info("Desired back buffer count ({}) is smaller than the minimal image count supported for this surface ({}). Resetting to {}", m_uiDesiredBufferCount, surfaceCapabilities.minImageCount, surfaceCapabilities.minImageCount);

    m_uiDesiredBufferCount = surfaceCapabilities.minImageCount;
  }
  if (surfaceCapabilities.maxImageCount != 0 && m_uiDesiredBufferCount > surfaceCapabilities.maxImageCount)
  {
    xiiLog::Info("Desired back buffer count ({}) is greater than the maximal image count supported for this surface ({}). Resetting to {}", m_uiDesiredBufferCount, surfaceCapabilities.maxImageCount, surfaceCapabilities.maxImageCount);

    m_uiDesiredBufferCount = surfaceCapabilities.maxImageCount;
  }

  // We must use m_DesiredBufferCount instead of m_SwapChainDesc.BufferCount, because Vulkan on Android
  // may decide to always add extra buffers, causing infinite growth of the swap chain when it is recreated:
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

  auto vkOldSwapChain = m_vkSwapChain;
  m_vkSwapChain       = VK_NULL_HANDLE;

  vk::SwapchainCreateInfoKHR swapChainCreateInfo = {};
  swapChainCreateInfo.flags                      = {};
  swapChainCreateInfo.pNext                      = nullptr;
  swapChainCreateInfo.surface                    = m_vkSurface;
  swapChainCreateInfo.minImageCount              = uiDesiredSwapChainImageCount;
  swapChainCreateInfo.imageFormat                = m_vkColorFormat;
  swapChainCreateInfo.imageExtent.width          = swapchainExtent.width;
  swapChainCreateInfo.imageExtent.height         = swapchainExtent.height;
  swapChainCreateInfo.preTransform               = static_cast<vk::SurfaceTransformFlagBitsKHR>(xiiVulkanTypeConversions::GetUnderlyingFlagsValue(vkPreTransform));
  swapChainCreateInfo.compositeAlpha             = compositeAlpha;
  swapChainCreateInfo.imageArrayLayers           = 1U;
  swapChainCreateInfo.presentMode                = presentMode;
  swapChainCreateInfo.oldSwapchain               = vkOldSwapChain;
  swapChainCreateInfo.clipped                    = vk::True;
  swapChainCreateInfo.imageColorSpace            = colorSpace;

  XII_ASSERT_DEV(m_Description.m_Usage != xiiGALSwapChainUsageFlags::None, "No swap chain flags are defined.");
  if (m_Description.m_Usage.IsSet(xiiGALSwapChainUsageFlags::RenderTarget))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  if (m_Description.m_Usage.IsSet(xiiGALSwapChainUsageFlags::ShaderResource))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eSampled;
  if (m_Description.m_Usage.IsSet(xiiGALSwapChainUsageFlags::InputAttachment))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eInputAttachment;
  if (m_Description.m_Usage.IsSet(xiiGALSwapChainUsageFlags::CopySource))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;

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
    xiiLog::Info("Created swap chain with {} images vs {} requested.", uiSwapChainImageCount, m_Description.m_uiBufferCount);

    m_Description.m_uiBufferCount = uiSwapChainImageCount;
  }

  m_ImageAcquiredSemaphores.SetCount(uiSwapChainImageCount);
  m_DrawCompleteSemaphores.SetCount(uiSwapChainImageCount);
  m_ImageAcquiredFences.SetCount(uiSwapChainImageCount);

  for (xiiUInt32 i = 0; i < uiSwapChainImageCount; ++i)
  {
    vk::SemaphoreCreateInfo vkSemaphoreCreateInfo = {};
    vkSemaphoreCreateInfo.flags                   = {};
    vkSemaphoreCreateInfo.pNext                   = nullptr;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createSemaphore(&vkSemaphoreCreateInfo, nullptr, &m_ImageAcquiredSemaphores[i], pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createSemaphore(&vkSemaphoreCreateInfo, nullptr, &m_DrawCompleteSemaphores[i], pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    vk::FenceCreateInfo vkFenceCreateInfo = {};
    vkFenceCreateInfo.flags               = {};
    vkFenceCreateInfo.pNext               = nullptr;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createFence(&vkFenceCreateInfo, nullptr, &m_ImageAcquiredFences[i], pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainVulkan::CreateBackBufferInternal()
{
  xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  {
    xiiUInt32  uiSwapChainImageCount = 0U;
    vk::Result result                = vkLogicalDevice.getSwapchainImagesKHR(m_vkSwapChain, &uiSwapChainImageCount, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    XII_ASSERT_DEBUG(result == vk::Result::eSuccess, "");
    XII_ASSERT_DEBUG(uiSwapChainImageCount == m_Description.m_uiBufferCount, "Unexpected swap chain buffer count.");
  }
#endif

  m_SwapChainImages.SetCount(m_Description.m_uiBufferCount);
  m_SwapChainTextures.SetCount(m_Description.m_uiBufferCount);
  m_SwapChainImagesInitialized.SetCount(m_Description.m_uiBufferCount, false);
  m_ImageAcquiredFenceSubmitted.SetCount(m_Description.m_uiBufferCount, false);

  xiiUInt32 uiSwapChainImageCount = m_Description.m_uiBufferCount;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.getSwapchainImagesKHR(m_vkSwapChain, &uiSwapChainImageCount, m_SwapChainImages.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiSwapChainImageCount == m_SwapChainImages.GetCount(), "");

  for (xiiUInt32 i = 0; i < uiSwapChainImageCount; ++i)
  {
    // No Special bind flag needed for xiiGALSwapChainUsageFlags::CopySource.
    xiiBitflags<xiiGALBindFlags> swapChainBindFlags = xiiGALBindFlags::None;
    if (m_Description.m_Usage.IsSet(xiiGALSwapChainUsageFlags::RenderTarget))
      swapChainBindFlags |= xiiGALBindFlags::RenderTarget;
    if (m_Description.m_Usage.IsSet(xiiGALSwapChainUsageFlags::ShaderResource))
      swapChainBindFlags |= xiiGALBindFlags::ShaderResource;
    if (m_Description.m_Usage.IsSet(xiiGALSwapChainUsageFlags::InputAttachment))
      swapChainBindFlags |= xiiGALBindFlags::InputAttachment;

    xiiGALTextureCreationDescription textureCreationDescription;
    textureCreationDescription.m_Type                   = xiiGALResourceDimension::Texture2D;
    textureCreationDescription.m_Size.width             = m_Description.m_Resolution.width;
    textureCreationDescription.m_Size.height            = m_Description.m_Resolution.height;
    textureCreationDescription.m_Format                 = m_Description.m_ColorBufferFormat;
    textureCreationDescription.m_uiArraySizeOrDepth     = 1U;
    textureCreationDescription.m_uiMipLevels            = 1U;
    textureCreationDescription.m_uiSampleCount          = 1U;
    textureCreationDescription.m_BindFlags              = swapChainBindFlags;
    textureCreationDescription.m_Usage                  = xiiGALResourceUsage::Default;
    textureCreationDescription.m_CPUAccessFlags         = xiiGALCPUAccessFlag::None;
    textureCreationDescription.m_MiscFlags              = xiiGALMiscTextureFlags::None;
    textureCreationDescription.m_pExisitingNativeObject = m_SwapChainImages[i];

    m_SwapChainTextures[i] = pDeviceVulkan->CreateTexture(textureCreationDescription);
    XII_ASSERT_RELEASE(!m_SwapChainTextures[i].IsInvalidated(), "Failed to create native backbuffer texture object!");
  }
  return XII_FAILURE;
}

void xiiGALSwapChainVulkan::DestroyBackBufferInternal()
{
}

void xiiGALSwapChainVulkan::AcquireNextRenderTarget()
{
  xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  // Applications should not rely on vkAcquireNextImageKHR blocking in order to meter their rendering speed.
  // The implementation may return from this function immediately regardless of how many presentation requests are queued,
  // and regardless of when queued presentation requests will complete relative to the call. Instead, applications can use fences
  // to meter their frame generation work to match the presentation rate.

  // Explicitly make sure that there are no more pending frames in the command queue than the number of the swap chain images.
  //
  // Nsc = 3 - number of the swap chain images
  //
  //   N-Ns          N-2           N-1            N (Current frame)
  //    |             |             |             |
  //                  |
  //          Wait for this fence
  //
  // When acquiring swap chain image for frame N, we need to make sure that frame N-Nsc has completed. To achieve that, we wait for the image acquire
  // fence for frame N-Nsc-1. Thus we will have no more than Nsc frames in the queue.
  xiiUInt32 uiOldestSubmittedImageFenceIndex = (m_uiSemaphoreIndex % 1U) % m_ImageAcquiredFenceSubmitted.GetCount();
  if (m_ImageAcquiredFenceSubmitted[uiOldestSubmittedImageFenceIndex])
  {
    const vk::Fence& oldestSubmittedFence = m_ImageAcquiredFences[uiOldestSubmittedImageFenceIndex];
    if (vkLogicalDevice.getFenceStatus(oldestSubmittedFence, pDeviceVulkan->GetVulkanDynamicDispatchLoader()) == vk::Result::eNotReady)
    {
      VK_ASSERT_DEV(vkLogicalDevice.waitForFences(1U, &oldestSubmittedFence, vk::True, xiiMath::MaxValue<xiiUInt64>(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    }

    VK_ASSERT_DEV(vkLogicalDevice.resetFences(1U, &oldestSubmittedFence, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    m_ImageAcquiredFenceSubmitted[uiOldestSubmittedImageFenceIndex] = false;
  }

  const vk::Fence&     imageAcquiredFence     = m_ImageAcquiredFences[m_uiSemaphoreIndex];
  const vk::Semaphore& imageAcquiredSemaphore = m_ImageAcquiredSemaphores[m_uiSemaphoreIndex];

  vk::Result result = vkLogicalDevice.acquireNextImageKHR(m_vkSwapChain, xiiMath::MaxValue<xiiUInt64>(), imageAcquiredSemaphore, imageAcquiredFence, &m_uiBackBufferIndex, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  m_ImageAcquiredFenceSubmitted[m_uiSemaphoreIndex] = (result == vk::Result::eSuccess);
  if (result == vk::Result::eSuccess)
  {
    // Next command in the device context must wait for the next image to be acquired.
    // Unlike fences or events, the act of waiting for a semaphore also unsignals that semaphore (6.4.2).
    // Swapchain image may be used as render target or as destination for copy command.
    /// \todo Wait semaphore here.
    /// \todo Clear render target to free uninitialized memory by clearing the render target.
    m_SwapChainImagesInitialized[m_uiBackBufferIndex] = true;
  }
}

void xiiGALSwapChainVulkan::Present()
{
}

xiiResult xiiGALSwapChainVulkan::Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform)
{
  return XII_FAILURE;
}

void xiiGALSwapChainVulkan::SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode)
{
}

void xiiGALSwapChainVulkan::SetWindowedMode()
{
}

void xiiGALSwapChainVulkan::SetMaximumFrameLatency(xiiUInt32 uiMaxLatency)
{
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_SwapChainVulkan);
