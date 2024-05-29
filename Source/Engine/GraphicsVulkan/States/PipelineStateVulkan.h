#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

class XII_GRAPHICSVULKAN_DLL xiiGALPipelineStateVulkan final : public xiiGALPipelineState
{
public:
protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALPipelineStateVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};


#include <GraphicsVulkan/States/Implementation/PipelineStateVulkan_inl.h>
