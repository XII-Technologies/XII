#include <RendererVulkan/RendererVulkanPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/PlatformFeatures.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Device/PassVulkan.h>
#include <RendererVulkan/Device/SwapChainVulkan.h>
#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Pools/FencePoolVulkan.h>
#include <RendererVulkan/Pools/QueryPoolVulkan.h>
#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/FallbackResourcesVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ImageCopyVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

#if XII_ENABLED(XII_SUPPORTS_GLFW)
#  include <GLFW/glfw3.h>
#endif

XII_DEFINE_AS_POD_TYPE(VkLayerProperties);

namespace
{
  VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
  {
    switch (messageSeverity)
    {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        xiiLog::Debug("VK: {}", pCallbackData->pMessage);
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        xiiLog::Info("VK: {}", pCallbackData->pMessage);
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        xiiLog::Warning("VK: {}", pCallbackData->pMessage);
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        xiiLog::Error("VK: {}", pCallbackData->pMessage);
        break;
      default:
        break;
    }
    // Only layers are allowed to return true here.
    return VK_FALSE;
  }

  bool isInstanceLayerPresent(const char* layerName)
  {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    xiiDynamicArray<VkLayerProperties> availableLayers;
    availableLayers.SetCountUninitialized(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.GetData());

    for (const auto& layerProperties : availableLayers)
    {
      if (strcmp(layerName, layerProperties.layerName) == 0)
      {
        return true;
      }
    }

    return false;
  }
} // namespace

// Need to implement these extension functions so vulkan hpp can call them.
// They're basically just adapters calling the function pointer retrieved previously.

PFN_vkSetDebugUtilsObjectNameEXT   vkSetDebugUtilsObjectNameEXTFunc;
PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXTFunc;
PFN_vkQueueEndDebugUtilsLabelEXT   vkQueueEndDebugUtilsLabelEXTFunc;
PFN_vkCmdBeginDebugUtilsLabelEXT   vkCmdBeginDebugUtilsLabelEXTFunc;
PFN_vkCmdEndDebugUtilsLabelEXT     vkCmdEndDebugUtilsLabelEXTFunc;
PFN_vkCmdInsertDebugUtilsLabelEXT  vkCmdInsertDebugUtilsLabelEXTFunc;


VkResult vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pObjectName)
{
  return vkSetDebugUtilsObjectNameEXTFunc(device, pObjectName);
}

void vkQueueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo)
{
  return vkQueueBeginDebugUtilsLabelEXTFunc(queue, pLabelInfo);
}

void vkQueueEndDebugUtilsLabelEXT(VkQueue queue)
{
  return vkQueueEndDebugUtilsLabelEXTFunc(queue);
}
//
// void vkQueueInsertDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo)
//{
//  return vkQueueInsertDebugUtilsLabelEXTFunc(queue, pLabelInfo);
//}

void vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo)
{
  return vkCmdBeginDebugUtilsLabelEXTFunc(commandBuffer, pLabelInfo);
}

void vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
{
  return vkCmdEndDebugUtilsLabelEXTFunc(commandBuffer);
}

void vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo)
{
  return vkCmdInsertDebugUtilsLabelEXTFunc(commandBuffer, pLabelInfo);
}

xiiInternal::NewInstance<xiiGALDevice> CreateVulkanDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description)
{
  return XII_NEW(pAllocator, xiiGALDeviceVulkan, Description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RendererVulkan, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  xiiGALDeviceFactory::RegisterCreatorFunc("Vulkan", &CreateVulkanDevice, "VULKAN", "xiiShaderCompilerDXC");
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiGALDeviceFactory::UnregisterCreatorFunc("Vulkan");
}

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiGALDeviceVulkan::xiiGALDeviceVulkan(const xiiGALDeviceCreationDescription& Description) :
  xiiGALDevice(Description)
{
}

xiiGALDeviceVulkan::~xiiGALDeviceVulkan() = default;

// Init & shutdown functions


vk::Result xiiGALDeviceVulkan::SelectInstanceExtensions(xiiHybridArray<const char*, 6>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  xiiUInt32 extensionCount;
  VK_SUCCEED_OR_RETURN_LOG(vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
  xiiDynamicArray<vk::ExtensionProperties> extensionProperties;
  extensionProperties.SetCount(extensionCount);
  VK_SUCCEED_OR_RETURN_LOG(vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.GetData()));

  XII_LOG_BLOCK("InstanceExtensions");
  for (auto& ext : extensionProperties)
  {
    xiiLog::Info("{}", ext.extensionName.data());
  }

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> vk::Result {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const vk::ExtensionProperties& prop) { return xiiStringUtils::IsEqual(prop.extensionName.data(), extensionName); });
    if (it != end(extensionProperties))
    {
      extensions.PushBack(extensionName);
      enableFlag = true;
      return vk::Result::eSuccess;
    }
    enableFlag = false;
    return vk::Result::eErrorExtensionNotPresent;
  };

  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_SURFACE_EXTENSION_NAME, m_extensions.m_bSurface));
#if XII_ENABLED(XII_SUPPORTS_GLFW)
  uint32_t     iNumGlfwExtensions = 0;
  const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&iNumGlfwExtensions);
  bool         dummy              = false;
  for (uint32_t i = 0; i < iNumGlfwExtensions; ++i)
  {
    VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(glfwExtensions[i], dummy));
  }
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, m_extensions.m_bWin32Surface));
#else
#  error "Vulkan platform not supported"
#endif
  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, m_extensions.m_bDebugUtils));

  return vk::Result::eSuccess;
}


vk::Result xiiGALDeviceVulkan::SelectDeviceExtensions(vk::DeviceCreateInfo& deviceCreateInfo, xiiHybridArray<const char*, 6>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  xiiUInt32 extensionCount;
  VK_SUCCEED_OR_RETURN_LOG(m_physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr));
  xiiDynamicArray<vk::ExtensionProperties> extensionProperties;
  extensionProperties.SetCount(extensionCount);
  VK_SUCCEED_OR_RETURN_LOG(m_physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, extensionProperties.GetData()));

  XII_LOG_BLOCK("DeviceExtensions");
  for (auto& ext : extensionProperties)
  {
    xiiLog::Info("{}", ext.extensionName.data());
  }

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> vk::Result {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const vk::ExtensionProperties& prop) { return xiiStringUtils::IsEqual(prop.extensionName.data(), extensionName); });
    if (it != end(extensionProperties))
    {
      extensions.PushBack(extensionName);
      enableFlag = true;
      return vk::Result::eSuccess;
    }
    enableFlag = false;
    xiiLog::Warning("Extension '{}' not supported", extensionName);
    return vk::Result::eErrorExtensionNotPresent;
  };

  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, m_extensions.m_bDeviceSwapChain));
  AddExtIfSupported(VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME, m_extensions.m_bShaderViewportIndexLayer);

  vk::PhysicalDeviceFeatures2 features;
  features.pNext = &m_extensions.m_borderColorEXT;
  m_physicalDevice.getFeatures2(&features);

  m_supportedStages = vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader;
  if (features.features.geometryShader)
  {
    m_supportedStages |= vk::PipelineStageFlagBits::eGeometryShader;
  }
  else
  {
    xiiLog::Warning("Geometry shaders are not supported.");
  }

  if (features.features.tessellationShader)
  {
    m_supportedStages |= vk::PipelineStageFlagBits::eTessellationControlShader | vk::PipelineStageFlagBits::eTessellationEvaluationShader;
  }
  else
  {
    xiiLog::Warning("Tessellation shaders are not supported.");
  }

  // Only use the extension if it allows us to not specify a format or we would need to create different samplers for every texture.
  if (m_extensions.m_borderColorEXT.customBorderColors && m_extensions.m_borderColorEXT.customBorderColorWithoutFormat)
  {
    AddExtIfSupported(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME, m_extensions.m_bBorderColorFloat);
    if (m_extensions.m_bBorderColorFloat)
    {
      deviceCreateInfo.pNext = &m_extensions.m_borderColorEXT;
    }
  }
  return vk::Result::eSuccess;
}

#define XII_GET_INSTANCE_PROC_ADDR(name) m_extensions.pfn_##name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(m_instance, #name));

