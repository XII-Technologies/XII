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
  const vk::PipelineDepthStencilStateCreateInfo* GetDepthStencilState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
  vk::PipelineDepthStencilStateCreateInfo m_DepthStencilState = {};
};

#include <GraphicsVulkan/States/Implementation/DepthStencilStateVulkan_inl.h>
