#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

namespace vk
{
  struct PipelineRasterizationStateCreateInfo;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALRasterizerStateVulkan final : public xiiGALRasterizerState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALRasterizerStateVulkan, xiiGALRasterizerState);

public:
  const vk::PipelineRasterizationStateCreateInfo* GetRasterizerState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
  vk::PipelineRasterizationStateCreateInfo m_RasterizerState = {};
};

#include <GraphicsVulkan/States/Implementation/RasterizerStateVulkan_inl.h>
