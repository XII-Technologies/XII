/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/States/TilePipelineStateD3D12.h>

xiiGALTilePipelineStateD3D12::xiiGALTilePipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALTilePipelineStateCreationDescription& creationDescription) :
  xiiGALTilePipelineState(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALTilePipelineStateD3D12::~xiiGALTilePipelineStateD3D12() = default;

xiiResult xiiGALTilePipelineStateD3D12::InitPlatform()
{
  return XII_FAILURE;
}

void xiiGALTilePipelineStateD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

XII_STATICLINK_FILE(TileD3D12, TileD3D12_States_Implementation_TilePipelineStateD3D12);