xiiResult xiiGALDeviceVulkan::InitPlatform()
{
  XII_LOG_BLOCK("xiiGALDeviceVulkan::InitPlatform");

  const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
  {
    // Create instance
    // We use Vulkan 1.1 because of two features:
    // 1. Descriptor set pools return vk::Result::eErrorOutOfPoolMemory if exhaused. Removing the requirement to count usage yourself.
    // 2. Viewport height can be negative which performs y-inversion of the clip-space to framebuffer-space transform.
    vk::ApplicationInfo applicationInfo = {};
    applicationInfo.apiVersion          = VK_API_VERSION_1_1;
    applicationInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0); // TODO put xiiEngine version here
    applicationInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0); // TODO put xiiEngine version here
    applicationInfo.pApplicationName    = "xiiEngine";
    applicationInfo.pEngineName         = "xiiEngine";

    xiiHybridArray<const char*, 6> instanceExtensions;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(SelectInstanceExtensions(instanceExtensions));

    vk::InstanceCreateInfo instanceCreateInfo;
    // enabling support for win32 surfaces
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    instanceCreateInfo.enabledExtensionCount   = instanceExtensions.GetCount();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.GetData();

    instanceCreateInfo.enabledLayerCount = 0;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_Description.m_bDebugDevice)
    {
      debugCreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      debugCreateInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      debugCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      debugCreateInfo.pfnUserCallback = debugCallback;
      debugCreateInfo.pUserData       = nullptr;

      if (isInstanceLayerPresent(layers[0]))
      {
        instanceCreateInfo.enabledLayerCount   = XII_ARRAY_SIZE(layers);
        instanceCreateInfo.ppEnabledLayerNames = layers;
      }
      else
      {
        xiiLog::Warning("The khronos validation layer is not supported on this device. Will run without validation layer.");
      }
      instanceCreateInfo.pNext = &debugCreateInfo;
    }

    m_instance = vk::createInstance(instanceCreateInfo);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (m_Description.m_bDebugDevice)
    {
      XII_GET_INSTANCE_PROC_ADDR(vkCreateDebugUtilsMessengerEXT);
      XII_GET_INSTANCE_PROC_ADDR(vkDestroyDebugUtilsMessengerEXT);
      XII_GET_INSTANCE_PROC_ADDR(vkSetDebugUtilsObjectNameEXT);
      VK_SUCCEED_OR_RETURN_XII_FAILURE(m_extensions.pfn_vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger));
    }
#endif

    if (!m_instance)
    {
      xiiLog::Error("Failed to create Vulkan instance!");
      return XII_FAILURE;
    }
  }

  {
    // physical device
    xiiUInt32                             physicalDeviceCount = 0;
    xiiHybridArray<vk::PhysicalDevice, 2> physicalDevices;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_instance.enumeratePhysicalDevices(&physicalDeviceCount, nullptr));
    if (physicalDeviceCount == 0)
    {
      xiiLog::Error("No available physical device to create a Vulkan device on!");
      return XII_FAILURE;
    }

    physicalDevices.SetCount(physicalDeviceCount);
    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_instance.enumeratePhysicalDevices(&physicalDeviceCount, physicalDevices.GetData()));

    // TODO choosable physical device?
    // TODO making sure we have a hardware device?
    m_physicalDevice = physicalDevices[0];
    m_properties     = m_physicalDevice.getProperties();
    xiiLog::Info("Selected physical device \"{}\" for device creation.", m_properties.deviceName);
  }

  xiiHybridArray<vk::QueueFamilyProperties, 4> queueFamilyProperties;
  {
    // Device
    xiiUInt32 queueFamilyPropertyCount = 0;
    m_physicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, nullptr);
    if (queueFamilyPropertyCount == 0)
    {
      xiiLog::Error("No available device queues on physical device!");
      return XII_FAILURE;
    }
    queueFamilyProperties.SetCount(queueFamilyPropertyCount);
    m_physicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, queueFamilyProperties.GetData());

    {
      XII_LOG_BLOCK("Queue Families");
      for (xiiUInt32 i = 0; i < queueFamilyProperties.GetCount(); ++i)
      {
        const vk::QueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[i];
        xiiLog::Info("Queue count: {}, flags: {}", queueFamilyProperty.queueCount, vk::to_string(queueFamilyProperty.queueFlags).data());
      }
    }

    // Select best queue family for graphics and transfers.
    for (xiiUInt32 i = 0; i < queueFamilyProperties.GetCount(); ++i)
    {
      const vk::QueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[i];
      if (queueFamilyProperty.queueCount == 0)
        continue;
      constexpr auto graphicsFlags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;
      if ((queueFamilyProperty.queueFlags & graphicsFlags) == graphicsFlags)
      {
        m_graphicsQueue.m_uiQueueFamily = i;
      }
      if (queueFamilyProperty.queueFlags & vk::QueueFlagBits::eTransfer)
      {
        if (m_transferQueue.m_uiQueueFamily == -1)
        {
          m_transferQueue.m_uiQueueFamily = i;
        }
        else if ((queueFamilyProperty.queueFlags & graphicsFlags) == vk::QueueFlagBits())
        {
          // Prefer a queue that can't be used for graphics.
          m_transferQueue.m_uiQueueFamily = i;
        }
      }
    }
    if (m_graphicsQueue.m_uiQueueFamily == -1)
    {
      xiiLog::Error("No graphics queue found.");
      return XII_FAILURE;
    }
    if (m_transferQueue.m_uiQueueFamily == -1)
    {
      xiiLog::Error("No transfer queue found.");
      return XII_FAILURE;
    }

    constexpr float queuePriority = 0.f;

    xiiHybridArray<vk::DeviceQueueCreateInfo, 2> queues;

    vk::DeviceQueueCreateInfo& graphicsQueueCreateInfo = queues.ExpandAndGetRef();
    graphicsQueueCreateInfo.pQueuePriorities           = &queuePriority;
    graphicsQueueCreateInfo.queueCount                 = 1;
    graphicsQueueCreateInfo.queueFamilyIndex           = m_graphicsQueue.m_uiQueueFamily;
    if (m_graphicsQueue.m_uiQueueFamily != m_transferQueue.m_uiQueueFamily)
    {
      vk::DeviceQueueCreateInfo& transferQueueCreateInfo = queues.ExpandAndGetRef();
      transferQueueCreateInfo.pQueuePriorities           = &queuePriority;
      transferQueueCreateInfo.queueCount                 = 1;
      transferQueueCreateInfo.queueFamilyIndex           = m_transferQueue.m_uiQueueFamily;
    }

    //#TODO_VULKAN test that this returns the same as 'layers' passed into the instance.
    xiiUInt32 uiLayers;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_physicalDevice.enumerateDeviceLayerProperties(&uiLayers, nullptr));
    xiiDynamicArray<vk::LayerProperties> deviceLayers;
    deviceLayers.SetCount(uiLayers);
    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_physicalDevice.enumerateDeviceLayerProperties(&uiLayers, deviceLayers.GetData()));

    vk::DeviceCreateInfo           deviceCreateInfo = {};
    xiiHybridArray<const char*, 6> deviceExtensions;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(SelectDeviceExtensions(deviceCreateInfo, deviceExtensions));

    deviceCreateInfo.enabledExtensionCount   = deviceExtensions.GetCount();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.GetData();
    // Device layers are deprecated but provided (same as in instance) for backwards compatibility.
    deviceCreateInfo.enabledLayerCount   = XII_ARRAY_SIZE(layers);
    deviceCreateInfo.ppEnabledLayerNames = layers;

    vk::PhysicalDeviceFeatures physicalDeviceFeatures = m_physicalDevice.getFeatures();
    deviceCreateInfo.pEnabledFeatures                 = &physicalDeviceFeatures; // Enabling all available features for now
    deviceCreateInfo.queueCreateInfoCount             = queues.GetCount();
    deviceCreateInfo.pQueueCreateInfos                = queues.GetData();

    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_physicalDevice.createDevice(&deviceCreateInfo, nullptr, &m_device));
    m_device.getQueue(m_graphicsQueue.m_uiQueueFamily, m_graphicsQueue.m_uiQueueIndex, &m_graphicsQueue.m_queue);
    m_device.getQueue(m_transferQueue.m_uiQueueFamily, m_transferQueue.m_uiQueueIndex, &m_transferQueue.m_queue);
  }

  VK_SUCCEED_OR_RETURN_XII_FAILURE(xiiMemoryAllocatorVulkan::Initialize(m_physicalDevice, m_device, m_instance));

  vkSetDebugUtilsObjectNameEXTFunc   = (PFN_vkSetDebugUtilsObjectNameEXT)m_device.getProcAddr("vkSetDebugUtilsObjectNameEXT");
  vkQueueBeginDebugUtilsLabelEXTFunc = (PFN_vkQueueBeginDebugUtilsLabelEXT)m_device.getProcAddr("vkQueueBeginDebugUtilsLabelEXT");
  vkQueueEndDebugUtilsLabelEXTFunc   = (PFN_vkQueueEndDebugUtilsLabelEXT)m_device.getProcAddr("vkQueueEndDebugUtilsLabelEXT");
  vkCmdBeginDebugUtilsLabelEXTFunc   = (PFN_vkCmdBeginDebugUtilsLabelEXT)m_device.getProcAddr("vkCmdBeginDebugUtilsLabelEXT");
  vkCmdEndDebugUtilsLabelEXTFunc     = (PFN_vkCmdEndDebugUtilsLabelEXT)m_device.getProcAddr("vkCmdEndDebugUtilsLabelEXT");
  vkCmdInsertDebugUtilsLabelEXTFunc  = (PFN_vkCmdInsertDebugUtilsLabelEXT)m_device.getProcAddr("vkCmdInsertDebugUtilsLabelEXT");


  m_memoryProperties = m_physicalDevice.getMemoryProperties();

  // Fill lookup table
  FillFormatLookupTable();

  xiiClipSpaceDepthRange::Default = xiiClipSpaceDepthRange::ZeroToOne;
  // We use xiiClipSpaceYMode::Regular and rely in the Vulkan 1.1 feature that a negative height performs y-inversion of the clip-space to framebuffer-space transform.
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance1.html
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  m_pPipelineBarrier   = XII_NEW(&m_Allocator, xiiPipelineBarrierVulkan);
  m_pCommandBufferPool = XII_NEW(&m_Allocator, xiiCommandBufferPoolVulkan);
  m_pCommandBufferPool->Initialize(m_device, m_graphicsQueue.m_uiQueueFamily);
  m_pStagingBufferPool = XII_NEW(&m_Allocator, xiiStagingBufferPoolVulkan);
  m_pStagingBufferPool->Initialize(this);
  m_pQueryPool = XII_NEW(&m_Allocator, xiiQueryPoolVulkan);
  m_pQueryPool->Initialize(this, queueFamilyProperties[m_graphicsQueue.m_uiQueueFamily].timestampValidBits);
  m_pInitContext = XII_NEW(&m_Allocator, xiiInitContextVulkan, this);

  xiiSemaphorePoolVulkan::Initialize(m_device);
  xiiFencePoolVulkan::Initialize(m_device);
  xiiResourceCacheVulkan::Initialize(this, m_device);
  xiiDescriptorSetPoolVulkan::Initialize(m_device);
  xiiFallbackResourcesVulkan::Initialize(this);
  xiiImageCopyVulkan::Initialize(*this);

  m_pDefaultPass = XII_NEW(&m_Allocator, xiiGALPassVulkan, *this);

  xiiGALWindowSwapChain::SetFactoryMethod([this](const xiiGALWindowSwapChainCreationDescription& desc) -> xiiGALSwapChainHandle { return CreateSwapChain([this, &desc](xiiAllocatorBase* pAllocator) -> xiiGALSwapChain* { return XII_NEW(pAllocator, xiiGALSwapChainVulkan, desc); }); });

  return XII_SUCCESS;
}

