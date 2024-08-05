#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Profiling/Profiling.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/SwapChainVulkan.h>
#include <GraphicsVulkan/Resources/BottomLevelASVulkan.h>
#include <GraphicsVulkan/Resources/BufferViewVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/QueryVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Resources/SamplerVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>
#include <GraphicsVulkan/Resources/TopLevelASVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>
#include <GraphicsVulkan/States/BlendStateVulkan.h>
#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>
#include <GraphicsVulkan/States/PipelineStateVulkan.h>
#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

// Debug Utilities.
PFN_vkCreateDebugUtilsMessengerEXT  CreateDebugUtilsMessengerEXT  = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT = nullptr;
PFN_vkSetDebugUtilsObjectNameEXT    SetDebugUtilsObjectNameEXT    = nullptr;
PFN_vkSetDebugUtilsObjectTagEXT     SetDebugUtilsObjectTagEXT     = nullptr;
PFN_vkQueueBeginDebugUtilsLabelEXT  QueueBeginDebugUtilsLabelEXT  = nullptr;
PFN_vkQueueEndDebugUtilsLabelEXT    QueueEndDebugUtilsLabelEXT    = nullptr;
PFN_vkQueueInsertDebugUtilsLabelEXT QueueInsertDebugUtilsLabelEXT = nullptr;

// Debug Report.
PFN_vkCreateDebugReportCallbackEXT  CreateDebugReportCallbackEXT  = nullptr;
PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT = nullptr;

xiiInternal::NewInstance<xiiGALDevice> CreateVulkanDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  return XII_NEW(pAllocator, xiiGALDeviceVulkan, description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsVulkan, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  const xiiGALDeviceImplementationDescription implementation = {.m_APIType = xiiGALGraphicsDeviceType::Vulkan, .m_sShaderModel = "VK_SM60", .m_sShaderCompiler = "xiiShaderCompiler" };

  xiiGALDeviceFactory::RegisterImplementation("Vulkan", &CreateVulkanDevice, implementation);
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiGALDeviceFactory::UnregisterImplementation("Vulkan");
}

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  VKAPI_ATTR vk::Bool32 VKAPI_CALL xiiVulkanDebugMessengerCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageType, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
  {
    switch (messageSeverity)
    {
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
      {
        xiiLog::Debug("Vulkan: {}.", pCallbackData->pMessage);
      }
      break;
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
      {
        xiiLog::Info("Vulkan: {}.", pCallbackData->pMessage);
      }
      break;
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
      {
        xiiLog::Warning("Vulkan: {}.", pCallbackData->pMessage);
      }
      break;
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
      {
        xiiLog::Error("Vulkan: {}.", pCallbackData->pMessage);
      }
      break;
      default:
        break;
    }
    // The application should always return VK_FALSE. The VK_TRUE value is reserved for use in layer development.
    return VK_FALSE;
  }

  VKAPI_ATTR xiiUInt32 VKAPI_CALL xiiVulkanDebugReportCallback(vk::DebugReportFlagsEXT reportFlags, vk::DebugReportObjectTypeEXT objectType, xiiUInt64 uiObject, size_t uiLocation, xiiInt32 iMessageCode, const char* szLayerPrefix, const char* szMessage, void* pUserData)
  {
    if (reportFlags & vk::DebugReportFlagBitsEXT::eError)
    {
      xiiLog::Error("Vulkan: {}", szMessage);
    }
    else if (reportFlags & (vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning))
    {
      xiiLog::Warning("Vulkan: {}", szMessage);
    }
    else if (reportFlags & vk::DebugReportFlagBitsEXT::eDebug)
    {
      xiiLog::Debug("Vulkan: {}", szMessage);
    }
    else
    {
      xiiLog::Info("Vulkan: {}", szMessage);
    }
    return 0U;
  }
} // namespace

xiiGALDeviceVulkan::xiiGALDeviceVulkan(const xiiGALDeviceCreationDescription& description) :
  xiiGALDevice(description)
{
}

xiiGALDeviceVulkan::~xiiGALDeviceVulkan() = default;

xiiResult xiiGALDeviceVulkan::InitializePlatform()
{
  // Enumerate available layers.
  {
    xiiUInt32 uiLayerCount = 0U;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vk::enumerateInstanceLayerProperties(&uiLayerCount, nullptr));

    m_Layers.SetCount(uiLayerCount);

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vk::enumerateInstanceLayerProperties(&uiLayerCount, m_Layers.GetData()));

    XII_ASSERT_DEV(m_Layers.GetCount() == uiLayerCount, "Expected layer count ({0}) does not match the retrieved layer count ({1}).", uiLayerCount, m_Layers.GetCount());
  }

  {
    XII_LOG_BLOCK("Available Vulkan Instance Layers");

    for (const auto& layer : m_Layers)
    {
      xiiLog::Info("{} {}.{}.{}", layer.layerName, VK_API_VERSION_MAJOR(layer.specVersion), VK_API_VERSION_MINOR(layer.specVersion), VK_API_VERSION_PATCH(layer.specVersion));
    }
  }

  // Enumerate available instance extensions.
  {
    XII_LOG_BLOCK("Supported Vulkan Instance Extensions");

    std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties(nullptr);

    m_Extensions.Reserve(static_cast<xiiUInt32>(extensions.size()));

    for (const auto& extension : extensions)
    {
      m_Extensions.PushBack(extension);

      xiiLog::Info("{} {}.{}.{}", extension.extensionName, VK_API_VERSION_MAJOR(extension.specVersion), VK_API_VERSION_MINOR(extension.specVersion), VK_API_VERSION_PATCH(extension.specVersion));
    }
  }

