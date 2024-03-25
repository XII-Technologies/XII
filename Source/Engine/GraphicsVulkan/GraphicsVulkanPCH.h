#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#include <vulkan/vulkan.hpp>

// Some of the functionality we need has moved from vulkan.hpp to vulkan_format_traits.hpp in later versions of the Vulkan SDK.
#if __has_include(<vulkan/vulkan_format_traits.hpp>)
#  include <vulkan/vulkan_format_traits.hpp>
#endif

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>
