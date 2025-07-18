#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Core/System/Window.h>
#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>
#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/SwapChainVulkan.h>
#include <GraphicsVulkan/Pools/FencePoolVulkan.h>
#include <GraphicsVulkan/Pools/SemaphorePoolVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

#if XII_ENABLED(XII_SUPPORTS_SDL)
#  include <SDL3/SDL_init.h>
#  include <SDL3/SDL_video.h>
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#  include <wayland-client.h>
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
#  include <xcb/xcb.h>
#endif

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALSwapChainVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALSwapChainVulkan::xiiGALSwapChainVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(std::move(pDeviceVulkan), creationDescription), m_ImageAcquiredSemaphores(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator()), m_DrawCompleteSemaphores(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator()), m_ImageAcquiredFences(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator()), m_SwapChainImages(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator()), m_SwapChainTextures(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator()), m_SwapChainImagesInitialized(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator()), m_ImageAcquiredFenceSubmitted(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator())
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
  VK_SUCCEED_OR_RETURN_XII_FAILURE(AcquireNextImage());

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
  vkSurfaceCreateInfo.display                         = static_cast<wl_display*>(SDL_GetPointerProperty(SDL_GetWindowProperties(m_Description.m_pWindow->GetNativeWindowHandle()), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr));
  vkSurfaceCreateInfo.surface                         = static_cast<wl_surface*>(SDL_GetPointerProperty(SDL_GetWindowProperties(m_Description.m_pWindow->GetNativeWindowHandle()), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkInstance.createWaylandSurfaceKHR(&vkSurfaceCreateInfo, nullptr, &m_vkSurface, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
#elif defined(VK_USE_PLATFORM_XCB_KHR)

  vk::XcbSurfaceCreateInfoKHR vkSurfaceCreateInfo = {};
  vkSurfaceCreateInfo.pNext                       = nullptr;
  vkSurfaceCreateInfo.flags                       = {};
  vkSurfaceCreateInfo.window                      = (xcb_window_t)SDL_GetPointerProperty(SDL_GetWindowProperties(m_Description.m_pWindow->GetNativeWindowHandle()), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, nullptr);
  vkSurfaceCreateInfo.connection                  = XGetXCBConnection(SDL_GetPointerProperty(SDL_GetWindowProperties(m_Description.m_pWindow->GetNativeWindowHandle()), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr));

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkInstance.createXcbSurfaceKHR(&vkSurfaceCreateInfo, nullptr, &m_vkSurface, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
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

  xiiDynamicArray<vk::SurfaceFormatKHR> supportedFormats(pDeviceVulkan->GetAllocator());
  supportedFormats.SetCountUninitialized(uiFormatCount);
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceFormatsKHR(m_vkSurface, &uiFormatCount, supportedFormats.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiFormatCount == supportedFormats.GetCount(), "");

  m_vkColorFormat = xiiVulkanTypeConversions::GetFormat(m_Description.m_ColorBufferFormat);

  vk::ColorSpaceKHR colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
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
        xiiLog::Dev("Requested color buffer format '{}' is not supported by the surface and will be replaced with '{}'.", vk::to_string(m_vkColorFormat).data(), vk::to_string(vkReplacementColorFormat).data());

        m_vkColorFormat                   = vkReplacementColorFormat;
        m_Description.m_ColorBufferFormat = xiiVulkanTypeConversions::GetGALResourceFormat(vkReplacementColorFormat);
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
    // https://android-developers.googleblog.com/2020/02/handling-device-orientation-efficiently.html
    // https://community.arm.com/developer/tools-software/graphics/b/blog/posts/appropriate-use-of-surface-rotation

    vkPreTransform               = surfaceCapabilities.currentTransform;
    m_Description.m_PreTransform = xiiVulkanTypeConversions::GetGALSurfaceTransform(vkPreTransform);

    xiiLog::Dev("Using {} swap chain pre-transform.", vk::to_string(vkPreTransform).data());
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

  vk::SwapchainKHR vkOldSwapChain = m_vkSwapChain;
  m_vkSwapChain                   = VK_NULL_HANDLE;

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

  XII_ASSERT_DEV(m_Description.m_UsageFlags != xiiGALSwapChainUsageFlags::None, "No swap chain flags are defined.");
  if (m_Description.m_UsageFlags.IsSet(xiiGALSwapChainUsageFlags::RenderTarget))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  if (m_Description.m_UsageFlags.IsSet(xiiGALSwapChainUsageFlags::ShaderResource))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eSampled;
  if (m_Description.m_UsageFlags.IsSet(xiiGALSwapChainUsageFlags::InputAttachment))
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eInputAttachment;
  if (m_Description.m_UsageFlags.IsSet(xiiGALSwapChainUsageFlags::CopySource))
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
    xiiLog::Dev("Created swap chain with {} images vs {} requested.", uiSwapChainImageCount, m_Description.m_uiBufferCount);

    m_Description.m_uiBufferCount = uiSwapChainImageCount;
  }

  m_ImageAcquiredSemaphores.SetCountUninitialized(uiSwapChainImageCount);
  m_DrawCompleteSemaphores.SetCountUninitialized(uiSwapChainImageCount);
  m_ImageAcquiredFences.SetCountUninitialized(uiSwapChainImageCount);

  auto pSemaphorePool = pDeviceVulkan->GetVulkanSemaphorePool();
  auto pFencePool     = pDeviceVulkan->GetVulkanFencePool();

  for (xiiUInt32 i = 0; i < uiSwapChainImageCount; ++i)
  {
    m_ImageAcquiredSemaphores[i] = pSemaphorePool->RequestSemaphore();
    m_DrawCompleteSemaphores[i]  = pSemaphorePool->RequestSemaphore();
    m_ImageAcquiredFences[i]     = pFencePool->RequestFence();
  }

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainVulkan::RecreateVulkanSwapChain()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::PhysicalDevice               vkPhysicalDevice = pDeviceVulkan->GetVulkanPhysicalDevice();
  vk::Device                       vkLogicalDevice  = pDeviceVulkan->GetVulkanLogicalDevice();

  // Do not release the Vulakn swap chain as we will use use it as oldSwapchain paramter.
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

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  // VERIFY: Flush to submit all pending commands and semaphores to the queue.

  // All references to the swap chain must be released before it can be destroyed.
  for (xiiUInt32 i = 0; i < m_SwapChainTextures.GetCount(); ++i)
  {
    m_SwapChainTextures.Clear();
  }
  m_pBackBufferTexture.Clear();

  // We need to explicitly wait for all submitted Image Acquired Fences to signal.
  // Just idling the GPU is not enough and results in validation warnings.
  // As a matter of fact, it is only required to check the fence status.
  WaitForImageAcquiredFences();

  m_SwapChainImages.Clear();
  m_SwapChainTextures.Clear();
  m_SwapChainImagesInitialized.Clear();

  // We must wait until GPU is idled before destroying the fences as they are destroyed immediately.
  // The semaphores are managed and will be kept alive by the command queue they are submitted to.
  m_uiSemaphoreIndex = 0U;

  auto pSemaphorePool = pDeviceVulkan->GetVulkanSemaphorePool();

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

  auto pFencePool = pDeviceVulkan->GetVulkanFencePool();
  for (xiiUInt32 i = 0; i < m_ImageAcquiredFences.GetCount(); ++i)
  {
    pFencePool->ReclaimFence(std::move(m_ImageAcquiredFences[i]));

    m_ImageAcquiredFences[i] = VK_NULL_HANDLE;
  }
  m_ImageAcquiredFences.Clear();
  m_ImageAcquiredFenceSubmitted.Clear();

  if (bReleaseSwapChain)
  {
    vkLogicalDevice.destroySwapchainKHR(m_vkSwapChain, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    m_vkSwapChain = VK_NULL_HANDLE;
  }
}

xiiResult xiiGALSwapChainVulkan::CreateBackBufferInternal()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  {
    xiiUInt32  uiSwapChainImageCount = 0U;
    vk::Result result                = vkLogicalDevice.getSwapchainImagesKHR(m_vkSwapChain, &uiSwapChainImageCount, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    XII_ASSERT_DEBUG(result == vk::Result::eSuccess, "");
    XII_ASSERT_DEBUG(uiSwapChainImageCount == m_Description.m_uiBufferCount, "Unexpected swap chain buffer count.");
  }
#endif

  m_SwapChainImages.SetCountUninitialized(m_Description.m_uiBufferCount);
  m_SwapChainTextures.SetCount(m_Description.m_uiBufferCount);
  m_SwapChainImagesInitialized.SetCount(m_Description.m_uiBufferCount, false);
  m_ImageAcquiredFenceSubmitted.SetCount(m_Description.m_uiBufferCount, false);

  xiiUInt32 uiSwapChainImageCount = m_Description.m_uiBufferCount;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.getSwapchainImagesKHR(m_vkSwapChain, &uiSwapChainImageCount, m_SwapChainImages.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiSwapChainImageCount == m_SwapChainImages.GetCount(), "");

  xiiStringBuilder sb;
  for (xiiUInt32 i = 0; i < uiSwapChainImageCount; ++i)
  {
    xiiGALTextureCreationDescription textureCreationDescription;
    textureCreationDescription.m_Type                  = xiiGALResourceDimension::Texture2D;
    textureCreationDescription.m_Size.width            = m_Description.m_Resolution.width;
    textureCreationDescription.m_Size.height           = m_Description.m_Resolution.height;
    textureCreationDescription.m_Format                = m_Description.m_ColorBufferFormat;
    textureCreationDescription.m_uiArraySizeOrDepth    = 1U;
    textureCreationDescription.m_uiMipLevels           = 1U;
    textureCreationDescription.m_uiSampleCount         = 1U;
    textureCreationDescription.m_BindFlags             = xiiGALGraphicsUtilities::SwapChainUsageFlagsToBindFlags(m_Description.m_UsageFlags);
    textureCreationDescription.m_Usage                 = xiiGALResourceUsage::Mutable;
    textureCreationDescription.m_CPUAccessFlags        = xiiGALCPUAccessFlag::None;
    textureCreationDescription.m_MiscFlags             = xiiGALMiscTextureFlags::None;
    textureCreationDescription.m_pExistingNativeObject = m_SwapChainImages[i];

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
  xiiUInt32 uiOldestSubmittedImageFenceIndex = (m_uiSemaphoreIndex + 1U) % m_ImageAcquiredFenceSubmitted.GetCount();
  if (m_ImageAcquiredFenceSubmitted[uiOldestSubmittedImageFenceIndex])
  {
    const vk::Fence& vkOldestSubmittedFence = m_ImageAcquiredFences[uiOldestSubmittedImageFenceIndex];
    if (vkLogicalDevice.getFenceStatus(vkOldestSubmittedFence, pDeviceVulkan->GetVulkanDynamicDispatchLoader()) == vk::Result::eNotReady)
    {
      VK_ASSERT_DEV(vkLogicalDevice.waitForFences(1U, &vkOldestSubmittedFence, vk::True, xiiMath::MaxValue<xiiUInt64>(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    }

    VK_ASSERT_DEV(vkLogicalDevice.resetFences(1U, &vkOldestSubmittedFence, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    m_ImageAcquiredFenceSubmitted[uiOldestSubmittedImageFenceIndex] = false;
  }

  const vk::Fence&     imageAcquiredFence     = m_ImageAcquiredFences[m_uiSemaphoreIndex];
  const vk::Semaphore& imageAcquiredSemaphore = m_ImageAcquiredSemaphores[m_uiSemaphoreIndex];

  vk::Result result = vkLogicalDevice.acquireNextImageKHR(m_vkSwapChain, xiiMath::MaxValue<xiiUInt64>(), imageAcquiredSemaphore, imageAcquiredFence, &m_uiBackBufferIndex, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  m_ImageAcquiredFenceSubmitted[m_uiSemaphoreIndex] = (result == vk::Result::eSuccess);
  if (result == vk::Result::eSuccess)
  {
    // Next command in the device context must wait for the next image to be acquired.
    // Unlike fences or events, the act of waiting for a semaphore also un-signals that semaphore (6.4.2).
    // SwapChain image may be used as render target or as destination for copy command.

    if (auto pCommandListVulkan = pDeviceVulkan->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics}).Downcast<xiiGALCommandListVulkan>())
    {
      {
        xiiGALScopedDebugGroup debugGroup(pCommandListVulkan, "Add Swap Chain Wait Semaphore");

        pCommandListVulkan->AddWaitSemaphore(m_ImageAcquiredSemaphores[m_uiSemaphoreIndex], vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer);

        // Vulkan validation layers do not like uninitialized memory. Clear back buffer the first time we acquire it.
        if (!m_SwapChainImagesInitialized[m_uiBackBufferIndex])
        {
          pCommandListVulkan->ClearRenderTargetView(m_SwapChainTextures[m_uiBackBufferIndex]->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black);

          m_SwapChainImagesInitialized[m_uiBackBufferIndex] = true;
        }
      }

      auto pCommandQueue = pDeviceVulkan->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);

      pCommandQueue->Submit(pCommandListVulkan);
    }
  }

  m_pBackBufferTexture = m_SwapChainTextures[m_uiBackBufferIndex];

  return result;
}

void xiiGALSwapChainVulkan::WaitForImageAcquiredFences()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  for (xiiUInt32 i = 0; i < m_ImageAcquiredFences.GetCount(); ++i)
  {
    if (m_ImageAcquiredFenceSubmitted[i])
    {
      const vk::Fence& vkFence = m_ImageAcquiredFences[i];

      if (vkLogicalDevice.getFenceStatus(vkFence, pDeviceVulkan->GetVulkanDynamicDispatchLoader()) == vk::Result::eNotReady)
      {
        VK_ASSERT_DEV(vkLogicalDevice.waitForFences(1U, &vkFence, vk::True, xiiMath::MaxValue<xiiUInt64>(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
      }
    }
  }
}

void xiiGALSwapChainVulkan::Present()
{
  if (m_bIsMinimized)
    return;

  xiiSharedPtr<xiiGALDeviceVulkan>  pDeviceVulkan            = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiSharedPtr<xiiGALTextureVulkan> pCurrentBackbufferVulkan = m_pBackBufferTexture.Downcast<xiiGALTextureVulkan>();

  if (auto pCommandListVulkan = pDeviceVulkan->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics}).Downcast<xiiGALCommandListVulkan>())
  {
    pCommandListVulkan->TransitionImageLayout(pCurrentBackbufferVulkan, vk::ImageLayout::ePresentSrcKHR);
    pCommandListVulkan->AddSignalSemaphore(m_DrawCompleteSemaphores[m_uiSemaphoreIndex]);

    auto pCommandQueue = pDeviceVulkan->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);

    pCommandQueue->Submit(pCommandListVulkan);
  }

  {
    // Unlike fences or events, the act of waiting for a semaphore also un-signals that semaphore. (6.4.2)
    vk::Result result = vk::Result::eSuccess;

    vk::PresentInfoKHR vkPresentInformation = {};
    vkPresentInformation.pNext              = nullptr;
    vkPresentInformation.pResults           = &result;
    vkPresentInformation.pSwapchains        = &m_vkSwapChain;
    vkPresentInformation.pImageIndices      = &m_uiBackBufferIndex;
    vkPresentInformation.swapchainCount     = 1U;
    vkPresentInformation.pWaitSemaphores    = &m_DrawCompleteSemaphores[m_uiSemaphoreIndex];
    vkPresentInformation.waitSemaphoreCount = 1U;

    vk::Queue vkQueue = pDeviceVulkan->GetCommandQueueInformation(xiiGALCommandQueueFlags::Graphics).m_vkQueue;
    XII_IGNORE_UNUSED(vkQueue.presentKHR(&vkPresentInformation, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
    {
      RecreateVulkanSwapChain().AssertSuccess();

      m_uiSemaphoreIndex = m_Description.m_uiBufferCount - 1; // To start with 0 index when acquire next image.
    }
    else
    {
      XII_ASSERT_DEV(result == vk::Result::eSuccess, "Swap Chain presentation failed.");
    }
  }

  {
    ++m_uiSemaphoreIndex;
    if (m_uiSemaphoreIndex >= m_Description.m_uiBufferCount)
      m_uiSemaphoreIndex = 0U;

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
    XII_ASSERT_DEV(result == vk::Result::eSuccess, "Failed to acquire next swap chain image.");
  }
}

xiiResult xiiGALSwapChainVulkan::Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform)
{
  bool bRecreateSwapChain = false;

#if XII_ENABLED(XII_PLATFORM_ANDROID)
  if (m_vkSurface != VK_NULL_HANDLE)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
    vk::PhysicalDevice               vkPhysicalDevice = pDeviceVulkan->GetVulkanPhysicalDevice();

    // Check orientation.
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = {};
    VK_ASSERT_DEV(vkPhysicalDevice.getSurfaceCapabilitiesKHR(m_vkSurface, &surfaceCapabilities, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    if (m_vkCurrentSurfaceTransform != surfaceCapabilities.currentTransform)
    {
      // Surface orientation - we need to recreate the swap chain.
      bRecreateSwapChain = true;
    }

    constexpr vk::SurfaceTransformFlagsKHR rotate90TransformFlags = vk::SurfaceTransformFlagBitsKHR::eRotate90 | vk::SurfaceTransformFlagBitsKHR::eRotate270 | vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90 | vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270;

    if (!newSize.HasNonZeroArea())
    {
      newSize.width  = m_vkSurfaceIdentityExtent.width;
      newSize.height = m_vkSurfaceIdentityExtent.height;

      if (surfaceCapabilities.currentTransform & rotate90TransformFlags)
      {
        // Swap to get the logical dimensions as input new width and new height are expected to be logical sizes.
        xiiMath::Swap(newSize.width, newSize.height);
      }
    }

    if (newTransform == xiiGALSurfaceTransform::Optimal)
    {
      if (surfaceCapabilities.currentTransform & rotate90TransformFlags)
      {
        // Swap to get physical dimensions.
        xiiMath::Swap(newSize.width, newSize.height);
      }
    }
    else
    {
      // Swap if necessary to get the desired sizes after pre-transform.
      if (newTransform == xiiGALSurfaceTransform::Rotate90 || newTransform == xiiGALSurfaceTransform::Rotate270 || newTransform == xiiGALSurfaceTransform::HorizontalMirrorRotate90 || newTransform == xiiGALSurfaceTransform::HorizontalMirrorRotate270)
      {
        xiiMath::Swap(newSize.width, newSize.height);
      }
    }
  }
#endif

  if (newSize.HasNonZeroArea() && (newSize != m_Description.m_Resolution || m_DesiredSurfaceTransform != newTransform))
  {
    m_Description.m_Resolution = newSize;
    m_DesiredSurfaceTransform  = newTransform;
    bRecreateSwapChain         = true;

    xiiLog::Dev("Resizing swap chain to {}x{}.", m_Description.m_Resolution.width, m_Description.m_Resolution.height);
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

void xiiGALSwapChainVulkan::SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode)
{
  XII_IGNORE_UNUSED(displayMode);
}

void xiiGALSwapChainVulkan::SetWindowedMode()
{
}

void xiiGALSwapChainVulkan::SetMaximumFrameLatency(xiiUInt32 uiMaxLatency)
{
  XII_IGNORE_UNUSED(uiMaxLatency);
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_SwapChainVulkan);
