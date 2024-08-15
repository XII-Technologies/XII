#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Profiling/Profiling.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

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
#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

#include <bitset>

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

// Shading Rates
PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR GetPhysicalDeviceFragmentShadingRatesKHR = nullptr;

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
  XII_LOG_BLOCK("xiiGALDeviceVulkan::InitializePlatform");

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
    m_uiVulkanVersion = VK_API_VERSION_1_1;

    vk::ApplicationInfo applicationInformation = {};
    applicationInformation.sType               = vk::StructureType::eApplicationInfo;
    applicationInformation.apiVersion          = m_uiVulkanVersion;
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

    m_InstanceDispatchLoader.init(m_Instance, vkGetInstanceProcAddr);

    if (m_Instance == VK_NULL_HANDLE)
    {
      xiiLog::Error("Failed to create Vulkan instance.");
      return XII_FAILURE;
    }
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
      XII_ASSERT_DEV(SetDebugUtilsObjectNameEXT != nullptr, "Failed to load vkSetDebugUtilsObjectNameEXT function pointer");
      SetDebugUtilsObjectTagEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectTagEXT"));
      XII_ASSERT_DEV(SetDebugUtilsObjectTagEXT != nullptr, "Failed to load vkSetDebugUtilsObjectTagEXT function pointer");

      QueueBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(m_Instance, "vkQueueBeginDebugUtilsLabelEXT"));
      XII_ASSERT_DEV(QueueBeginDebugUtilsLabelEXT != nullptr, "Failed to load vkQueueBeginDebugUtilsLabelEXT function pointer");
      QueueEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(m_Instance, "vkQueueEndDebugUtilsLabelEXT"));
      XII_ASSERT_DEV(QueueEndDebugUtilsLabelEXT != nullptr, "Failed to load vkQueueEndDebugUtilsLabelEXT function pointer");
      QueueInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(m_Instance, "vkQueueInsertDebugUtilsLabelEXT"));
      XII_ASSERT_DEV(QueueInsertDebugUtilsLabelEXT != nullptr, "Failed to load vkQueueInsertDebugUtilsLabelEXT function pointer");
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

    m_PhysicalDeviceProperties       = m_PhysicalDevice.getProperties();
    m_PhysicalDeviceFeatures         = m_PhysicalDevice.getFeatures();
    m_PhysicalDeviceMemoryProperties = m_PhysicalDevice.getMemoryProperties();

    xiiUInt32 uiQueueFamilyCount = 0U;
    m_PhysicalDevice.getQueueFamilyProperties(&uiQueueFamilyCount, nullptr);

    XII_ASSERT_DEV(uiQueueFamilyCount > 0U, "");

    m_PhysicalDeviceQueueFamilyProperties.SetCount(uiQueueFamilyCount);
    m_PhysicalDevice.getQueueFamilyProperties(&uiQueueFamilyCount, m_PhysicalDeviceQueueFamilyProperties.GetData());

    XII_ASSERT_DEV(m_PhysicalDeviceQueueFamilyProperties.GetCount() == uiQueueFamilyCount, "");

    // Get list of supported extensions.
    xiiUInt32 uiExtensionCount = 0U;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_PhysicalDevice.enumerateDeviceExtensionProperties(nullptr, &uiExtensionCount, nullptr));

    if (uiExtensionCount > 0U)
    {
      m_PhysicalDeviceSupportedExtensions.SetCount(uiExtensionCount);

      VK_SUCCEED_OR_RETURN_XII_FAILURE(m_PhysicalDevice.enumerateDeviceExtensionProperties(nullptr, &uiExtensionCount, m_PhysicalDeviceSupportedExtensions.GetData()));

      XII_ASSERT_DEV(m_PhysicalDeviceSupportedExtensions.GetCount() == uiExtensionCount, "");

      {
        xiiStringBuilder sb;
        sb.SetFormat("Device '{}' Supported Extensions", m_PhysicalDeviceProperties.deviceName);

        XII_LOG_BLOCK(sb);

        for (const auto& extensionProperty : m_PhysicalDeviceSupportedExtensions)
        {
          xiiLog::Info("{} {}.{}.{}", extensionProperty.extensionName, VK_API_VERSION_MAJOR(extensionProperty.specVersion), VK_API_VERSION_MINOR(extensionProperty.specVersion), VK_API_VERSION_PATCH(extensionProperty.specVersion));
        }
      }
    }

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

    XII_SUCCEED_OR_RETURN(InitializePhysicalDeviceProperties());
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

