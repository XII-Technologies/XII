/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/States/TilePipelineStateD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTilePipelineStateD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALTilePipelineStateD3D12::xiiGALTilePipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALTilePipelineStateCreationDescription& creationDescription) :
  xiiGALTilePipelineState(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALTilePipelineStateD3D12::~xiiGALTilePipelineStateD3D12() = default;

xiiResult xiiGALTilePipelineStateD3D12::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();

  if (pDeviceD3D12->GetGraphicsDeviceAdapterProperties().m_Features.m_TileShaders != xiiGALDeviceFeatureState::Enabled)
  {
    xiiLog::Error("Tile pipeline creation failed: Tile Shaders are disabled on the current D3D12 device.");
    return XII_FAILURE;
  }

  xiiLog::Error("Tile pipeline creation is not supported by the current GraphicsD3D12 implementation.");

  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_TilePipelineStateD3D12);
