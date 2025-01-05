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
  XII_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* GetRasterizerState() const { return &m_RasterizerState; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

private:
  vk::PipelineRasterizationStateCreateInfo m_RasterizerState = {};
};