xiiResult xiiGALDeviceVulkan::FillCapabilitiesPlatform()
{
  XII_LOG_BLOCK("xiiGALDeviceVulkan::FillCapabilitiesPlatform");

  // Graphics Adapter Properties
  {
    m_Description.m_GraphicsDeviceType = xiiGALGraphicsDeviceType::Vulkan;

    m_AdapterDescription.m_sAdapterName = m_PhysicalDeviceProperties.deviceName;

    switch (m_PhysicalDeviceProperties.deviceType)
    {
      case vk::PhysicalDeviceType::eIntegratedGpu:
        m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Integrated;
        break;

      case vk::PhysicalDeviceType::eDiscreteGpu:
        m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Discrete;
        break;

      case vk::PhysicalDeviceType::eCpu:
        m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Software;
        break;

      default:
        m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Unknown;
        break;
    }

    m_AdapterDescription.m_Vendor             = xiiGALGraphicsUtilities::GetVendorFromID(m_PhysicalDeviceProperties.vendorID);
    m_AdapterDescription.m_uiVendorID         = m_PhysicalDeviceProperties.vendorID;
    m_AdapterDescription.m_uiDeviceID         = m_PhysicalDeviceProperties.deviceID;
    m_AdapterDescription.m_uiVideoOutputCount = 0U;
  }

  // Buffer Properties
  {
    m_AdapterDescription.m_BufferProperties.m_uiConstantBufferAlignment         = static_cast<xiiUInt32>(m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
    m_AdapterDescription.m_BufferProperties.m_uiStructuredBufferOffsetAlignment = static_cast<xiiUInt32>(m_PhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment);

    static_assert(sizeof(m_AdapterDescription.m_BufferProperties) == 8, "There may be uninitialized buffer properties.");
  }

  // Texture Properties
  {
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DDimension     = m_PhysicalDeviceProperties.limits.maxImageDimension1D;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DArraySlices   = m_PhysicalDeviceProperties.limits.maxImageArrayLayers;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DDimension     = m_PhysicalDeviceProperties.limits.maxImageDimension2D;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DArraySlices   = m_PhysicalDeviceProperties.limits.maxImageArrayLayers;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture3DDimension     = m_PhysicalDeviceProperties.limits.maxImageDimension3D;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTextureCubeDimension   = m_PhysicalDeviceProperties.limits.maxImageDimensionCube;
    m_AdapterDescription.m_TextureProperties.m_bTexture2DMSSupported       = true;
    m_AdapterDescription.m_TextureProperties.m_bTexture2DMSArraySupported  = true;
    m_AdapterDescription.m_TextureProperties.m_bTextureViewSupported       = true;
    m_AdapterDescription.m_TextureProperties.m_bCubeMapArraysSupported     = m_PhysicalDeviceFeatures.imageCubeArray == VK_TRUE;
    m_AdapterDescription.m_TextureProperties.m_bTextureView2DOn3DSupported = m_PhysicalDeviceExtensionFeatures.m_bHasPortabilitySubset ? m_PhysicalDeviceExtensionFeatures.m_PortabilitySubset.imageView2DOn3DImage == VK_TRUE : true;

    static_assert(sizeof(m_AdapterDescription.m_TextureProperties) == 32, "There may be uninitialized texture properties.");
  }

  // Sampler Properties
  {
    m_AdapterDescription.m_SamplerProperties.m_bBorderSamplingModeSupported = true;
    m_AdapterDescription.m_SamplerProperties.m_uiMaxAnisotropy              = static_cast<xiiUInt8>(m_PhysicalDeviceProperties.limits.maxSamplerAnisotropy);
    m_AdapterDescription.m_SamplerProperties.m_bLODBiasSupported            = true;

    static_assert(sizeof(m_AdapterDescription.m_SamplerProperties) == 3, "There may be uninitialized sampler properties.");
  }

  // Ray Tracing Properties
  if (m_AdapterDescription.m_Features.m_RayTracing != xiiGALDeviceFeatureState::Disabled)
  {
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxRecursionDepth        = m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.maxRayRecursionDepth;
    m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupHandleSize    = m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.shaderGroupHandleSize;
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxShaderRecordStride    = m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.maxShaderGroupStride;
    m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupBaseAlignment = m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.shaderGroupBaseAlignment;
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxRayGenThreads         = m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.maxRayDispatchInvocationCount;
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxInstancesPerTLAS      = static_cast<xiiUInt32>(m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.maxInstanceCount);
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxPrimitivesPerBLAS     = static_cast<xiiUInt32>(m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.maxPrimitiveCount);
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxGeometriesPerBLAS     = static_cast<xiiUInt32>(m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.maxGeometryCount);
    m_AdapterDescription.m_RayTracingProperties.m_uiVertexBufferAlignment    = 1U;
    m_AdapterDescription.m_RayTracingProperties.m_uiIndexBufferAlignment     = 1U;
    m_AdapterDescription.m_RayTracingProperties.m_uiTransformBufferAlignment = 16; // From the specification.
    m_AdapterDescription.m_RayTracingProperties.m_uiBoxBufferAlignment       = 8;  // From the specification.
    m_AdapterDescription.m_RayTracingProperties.m_uiInstanceBufferAlignment  = 16; // From the specification.
    m_AdapterDescription.m_RayTracingProperties.m_uiScratchBufferAlignment   = m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.minAccelerationStructureScratchOffsetAlignment;
    m_AdapterDescription.m_RayTracingProperties.m_uiInstanceBufferAlignment  = 16; // From the specification.

    if (m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline.rayTracingPipeline)
      m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::StandaloneShaders;

    if (m_PhysicalDeviceExtensionFeatures.m_RayQuery.rayQuery)
      m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::InlineRayTracing;

    if (m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline.rayTracingPipelineTraceRaysIndirect)
      m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::IndirectRayTracing;

    static_assert(sizeof(m_AdapterDescription.m_RayTracingProperties) == 60, "There may be uninitialized ray tracing properties.");
  }

  // Wave Operation Properties
  {
    vk::ShaderStageFlags supportedStages = m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedStages & (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute);

    if (m_PhysicalDeviceFeatures.geometryShader != VK_FALSE)
      supportedStages |= m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedStages & vk::ShaderStageFlagBits::eGeometry;

    if (m_PhysicalDeviceFeatures.tessellationShader != VK_FALSE)
      supportedStages |= m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedStages & (vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation);

    if (m_PhysicalDeviceExtensionFeatures.m_MeshShader.meshShader != VK_FALSE && m_PhysicalDeviceExtensionFeatures.m_MeshShader.taskShader != VK_FALSE)
      supportedStages |= m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedStages & (vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT);

    if (m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline.rayTracingPipeline != VK_FALSE)
    {
      constexpr auto allRayTracingFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eCallableKHR;

      supportedStages |= m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedStages & allRayTracingFlags;
    }

    m_AdapterDescription.m_WaveOperationProperties.m_uiMinSize             = m_PhysicalDeviceExtensionProperties.m_Subgroup.subgroupSize;
    m_AdapterDescription.m_WaveOperationProperties.m_uiMaxSize             = m_PhysicalDeviceExtensionProperties.m_Subgroup.subgroupSize;
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages = xiiVulkanTypeConversions::GetGALShaderStageFlags(supportedStages);
    m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures          = xiiGALWaveFeature::Unknown;

    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eBasic)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Basic;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eVote)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Vote;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eArithmetic)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Arithmetic;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eBallot)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Ballot;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eShuffle)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Shuffle;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eShuffleRelative)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::ShuffleRelative;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eClustered)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Clustered;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eQuad)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Quad;

    static_assert(sizeof(m_AdapterDescription.m_WaveOperationProperties) == 16, "There may be uninitialized wave operation properties.");
  }

  // Mesh Shader Properties
  {
    m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountX     = m_PhysicalDeviceExtensionProperties.m_MeshShader.maxMeshWorkGroupCount[0];
    m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountY     = m_PhysicalDeviceExtensionProperties.m_MeshShader.maxMeshWorkGroupCount[1];
    m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountZ     = m_PhysicalDeviceExtensionProperties.m_MeshShader.maxMeshWorkGroupCount[2];
    m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupTotalCount = m_PhysicalDeviceExtensionProperties.m_MeshShader.maxMeshWorkGroupTotalCount;

    static_assert(sizeof(m_AdapterDescription.m_MeshShaderProperties) == 16, "There may be uninitialized mesh shader properties.");
  }

  // Compute Shader Properties
  {
    m_AdapterDescription.m_ComputeShaderProperties.m_uiSharedMemorySize          = m_PhysicalDeviceProperties.limits.maxComputeSharedMemorySize;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupInvocations = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupInvocations;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeX       = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[0];
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeY       = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[1];
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeZ       = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[2];
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountX      = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[0];
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountY      = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[1];
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountZ      = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[2];

    static_assert(sizeof(m_AdapterDescription.m_ComputeShaderProperties) == 32, "There may be uninitialized compute shader properties.");
  }

  // Shading Rate Properties
  if (m_AdapterDescription.m_Features.m_VariableRateShading != xiiGALDeviceFeatureState::Disabled)
  {
    // VK_KHR_fragment_shading_rate
    if (m_PhysicalDeviceExtensionFeatures.m_ShadingRate.pipelineFragmentShadingRate != VK_FALSE || m_PhysicalDeviceExtensionFeatures.m_ShadingRate.primitiveFragmentShadingRate != VK_FALSE || m_PhysicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate != VK_FALSE)
    {
      auto& shadingRateCapabilityFlags = m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags;
      auto  SetShadingRateCapability   = [&shadingRateCapabilityFlags](VkBool32 vkFlag, xiiGALShadingRateCapabilityFlags::Enum capabilityFlag) {
        if (vkFlag != VK_FALSE)
        {
          shadingRateCapabilityFlags |= capabilityFlag;
        }
      };

      SetShadingRateCapability(m_PhysicalDeviceExtensionFeatures.m_ShadingRate.pipelineFragmentShadingRate, xiiGALShadingRateCapabilityFlags::PerDraw);
      SetShadingRateCapability(m_PhysicalDeviceExtensionFeatures.m_ShadingRate.primitiveFragmentShadingRate, xiiGALShadingRateCapabilityFlags::PerPrimitive);
      SetShadingRateCapability(m_PhysicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate, xiiGALShadingRateCapabilityFlags::TextureBased);
      SetShadingRateCapability(m_PhysicalDeviceExtensionProperties.m_ShadingRate.fragmentShadingRateWithSampleMask, xiiGALShadingRateCapabilityFlags::SampleMask);
      SetShadingRateCapability(m_PhysicalDeviceExtensionProperties.m_ShadingRate.fragmentShadingRateWithShaderSampleMask, xiiGALShadingRateCapabilityFlags::ShaderSampleMask);
      SetShadingRateCapability(m_PhysicalDeviceExtensionProperties.m_ShadingRate.fragmentShadingRateWithShaderDepthStencilWrites, xiiGALShadingRateCapabilityFlags::ShaderDepthStencilWrite);
      SetShadingRateCapability(m_PhysicalDeviceExtensionProperties.m_ShadingRate.primitiveFragmentShadingRateWithMultipleViewports, xiiGALShadingRateCapabilityFlags::PerPrimitiveWithMultipleViewports);
      SetShadingRateCapability(m_PhysicalDeviceExtensionProperties.m_ShadingRate.layeredShadingRateAttachments, xiiGALShadingRateCapabilityFlags::TextureArray);

      if (shadingRateCapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureBased))
        shadingRateCapabilityFlags |= xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget;

      // Always enabled in Vulkan.
      shadingRateCapabilityFlags |= xiiGALShadingRateCapabilityFlags::ShadingRateShaderInput;

      m_AdapterDescription.m_ShadingRateProperties.m_CombinerFlags = xiiGALShadingRateCombinerFlags::PassThrough | xiiGALShadingRateCombinerFlags::CombinerOverride;

      if (m_PhysicalDeviceExtensionProperties.m_ShadingRate.fragmentShadingRateNonTrivialCombinerOps != VK_FALSE)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_CombinerFlags |= xiiGALShadingRateCombinerFlags::CombinerMin | xiiGALShadingRateCombinerFlags::CombinerMax;
        m_AdapterDescription.m_ShadingRateProperties.m_CombinerFlags |= (m_PhysicalDeviceExtensionProperties.m_ShadingRate.fragmentShadingRateStrictMultiplyCombiner != VK_FALSE) ? xiiGALShadingRateCombinerFlags::CombinerMul : xiiGALShadingRateCombinerFlags::CombinerSum;
      }

      if (m_PhysicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate != VK_FALSE)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_Format             = xiiGALShadingRateFormat::Palette;
        m_AdapterDescription.m_ShadingRateProperties.m_MinTileSize.width  = m_PhysicalDeviceExtensionProperties.m_ShadingRate.minFragmentShadingRateAttachmentTexelSize.width;
        m_AdapterDescription.m_ShadingRateProperties.m_MinTileSize.height = m_PhysicalDeviceExtensionProperties.m_ShadingRate.minFragmentShadingRateAttachmentTexelSize.height;
        m_AdapterDescription.m_ShadingRateProperties.m_MaxTileSize.width  = m_PhysicalDeviceExtensionProperties.m_ShadingRate.maxFragmentShadingRateAttachmentTexelSize.width;
        m_AdapterDescription.m_ShadingRateProperties.m_MaxTileSize.height = m_PhysicalDeviceExtensionProperties.m_ShadingRate.maxFragmentShadingRateAttachmentTexelSize.height;
      }

      std::vector<VkPhysicalDeviceFragmentShadingRateKHR> shadingRates;
      {
        GetPhysicalDeviceFragmentShadingRatesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR>(vkGetInstanceProcAddr(m_Instance, "vkGetPhysicalDeviceFragmentShadingRatesKHR"));
        XII_ASSERT_DEV(GetPhysicalDeviceFragmentShadingRatesKHR, "Failed to load vkGetPhysicalDeviceFragmentShadingRatesKHR extension functions.");

        xiiUInt32 uiShadingRateCount = 0U;
        VK_SUCCEED_OR_RETURN_XII_FAILURE(GetPhysicalDeviceFragmentShadingRatesKHR(m_PhysicalDevice, &uiShadingRateCount, nullptr));

        shadingRates.resize(uiShadingRateCount);

        for (auto& rate : shadingRates)
          rate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR;

        VK_SUCCEED_OR_RETURN_XII_FAILURE(GetPhysicalDeviceFragmentShadingRatesKHR(m_PhysicalDevice, &uiShadingRateCount, shadingRates.data()));
      }

      constexpr VkSampleCountFlags VK_SAMPLE_COUNT_ALL = ((xiiUInt32)vk::SampleCountFlagBits::e64 << 1) - 1;

      const xiiUInt32 uiShadingRateCount = static_cast<xiiUInt8>(xiiMath::Min(shadingRates.size(), size_t{XII_GAL_MAX_SHADING_RATE}));
      for (xiiUInt32 i = 0; i < uiShadingRateCount; ++i)
      {
        const auto& srcShadingRate = shadingRates[i];
        auto&       dstShadingRate = m_AdapterDescription.m_ShadingRateProperties.m_Modes.ExpandAndGetRef();

        // maxFragmentShadingRateRasterizationSamples - contains only maximum bit
        // sampleCounts - contains all supported bits
        XII_ASSERT_DEV((srcShadingRate.fragmentSize.width == 1 && srcShadingRate.fragmentSize.height == 1) || (xiiUInt32{srcShadingRate.sampleCounts} <= ((static_cast<xiiUInt32>(m_PhysicalDeviceExtensionProperties.m_ShadingRate.maxFragmentShadingRateRasterizationSamples) << 1) - 1)), "");

        switch (srcShadingRate.sampleCounts)
        {
          case VK_SAMPLE_COUNT_1_BIT:
            dstShadingRate.m_SampleBits = xiiGALSampleCount::OneSample;
            break;
          case VK_SAMPLE_COUNT_2_BIT:
            dstShadingRate.m_SampleBits = xiiGALSampleCount::TwoSamples;
            break;
          case VK_SAMPLE_COUNT_4_BIT:
            dstShadingRate.m_SampleBits = xiiGALSampleCount::FourSamples;
            break;
          case VK_SAMPLE_COUNT_8_BIT:
            dstShadingRate.m_SampleBits = xiiGALSampleCount::EightSamples;
            break;
          case VK_SAMPLE_COUNT_16_BIT:
            dstShadingRate.m_SampleBits = xiiGALSampleCount::SixteenSamples;
            break;
          case VK_SAMPLE_COUNT_32_BIT:
            dstShadingRate.m_SampleBits = xiiGALSampleCount::ThirtyTwoSamples;
            break;
          case VK_SAMPLE_COUNT_64_BIT:
            dstShadingRate.m_SampleBits = xiiGALSampleCount::SixtyFourSamples;
            break;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }

        dstShadingRate.m_ShadingRate = xiiVulkanTypeConversions::FragmentSizeToShadingRate(vk::Extent2D{srcShadingRate.fragmentSize});
      }
    }
    // VK_EXT_fragment_density_map
    else if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMap != VK_FALSE)
    {
      m_AdapterDescription.m_ShadingRateProperties.m_Format          = xiiGALShadingRateFormat::RG8UNormalized;
      m_AdapterDescription.m_ShadingRateProperties.m_CombinerFlags   = xiiGALShadingRateCombinerFlags::PassThrough | xiiGALShadingRateCombinerFlags::CombinerOverride;
      m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags = xiiGALShadingRateCapabilityFlags::TextureBased | xiiGALShadingRateCapabilityFlags::SameTextureForWholeRenderPass | xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget;

      if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMapDynamic != VK_FALSE)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_TextureAccess = xiiGALShadingRateTextureAccess::OnGPU;
      }
      else if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap2.fragmentDensityMapDeferred != VK_FALSE)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_TextureAccess = xiiGALShadingRateTextureAccess::OnSubmit;
      }
      else
      {
        m_AdapterDescription.m_ShadingRateProperties.m_TextureAccess = xiiGALShadingRateTextureAccess::OnSetRenderTarget;
      }

      if (m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.fragmentDensityInvocations != VK_FALSE)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags |= xiiGALShadingRateCapabilityFlags::AdditionalInvocations;
      }
      if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMapNonSubsampledImages != VK_FALSE)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags |= xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget;
      }

      // This is zero if VK_EXT_fragment_density_map2 is not supported.
      m_AdapterDescription.m_ShadingRateProperties.m_uiMaxSubSampledArraySlices = m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap2.maxSubsampledArrayLayers;

      m_AdapterDescription.m_ShadingRateProperties.m_MinTileSize.width  = m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.minFragmentDensityTexelSize.width;
      m_AdapterDescription.m_ShadingRateProperties.m_MinTileSize.height = m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.minFragmentDensityTexelSize.height;
      m_AdapterDescription.m_ShadingRateProperties.m_MaxTileSize.width  = m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.maxFragmentDensityTexelSize.width;
      m_AdapterDescription.m_ShadingRateProperties.m_MaxTileSize.height = m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.maxFragmentDensityTexelSize.height;

      xiiGALShadingRateMode shadingMode{.m_ShadingRate = xiiGALShadingRateFlags::_1X1, .m_SampleBits = xiiGALSampleCount::AllSamples};

      m_AdapterDescription.m_ShadingRateProperties.m_Modes.PushBack(shadingMode);
    }

    // Retrieve supported bind flags.
    if (m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureBased))
    {
      vk::Format          vkShadingRateTextureFormat = vk::Format::eUndefined;
      vk::ImageUsageFlags vkShadingRateTextureUsage  = (vk::ImageUsageFlagBits)0U;

      if (m_AdapterDescription.m_ShadingRateProperties.m_Format == xiiGALShadingRateFormat::RG8UNormalized)
      {
        vkShadingRateTextureFormat = vk::Format::eR8G8Unorm;
        vkShadingRateTextureUsage  = vk::ImageUsageFlagBits::eFragmentDensityMapEXT;
      }
      else
      {
        vkShadingRateTextureFormat = vk::Format::eR8Uint;
        vkShadingRateTextureUsage  = vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR;
      }

      auto TestImageUsage = [&](vk::ImageUsageFlags usageFlags) -> bool {
        vk::ImageFormatProperties imageFormatProperties = {};

        vk::Result result = m_PhysicalDevice.getImageFormatProperties(vkShadingRateTextureFormat, vk::ImageType::e2D, vk::ImageTiling::eOptimal, vkShadingRateTextureUsage | usageFlags, {}, &imageFormatProperties);

        return result == vk::Result::eSuccess;
      };

      vk::FormatProperties formatProperties = {};
      m_PhysicalDevice.getFormatProperties(vkShadingRateTextureFormat, &formatProperties);
      XII_ASSERT_DEV(formatProperties.optimalTilingFeatures & (vk::FormatFeatureFlagBits::eFragmentShadingRateAttachmentKHR | vk::FormatFeatureFlagBits::eFragmentDensityMapEXT), "");

      m_AdapterDescription.m_ShadingRateProperties.m_BindFlags = xiiGALBindFlags::ShadingRate;
      if ((formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage) && TestImageUsage(vk::ImageUsageFlagBits::eSampled))
      {
        m_AdapterDescription.m_ShadingRateProperties.m_BindFlags |= xiiGALBindFlags::ShaderResource;
      }
      if ((formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eStorageImage) && TestImageUsage(vk::ImageUsageFlagBits::eStorage))
      {
        m_AdapterDescription.m_ShadingRateProperties.m_BindFlags |= xiiGALBindFlags::UnorderedAccess;
      }
      if ((formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment) && TestImageUsage(vk::ImageUsageFlagBits::eColorAttachment))
      {
        m_AdapterDescription.m_ShadingRateProperties.m_BindFlags |= xiiGALBindFlags::RenderTarget;
      }
    }

    static_assert(sizeof(m_AdapterDescription.m_ShadingRateProperties) == 80, "There may be uninitialized shading rate properties.");
  }

  // Draw Command Properties
  {
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxIndexValue        = m_PhysicalDeviceProperties.limits.maxDrawIndexedIndexValue;
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxDrawIndirectCount = m_PhysicalDeviceProperties.limits.maxDrawIndirectCount;
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags        = xiiGALDrawCommandCapabilityFlags::DrawIndirect | xiiGALDrawCommandCapabilityFlags::BaseVertex;

    if (m_PhysicalDeviceFeatures.multiDrawIndirect != VK_FALSE || m_PhysicalDeviceExtensionFeatures.m_bDrawIndirectCount)
    {
      m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::NativeMultiDrawIndirect;
    }
    if (m_PhysicalDeviceFeatures.drawIndirectFirstInstance != VK_FALSE)
    {
      m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::DrawIndirectFirstInstance;
    }
    if (m_PhysicalDeviceExtensionFeatures.m_bDrawIndirectCount)
    {
      m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer;
    }

    static_assert(sizeof(m_AdapterDescription.m_DrawCommandProperties) == 12, "There may be uninitialized draw command properties.");
  }

  // Sparse Memory Properties
  {
    XII_ASSERT_DEV(m_PhysicalDeviceFeatures.sparseBinding && (m_PhysicalDeviceFeatures.sparseResidencyBuffer || m_PhysicalDeviceFeatures.sparseResidencyImage2D), "");

    m_AdapterDescription.m_SparseResourceProperties.m_uiAddressSpaceSize  = m_PhysicalDeviceProperties.limits.sparseAddressSpaceSize;
    m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize = m_PhysicalDeviceProperties.limits.sparseAddressSpaceSize; // Currently no way to query.
    m_AdapterDescription.m_SparseResourceProperties.m_uiStandardBlockSize = 64U << 10U;                                               // Documentation: "All currently defined standard sparse image block shapes are 64 KB in size."

    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags       = xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::IndexBuffer | xiiGALBindFlags::UniformBuffer | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments | xiiGALBindFlags::RayTracing;
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags = xiiGALSparseResourceCapabilityFlags::NonResidentSafe | xiiGALSparseResourceCapabilityFlags::MixedResourceTypeSupport;

    auto& sparseResourceCapabilityFlags = m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags;
    auto  SetResourceCapabilityFlag     = [&sparseResourceCapabilityFlags](vk::Bool32 feature, xiiBitflags<xiiGALSparseResourceCapabilityFlags> flag) -> void {
      if (feature != VK_FALSE)
      {
        sparseResourceCapabilityFlags |= flag;
      }
    };

    SetResourceCapabilityFlag(m_PhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape, xiiGALSparseResourceCapabilityFlags::Standard2DTileShape);
    SetResourceCapabilityFlag(m_PhysicalDeviceProperties.sparseProperties.residencyStandard2DMultisampleBlockShape, xiiGALSparseResourceCapabilityFlags::Standard2DMSTileShape);
    SetResourceCapabilityFlag(m_PhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape, xiiGALSparseResourceCapabilityFlags::Standard3DTileShape);
    SetResourceCapabilityFlag(m_PhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize, xiiGALSparseResourceCapabilityFlags::AlignedMipSize);
    SetResourceCapabilityFlag(m_PhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict, xiiGALSparseResourceCapabilityFlags::NonResidentStrict);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.shaderResourceResidency, xiiGALSparseResourceCapabilityFlags::ShaderResourceResidency);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidencyBuffer, xiiGALSparseResourceCapabilityFlags::Buffer);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidencyImage2D, xiiGALSparseResourceCapabilityFlags::Texture2D | xiiGALSparseResourceCapabilityFlags::Texture2DArrayMipTail);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidencyImage3D, xiiGALSparseResourceCapabilityFlags::Texture3D);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidency2Samples, xiiGALSparseResourceCapabilityFlags::Texture2Samples);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidency4Samples, xiiGALSparseResourceCapabilityFlags::Texture4Samples);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidency8Samples, xiiGALSparseResourceCapabilityFlags::Texture8Samples);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidency16Samples, xiiGALSparseResourceCapabilityFlags::Texture16Samples);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidencyAliased, xiiGALSparseResourceCapabilityFlags::Aliased);

    static_assert(sizeof(m_AdapterDescription.m_SparseResourceProperties) == 32, "There may be uninitialized sparse resource properties.");
  }

  // Memory Properties
  {
    m_AdapterDescription.m_MemoryProperties.m_uiLocalMemory         = 0U;
    m_AdapterDescription.m_MemoryProperties.m_uiHostVisibleMemory   = 0U;
    m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory       = 0U;
    m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation = m_PhysicalDeviceExtensionProperties.m_Maintenance3.maxMemoryAllocationSize;

    std::bitset<VK_MAX_MEMORY_HEAPS> deviceLocalHeap;
    std::bitset<VK_MAX_MEMORY_HEAPS> hostVisibleHeap;
    std::bitset<VK_MAX_MEMORY_HEAPS> unifiedHeap;

    for (xiiUInt32 uiType = 0U; uiType < m_PhysicalDeviceMemoryProperties.memoryTypeCount; ++uiType)
    {
      constexpr vk::MemoryPropertyFlags unifiedMemoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible;

      const vk::MemoryType& memoryTypeInformation = m_PhysicalDeviceMemoryProperties.memoryTypes[uiType];

      if (memoryTypeInformation.propertyFlags & vk::MemoryPropertyFlagBits::eLazilyAllocated)
      {
        m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil | xiiGALBindFlags::InputAttachment;
      }
      else if ((memoryTypeInformation.propertyFlags & unifiedMemoryFlags) == unifiedMemoryFlags)
      {
        unifiedHeap[memoryTypeInformation.heapIndex] = true;

        if (memoryTypeInformation.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent)
        {
          m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags |= xiiGALCPUAccessFlag::Write;
        }
        if (memoryTypeInformation.propertyFlags & vk::MemoryPropertyFlagBits::eHostCached)
        {
          m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags |= xiiGALCPUAccessFlag::Read;
        }
      }
      else if (memoryTypeInformation.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
      {
        deviceLocalHeap[memoryTypeInformation.heapIndex] = true;
      }
      else if (memoryTypeInformation.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
      {
        hostVisibleHeap[memoryTypeInformation.heapIndex] = true;
      }

      // In Metal, input attachment with memoryless texture must be used as an imageblock, which is not supported in SPIRV to MSL translator.
#if XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_IOS)
      if (m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags.IsAnyFlagSet())
      {
        m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil;
      }
#endif
    }

    for (xiiUInt32 uiHeapIndex = 0U; uiHeapIndex < m_PhysicalDeviceMemoryProperties.memoryHeapCount; ++uiHeapIndex)
    {
      const vk::MemoryHeap& heapInformation = m_PhysicalDeviceMemoryProperties.memoryHeaps[uiHeapIndex];

      if (unifiedHeap[uiHeapIndex])
      {
        m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory += static_cast<xiiUInt64>(heapInformation.size);
      }
      else if (deviceLocalHeap[uiHeapIndex])
      {
        m_AdapterDescription.m_MemoryProperties.m_uiLocalMemory += static_cast<xiiUInt64>(heapInformation.size);
      }
      else if (hostVisibleHeap[uiHeapIndex])
      {
        m_AdapterDescription.m_MemoryProperties.m_uiHostVisibleMemory += static_cast<xiiUInt64>(heapInformation.size);
      }
    }

    static_assert(sizeof(m_AdapterDescription.m_MemoryProperties) == 40, "There may be uninitialized memory properties.");
  }

  // Queue Information
  {
    const xiiUInt32 uiMaxAdapterQueues = xiiMath::Min(XII_GAL_MAX_ADAPTER_QUEUE_COUNT, m_PhysicalDeviceQueueFamilyProperties.GetCount());

    for (xiiUInt32 uiQueueIndex = 0U; uiQueueIndex < uiMaxAdapterQueues; ++uiQueueIndex)
    {
      const vk::QueueFamilyProperties& sourceQueue      = m_PhysicalDeviceQueueFamilyProperties[uiQueueIndex];
      xiiGALCommandQueueProperties&    destinationQueue = m_AdapterDescription.m_CommandQueueProperties.ExpandAndGetRef();

      destinationQueue.m_Type                      = xiiVulkanTypeConversions::GetGALCommandQueueType(sourceQueue.queueFlags);
      destinationQueue.m_uiMaxDeviceContexts       = sourceQueue.queueCount;
      destinationQueue.m_TextureCopyGranularity[0] = sourceQueue.minImageTransferGranularity.width;
      destinationQueue.m_TextureCopyGranularity[1] = sourceQueue.minImageTransferGranularity.height;
      destinationQueue.m_TextureCopyGranularity[2] = sourceQueue.minImageTransferGranularity.depth;
    }
  }

  return XII_FAILURE;
}

void xiiGALDeviceVulkan::CreateCommandQueues()
{
  xiiHybridArray<const char*, 4U> deviceExtensions;

  if (IsExtensionEnabled(VK_KHR_SURFACE_EXTENSION_NAME))
  {
    deviceExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);
  }

  if (IsExtensionAvailable(m_Extensions, VK_KHR_MAINTENANCE1_EXTENSION_NAME))
  {
    // To allow negative viewport height.
    deviceExtensions.PushBack(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
  }
  else
  {
    xiiLog::Error("{} is not supported.", VK_KHR_MAINTENANCE1_EXTENSION_NAME);
  }
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

xiiResult xiiGALDeviceVulkan::InitializePhysicalDeviceProperties()
{
  XII_ASSERT_DEV(m_PhysicalDevice != VK_NULL_HANDLE, "");

  if (m_PhysicalDevice == VK_NULL_HANDLE)
    return XII_FAILURE;

  if (!IsExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    return XII_SUCCESS;

  vk::PhysicalDeviceFeatures2 features2    = {};
  void**                      pNextFeature = &features2.pNext;

  vk::PhysicalDeviceProperties2 properties2   = {};
  void**                        pNextProperty = &properties2.pNext;

  if (IsExtensionAvailable(m_Extensions, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_ShaderFloat16Int8;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_ShaderFloat16Int8.pNext;
  }

  // VK_KHR_16bit_storage and VK_KHR_8bit_storage extensions require VK_KHR_storage_buffer_storage_class extension.
  if (IsExtensionAvailable(m_Extensions, VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME))
  {
    if (IsExtensionAvailable(m_Extensions, VK_KHR_16BIT_STORAGE_EXTENSION_NAME))
    {
      *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_Storage16Bit;
      pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_Storage16Bit.pNext;
    }

    if (IsExtensionAvailable(m_Extensions, VK_KHR_8BIT_STORAGE_EXTENSION_NAME))
    {
      *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_Storage8Bit;
      pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_Storage8Bit.pNext;
    }
  }

  // Get mesh shader features and properties.
  if (IsExtensionAvailable(m_Extensions, VK_EXT_MESH_SHADER_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_MeshShader;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_MeshShader.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_MeshShader;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_MeshShader.pNext;
  }

  // Get acceleration structure features and properties.
  if (IsExtensionAvailable(m_Extensions, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_AccelerationStructure;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_AccelerationStructure.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_AccelerationStructure;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.pNext;
  }

  // Get ray tracing pipeline features and properties.
  if (IsExtensionAvailable(m_Extensions, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.pNext;
  }

  // Get inline ray tracing features.
  if (IsExtensionAvailable(m_Extensions, VK_KHR_RAY_QUERY_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_RayQuery;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_RayQuery.pNext;
  }

  // Additional extension that is required for ray tracing.
  if (IsExtensionAvailable(m_Extensions, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_BufferDeviceAddress;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_BufferDeviceAddress.pNext;
  }

  // Additional extension that is required for ray tracing.
  if (IsExtensionAvailable(m_Extensions, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_DescriptorIndexing;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_DescriptorIndexing.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_DescriptorIndexing;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_DescriptorIndexing.pNext;
  }

  // Additional extension that is required for ray tracing shader.
  if (IsExtensionAvailable(m_Extensions, VK_KHR_SPIRV_1_4_EXTENSION_NAME))
    m_PhysicalDeviceExtensionFeatures.m_bSpirv14 = true;

  // Some features require SPIRV 1.4 or 1.5 which was added to the Vulkan 1.2 core.
  if (m_uiVulkanVersion >= VK_API_VERSION_1_2)
  {
    m_PhysicalDeviceExtensionFeatures.m_bSpirv14 = true;
    m_PhysicalDeviceExtensionFeatures.m_bSpirv15 = true;
  }

  // Extension required for MoltenVk
  if (IsExtensionAvailable(m_Extensions, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_PortabilitySubset;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_PortabilitySubset.pNext;

    m_PhysicalDeviceExtensionFeatures.m_bHasPortabilitySubset = true;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_PortabilitySubset;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_PortabilitySubset.pNext;
  }

  // Subgroup feature requires Vulkan 1.1 core.
  if (m_uiVulkanVersion >= VK_API_VERSION_1_1)
  {
    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_Subgroup;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_Subgroup.pNext;

    m_PhysicalDeviceExtensionFeatures.m_bSubgroupOps = true;
  }

  if (IsExtensionAvailable(m_Extensions, VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_VertexAttributeDivisor;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_VertexAttributeDivisor.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_VertexAttributeDivisor;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_VertexAttributeDivisor.pNext;
  }

  if (IsExtensionAvailable(m_Extensions, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_TimelineSemaphore;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_TimelineSemaphore.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_TimelineSemaphore;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_TimelineSemaphore.pNext;
  }

  if (IsExtensionAvailable(m_Extensions, VK_KHR_MULTIVIEW_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_Multiview;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_Multiview.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_Multiview;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_Multiview.pNext;
  }

  if (IsExtensionAvailable(m_Extensions, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bRenderPass2 = true;
  }

  if (IsExtensionAvailable(m_Extensions, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_ShadingRate;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_ShadingRate.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_ShadingRate;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_ShadingRate.pNext;
  }

  if (IsExtensionAvailable(m_Extensions, VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.pNext;
  }

  if (IsExtensionAvailable(m_Extensions, VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_HostQueryReset;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_HostQueryReset.pNext;
  }

  if (IsExtensionAvailable(m_Extensions, VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bDrawIndirectCount = true;
  }

  if (IsExtensionAvailable(m_Extensions, VK_KHR_MAINTENANCE3_EXTENSION_NAME))
  {
    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_Maintenance3;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_Maintenance3.pNext;
  }

  if (IsExtensionAvailable(m_Extensions, VK_EXT_MULTI_DRAW_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_MultiDraw;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_MultiDraw.pNext;

    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_ShaderDrawParameters;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_ShaderDrawParameters.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_MultiDraw;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_MultiDraw.pNext;
  }

  // Ensure that last pNext is null
  *pNextFeature  = nullptr;
  *pNextProperty = nullptr;

  auto GetPhysicalDevice2FeaturesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(m_Instance.getProcAddr("vkGetPhysicalDeviceFeatures2KHR", m_InstanceDispatchLoader));
  XII_ASSERT_DEV(GetPhysicalDevice2FeaturesKHR != nullptr, "Failed to load vkGetPhysicalDeviceFeatures2KHR function pointer.");

  auto GetPhysicalDevice2PropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(m_Instance.getProcAddr("vkGetPhysicalDeviceProperties2KHR", m_InstanceDispatchLoader));
  XII_ASSERT_DEV(GetPhysicalDevice2PropertiesKHR != nullptr, "Failed to load vkGetPhysicalDeviceProperties2KHR function pointer.");

  return XII_SUCCESS;
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
