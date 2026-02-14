#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/States/RayTracingPipelineStateD3D12.h>

xiiGALRayTracingPipelineStateD3D12::xiiGALRayTracingPipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALRayTracingPipelineStateCreationDescription& creationDescription) :
  xiiGALRayTracingPipelineState(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALRayTracingPipelineStateD3D12::~xiiGALRayTracingPipelineStateD3D12() = default;

xiiResult xiiGALRayTracingPipelineStateD3D12::InitPlatform()
{
  return XII_FAILURE;
}

void xiiGALRayTracingPipelineStateD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

XII_STATICLINK_FILE(RayTracingD3D12, RayTracingD3D12_States_Implementation_RayTracingPipelineStateD3D12);