#if XII_ENABLED(XII_PLATFORM_OSX)
  // From the 1.3.216 Vulkan SDK and later, the Vulkan Loader is strictly enforcing the new VK_KHR_PORTABILITY_subset extension.
  constexpr bool bUsePortabilityEnumeration = true;
#else
  constexpr bool bUsePortabilityEnumeration = false;
#endif

  // Request instance extensions.
  xiiHybridArray<const char*, 6U> instanceExtensions;
  {
    if (IsExtensionAvailable(m_Extensions, VK_KHR_SURFACE_EXTENSION_NAME))
    {
      instanceExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);

      // Enable surface extensions depending on build configuration.
#if defined(VK_USE_PLATFORM_WIN32_KHR)
      instanceExtensions.PushBack(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
      instanceExtensions.PushBack(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
      instanceExtensions.PushBack(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
      instanceExtensions.PushBack(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
      instanceExtensions.PushBack(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_IOS_MVK)
      instanceExtensions.PushBack(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
      instanceExtensions.PushBack(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif
    }

    if (bUsePortabilityEnumeration)
    {
      instanceExtensions.PushBack(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    }

    // This extension added to core in 1.1, but current version is 1.0
    if (IsExtensionAvailable(m_Extensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
      instanceExtensions.PushBack(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    for (const auto* szExtensionName : instanceExtensions)
    {
      XII_SUCCEED_OR_RETURN_FAILURE(IsExtensionAvailable(m_Extensions, szExtensionName), "Required extension () is not available.", szExtensionName);
    }
  }

  // Request instance layers.
  xiiHybridArray<const char*, 6U> instanceLayers;
  {
    // Validation instance layers.
    if (m_Description.m_ValidationLevel > xiiGALDeviceValidationLevel::Disabled)
    {
      if (IsExtensionAvailable(m_Extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
      {
        // Prefer VK_EXT_debug_utils.
        m_DebugMode = DebugMode::Utils;
      }
      else if (IsExtensionAvailable(m_Extensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
      {
        // If debug utils are unavailable (e.g. on Android), use VK_EXT_debug_report.
        m_DebugMode = DebugMode::Report;
      }

      const char* validationLayerNames[] = {
        "VK_LAYER_KHRONOS_validation", // Unified validation layer used on Desktop and Mobile platforms.
      };

      for (const char* szValidationLayerName : validationLayerNames)
      {
        xiiUInt32 uiLayerVersion = 0xFFFFFFFFU;
        if (!IsLayerAvailable(m_Layers, szValidationLayerName, &uiLayerVersion))
        {
          xiiLog::Error("Instance layer ({0}) is not available.", szValidationLayerName);
          continue;
        }

        // Beta extensions may vary and result in a crash.
        // New enums are not supported and may cause validation error.
        if (uiLayerVersion < VK_HEADER_VERSION_COMPLETE)
        {
          xiiLog::Warning("Layer '{}' version ({}.{}.{}) is less than the header version ({}.{}.{}).", szValidationLayerName, VK_API_VERSION_MAJOR(uiLayerVersion), VK_API_VERSION_MINOR(uiLayerVersion), VK_API_VERSION_PATCH(uiLayerVersion),
                          VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE), VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE), VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE));
        }

        instanceLayers.PushBack(szValidationLayerName);

        if (m_DebugMode != DebugMode::Utils)
        {
          // On Android, VK_EXT_debug_utils extension may not be supported by the loader,
          // but supported by the layer.

          xiiDynamicArray<vk::ExtensionProperties> layerExtensions;
          if (EnumerateInstanceExtensions(szValidationLayerName, layerExtensions))
          {
            if (IsExtensionAvailable(layerExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            {
              m_DebugMode = DebugMode::Utils;
            }

            if (m_DebugMode == DebugMode::Disabled && IsExtensionAvailable(layerExtensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
            {
              m_DebugMode = DebugMode::Report;
            }
          }
          else
          {
            xiiLog::Error("Failed to enumerate extensions for {} layer.", szValidationLayerName);
          }
        }
      }

      if (m_DebugMode == DebugMode::Utils)
      {
        instanceExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }
      else if (m_DebugMode == DebugMode::Report)
      {
        instanceExtensions.PushBack(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
      }
      else
      {
        xiiLog::Error("Neither {} nor {} extension is available. Debug tools (validation layer message logging, performance markers, etc.) will be disabled.", VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        m_DebugMode                     = DebugMode::Disabled;
        m_Description.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
      }
    }

    m_EnabledExtensions.PushBackRange(instanceLayers.GetArrayPtr());
  }

  // Create Vulkan Instance.
  {
    vk::ApplicationInfo applicationInformation = {};
    applicationInformation.sType               = vk::StructureType::eApplicationInfo;
    applicationInformation.apiVersion          = s_uiVulkanVersion;
    applicationInformation.applicationVersion  = VK_MAKE_VERSION(BUILDSYSTEM_SDKVERSION_MAJOR, BUILDSYSTEM_SDKVERSION_MINOR, BUILDSYSTEM_SDKVERSION_PATCH);
    applicationInformation.engineVersion       = VK_MAKE_VERSION(BUILDSYSTEM_SDKVERSION_MAJOR, BUILDSYSTEM_SDKVERSION_MINOR, BUILDSYSTEM_SDKVERSION_PATCH);
    applicationInformation.pApplicationName    = "XII";
    applicationInformation.pEngineName         = "XII";
    applicationInformation.pNext               = nullptr;

    vk::InstanceCreateInfo instanceCreateInformation  = {};
    instanceCreateInformation.sType                   = vk::StructureType::eInstanceCreateInfo;
    instanceCreateInformation.pNext                   = nullptr;
    instanceCreateInformation.pApplicationInfo        = &applicationInformation;
    instanceCreateInformation.enabledExtensionCount   = instanceExtensions.GetCount();
    instanceCreateInformation.ppEnabledExtensionNames = instanceExtensions.GetData();
    instanceCreateInformation.enabledLayerCount       = instanceLayers.GetCount();
    instanceCreateInformation.ppEnabledLayerNames     = instanceLayers.GetData();
    instanceCreateInformation.flags                   = {};

    if (bUsePortabilityEnumeration)
    {
      // The instance will enumerate available Vulkan Portability-compliant physical devices and groups in addition to the Vulkan physical devices and groups that
      // are enumerated by default.
      instanceCreateInformation.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    }

    m_Instance = vk::createInstance(instanceCreateInformation);

    XII_SUCCEED_OR_RETURN_FAILURE(m_Instance, "Failed to create Vulkan instance.");
  }

  // If requested, we enable the default validation layers for debugging purposes.
  if (m_DebugMode == DebugMode::Utils)
  {
    CreateDebugUtilsMessengerEXT  = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
    DestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));

    XII_ASSERT_DEV(CreateDebugUtilsMessengerEXT != nullptr && DestroyDebugUtilsMessengerEXT != nullptr, "Failed to load DebugUtilsMessenger extension functions.");

    constexpr VkDebugUtilsMessageSeverityFlagsEXT messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    constexpr VkDebugUtilsMessageTypeFlagsEXT     messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {};
    debugMessengerCreateInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerCreateInfo.pNext                              = nullptr;
    debugMessengerCreateInfo.flags                              = 0U;
    debugMessengerCreateInfo.messageSeverity                    = messageSeverity;
    debugMessengerCreateInfo.messageType                        = messageType;
    debugMessengerCreateInfo.pfnUserCallback                    = reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(xiiVulkanDebugMessengerCallback);
    debugMessengerCreateInfo.pUserData                          = nullptr;

    if (CreateDebugUtilsMessengerEXT != nullptr && DestroyDebugUtilsMessengerEXT != nullptr)
    {
      VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
      VK_SUCCEED_OR_RETURN_XII_FAILURE(CreateDebugUtilsMessengerEXT(m_Instance, &debugMessengerCreateInfo, nullptr, &debugMessenger));

      m_DebugMessenger = debugMessenger;

      // Load function pointers.
      SetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectNameEXT"));
      XII_ASSERT_DEV(SetDebugUtilsObjectNameEXT != nullptr, "Failed to load function pointer");
      SetDebugUtilsObjectTagEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectTagEXT"));
      XII_ASSERT_DEV(SetDebugUtilsObjectTagEXT != nullptr, "Failed to load function pointer");

      QueueBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(m_Instance, "vkQueueBeginDebugUtilsLabelEXT"));
      XII_ASSERT_DEV(QueueBeginDebugUtilsLabelEXT != nullptr, "Failed to load function pointer");
      QueueEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(m_Instance, "vkQueueEndDebugUtilsLabelEXT"));
      XII_ASSERT_DEV(QueueEndDebugUtilsLabelEXT != nullptr, "Failed to load function pointer");
      QueueInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(m_Instance, "vkQueueInsertDebugUtilsLabelEXT"));
      XII_ASSERT_DEV(QueueInsertDebugUtilsLabelEXT != nullptr, "Failed to load function pointer");
    }
  }
  else if (m_DebugMode == DebugMode::Report)
  {
    CreateDebugReportCallbackEXT  = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugReportCallbackEXT"));
    DestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugReportCallbackEXT"));

    XII_ASSERT_DEV(CreateDebugReportCallbackEXT != nullptr && DestroyDebugReportCallbackEXT != nullptr, "Failed to load DebugReportCallback extension functions.");

    constexpr VkDebugReportFlagBitsEXT reportFlags = static_cast<VkDebugReportFlagBitsEXT>(VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT);

    VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
    debugReportCallbackCreateInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugReportCallbackCreateInfo.pNext                              = nullptr;
    debugReportCallbackCreateInfo.flags                              = reportFlags;
    debugReportCallbackCreateInfo.pfnCallback                        = reinterpret_cast<PFN_vkDebugReportCallbackEXT>(xiiVulkanDebugReportCallback);
    debugReportCallbackCreateInfo.pUserData                          = nullptr;

    if (CreateDebugReportCallbackEXT != nullptr && DestroyDebugReportCallbackEXT != nullptr)
    {
      VkDebugReportCallbackEXT debugCallback = VK_NULL_HANDLE;
      VK_SUCCEED_OR_RETURN_XII_FAILURE(CreateDebugReportCallbackEXT(m_Instance, &debugReportCallbackCreateInfo, nullptr, &debugCallback));

      m_DebugCallback = debugCallback;
    }
  }

  // Enumerate physical devices.
  {
    xiiUInt32 uiPhysicalDeviceCount = 0U;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_Instance.enumeratePhysicalDevices(&uiPhysicalDeviceCount, nullptr));

    XII_ASSERT_ALWAYS(uiPhysicalDeviceCount != 0U, "No physical devices are found on the system.");

    m_PhysicalDevices.SetCount(uiPhysicalDeviceCount);

    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_Instance.enumeratePhysicalDevices(&uiPhysicalDeviceCount, m_PhysicalDevices.GetData()));

    XII_ASSERT_DEV(m_PhysicalDevices.GetCount() == uiPhysicalDeviceCount, "Expected physical device count ({0}) does not match the retrieved physical device count ({1}).", uiPhysicalDeviceCount, m_PhysicalDevices.GetCount());
  }

  // Select physical device.
  {
    m_PhysicalDevice = SelectPhysicalDevice(m_Description.m_uiAdapterID);

    if (m_PhysicalDevice != VK_NULL_HANDLE)
    {
      const vk::PhysicalDeviceProperties& deviceProperties = m_PhysicalDevice.getProperties();

      xiiLog::Info("Using physical device '{}', API version {}.{}.{}, Driver version {}.{}.{}.", deviceProperties.deviceName,
                   VK_API_VERSION_MAJOR(deviceProperties.apiVersion), VK_API_VERSION_MINOR(deviceProperties.apiVersion), VK_API_VERSION_PATCH(deviceProperties.apiVersion),
                   VK_API_VERSION_MAJOR(deviceProperties.driverVersion), VK_API_VERSION_MINOR(deviceProperties.driverVersion), VK_API_VERSION_PATCH(deviceProperties.driverVersion));
    }
    else
    {
      xiiLog::Error("Failed to find suitable Vulkan physical device.");

      return XII_FAILURE;
    }
  }

  xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  return XII_FAILURE;
}

void xiiGALDeviceVulkan::ReportLiveGPUObjects()
{
}

void xiiGALDeviceVulkan::FlushPendingObjects()
{
  FlushDestroyedObjects();
}

xiiResult xiiGALDeviceVulkan::ShutdownPlatform()
{
  ReportLiveGPUObjects();

  if (m_DebugMode != DebugMode::Disabled)
  {
    if (m_DebugMessenger != VK_NULL_HANDLE)
    {
      DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
    }

    if (m_DebugCallback != VK_NULL_HANDLE)
    {
      DestroyDebugReportCallbackEXT(m_Instance, m_DebugCallback, nullptr);
    }
  }

  m_Instance.destroy();

  return XII_SUCCESS;
}

xiiResult xiiGALDeviceVulkan::CreateCommandQueuesPlatform()
{
  return XII_FAILURE;
}

void xiiGALDeviceVulkan::BeginFramePlatform(xiiArrayPtr<xiiGALSwapChain*> swapchains, const xiiUInt64 uiRenderFrame)
{
  for (auto pSwapChain : swapchains)
  {
    pSwapChain->AcquireNextRenderTarget();
  }
}

void xiiGALDeviceVulkan::EndFramePlatform(xiiArrayPtr<xiiGALSwapChain*> swapchains)
{
  for (auto pSwapChain : swapchains)
  {
    pSwapChain->Present();
  }
}

xiiGALSwapChain* xiiGALDeviceVulkan::CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description)
{
  xiiGALSwapChainVulkan* pSwapChainVulkan = XII_NEW(&m_Allocator, xiiGALSwapChainVulkan, this, description);

  if (pSwapChainVulkan->InitPlatform().Succeeded())
    return pSwapChainVulkan;

  XII_DELETE(&m_Allocator, pSwapChainVulkan);

  return pSwapChainVulkan;
}

void xiiGALDeviceVulkan::DestroySwapChainPlatform(xiiGALSwapChain* pSwapChain)
{
  xiiGALSwapChainVulkan* pSwapChainVulkan = static_cast<xiiGALSwapChainVulkan*>(pSwapChain);

  pSwapChainVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pSwapChainVulkan);
}

xiiGALCommandQueue* xiiGALDeviceVulkan::CreateCommandQueuePlatform(const xiiGALCommandQueueCreationDescription& description)
{
  xiiGALCommandQueueVulkan* pCommandQueueVulkan = XII_NEW(&m_Allocator, xiiGALCommandQueueVulkan, this, description);

  if (pCommandQueueVulkan->InitPlatform().Succeeded())
    return pCommandQueueVulkan;

  XII_DELETE(&m_Allocator, pCommandQueueVulkan);

  return pCommandQueueVulkan;
}

void xiiGALDeviceVulkan::DestroyCommandQueuePlatform(xiiGALCommandQueue* pCommandQueue)
{
  xiiGALCommandQueueVulkan* pCommandQueueVulkan = static_cast<xiiGALCommandQueueVulkan*>(pCommandQueue);

  pCommandQueueVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pCommandQueueVulkan);
}

xiiGALBlendState* xiiGALDeviceVulkan::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)
{
  xiiGALBlendStateVulkan* pBlendStateVulkan = XII_NEW(&m_Allocator, xiiGALBlendStateVulkan, this, description);

  if (pBlendStateVulkan->InitPlatform().Succeeded())
    return pBlendStateVulkan;

  XII_DELETE(&m_Allocator, pBlendStateVulkan);

  return pBlendStateVulkan;
}

void xiiGALDeviceVulkan::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
  xiiGALBlendStateVulkan* pBlendStateVulkan = static_cast<xiiGALBlendStateVulkan*>(pBlendState);

  pBlendStateVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBlendStateVulkan);
}

xiiGALDepthStencilState* xiiGALDeviceVulkan::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)
{
  xiiGALDepthStencilStateVulkan* pDepthStencilStateVulkan = XII_NEW(&m_Allocator, xiiGALDepthStencilStateVulkan, this, description);

  if (pDepthStencilStateVulkan->InitPlatform().Succeeded())
    return pDepthStencilStateVulkan;

  XII_DELETE(&m_Allocator, pDepthStencilStateVulkan);

  return pDepthStencilStateVulkan;
}

void xiiGALDeviceVulkan::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
  xiiGALDepthStencilStateVulkan* pDepthStencilStateVulkan = static_cast<xiiGALDepthStencilStateVulkan*>(pDepthStencilState);

  pDepthStencilStateVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pDepthStencilStateVulkan);
}

