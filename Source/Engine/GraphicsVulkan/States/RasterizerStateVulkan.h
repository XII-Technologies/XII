#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

namespace vk
{
  struct PipelineRasterizationStateCreateInfo;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALRasterizerStateVulkan : public xiiGALRasterizerState
{
public:
  XII_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* GetRasterizerState() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateVulkan(const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  vk::PipelineRasterizationStateCreateInfo m_RasterizerState = {};
};

#include <GraphicsVulkan/States/Implementation/RasterizerStateVulkan_inl.h>
