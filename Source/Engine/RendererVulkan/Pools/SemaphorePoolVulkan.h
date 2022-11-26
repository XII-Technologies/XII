#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for semaphores
///
/// Do not call ReclaimSemaphore manually, instead call xiiGALDeviceVulkan::ReclaimLater which will make sure to reclaim the semaphore once it is no longer in use.
/// Usage:
/// \code{.cpp}
///   vk::Semaphore s = xiiSemaphorePoolVulkan::RequestSemaphore();
///   ...
///   xiiGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(s);
/// \endcode
class XII_RENDERERVULKAN_DLL xiiSemaphorePoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();

  static vk::Semaphore RequestSemaphore();
  static void          ReclaimSemaphore(vk::Semaphore& semaphore);

private:
  static xiiHybridArray<vk::Semaphore, 4> s_semaphores;
  static vk::Device                       s_device;
};