xiiGALRasterizerState* xiiGALDeviceVulkan::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)
{
  xiiGALRasterizerStateVulkan* pRasterizerStateVulkan = XII_NEW(&m_Allocator, xiiGALRasterizerStateVulkan, this, description);

  if (pRasterizerStateVulkan->InitPlatform().Succeeded())
    return pRasterizerStateVulkan;

  XII_DELETE(&m_Allocator, pRasterizerStateVulkan);

  return pRasterizerStateVulkan;
}

void xiiGALDeviceVulkan::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  xiiGALRasterizerStateVulkan* pRasterizerStateVulkan = static_cast<xiiGALRasterizerStateVulkan*>(pRasterizerState);

  pRasterizerStateVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pRasterizerStateVulkan);
}

xiiGALShader* xiiGALDeviceVulkan::CreateShaderPlatform(const xiiGALShaderCreationDescription& description)
{
  xiiGALShaderVulkan* pShaderVulkan = XII_NEW(&m_Allocator, xiiGALShaderVulkan, this, description);

  if (pShaderVulkan->InitPlatform().Succeeded())
    return pShaderVulkan;

  XII_DELETE(&m_Allocator, pShaderVulkan);

  return pShaderVulkan;
}

void xiiGALDeviceVulkan::DestroyShaderPlatform(xiiGALShader* pShader)
{
  xiiGALShaderVulkan* pShaderVulkan = static_cast<xiiGALShaderVulkan*>(pShader);

  pShaderVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pShaderVulkan);
}