void xiiGALDeviceVulkan::SetDebugName(const vk::DebugUtilsObjectNameInfoEXT& info, xiiVulkanAllocation allocation)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_device.setDebugUtilsObjectNameEXT(info);
  if (allocation)
    xiiMemoryAllocatorVulkan::SetAllocationUserData(allocation, info.pObjectName);
#endif
}

void xiiGALDeviceVulkan::ReportLiveGpuObjects()
{
  // This is automatically done in the validation layer and can't be easily done manually.
}

void xiiGALDeviceVulkan::UploadBufferStaging(xiiStagingBufferPoolVulkan* pStagingBufferPool, xiiPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const xiiGALBufferVulkan* pBuffer, xiiArrayPtr<const xiiUInt8> pInitialData, vk::DeviceSize dstOffset)
{
  void* pData = nullptr;

  //#TODO_VULKAN Use transfer queue
  xiiStagingBufferVulkan stagingBuffer = pStagingBufferPool->AllocateBuffer(0, pInitialData.GetCount());
  // xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(stagingBuffer.m_allocInfo.m_pMappedData), pInitialData.GetPtr(), pInitialData.GetCount());
  xiiMemoryAllocatorVulkan::MapMemory(stagingBuffer.m_alloc, &pData);
  xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(pData), pInitialData.GetPtr(), pInitialData.GetCount());
  xiiMemoryAllocatorVulkan::UnmapMemory(stagingBuffer.m_alloc);

  vk::BufferCopy region;
  region.srcOffset = 0;
  region.dstOffset = dstOffset;
  region.size      = pInitialData.GetCount();

  //#TODO_VULKAN atomic min size violation?
  commandBuffer.copyBuffer(stagingBuffer.m_buffer, pBuffer->GetVkBuffer(), 1, &region);

  pPipelineBarrier->AccessBuffer(pBuffer, region.dstOffset, region.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, pBuffer->GetUsedByPipelineStage(), pBuffer->GetAccessMask());

  //#TODO_VULKAN Custom delete later / return to xiiStagingBufferPoolVulkan once this is on the transfer queue and runs async to graphics queue.
  pStagingBufferPool->ReclaimBuffer(stagingBuffer);
}

