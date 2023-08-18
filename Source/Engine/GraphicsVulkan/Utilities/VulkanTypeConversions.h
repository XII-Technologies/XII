#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <vulkan/vulkan.hpp>

XII_DEFINE_AS_POD_TYPE(vk::PresentModeKHR);

class XII_GRAPHICSVULKAN_DLL xiiVulkanTypeConversions
{
public:
  /// \brief Helper function to hash Vulkan enumerations.
  template <typename T, typename R = typename std::underlying_type<T>::type>
  static R GetUnderlyingValue(T value)
  {
    return static_cast<typename std::underlying_type<T>::type>(value);
  }

  /// \brief Helper function to hash vk flags.
  template <typename T>
  static auto GetUnderlyingFlagsValue(T value)
  {
    return static_cast<typename T::MaskType>(value);
  }
};

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>
