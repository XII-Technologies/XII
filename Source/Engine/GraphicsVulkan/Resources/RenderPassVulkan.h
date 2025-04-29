#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

namespace vk
{
  class RenderPass;
}

class XII_GRAPHICSVULKAN_DLL xiiGALRenderPassVulkan final : public xiiGALRenderPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALRenderPassVulkan, xiiGALRenderPass);

public:
  XII_ALWAYS_INLINE vk::RenderPass GetVulkanRenderPass() const { return m_vkRenderPass; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRenderPassVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALRenderPassCreationDescription& creationDescription);

  virtual ~xiiGALRenderPassVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  template <xiiUInt8 RenderPassVersion>
  vk::Result CreateRenderPassForVersion();

  vk::RenderPass m_vkRenderPass;
};
