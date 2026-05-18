/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/States/ComputePipelineStateD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALComputePipelineStateD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALComputePipelineStateD3D12::xiiGALComputePipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALComputePipelineStateCreationDescription& creationDescription) :
  xiiGALComputePipelineState(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALComputePipelineStateD3D12::~xiiGALComputePipelineStateD3D12() = default;

xiiResult xiiGALComputePipelineStateD3D12::InitPlatform()
{
  return XII_FAILURE;
}

void xiiGALComputePipelineStateD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_ComputePipelineStateD3D12);
