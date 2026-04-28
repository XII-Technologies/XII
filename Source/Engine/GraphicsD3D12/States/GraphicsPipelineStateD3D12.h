/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

class XII_GRAPHICSD3D12_DLL xiiGALGraphicsPipelineStateD3D12 final : public xiiGALGraphicsPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALGraphicsPipelineStateD3D12, xiiGALGraphicsPipelineState);

public:
protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALGraphicsPipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALGraphicsPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALGraphicsPipelineStateD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
};
