#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

namespace vk
{
  struct PipelineDepthStencilStateCreateInfo;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALDepthStencilStateVulkan final : public xiiGALDepthStencilState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDepthStencilStateVulkan, xiiGALDepthStencilState);

public:
  XII_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* GetDepthStencilState() const { return &m_DepthStencilState; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

private:
  vk::PipelineDepthStencilStateCreateInfo m_DepthStencilState = {};
};
