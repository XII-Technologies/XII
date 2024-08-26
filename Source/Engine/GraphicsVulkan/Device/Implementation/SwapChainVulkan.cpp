#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Core/System/Window.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/SwapChainVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

xiiGALSwapChainVulkan::xiiGALSwapChainVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(pDeviceVulkan, creationDescription)
{
}

xiiGALSwapChainVulkan::~xiiGALSwapChainVulkan() = default;

xiiResult xiiGALSwapChainVulkan::InitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  return CreateBackBufferInternal(pDeviceVulkan);
}

xiiResult xiiGALSwapChainVulkan::DeInitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);
  XII_ASSERT_NOT_IMPLEMENTED;
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

  // Retrieve the list of vk::Formats that are supported.
  xiiUInt32 uiFormatCount = 0U;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceFormatsKHR(m_vkSurface, &uiFormatCount, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiFormatCount > 0U, "");

  xiiDynamicArray<vk::SurfaceFormatKHR> supportedFormats(pDeviceVulkan->GetAllocator());
  supportedFormats.SetCount(uiFormatCount);
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkPhysicalDevice.getSurfaceFormatsKHR(m_vkSurface, &uiFormatCount, supportedFormats.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  XII_ASSERT_DEV(uiFormatCount == supportedFormats.GetCount(), "");

  m_vkColorFormat = xiiVulkanTypeConversions::GetFormat(m_Description.m_ColorBufferFormat);

  vk::ColorSpaceKHR colorSpace = vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
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
        case vk::Format::eR8G8B8A8Unorm: vk::Format::eB8G8R8A8Unorm; break;
        case vk::Format::eB8G8R8A8Unorm: vk::Format::eR8G8B8A8Unorm; break;
        case vk::Format::eB8G8R8A8Srgb: vk::Format::eR8G8B8A8Srgb; break;
        case vk::Format::eR8G8B8A8Srgb: vk::Format::eB8G8R8A8Srgb; break;

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

  return XII_FAILURE;
}

xiiResult xiiGALSwapChainVulkan::CreateBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan)
{
  return XII_FAILURE;
}

void xiiGALSwapChainVulkan::DestroyBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan)
{
}

void xiiGALSwapChainVulkan::AcquireNextRenderTarget()
{
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