xiiGALBuffer* xiiGALDeviceVulkan::CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData)
{
  xiiGALBufferVulkan* pBufferVulkan = XII_NEW(&m_Allocator, xiiGALBufferVulkan, this, description);

  if (pBufferVulkan->InitPlatform(pInitialData).Succeeded())
    return pBufferVulkan;

  XII_DELETE(&m_Allocator, pBufferVulkan);

  return pBufferVulkan;
}

void xiiGALDeviceVulkan::DestroyBufferPlatform(xiiGALBuffer* pBuffer)
{
  xiiGALBufferVulkan* pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pBuffer);

  pBufferVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferVulkan);
}

xiiGALBufferView* xiiGALDeviceVulkan::CreateBufferViewPlatform(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& description)
{
  xiiGALBufferViewVulkan* pBufferViewVulkan = XII_NEW(&m_Allocator, xiiGALBufferViewVulkan, this, pBuffer, description);

  if (pBufferViewVulkan->InitPlatform().Succeeded())
    return pBufferViewVulkan;

  XII_DELETE(&m_Allocator, pBufferViewVulkan);

  return pBufferViewVulkan;
}

void xiiGALDeviceVulkan::DestroyBufferViewPlatform(xiiGALBufferView* pBufferView)
{
  xiiGALBufferViewVulkan* pBufferViewVulkan = static_cast<xiiGALBufferViewVulkan*>(pBufferView);

  pBufferViewVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferViewVulkan);
}

