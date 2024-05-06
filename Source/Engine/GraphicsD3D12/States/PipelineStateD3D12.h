#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

class XII_GRAPHICSD3D12_DLL xiiGALPipelineStateD3D12 final : public xiiGALPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineStateD3D12, xiiGALPipelineState);

public:
protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALPipelineStateD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineStateD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};


#include <GraphicsD3D12/States/Implementation/PipelineStateD3D12_inl.h>
