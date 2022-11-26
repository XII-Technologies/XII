#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for fences
///
/// Do not call ReclaimFence manually, instead call xiiGALDeviceVulkan::ReclaimLater which will make sure to reclaim the fence once it is no longer in use.
/// Fences are reclaimed once the frame in xiiGALDeviceVulkan is reused (currently 4 frames are in rotation). Do not call resetFences, this is already done by ReclaimFence.
/// Usage:
/// \code{.cpp}
///   vk::Fence f = xiiFencePoolVulkan::RequestFence();
///   <insert fence somewhere>
///   <wait for fence>
///   xiiGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(f);
/// \endcode
class XII_RENDERERVULKAN_DLL xiiFencePoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();

  static vk::Fence RequestFence();
  static void      ReclaimFence(vk::Fence& fence);

private:
  static xiiHybridArray<vk::Fence, 4> s_Fences;
  static vk::Device                   s_device;
};
