#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for command buffers
///
/// Do not call ReclaimCommandBuffer manually, instead call xiiGALDeviceVulkan::ReclaimLater which will make sure to reclaim the command buffer once it is no longer in use.
/// Usage:
/// \code{.cpp}
///   vk::CommandBuffer c = pPool->RequestCommandBuffer();
///   c.begin();
///   ...
///   c.end();
///   xiiGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(c);
/// \endcode
class XII_RENDERERVULKAN_DLL xiiCommandBufferPoolVulkan
{
public:
  void Initialize(vk::Device device, xiiUInt32 graphicsFamilyIndex);
  void DeInitialize();

  vk::CommandBuffer RequestCommandBuffer();
  void              ReclaimCommandBuffer(vk::CommandBuffer& CommandBuffer);

private:
  vk::Device                           m_device;
  vk::CommandPool                      m_commandPool;
  xiiHybridArray<vk::CommandBuffer, 4> m_CommandBuffers;
};