xiiGALTexture* xiiGALDeviceVulkan::CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData)
{
  xiiGALTextureVulkan* pTextureVulkan = XII_NEW(&m_Allocator, xiiGALTextureVulkan, this, description);

  if (pTextureVulkan->InitPlatform(pInitialData).Succeeded())
    return pTextureVulkan;

  XII_DELETE(&m_Allocator, pTextureVulkan);

  return pTextureVulkan;
}

void xiiGALDeviceVulkan::DestroyTexturePlatform(xiiGALTexture* pTexture)
{
  xiiGALTextureVulkan* pTextureVulkan = static_cast<xiiGALTextureVulkan*>(pTexture);

  pTextureVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureVulkan);
}

xiiGALTextureView* xiiGALDeviceVulkan::CreateTextureViewPlatform(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& description)
{
  xiiGALTextureViewVulkan* pTextureViewVulkan = XII_NEW(&m_Allocator, xiiGALTextureViewVulkan, this, pTexture, description);

  if (pTextureViewVulkan->InitPlatform().Succeeded())
    return pTextureViewVulkan;

  XII_DELETE(&m_Allocator, pTextureViewVulkan);

  return pTextureViewVulkan;
}

void xiiGALDeviceVulkan::DestroyTextureViewPlatform(xiiGALTextureView* pTextureView)
{
  xiiGALTextureViewVulkan* pTextureViewVulkan = static_cast<xiiGALTextureViewVulkan*>(pTextureView);

  pTextureViewVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureViewVulkan);
}

