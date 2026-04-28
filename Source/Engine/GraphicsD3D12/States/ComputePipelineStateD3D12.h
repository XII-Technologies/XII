/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

class XII_GRAPHICSD3D12_DLL xiiGALComputePipelineStateD3D12 final : public xiiGALComputePipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALComputePipelineStateD3D12, xiiGALComputePipelineState);

public:
protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALComputePipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALComputePipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALComputePipelineStateD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
};
