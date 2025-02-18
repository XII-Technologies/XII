#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#endif

#define VK_ENABLE_BETA_EXTENSIONS
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  define VK_USE_PLATFORM_WIN32_KHR
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  define VK_USE_PLATFORM_XCB_KHR
#  define VK_USE_PLATFORM_WAYLAND_KHR
#elif XII_ENABLED(XII_PLATFORM_OSX)
#  define VK_USE_PLATFORM_MACOS_MVK
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  define VK_USE_PLATFORM_ANDROID_KHR
#elif XII_ENABLED(XII_PLATFORM_IOS)
#  define VK_USE_PLATFORM_IOS_MVK
#endif

#include <GraphicsFoundation/Utilities/DeviceUtilities.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.hpp>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <vulkan/vulkan_win32.h>
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <vulkan/vulkan_android.h>
#endif

namespace VulkanUtilities
{
  constexpr vk::PipelineStageFlags VK_PIPELINE_STAGE_ALL_SHADERS =
    vk::PipelineStageFlagBits::eVertexShader |
    vk::PipelineStageFlagBits::eTessellationControlShader |
    vk::PipelineStageFlagBits::eTessellationEvaluationShader |
    vk::PipelineStageFlagBits::eGeometryShader |
    vk::PipelineStageFlagBits::eFragmentShader |
    vk::PipelineStageFlagBits::eComputeShader |
    vk::PipelineStageFlagBits::eRayTracingShaderKHR |
    vk::PipelineStageFlagBits::eTaskShaderEXT |
    vk::PipelineStageFlagBits::eMeshShaderEXT;

  constexpr vk::PipelineStageFlags VK_PIPELINE_STAGE_ALL_TRANSFER =
    vk::PipelineStageFlagBits::eTopOfPipe |
    vk::PipelineStageFlagBits::eTransfer |
    vk::PipelineStageFlagBits::eBottomOfPipe |
    vk::PipelineStageFlagBits::eHost |
    vk::PipelineStageFlagBits::eAllCommands;

} // namespace VulkanUtilities

XII_DEFINE_AS_POD_TYPE(vk::Format);
XII_DEFINE_AS_POD_TYPE(vk::PresentModeKHR);
XII_DEFINE_AS_POD_TYPE(vk::SurfaceFormatKHR);
XII_DEFINE_AS_POD_TYPE(vk::LayerProperties);
XII_DEFINE_AS_POD_TYPE(vk::Semaphore);
XII_DEFINE_AS_POD_TYPE(vk::ExtensionProperties);
XII_DEFINE_AS_POD_TYPE(vk::Fence);
XII_DEFINE_AS_POD_TYPE(vk::PhysicalDevice);
XII_DEFINE_AS_POD_TYPE(vk::Image);
XII_DEFINE_AS_POD_TYPE(vk::QueueFamilyProperties);
XII_DEFINE_AS_POD_TYPE(vk::PhysicalDeviceFragmentShadingRateKHR);
XII_DEFINE_AS_POD_TYPE(vk::DescriptorType);
XII_DEFINE_AS_POD_TYPE(vk::DescriptorSet);
XII_DEFINE_AS_POD_TYPE(vk::WriteDescriptorSet);

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

#include <GraphicsVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <GraphicsVulkan/Pools/DynamicBufferPoolVulkan.h>
#include <GraphicsVulkan/Pools/FencePoolVulkan.h>
#include <GraphicsVulkan/Pools/QueryPoolVulkan.h>
#include <GraphicsVulkan/Pools/SemaphorePoolVulkan.h>
#include <GraphicsVulkan/Pools/StagingBufferPoolVulkan.h>