xiiGALSampler* xiiGALDeviceVulkan::CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)
{
  xiiGALSamplerVulkan* pSamplerVulkan = XII_NEW(&m_Allocator, xiiGALSamplerVulkan, this, description);

  if (pSamplerVulkan->InitPlatform().Succeeded())
    return pSamplerVulkan;

  XII_DELETE(&m_Allocator, pSamplerVulkan);

  return pSamplerVulkan;
}

void xiiGALDeviceVulkan::DestroySamplerPlatform(xiiGALSampler* pSampler)
{
  xiiGALSamplerVulkan* pSamplerVulkan = static_cast<xiiGALSamplerVulkan*>(pSampler);

  pSamplerVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pSamplerVulkan);
}

xiiGALInputLayout* xiiGALDeviceVulkan::CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description)
{
  xiiGALInputLayoutVulkan* pInputLayoutVulkan = XII_NEW(&m_Allocator, xiiGALInputLayoutVulkan, this, description);

  if (pInputLayoutVulkan->InitPlatform().Succeeded())
    return pInputLayoutVulkan;

  XII_DELETE(&m_Allocator, pInputLayoutVulkan);

  return pInputLayoutVulkan;
}

void xiiGALDeviceVulkan::DestroyInputLayoutPlatform(xiiGALInputLayout* pInputLayout)
{
  xiiGALInputLayoutVulkan* pInputLayoutVulkan = static_cast<xiiGALInputLayoutVulkan*>(pInputLayout);

  pInputLayoutVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pInputLayoutVulkan);
}

xiiGALQuery* xiiGALDeviceVulkan::CreateQueryPlatform(const xiiGALQueryCreationDescription& description)
{
  xiiGALQueryVulkan* pQueryVulkan = XII_NEW(&m_Allocator, xiiGALQueryVulkan, this, description);

  if (pQueryVulkan->InitPlatform().Succeeded())
    return pQueryVulkan;

  XII_DELETE(&m_Allocator, pQueryVulkan);

  return pQueryVulkan;
}

void xiiGALDeviceVulkan::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
  xiiGALQueryVulkan* pQueryVulkan = static_cast<xiiGALQueryVulkan*>(pQuery);

  pQueryVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pQueryVulkan);
}

xiiGALFence* xiiGALDeviceVulkan::CreateFencePlatform(const xiiGALFenceCreationDescription& description)
{
  xiiGALFenceVulkan* pFenceVulkan = XII_NEW(&m_Allocator, xiiGALFenceVulkan, this, description);

  if (pFenceVulkan->InitPlatform().Succeeded())
    return pFenceVulkan;

  XII_DELETE(&m_Allocator, pFenceVulkan);

  return pFenceVulkan;
}

