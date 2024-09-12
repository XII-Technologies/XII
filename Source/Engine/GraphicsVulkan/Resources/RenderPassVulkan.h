#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

namespace vk
{
  class RenderPass;
}

class XII_GRAPHICSVULKAN_DLL xiiGALRenderPassVulkan final : public xiiGALRenderPass
{
public:
  XII_ALWAYS_INLINE vk::RenderPass GetVulkanRenderPass() const { return m_vkRenderPass; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRenderPassVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALRenderPassCreationDescription& creationDescription);

  virtual ~xiiGALRenderPassVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
  vk::RenderPass m_vkRenderPass;
};