void xiiGALDeviceVulkan::UploadTextureStaging(xiiStagingBufferPoolVulkan* pStagingBufferPool, xiiPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const xiiGALTextureVulkan* pTexture, const vk::ImageSubresourceLayers& subResource, const xiiGALSystemMemoryDescription& data)
{
  const vk::Offset3D imageOffset = {0, 0, 0};
  const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(subResource.mipLevel);

  auto getRange = [](const vk::ImageSubresourceLayers& layers) -> vk::ImageSubresourceRange {
    vk::ImageSubresourceRange range;
    range.aspectMask     = layers.aspectMask;
    range.baseMipLevel   = layers.mipLevel;
    range.levelCount     = 1;
    range.baseArrayLayer = layers.baseArrayLayer;
    range.layerCount     = layers.layerCount;
    return range;
  };

  pPipelineBarrier->EnsureImageLayout(pTexture, getRange(subResource), vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
  pPipelineBarrier->Flush();

  for (xiiUInt32 i = 0; i < subResource.layerCount; i++)
  {
    auto             pLayerData  = reinterpret_cast<const xiiUInt8*>(data.m_pData) + i * data.m_uiSlicePitch;
    const vk::Format format      = pTexture->GetImageFormat();
    const xiiUInt8   uiBlockSize = vk::blockSize(format);
    const auto       blockExtent = vk::blockExtent(format);
    const VkExtent3D blockCount  = {
      (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
      (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
      (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

    const vk::DeviceSize   uiTotalSize   = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
    xiiStagingBufferVulkan stagingBuffer = pStagingBufferPool->AllocateBuffer(0, uiTotalSize);

    const xiiUInt32 uiBufferRowPitch   = uiBlockSize * blockCount.width;
    const xiiUInt32 uiBufferSlicePitch = uiBufferRowPitch * blockCount.height;
    XII_ASSERT_DEV(uiBufferRowPitch == data.m_uiRowPitch, "Row pitch with padding is not implemented yet.");
    XII_ASSERT_DEV(uiBufferSlicePitch == data.m_uiSlicePitch, "Row pitch with padding is not implemented yet.");

    void* pData = nullptr;
    xiiMemoryAllocatorVulkan::MapMemory(stagingBuffer.m_alloc, &pData);
    xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(pData), pLayerData, uiTotalSize);
    xiiMemoryAllocatorVulkan::UnmapMemory(stagingBuffer.m_alloc);

    vk::BufferImageCopy region = {};
    region.imageSubresource    = subResource;
    region.imageOffset         = imageOffset;
    region.imageExtent         = imageExtent;

    region.bufferOffset      = 0;
    region.bufferRowLength   = blockExtent[0] * uiBufferRowPitch / uiBlockSize;
    region.bufferImageHeight = blockExtent[1] * uiBufferSlicePitch / uiBufferRowPitch;

    //#TODO_VULKAN atomic min size violation?
    commandBuffer.copyBufferToImage(stagingBuffer.m_buffer, pTexture->GetImage(), pTexture->GetPreferredLayout(vk::ImageLayout::eTransferDstOptimal), 1, &region);
    pStagingBufferPool->ReclaimBuffer(stagingBuffer);
  }

  pPipelineBarrier->EnsureImageLayout(pTexture, getRange(subResource), pTexture->GetPreferredLayout(), pTexture->GetUsedByPipelineStage(), pTexture->GetAccessMask());
}

xiiResult xiiGALDeviceVulkan::ShutdownPlatform()
{
  xiiImageCopyVulkan::DeInitialize(*this);
  DestroyDeadObjects(); // xiiImageCopyVulkan might add dead objects, so make sure the list is cleared again

  xiiFallbackResourcesVulkan::DeInitialize();

  xiiGALWindowSwapChain::SetFactoryMethod({});
  if (m_lastCommandBufferFinished)
    ReclaimLater(m_lastCommandBufferFinished, m_pCommandBufferPool.Borrow());
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  // We couldn't create a device in the first place, so early out of shutdown
  if (!m_device)
  {
    return XII_SUCCESS;
  }

  WaitIdlePlatform();

  m_pDefaultPass     = nullptr;
  m_pPipelineBarrier = nullptr;
  m_pCommandBufferPool->DeInitialize();
  m_pCommandBufferPool = nullptr;
  m_pStagingBufferPool->DeInitialize();
  m_pStagingBufferPool = nullptr;
  m_pQueryPool->DeInitialize();
  m_pQueryPool   = nullptr;
  m_pInitContext = nullptr;

  xiiSemaphorePoolVulkan::DeInitialize();
  xiiFencePoolVulkan::DeInitialize();
  xiiResourceCacheVulkan::DeInitialize();
  xiiDescriptorSetPoolVulkan::DeInitialize();

  xiiMemoryAllocatorVulkan::DeInitialize();

  m_device.waitIdle();
  m_device.destroy();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (m_extensions.pfn_vkDestroyDebugUtilsMessengerEXT != nullptr)
  {
    m_extensions.pfn_vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
  }
#endif

  m_instance.destroy();
  ReportLiveGpuObjects();

  return XII_SUCCESS;
}

// Pipeline & Pass functions

vk::CommandBuffer& xiiGALDeviceVulkan::GetCurrentCommandBuffer()
{
  vk::CommandBuffer& commandBuffer = m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;
  if (!commandBuffer)
  {
    // Restart new command buffer if none is active already.
    commandBuffer = m_pCommandBufferPool->RequestCommandBuffer();
    vk::CommandBufferBeginInfo beginInfo;
    VK_ASSERT_DEBUG(commandBuffer.begin(&beginInfo));
    GetCurrentPipelineBarrier().SetCommandBuffer(&commandBuffer);

    m_pDefaultPass->SetCurrentCommandBuffer(&commandBuffer, m_pPipelineBarrier.Borrow());
    // We can't carry state across individual command buffers.
    m_pDefaultPass->MarkDirty();
  }
  return commandBuffer;
}

xiiPipelineBarrierVulkan& xiiGALDeviceVulkan::GetCurrentPipelineBarrier()
{
  vk::CommandBuffer& commandBuffer = m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;
  if (!commandBuffer)
  {
    GetCurrentCommandBuffer();
  }
  return *m_pPipelineBarrier.Borrow();
}

xiiQueryPoolVulkan& xiiGALDeviceVulkan::GetQueryPool() const
{
  return *m_pQueryPool.Borrow();
}

xiiStagingBufferPoolVulkan& xiiGALDeviceVulkan::GetStagingBufferPool() const
{
  return *m_pStagingBufferPool.Borrow();
}

xiiInitContextVulkan& xiiGALDeviceVulkan::GetInitContext() const
{
  return *m_pInitContext.Borrow();
}

xiiProxyAllocator& xiiGALDeviceVulkan::GetAllocator()
{
  return m_Allocator;
}

xiiGALTextureHandle xiiGALDeviceVulkan::CreateTextureInternal(const xiiGALTextureCreationDescription& Description, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData, vk::Format OverrideFormat, bool bStaging)
{
  xiiGALTextureVulkan* pTexture = XII_NEW(&m_Allocator, xiiGALTextureVulkan, Description, OverrideFormat, bStaging);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    XII_DELETE(&m_Allocator, pTexture);
    return xiiGALTextureHandle();
  }

  return FinalizeTextureInternal(Description, pTexture);
}

xiiGALBufferHandle xiiGALDeviceVulkan::CreateBufferInternal(const xiiGALBufferCreationDescription& Description, xiiArrayPtr<const xiiUInt8> pInitialData, bool bCPU)
{
  xiiGALBufferVulkan* pBuffer = XII_NEW(&m_Allocator, xiiGALBufferVulkan, Description, bCPU);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    XII_DELETE(&m_Allocator, pBuffer);
    return xiiGALBufferHandle();
  }

  return FinalizeBufferInternal(Description, pBuffer);
}

void xiiGALDeviceVulkan::BeginPipelinePlatform(const char* szName, xiiGALSwapChain* pSwapChain)
{
  XII_PROFILE_SCOPE("BeginPipelinePlatform");

  GetCurrentCommandBuffer();
#if XII_ENABLED(XII_USE_PROFILING)
  m_pPipelineTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  if (pSwapChain)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void xiiGALDeviceVulkan::EndPipelinePlatform(xiiGALSwapChain* pSwapChain)
{
  XII_PROFILE_SCOPE("EndPipelinePlatform");

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPipelineTimingScope);
#endif
  if (pSwapChain)
  {
    pSwapChain->PresentRenderTarget(this);
  }

  // Render context is reset on every end pipeline so it will re-submit all state change for the next render pass. Thus it is safe at this point to do a full reset.
  // Technically don't have to reset here, MarkDirty would also be fine but we do need to do a Reset at the end of the frame as pointers held by the xiiGALCommandEncoderImplVulkan may not be valid in the next frame.
  m_pDefaultPass->Reset();
}

vk::Fence xiiGALDeviceVulkan::Submit(vk::Semaphore waitSemaphore, vk::PipelineStageFlags waitStage, vk::Semaphore signalSemaphore)
{
  m_pDefaultPass->SetCurrentCommandBuffer(nullptr, nullptr);

  vk::CommandBuffer initCommandBuffer = m_pInitContext->GetFinishedCommandBuffer();
  bool              bHasCmdBuffer     = initCommandBuffer || m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;

  xiiHybridArray<vk::CommandBuffer, 2> buffers;
  vk::SubmitInfo                       submitInfo = {};
  if (bHasCmdBuffer)
  {
    vk::CommandBuffer mainCommandBuffer = m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;
    if (initCommandBuffer)
    {
      // Any background loading that happened up to this point needs to be submitted first.
      // The main render command buffer assumes that all new resources are in their default state which is made sure by subitting this command buffer.
      buffers.PushBack(initCommandBuffer);
    }
    if (mainCommandBuffer)
    {
      GetCurrentPipelineBarrier().Submit();
      mainCommandBuffer.end();
      buffers.PushBack(mainCommandBuffer);
    }
    submitInfo.commandBufferCount = buffers.GetCount();
    submitInfo.pCommandBuffers    = buffers.GetData();
  }

  vk::Fence renderFence = xiiFencePoolVulkan::RequestFence();

  xiiHybridArray<vk::Semaphore, 2>          waitSemaphores;
  xiiHybridArray<vk::PipelineStageFlags, 2> waitStages;
  xiiHybridArray<vk::Semaphore, 2>          signalSemaphores;

  if (m_lastCommandBufferFinished)
  {
    waitSemaphores.PushBack(m_lastCommandBufferFinished);
    waitStages.PushBack(vk::PipelineStageFlagBits::eAllCommands);
    ReclaimLater(m_lastCommandBufferFinished);
  }

  if (waitSemaphore)
  {
    waitSemaphores.PushBack(waitSemaphore);
    waitStages.PushBack(waitStage);
  }

  if (signalSemaphore)
  {
    signalSemaphores.PushBack(signalSemaphore);
  }
  m_lastCommandBufferFinished = xiiSemaphorePoolVulkan::RequestSemaphore();
  signalSemaphores.PushBack(m_lastCommandBufferFinished);

  submitInfo.waitSemaphoreCount   = waitSemaphores.GetCount();
  submitInfo.pWaitSemaphores      = waitSemaphores.GetData();
  submitInfo.pWaitDstStageMask    = waitStages.GetData();
  submitInfo.signalSemaphoreCount = signalSemaphores.GetCount();
  submitInfo.pSignalSemaphores    = signalSemaphores.GetData();

  {
    m_PerFrameData[m_uiCurrentPerFrameData].m_CommandBufferFences.PushBack(renderFence);
    m_graphicsQueue.m_queue.submit(1, &submitInfo, renderFence);
  }

  auto res = renderFence;
  ReclaimLater(renderFence);
  ReclaimLater(m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer, m_pCommandBufferPool.Borrow());

  return res;
}

xiiGALPass* xiiGALDeviceVulkan::BeginPassPlatform(const char* szName)
{
  GetCurrentCommandBuffer();
#if XII_ENABLED(XII_USE_PROFILING)
  m_pPassTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif
  return m_pDefaultPass.Borrow();
}

void xiiGALDeviceVulkan::EndPassPlatform(xiiGALPass* pPass)
{
#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPassTimingScope);
#endif
}

// State creation functions

xiiGALBlendState* xiiGALDeviceVulkan::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& Description)
{
  xiiGALBlendStateVulkan* pState = XII_NEW(&m_Allocator, xiiGALBlendStateVulkan, Description);

  if (pState->InitPlatform(this).Succeeded())
  {
    return pState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pState);
    return nullptr;
  }
}