void xiiGALDeviceVulkan::DestroyFencePlatform(xiiGALFence* pFence)
{
  xiiGALFenceVulkan* pFenceVulkan = static_cast<xiiGALFenceVulkan*>(pFence);

  pFenceVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pFenceVulkan);
}

xiiGALRenderPass* xiiGALDeviceVulkan::CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)
{
  xiiGALRenderPassVulkan* pRenderPassVulkan = XII_NEW(&m_Allocator, xiiGALRenderPassVulkan, this, description);

  if (pRenderPassVulkan->InitPlatform().Succeeded())
    return pRenderPassVulkan;

  XII_DELETE(&m_Allocator, pRenderPassVulkan);

  return pRenderPassVulkan;
}

void xiiGALDeviceVulkan::DestroyRenderPassPlatform(xiiGALRenderPass* pRenderPass)
{
  xiiGALRenderPassVulkan* pRenderPassVulkan = static_cast<xiiGALRenderPassVulkan*>(pRenderPass);

  pRenderPassVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pRenderPassVulkan);
}

xiiGALFramebuffer* xiiGALDeviceVulkan::CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)
{
  xiiGALFramebufferVulkan* pFramebufferVulkan = XII_NEW(&m_Allocator, xiiGALFramebufferVulkan, this, description);

  if (pFramebufferVulkan->InitPlatform().Succeeded())
    return pFramebufferVulkan;

  XII_DELETE(&m_Allocator, pFramebufferVulkan);

  return pFramebufferVulkan;
}

void xiiGALDeviceVulkan::DestroyFramebufferPlatform(xiiGALFramebuffer* pFramebuffer)
{
  xiiGALFramebufferVulkan* pFramebufferVulkan = static_cast<xiiGALFramebufferVulkan*>(pFramebuffer);

  pFramebufferVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pFramebufferVulkan);
}

xiiGALBottomLevelAS* xiiGALDeviceVulkan::CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)
{
  xiiGALBottomLevelASVulkan* pBottomLevelASVulkan = XII_NEW(&m_Allocator, xiiGALBottomLevelASVulkan, this, description);

  if (pBottomLevelASVulkan->InitPlatform().Succeeded())
    return pBottomLevelASVulkan;

  XII_DELETE(&m_Allocator, pBottomLevelASVulkan);

  return pBottomLevelASVulkan;
}

void xiiGALDeviceVulkan::DestroyBottomLevelASPlatform(xiiGALBottomLevelAS* pBottomLevelAS)
{
  xiiGALBottomLevelASVulkan* pBottomLevelASVulkan = static_cast<xiiGALBottomLevelASVulkan*>(pBottomLevelAS);

  pBottomLevelASVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBottomLevelASVulkan);
}

xiiGALTopLevelAS* xiiGALDeviceVulkan::CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)
{
  xiiGALTopLevelASVulkan* pTopLevelASVulkan = XII_NEW(&m_Allocator, xiiGALTopLevelASVulkan, this, description);

  if (pTopLevelASVulkan->InitPlatform().Succeeded())
    return pTopLevelASVulkan;

  XII_DELETE(&m_Allocator, pTopLevelASVulkan);

  return pTopLevelASVulkan;
}

void xiiGALDeviceVulkan::DestroyTopLevelASPlatform(xiiGALTopLevelAS* pTopLevelAS)
{
  xiiGALTopLevelASVulkan* pTopLevelASVulkan = static_cast<xiiGALTopLevelASVulkan*>(pTopLevelAS);

  pTopLevelASVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pTopLevelASVulkan);
}

xiiGALPipelineResourceSignature* xiiGALDeviceVulkan::CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description)
{
  xiiGALPipelineResourceSignatureVulkan* pPipelineResourceSignatureVulkan = XII_NEW(&m_Allocator, xiiGALPipelineResourceSignatureVulkan, this, description);

  if (pPipelineResourceSignatureVulkan->InitPlatform().Succeeded())
    return pPipelineResourceSignatureVulkan;

  XII_DELETE(&m_Allocator, pPipelineResourceSignatureVulkan);

  return pPipelineResourceSignatureVulkan;
}

void xiiGALDeviceVulkan::DestroyPipelineResourceSignaturePlatform(xiiGALPipelineResourceSignature* pPipelineResourceSignature)
{
  xiiGALPipelineResourceSignatureVulkan* pPipelineResourceSignatureVulkan = static_cast<xiiGALPipelineResourceSignatureVulkan*>(pPipelineResourceSignature);

  pPipelineResourceSignatureVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pPipelineResourceSignatureVulkan);
}

xiiGALPipelineState* xiiGALDeviceVulkan::CreatePipelineStatePlatform(const xiiGALPipelineStateCreationDescription& description)
{
  xiiGALPipelineStateVulkan* pPipelineStateVulkan = XII_NEW(&m_Allocator, xiiGALPipelineStateVulkan, this, description);

  if (pPipelineStateVulkan->InitPlatform().Succeeded())
    return pPipelineStateVulkan;

  XII_DELETE(&m_Allocator, pPipelineStateVulkan);

  return pPipelineStateVulkan;
}

