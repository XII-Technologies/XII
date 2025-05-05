#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/States/PipelineStateNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALGraphicsPipelineStateNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALGraphicsPipelineStateNull::xiiGALGraphicsPipelineStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALGraphicsPipelineStateCreationDescription& creationDescription) :
  xiiGALGraphicsPipelineState(pDeviceNull, creationDescription)
{
}

xiiGALGraphicsPipelineStateNull::~xiiGALGraphicsPipelineStateNull() = default;

xiiResult xiiGALGraphicsPipelineStateNull::InitPlatform()
{
  return XII_SUCCESS;
}

///////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALComputePipelineStateNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALComputePipelineStateNull::xiiGALComputePipelineStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALComputePipelineStateCreationDescription& creationDescription) :
  xiiGALComputePipelineState(pDeviceNull, creationDescription)
{
}

xiiGALComputePipelineStateNull::~xiiGALComputePipelineStateNull() = default;

xiiResult xiiGALComputePipelineStateNull::InitPlatform()
{
  return XII_SUCCESS;
}

///////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALRayTracingPipelineStateNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALRayTracingPipelineStateNull::xiiGALRayTracingPipelineStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALRayTracingPipelineStateCreationDescription& creationDescription) :
  xiiGALRayTracingPipelineState(pDeviceNull, creationDescription)
{
}

xiiGALRayTracingPipelineStateNull::~xiiGALRayTracingPipelineStateNull() = default;

xiiResult xiiGALRayTracingPipelineStateNull::InitPlatform()
{
  return XII_SUCCESS;
}

///////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTilePipelineStateNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALTilePipelineStateNull::xiiGALTilePipelineStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALTilePipelineStateCreationDescription& creationDescription) :
  xiiGALTilePipelineState(pDeviceNull, creationDescription)
{
}

xiiGALTilePipelineStateNull::~xiiGALTilePipelineStateNull() = default;

xiiResult xiiGALTilePipelineStateNull::InitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_PipelineStateNull);