void xiiGALDeviceVulkan::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
  xiiGALBlendStateVulkan* pState = static_cast<xiiGALBlendStateVulkan*>(pBlendState);
  xiiResourceCacheVulkan::ResourceDeleted(pState);
  pState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pState);
}

xiiGALDepthStencilState* xiiGALDeviceVulkan::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& Description)
{
  xiiGALDepthStencilStateVulkan* pVulkanDepthStencilState = XII_NEW(&m_Allocator, xiiGALDepthStencilStateVulkan, Description);

  if (pVulkanDepthStencilState->InitPlatform(this).Succeeded())
  {
    return pVulkanDepthStencilState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pVulkanDepthStencilState);
    return nullptr;
  }
}

void xiiGALDeviceVulkan::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
  xiiGALDepthStencilStateVulkan* pVulkanDepthStencilState = static_cast<xiiGALDepthStencilStateVulkan*>(pDepthStencilState);
  xiiResourceCacheVulkan::ResourceDeleted(pVulkanDepthStencilState);
  pVulkanDepthStencilState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVulkanDepthStencilState);
}

xiiGALRasterizerState* xiiGALDeviceVulkan::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& Description)
{
  xiiGALRasterizerStateVulkan* pVulkanRasterizerState = XII_NEW(&m_Allocator, xiiGALRasterizerStateVulkan, Description);

  if (pVulkanRasterizerState->InitPlatform(this).Succeeded())
  {
    return pVulkanRasterizerState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pVulkanRasterizerState);
    return nullptr;
  }
}

void xiiGALDeviceVulkan::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  xiiGALRasterizerStateVulkan* pVulkanRasterizerState = static_cast<xiiGALRasterizerStateVulkan*>(pRasterizerState);
  xiiResourceCacheVulkan::ResourceDeleted(pVulkanRasterizerState);
  pVulkanRasterizerState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVulkanRasterizerState);
}

xiiGALSamplerState* xiiGALDeviceVulkan::CreateSamplerStatePlatform(const xiiGALSamplerStateCreationDescription& Description)
{
  xiiGALSamplerStateVulkan* pVulkanSamplerState = XII_NEW(&m_Allocator, xiiGALSamplerStateVulkan, Description);

  if (pVulkanSamplerState->InitPlatform(this).Succeeded())
  {
    return pVulkanSamplerState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pVulkanSamplerState);
    return nullptr;
  }
}

void xiiGALDeviceVulkan::DestroySamplerStatePlatform(xiiGALSamplerState* pSamplerState)
{
  xiiGALSamplerStateVulkan* pVulkanSamplerState = static_cast<xiiGALSamplerStateVulkan*>(pSamplerState);
  pVulkanSamplerState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVulkanSamplerState);
}


// Resource creation functions

xiiGALShader* xiiGALDeviceVulkan::CreateShaderPlatform(const xiiGALShaderCreationDescription& Description)
{
  xiiGALShaderVulkan* pShader = XII_NEW(&m_Allocator, xiiGALShaderVulkan, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void xiiGALDeviceVulkan::DestroyShaderPlatform(xiiGALShader* pShader)
{
  xiiGALShaderVulkan* pVulkanShader = static_cast<xiiGALShaderVulkan*>(pShader);
  xiiResourceCacheVulkan::ShaderDeleted(pVulkanShader);
  pVulkanShader->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVulkanShader);
}

xiiGALBuffer* xiiGALDeviceVulkan::CreateBufferPlatform(
  const xiiGALBufferCreationDescription& Description,
  xiiArrayPtr<const xiiUInt8>            pInitialData)
{
  xiiGALBufferVulkan* pBuffer = XII_NEW(&m_Allocator, xiiGALBufferVulkan, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    XII_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void xiiGALDeviceVulkan::DestroyBufferPlatform(xiiGALBuffer* pBuffer)
{
  xiiGALBufferVulkan* pVulkanBuffer = static_cast<xiiGALBufferVulkan*>(pBuffer);
  GetCurrentPipelineBarrier().BufferDestroyed(pVulkanBuffer);
  pVulkanBuffer->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVulkanBuffer);
}

xiiGALTexture* xiiGALDeviceVulkan::CreateTexturePlatform(
  const xiiGALTextureCreationDescription&    Description,
  xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData)
{
  xiiGALTextureVulkan* pTexture = XII_NEW(&m_Allocator, xiiGALTextureVulkan, Description);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    XII_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}


void xiiGALDeviceVulkan::DestroyTexturePlatform(xiiGALTexture* pTexture)
{
  xiiGALTextureVulkan* pVulkanTexture = static_cast<xiiGALTextureVulkan*>(pTexture);
  GetCurrentPipelineBarrier().TextureDestroyed(pVulkanTexture);
  m_pInitContext->TextureDestroyed(pVulkanTexture);

  pVulkanTexture->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVulkanTexture);
}

xiiGALResourceView* xiiGALDeviceVulkan::CreateResourceViewPlatform(
  xiiGALResourceBase*                          pResource,
  const xiiGALResourceViewCreationDescription& Description)
{
  xiiGALResourceViewVulkan* pResourceView = XII_NEW(&m_Allocator, xiiGALResourceViewVulkan, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void xiiGALDeviceVulkan::DestroyResourceViewPlatform(xiiGALResourceView* pResourceView)
{
  xiiGALResourceViewVulkan* pVulkanResourceView = static_cast<xiiGALResourceViewVulkan*>(pResourceView);
  pVulkanResourceView->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVulkanResourceView);
}

xiiGALRenderTargetView* xiiGALDeviceVulkan::CreateRenderTargetViewPlatform(
  xiiGALTexture*                                   pTexture,
  const xiiGALRenderTargetViewCreationDescription& Description)
{
  xiiGALRenderTargetViewVulkan* pRTView = XII_NEW(&m_Allocator, xiiGALRenderTargetViewVulkan, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void xiiGALDeviceVulkan::DestroyRenderTargetViewPlatform(xiiGALRenderTargetView* pRenderTargetView)
{
  xiiGALRenderTargetViewVulkan* pVulkanRenderTargetView = static_cast<xiiGALRenderTargetViewVulkan*>(pRenderTargetView);
  pVulkanRenderTargetView->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVulkanRenderTargetView);
}

xiiGALUnorderedAccessView* xiiGALDeviceVulkan::CreateUnorderedAccessViewPlatform(
  xiiGALResourceBase*                                 pTextureOfBuffer,
  const xiiGALUnorderedAccessViewCreationDescription& Description)
{
  xiiGALUnorderedAccessViewVulkan* pUnorderedAccessView = XII_NEW(&m_Allocator, xiiGALUnorderedAccessViewVulkan, pTextureOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void xiiGALDeviceVulkan::DestroyUnorderedAccessViewPlatform(xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  xiiGALUnorderedAccessViewVulkan* pUnorderedAccessViewVulkan = static_cast<xiiGALUnorderedAccessViewVulkan*>(pUnorderedAccessView);
  pUnorderedAccessViewVulkan->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pUnorderedAccessViewVulkan);
}



// Other rendering creation functions
xiiGALQuery* xiiGALDeviceVulkan::CreateQueryPlatform(const xiiGALQueryCreationDescription& Description)
{
  xiiGALQueryVulkan* pQuery = XII_NEW(&m_Allocator, xiiGALQueryVulkan, Description);

  if (!pQuery->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pQuery);
    return nullptr;
  }

  return pQuery;
}

void xiiGALDeviceVulkan::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
  xiiGALQueryVulkan* pQueryVulkan = static_cast<xiiGALQueryVulkan*>(pQuery);
  pQueryVulkan->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pQueryVulkan);
}

xiiGALVertexDeclaration* xiiGALDeviceVulkan::CreateVertexDeclarationPlatform(const xiiGALVertexDeclarationCreationDescription& Description)
{
  xiiGALVertexDeclarationVulkan* pVertexDeclaration = XII_NEW(&m_Allocator, xiiGALVertexDeclarationVulkan, Description);

  if (pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    XII_DELETE(&m_Allocator, pVertexDeclaration);
    return nullptr;
  }
}

void xiiGALDeviceVulkan::DestroyVertexDeclarationPlatform(xiiGALVertexDeclaration* pVertexDeclaration)
{
  xiiGALVertexDeclarationVulkan* pVertexDeclarationVulkan = static_cast<xiiGALVertexDeclarationVulkan*>(pVertexDeclaration);
  xiiResourceCacheVulkan::ResourceDeleted(pVertexDeclarationVulkan);
  pVertexDeclarationVulkan->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVertexDeclarationVulkan);
}

