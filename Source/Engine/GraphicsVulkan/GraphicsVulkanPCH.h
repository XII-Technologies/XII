#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#define VK_ENABLE_BETA_EXTENSIONS
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  define VK_USE_PLATFORM_WIN32_KHR
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  define VK_USE_PLATFORM_WAYLAND_KHR
#elif XII_ENABLED(XII_PLATFORM_OSX)
#  define VK_USE_PLATFORM_MACOS_MVK
#endif

// Enable dynamic Vulkan functions.
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

// Tell vk-hpp to generate only the plain C-style structs (aggregate init).
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_SETTERS

// Disable all smart-handle/RAII wrappers and exceptions.
#define VULKAN_HPP_NO_SMART_HANDLE
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_RAII_NO_EXCEPTIONS

// Turn off any "enhanced mode" function overloads (ArrayProxy, return-value transforms...).
#define VULKAN_HPP_DISABLE_ENHANCED_MODE

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  define VULKAN_HPP_NO_WIN32_PROTOTYPES
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#endif

#include <vulkan/vulkan.hpp>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <vulkan/vulkan_win32.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  ifdef VK_USE_PLATFORM_WAYLAND_KHR
#    include <vulkan/vulkan_wayland.h>
#  endif
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
XII_DEFINE_AS_POD_TYPE(vk::QueueFamilyProperties);
XII_DEFINE_AS_POD_TYPE(vk::PhysicalDeviceFragmentShadingRateKHR);
XII_DEFINE_AS_POD_TYPE(vk::DescriptorType);
XII_DEFINE_AS_POD_TYPE(vk::DescriptorSet);
XII_DEFINE_AS_POD_TYPE(vk::WriteDescriptorSet);
XII_DEFINE_AS_POD_TYPE(vk::MultiDrawInfoEXT);
XII_DEFINE_AS_POD_TYPE(vk::MultiDrawIndexedInfoEXT);
XII_DEFINE_AS_POD_TYPE(vk::CommandBuffer);
XII_DEFINE_AS_POD_TYPE(vk::Image);
XII_DEFINE_AS_POD_TYPE(vk::Buffer);
XII_DEFINE_AS_POD_TYPE(vk::Viewport);
XII_DEFINE_AS_POD_TYPE(vk::Rect2D);
XII_DEFINE_AS_POD_TYPE(vk::ExportMemoryAllocateInfo);

#define VK_REMAINING_ARRAY_LAYERS (~0U)
#define VK_REMAINING_MIP_LEVELS   (~0U)

#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>
