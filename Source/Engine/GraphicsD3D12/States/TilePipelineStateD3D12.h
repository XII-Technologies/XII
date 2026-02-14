#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

class XII_GRAPHICSD3D12_DLL xiiGALTilePipelineStateD3D12 final : public xiiGALTilePipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTilePipelineStateD3D12, xiiGALTilePipelineState);

public:
protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALTilePipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALTilePipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALTilePipelineStateD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
};