xiiGALTimestampHandle xiiGALDeviceVulkan::GetTimestampPlatform()
{
  return m_pQueryPool->GetTimestamp();
}

xiiResult xiiGALDeviceVulkan::GetTimestampResultPlatform(xiiGALTimestampHandle hTimestamp, xiiTime& result)
{
  return m_pQueryPool->GetTimestampResult(hTimestamp, result);
}

// Misc functions

void xiiGALDeviceVulkan::BeginFramePlatform(const xiiUInt64 uiRenderFrame)
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  // check if fence is reached
  if (m_PerFrameData[m_uiCurrentPerFrameData].m_uiFrame != ((xiiUInt64)-1))
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    for (vk::Fence fence : perFrameData.m_CommandBufferFences)
    {
      vk::Result fenceStatus = m_device.getFenceStatus(fence);
      if (fenceStatus == vk::Result::eNotReady)
      {
        m_device.waitForFences(1, &fence, true, 1000000000);
      }
    }
    perFrameData.m_CommandBufferFences.Clear();

    {
      XII_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletionsMutex);
      DeletePendingResources(m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletionsPrevious);
    }
    {
      XII_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResourcesMutex);
      ReclaimResources(m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResourcesPrevious);
    }
    m_uiSafeFrame = m_PerFrameData[m_uiCurrentPerFrameData].m_uiFrame;
  }
  {
    auto& perFrameData                = m_PerFrameData[m_uiNextPerFrameData];
    perFrameData.m_fInvTicksPerSecond = -1.0f;
  }

  m_PerFrameData[m_uiCurrentPerFrameData].m_uiFrame = m_uiFrameCounter;

  m_pQueryPool->BeginFrame(GetCurrentCommandBuffer());
  GetCurrentCommandBuffer();

#if XII_ENABLED(XII_USE_PROFILING)
  xiiStringBuilder sb;
  sb.Format("Frame {}", uiRenderFrame);
  m_pFrameTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), sb);
#endif
}

void xiiGALDeviceVulkan::EndFramePlatform()
{
#if XII_ENABLED(XII_USE_PROFILING)
  {
    //#TODO_VULKAN This is very wasteful, in normal cases the last endPipeline will have submitted the command buffer via the swapchain. Thus, we start and submit a command buffer here with only the timestamp in it.
    GetCurrentCommandBuffer();
    xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pFrameTimingScope);
  }
#endif

  if (m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer)
  {
    Submit({}, {}, {});
  }

  {
    // Resources can be added to deletion / reclaim outside of the render frame. These will not be covered by the fences. To handle this, we swap the resources arrays so for any newly added resources we know they are not part of the batch that is deleted / reclaimed with the frame.
    auto& currentFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    {
      XII_LOCK(currentFrameData.m_pendingDeletionsMutex);
      currentFrameData.m_pendingDeletionsPrevious.Swap(currentFrameData.m_pendingDeletions);
    }
    {
      XII_LOCK(currentFrameData.m_reclaimResourcesMutex);
      currentFrameData.m_reclaimResourcesPrevious.Swap(currentFrameData.m_reclaimResources);
    }
  }
  m_uiCurrentPerFrameData = (m_uiCurrentPerFrameData + 1) % XII_ARRAY_SIZE(m_PerFrameData);
  m_uiNextPerFrameData    = (m_uiCurrentPerFrameData + 1) % XII_ARRAY_SIZE(m_PerFrameData);
  ++m_uiFrameCounter;
}

void xiiGALDeviceVulkan::FillCapabilitiesPlatform()
{
  vk::PhysicalDeviceMemoryProperties memProperties = m_physicalDevice.getMemoryProperties();
  vk::PhysicalDeviceFeatures         features      = m_physicalDevice.getFeatures();

  xiiUInt64 dedicatedMemory = 0;
  xiiUInt64 systemMemory    = 0;
  for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i)
  {
    if (memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
    {
      dedicatedMemory += memProperties.memoryHeaps[i].size;
    }
    else
    {
      systemMemory += memProperties.memoryHeaps[i].size;
    }
  }

  {
    m_Capabilities.m_sAdapterName         = xiiStringUtf8(m_properties.deviceName).GetData();
    m_Capabilities.m_uiDedicatedVRAM      = static_cast<xiiUInt64>(dedicatedMemory);
    m_Capabilities.m_uiDedicatedSystemRAM = static_cast<xiiUInt64>(systemMemory);
    m_Capabilities.m_uiSharedSystemRAM    = static_cast<xiiUInt64>(0); // TODO
    m_Capabilities.m_bHardwareAccelerated = m_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
  }

  m_Capabilities.m_bMultithreadedResourceCreation = true;

  m_Capabilities.m_bB5G6R5Textures          = true; // TODO how to check
  m_Capabilities.m_bNoOverwriteBufferUpdate = true; // TODO how to check

  m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::VertexShader]   = true;
  m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::HullShader]     = features.tessellationShader;
  m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::DomainShader]   = features.tessellationShader;
  m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::GeometryShader] = features.geometryShader;
  m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::PixelShader]    = true;
  m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::ComputeShader]  = true; // we check this when creating the queue, always has to be supported
  m_Capabilities.m_bInstancing                                              = true;
  m_Capabilities.m_b32BitIndices                                            = true;
  m_Capabilities.m_bIndirectDraw                                            = true;
  m_Capabilities.m_bStreamOut                                               = true;
  m_Capabilities.m_uiMaxConstantBuffers                                     = m_properties.limits.maxDescriptorSetUniformBuffers;
  m_Capabilities.m_bTextureArrays                                           = true;
  m_Capabilities.m_bCubemapArrays                                           = true;
  m_Capabilities.m_uiMaxTextureDimension                                    = m_properties.limits.maxImageDimension1D;
  m_Capabilities.m_uiMaxCubemapDimension                                    = m_properties.limits.maxImageDimensionCube;
  m_Capabilities.m_uiMax3DTextureDimension                                  = m_properties.limits.maxImageDimension3D;
  m_Capabilities.m_uiMaxAnisotropy                                          = static_cast<xiiUInt16>(m_properties.limits.maxSamplerAnisotropy);
  m_Capabilities.m_uiMaxRendertargets                                       = m_properties.limits.maxColorAttachments;
  m_Capabilities.m_uiUAVCount                                               = xiiMath::Min(m_properties.limits.maxDescriptorSetStorageBuffers, m_properties.limits.maxDescriptorSetStorageImages);
  m_Capabilities.m_bAlphaToCoverage                                         = true;
  m_Capabilities.m_bVertexShaderRenderTargetArrayIndex                      = m_extensions.m_bShaderViewportIndexLayer;

  m_Capabilities.m_bConservativeRasterization = false; // need to query for VK_EXT_CONSERVATIVE_RASTERIZATION
}

void xiiGALDeviceVulkan::WaitIdlePlatform()
{
  m_device.waitIdle();
  DestroyDeadObjects();
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    // First, we wait for all fences for all submit calls. This is necessary to make sure no resources of the frame are still in use by the GPU.
    auto& perFrameData = m_PerFrameData[i];
    for (vk::Fence fence : perFrameData.m_CommandBufferFences)
    {
      vk::Result fenceStatus = m_device.getFenceStatus(fence);
      if (fenceStatus == vk::Result::eNotReady)
      {
        m_device.waitForFences(1, &fence, true, 1000000000);
      }
    }
    perFrameData.m_CommandBufferFences.Clear();
  }

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    {
      XII_LOCK(m_PerFrameData[i].m_pendingDeletionsMutex);
      DeletePendingResources(m_PerFrameData[i].m_pendingDeletionsPrevious);
      DeletePendingResources(m_PerFrameData[i].m_pendingDeletions);
    }
    {
      XII_LOCK(m_PerFrameData[i].m_reclaimResourcesMutex);
      ReclaimResources(m_PerFrameData[i].m_reclaimResourcesPrevious);
      ReclaimResources(m_PerFrameData[i].m_reclaimResources);
    }
  }
}

vk::PipelineStageFlags xiiGALDeviceVulkan::GetSupportedStages() const
{
  return m_supportedStages;
}

xiiInt32 xiiGALDeviceVulkan::GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const
{

  for (xiiUInt32 i = 0; i < m_memoryProperties.memoryTypeCount; ++i)
  {
    const vk::MemoryType& type = m_memoryProperties.memoryTypes[i];
    if (requirements.memoryTypeBits & (1 << i) && (type.propertyFlags & properties))
    {
      return i;
    }
  }

  return -1;
}

void xiiGALDeviceVulkan::DeleteLater(const PendingDeletion& deletion)
{
  XII_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletionsMutex);
  m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletions.PushBack(deletion);
}

