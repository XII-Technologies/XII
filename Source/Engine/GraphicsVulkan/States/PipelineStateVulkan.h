#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

class XII_GRAPHICSVULKAN_DLL xiiGALPipelineStateVulkan final : public xiiGALPipelineState
{
public:
  Diligent::IPipelineState* GetPipelineState() const;

  Diligent::IShaderResourceBinding* GetShaderResourceBinding() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALPipelineStateVulkan(const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::IPipelineState*         m_pPipelineState         = nullptr;
  Diligent::IShaderResourceBinding* m_pShaderResourceBinding = nullptr;
};


#include <GraphicsVulkan/States/Implementation/PipelineStateVulkan_inl.h>
