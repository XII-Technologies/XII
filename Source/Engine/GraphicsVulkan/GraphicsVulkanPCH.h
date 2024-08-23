#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#define VK_ENABLE_BETA_EXTENSIONS
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  define VK_USE_PLATFORM_WIN32_KHR
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  define VK_USE_PLATFORM_XCB_KHR
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  define VK_USE_PLATFORM_ANDROID_KHR
#endif

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