void xiiGALDeviceVulkan::ReclaimLater(const ReclaimResource& reclaim)
{
  XII_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResourcesMutex);
  m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResources.PushBack(reclaim);
}

void xiiGALDeviceVulkan::DeletePendingResources(xiiDeque<PendingDeletion>& pendingDeletions)
{
  for (PendingDeletion& deletion : pendingDeletions)
  {
    switch (deletion.m_type)
    {
      case vk::ObjectType::eImageView:
        m_device.destroyImageView(reinterpret_cast<vk::ImageView&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eImage:
      {
        auto& image = reinterpret_cast<vk::Image&>(deletion.m_pObject);
        OnBeforeImageDestroyed.Broadcast(OnBeforeImageDestroyedData{image, *this});
        xiiMemoryAllocatorVulkan::DestroyImage(image, deletion.m_allocation);
      }
      break;
      case vk::ObjectType::eBuffer:
        xiiMemoryAllocatorVulkan::DestroyBuffer(reinterpret_cast<vk::Buffer&>(deletion.m_pObject), deletion.m_allocation);
        break;
      case vk::ObjectType::eBufferView:
        m_device.destroyBufferView(reinterpret_cast<vk::BufferView&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eFramebuffer:
        m_device.destroyFramebuffer(reinterpret_cast<vk::Framebuffer&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eRenderPass:
        m_device.destroyRenderPass(reinterpret_cast<vk::RenderPass&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSampler:
        m_device.destroySampler(reinterpret_cast<vk::Sampler&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSwapchainKHR:
        m_device.destroySwapchainKHR(reinterpret_cast<vk::SwapchainKHR&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSurfaceKHR:
        m_instance.destroySurfaceKHR(reinterpret_cast<vk::SurfaceKHR&>(deletion.m_pObject));
        if (xiiWindowBase* pWindow = reinterpret_cast<xiiWindowBase*>(deletion.m_pContext))
        {
          pWindow->RemoveReference();
        }
        break;
      case vk::ObjectType::eShaderModule:
        m_device.destroyShaderModule(reinterpret_cast<vk::ShaderModule&>(deletion.m_pObject));
        break;
      case vk::ObjectType::ePipeline:
        m_device.destroyPipeline(reinterpret_cast<vk::Pipeline&>(deletion.m_pObject));
        break;
      default:
        XII_REPORT_FAILURE("This object type is not implemented");
        break;
    }
  }
  pendingDeletions.Clear();
}

void xiiGALDeviceVulkan::ReclaimResources(xiiDeque<ReclaimResource>& resources)
{
  for (ReclaimResource& resource : resources)
  {
    switch (resource.m_type)
    {
      case vk::ObjectType::eSemaphore:
        xiiSemaphorePoolVulkan::ReclaimSemaphore(reinterpret_cast<vk::Semaphore&>(resource.m_pObject));
        break;
      case vk::ObjectType::eFence:
        xiiFencePoolVulkan::ReclaimFence(reinterpret_cast<vk::Fence&>(resource.m_pObject));
        break;
      case vk::ObjectType::eCommandBuffer:
        static_cast<xiiCommandBufferPoolVulkan*>(resource.m_pContext)->ReclaimCommandBuffer(reinterpret_cast<vk::CommandBuffer&>(resource.m_pObject));
        break;
      case vk::ObjectType::eDescriptorPool:
        xiiDescriptorSetPoolVulkan::ReclaimPool(reinterpret_cast<vk::DescriptorPool&>(resource.m_pObject));
        break;
      default:
        XII_REPORT_FAILURE("This object type is not implemented");
        break;
    }
  }
  resources.Clear();
}

void xiiGALDeviceVulkan::FillFormatLookupTable()
{
  ///       The list below is in the same order as the xiiGALResourceFormat enum. No format should be missing except the ones that are just
  ///       different names for the same enum value.

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAFloat, xiiGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Sfloat).RT(vk::Format::eR32G32B32A32Sfloat).VA(vk::Format::eR32G32B32A32Sfloat).RV(vk::Format::eR32G32B32A32Sfloat));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUInt, xiiGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Uint).RT(vk::Format::eR32G32B32A32Uint).VA(vk::Format::eR32G32B32A32Uint).RV(vk::Format::eR32G32B32A32Uint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAInt, xiiGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Sint).RT(vk::Format::eR32G32B32A32Sint).VA(vk::Format::eR32G32B32A32Sint).RV(vk::Format::eR32G32B32A32Sint));

  // TODO 3-channel formats are not really supported under vulkan judging by experience
  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBFloat, xiiGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Sfloat).RT(vk::Format::eR32G32B32Sfloat).VA(vk::Format::eR32G32B32Sfloat).RV(vk::Format::eR32G32B32Sfloat));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBUInt, xiiGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Uint).RT(vk::Format::eR32G32B32Uint).VA(vk::Format::eR32G32B32Uint).RV(vk::Format::eR32G32B32Uint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBInt, xiiGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Sint).RT(vk::Format::eR32G32B32Sint).VA(vk::Format::eR32G32B32Sint).RV(vk::Format::eR32G32B32Sint));

  // TODO dunno if these are actually supported for the respective Vulkan device
  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::B5G6R5UNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eR5G6B5UnormPack16).RT(vk::Format::eR5G6B5UnormPack16).VA(vk::Format::eR5G6B5UnormPack16).RV(vk::Format::eR5G6B5UnormPack16));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BGRAUByteNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eB8G8R8A8Unorm).RT(vk::Format::eB8G8R8A8Unorm).VA(vk::Format::eB8G8R8A8Unorm).RV(vk::Format::eB8G8R8A8Unorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BGRAUByteNormalizedsRGB,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eB8G8R8A8Srgb).RT(vk::Format::eB8G8R8A8Srgb).RV(vk::Format::eB8G8R8A8Srgb));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAHalf, xiiGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Sfloat).RT(vk::Format::eR16G16B16A16Sfloat).VA(vk::Format::eR16G16B16A16Sfloat).RV(vk::Format::eR16G16B16A16Sfloat));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUShort, xiiGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Uint).RT(vk::Format::eR16G16B16A16Uint).VA(vk::Format::eR16G16B16A16Uint).RV(vk::Format::eR16G16B16A16Uint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUShortNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Unorm).RT(vk::Format::eR16G16B16A16Unorm).VA(vk::Format::eR16G16B16A16Unorm).RV(vk::Format::eR16G16B16A16Unorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAShort, xiiGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Sint).RT(vk::Format::eR16G16B16A16Sint).VA(vk::Format::eR16G16B16A16Sint).RV(vk::Format::eR16G16B16A16Sint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAShortNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Snorm).RT(vk::Format::eR16G16B16A16Snorm).VA(vk::Format::eR16G16B16A16Snorm).RV(vk::Format::eR16G16B16A16Snorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGFloat, xiiGALFormatLookupEntryVulkan(vk::Format::eR32G32Sfloat).RT(vk::Format::eR32G32Sfloat).VA(vk::Format::eR32G32Sfloat).RV(vk::Format::eR32G32Sfloat));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUInt, xiiGALFormatLookupEntryVulkan(vk::Format::eR32G32Uint).RT(vk::Format::eR32G32Uint).VA(vk::Format::eR32G32Uint).RV(vk::Format::eR32G32Uint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGInt, xiiGALFormatLookupEntryVulkan(vk::Format::eR32G32Sint).RT(vk::Format::eR32G32Sint).VA(vk::Format::eR32G32Sint).RV(vk::Format::eR32G32Sint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGB10A2UInt, xiiGALFormatLookupEntryVulkan(vk::Format::eA2B10G10R10UintPack32).RT(vk::Format::eA2B10G10R10UintPack32).VA(vk::Format::eA2B10G10R10UintPack32).RV(vk::Format::eA2B10G10R10UintPack32));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGB10A2UIntNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eA2B10G10R10UnormPack32).RT(vk::Format::eA2B10G10R10UnormPack32).VA(vk::Format::eA2B10G10R10UnormPack32).RV(vk::Format::eA2B10G10R10UnormPack32));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RG11B10Float, xiiGALFormatLookupEntryVulkan(vk::Format::eB10G11R11UfloatPack32).RT(vk::Format::eB10G11R11UfloatPack32).VA(vk::Format::eB10G11R11UfloatPack32).RV(vk::Format::eB10G11R11UfloatPack32));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUByteNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Unorm).RT(vk::Format::eR8G8B8A8Unorm).VA(vk::Format::eR8G8B8A8Unorm).RV(vk::Format::eR8G8B8A8Unorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUByteNormalizedsRGB,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Srgb).RT(vk::Format::eR8G8B8A8Srgb).RV(vk::Format::eR8G8B8A8Srgb));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUByte, xiiGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Uint).RT(vk::Format::eR8G8B8A8Uint).VA(vk::Format::eR8G8B8A8Uint).RV(vk::Format::eR8G8B8A8Uint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAByteNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Snorm).RT(vk::Format::eR8G8B8A8Snorm).VA(vk::Format::eR8G8B8A8Snorm).RV(vk::Format::eR8G8B8A8Snorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAByte, xiiGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Sint).RT(vk::Format::eR8G8B8A8Sint).VA(vk::Format::eR8G8B8A8Sint).RV(vk::Format::eR8G8B8A8Sint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGHalf, xiiGALFormatLookupEntryVulkan(vk::Format::eR16G16Sfloat).RT(vk::Format::eR16G16Sfloat).VA(vk::Format::eR16G16Sfloat).RV(vk::Format::eR16G16Sfloat));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUShort, xiiGALFormatLookupEntryVulkan(vk::Format::eR16G16Uint).RT(vk::Format::eR16G16Uint).VA(vk::Format::eR16G16Uint).RV(vk::Format::eR16G16Uint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUShortNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eR16G16Unorm).RT(vk::Format::eR16G16Unorm).VA(vk::Format::eR16G16Unorm).RV(vk::Format::eR16G16Unorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGShort, xiiGALFormatLookupEntryVulkan(vk::Format::eR16G16Sint).RT(vk::Format::eR16G16Sint).VA(vk::Format::eR16G16Sint).RV(vk::Format::eR16G16Sint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGShortNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eR16G16Snorm).RT(vk::Format::eR16G16Snorm).VA(vk::Format::eR16G16Snorm).RV(vk::Format::eR16G16Snorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUByte,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR8G8Uint).RT(vk::Format::eR8G8Uint).VA(vk::Format::eR8G8Uint).RV(vk::Format::eR8G8Uint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUByteNormalized,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR8G8Unorm).RT(vk::Format::eR8G8Unorm).VA(vk::Format::eR8G8Unorm).RV(vk::Format::eR8G8Unorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGByte,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR8G8Sint).RT(vk::Format::eR8G8Sint).VA(vk::Format::eR8G8Sint).RV(vk::Format::eR8G8Sint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGByteNormalized,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR8G8Snorm).RT(vk::Format::eR8G8Snorm).VA(vk::Format::eR8G8Snorm).RV(vk::Format::eR8G8Snorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RFloat,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR32Sfloat).RT(vk::Format::eR32Sfloat).VA(vk::Format::eR32Sfloat).RV(vk::Format::eR32Sfloat));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUInt,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR32Uint).RT(vk::Format::eR32Uint).VA(vk::Format::eR32Uint).RV(vk::Format::eR32Uint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RInt,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR32Sint).RT(vk::Format::eR32Sint).VA(vk::Format::eR32Sint).RV(vk::Format::eR32Sint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RHalf,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR16Sfloat).RT(vk::Format::eR16Sfloat).VA(vk::Format::eR16Sfloat).RV(vk::Format::eR16Sfloat));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUShort,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR16Uint).RT(vk::Format::eR16Uint).VA(vk::Format::eR16Uint).RV(vk::Format::eR16Uint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUShortNormalized,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR16Unorm).RT(vk::Format::eR16Unorm).VA(vk::Format::eR16Unorm).RV(vk::Format::eR16Unorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RShort,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR16Sint).RT(vk::Format::eR16Sint).VA(vk::Format::eR16Sint).RV(vk::Format::eR16Sint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RShortNormalized,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR16Snorm).RT(vk::Format::eR16Snorm).VA(vk::Format::eR16Snorm).RV(vk::Format::eR16Snorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUByte,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR8Uint).RT(vk::Format::eR8Uint).VA(vk::Format::eR8Uint).RV(vk::Format::eR8Uint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUByteNormalized,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR8Unorm).RT(vk::Format::eR8Unorm).VA(vk::Format::eR8Unorm).RV(vk::Format::eR8Unorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RByte,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR8Sint).RT(vk::Format::eR8Sint).VA(vk::Format::eR8Sint).RV(vk::Format::eR8Sint));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RByteNormalized,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR8Snorm).RT(vk::Format::eR8Snorm).VA(vk::Format::eR8Snorm).RV(vk::Format::eR8Snorm));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::AUByteNormalized,
                                    xiiGALFormatLookupEntryVulkan(vk::Format::eR8Unorm).RT(vk::Format::eR8Unorm).VA(vk::Format::eR8Unorm).RV(vk::Format::eR8Unorm));

  auto SelectDepthFormat = [&](const std::vector<vk::Format>& list) -> vk::Format {
    for (auto& format : list)
    {
      vk::FormatProperties formatProperties;
      m_physicalDevice.getFormatProperties(format, &formatProperties);
      if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        return format;
    }
    return vk::Format::eUndefined;
  };

  auto SelectStorageFormat = [](vk::Format depthFormat) -> vk::Format {
    switch (depthFormat)
    {
      case vk::Format::eD16Unorm:
        return vk::Format::eR16Unorm;
      case vk::Format::eD16UnormS8Uint:
        return vk::Format::eUndefined;
      case vk::Format::eD24UnormS8Uint:
        return vk::Format::eUndefined;
      case vk::Format::eD32Sfloat:
        return vk::Format::eR32Sfloat;
      case vk::Format::eD32SfloatS8Uint:
        return vk::Format::eR32Sfloat;
      default:
        return vk::Format::eUndefined;
    }
  };

  // Select smallest available depth format.  #TODO_VULKAN support packed eX8D24UnormPack32?
  vk::Format depthFormat = SelectDepthFormat({vk::Format::eD16Unorm, vk::Format::eD24UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint});
  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::D16,
                                    xiiGALFormatLookupEntryVulkan(SelectStorageFormat(depthFormat)).RT(depthFormat).RV(depthFormat).DS(depthFormat).D(depthFormat));

  // Select closest depth stencil format.
  depthFormat = SelectDepthFormat({vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint});
  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::D24S8, xiiGALFormatLookupEntryVulkan(SelectStorageFormat(depthFormat)).RT(depthFormat).RV(depthFormat).DS(depthFormat).D(depthFormat).S(depthFormat));

  // Select biggest depth format.
  depthFormat = SelectDepthFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm});
  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::DFloat,
                                    xiiGALFormatLookupEntryVulkan(SelectStorageFormat(depthFormat)).RT(depthFormat).RV(depthFormat).D(depthFormat).DS(depthFormat));

  // TODO is BC1 the rgba or the rgb format?
  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC1, xiiGALFormatLookupEntryVulkan(vk::Format::eBc1RgbaUnormBlock).RV(vk::Format::eBc1RgbaUnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC1sRGB, xiiGALFormatLookupEntryVulkan(vk::Format::eBc1RgbaSrgbBlock).RV(vk::Format::eBc1RgbaSrgbBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC2, xiiGALFormatLookupEntryVulkan(vk::Format::eBc2UnormBlock).RV(vk::Format::eBc2UnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC2sRGB, xiiGALFormatLookupEntryVulkan(vk::Format::eBc2SrgbBlock).RV(vk::Format::eBc2SrgbBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC3, xiiGALFormatLookupEntryVulkan(vk::Format::eBc3UnormBlock).RV(vk::Format::eBc3UnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC3sRGB, xiiGALFormatLookupEntryVulkan(vk::Format::eBc3SrgbBlock).RV(vk::Format::eBc3SrgbBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC4UNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eBc4UnormBlock).RV(vk::Format::eBc4UnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC4Normalized, xiiGALFormatLookupEntryVulkan(vk::Format::eBc4SnormBlock).RV(vk::Format::eBc4SnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC5UNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eBc5UnormBlock).RV(vk::Format::eBc5UnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC5Normalized, xiiGALFormatLookupEntryVulkan(vk::Format::eBc5SnormBlock).RV(vk::Format::eBc5SnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC6UFloat, xiiGALFormatLookupEntryVulkan(vk::Format::eBc6HUfloatBlock).RV(vk::Format::eBc6HUfloatBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC6Float, xiiGALFormatLookupEntryVulkan(vk::Format::eBc6HSfloatBlock).RV(vk::Format::eBc6HSfloatBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC7UNormalized, xiiGALFormatLookupEntryVulkan(vk::Format::eBc7UnormBlock).RV(vk::Format::eBc7UnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    xiiGALResourceFormat::BC7UNormalizedsRGB, xiiGALFormatLookupEntryVulkan(vk::Format::eBc7SrgbBlock).RV(vk::Format::eBc7SrgbBlock));
}



XII_STATICLINK_FILE(RendererVulkan, RendererVulkan_Device_Implementation_DeviceVulkan);