void xiiGALDeviceVulkan::DestroyPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
  xiiGALPipelineStateVulkan* pPipelineStateVulkan = static_cast<xiiGALPipelineStateVulkan*>(pPipelineState);

  pPipelineStateVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pPipelineStateVulkan);
}

void xiiGALDeviceVulkan::WaitIdlePlatform()
{
  FlushPendingObjects();
}

void xiiGALDeviceVulkan::FillCapabilitiesPlatform()
{
}

void xiiGALDeviceVulkan::CreateCommandQueues()
{
}

vk::PhysicalDevice xiiGALDeviceVulkan::SelectPhysicalDevice(xiiUInt32 uiAdapterID) const
{
  const auto IsGraphicsAndComputeQueueSupported = [](const vk::PhysicalDevice& physicalDevice) -> bool {
    xiiUInt32 uiQueueFamilyCount = 0U;
    physicalDevice.getQueueFamilyProperties(&uiQueueFamilyCount, nullptr);

    XII_ASSERT_DEV(uiQueueFamilyCount > 0, "");

    xiiHybridArray<vk::QueueFamilyProperties, 2U> queueFamilyProperties;
    queueFamilyProperties.SetCount(uiQueueFamilyCount);

    physicalDevice.getQueueFamilyProperties(&uiQueueFamilyCount, queueFamilyProperties.GetData());
    XII_ASSERT_DEV(queueFamilyProperties.GetCount() == uiQueueFamilyCount, "");

    // If an implementation exposes any queue family that supports graphics operations, at least one queue family of at least one physical device exposed by the implementation
    // must support both graphics and compute operations.
    for (const vk::QueueFamilyProperties& properties : queueFamilyProperties)
    {
      if ((properties.queueFlags & vk::QueueFlagBits::eGraphics) && (properties.queueFlags & vk::QueueFlagBits::eCompute))
      {
        return true;
      }
    }
    return false;
  };

  vk::PhysicalDevice selectedPhysicalDevice = VK_NULL_HANDLE;

  if (uiAdapterID < m_PhysicalDevices.GetCount() && IsGraphicsAndComputeQueueSupported(m_PhysicalDevices[uiAdapterID]))
  {
    selectedPhysicalDevice = m_PhysicalDevices[uiAdapterID];
  }

  // Device Selection Criteria:
  // - Exposes a queue family that supports both compute and graphics operations.
  // - Prefer discrete GPU.
  if (selectedPhysicalDevice == VK_NULL_HANDLE)
  {
    for (const vk::PhysicalDevice& physicalDevice : m_PhysicalDevices)
    {
      const vk::PhysicalDeviceProperties& deviceProperties = physicalDevice.getProperties();

      if (IsGraphicsAndComputeQueueSupported(physicalDevice))
      {
        selectedPhysicalDevice = physicalDevice;

        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
          break;
      }
    }
  }

  return selectedPhysicalDevice;
}

bool xiiGALDeviceVulkan::EnumerateInstanceExtensions(const char* szLayerName, xiiDynamicArray<vk::ExtensionProperties>& extensions)
{
  xiiUInt32 uiExtensionCount = 0U;

  if (vk::enumerateInstanceExtensionProperties(szLayerName, &uiExtensionCount, nullptr) != vk::Result::eSuccess)
    return false;

  extensions.SetCount(uiExtensionCount);

  if (vk::enumerateInstanceExtensionProperties(szLayerName, &uiExtensionCount, extensions.GetData()) != vk::Result::eSuccess)
  {
    extensions.Clear();

    return false;
  }

  XII_ASSERT_DEV(extensions.GetCount() == uiExtensionCount, "The number of extensions written by vk::enumerateInstanceExtensionProperties is not consistent with the count returned in the first call. This is likely a Vulkan loader bug.");

  return true;
}

bool xiiGALDeviceVulkan::IsLayerAvailable(xiiArrayPtr<const vk::LayerProperties> pLayers, const char* szLayerName, xiiUInt32* pVersion /*= nullptr*/)
{
  for (const auto& layer : pLayers)
  {
    if (strcmp(szLayerName, layer.layerName) == 0U)
    {
      if (pVersion != nullptr)
      {
        *pVersion = layer.specVersion;
      }
      return true;
    }
  }
  return false;
}

bool xiiGALDeviceVulkan::IsExtensionAvailable(xiiArrayPtr<const vk::ExtensionProperties> pExtensions, const char* szExtensionName)
{
  for (const auto& extension : pExtensions)
  {
    if (strcmp(szExtensionName, extension.extensionName) == 0U)
    {
      return true;
    }
  }
  return false;
}

bool xiiGALDeviceVulkan::IsExtensionEnabled(const char* szExtensionName)
{
  for (const auto* szEnabledExtension : m_EnabledExtensions)
  {
    if (strcmp(szExtensionName, szEnabledExtension) == 0)
    {
      return true;
    }
  }
  return false;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_DeviceVulkan);
