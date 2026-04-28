/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/States/PipelineState.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShaderVariableFlags, 1)
  XII_ENUM_CONSTANT(xiiGALShaderVariableFlags::None),
  XII_ENUM_CONSTANT(xiiGALShaderVariableFlags::NoDynamicBuffers),
  XII_ENUM_CONSTANT(xiiGALShaderVariableFlags::InputAttachment),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALPipelineShadingRateFlags, 1)
  XII_ENUM_CONSTANT(xiiGALPipelineShadingRateFlags::None),
  XII_ENUM_CONSTANT(xiiGALPipelineShadingRateFlags::PerPrimitive),
  XII_ENUM_CONSTANT(xiiGALPipelineShadingRateFlags::TextureBased),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALPipelineType, 1)
  XII_ENUM_CONSTANT(xiiGALPipelineType::Graphics),
  XII_ENUM_CONSTANT(xiiGALPipelineType::Compute),
  XII_ENUM_CONSTANT(xiiGALPipelineType::Mesh),
  XII_ENUM_CONSTANT(xiiGALPipelineType::RayTracing),
  XII_ENUM_CONSTANT(xiiGALPipelineType::Tile),
  XII_ENUM_CONSTANT(xiiGALPipelineType::Invalid),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALPipelineState, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALGraphicsPipelineState, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALComputePipelineState, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALRayTracingPipelineState, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTilePipelineState, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALPipelineState::xiiGALPipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALDeviceObject(std::move(pDevice)), m_Description(creationDescription)
{
}

xiiGALPipelineState::~xiiGALPipelineState() = default;

////////////////////////////////////////////////////////////////////////////////

xiiGALGraphicsPipelineState::xiiGALGraphicsPipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALGraphicsPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(std::move(pDevice), creationDescription), m_Description(creationDescription)
{
}

xiiGALGraphicsPipelineState::~xiiGALGraphicsPipelineState() = default;

////////////////////////////////////////////////////////////////////////////////

xiiGALComputePipelineState::xiiGALComputePipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALComputePipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(std::move(pDevice), creationDescription), m_Description(creationDescription)
{
}


xiiGALComputePipelineState::~xiiGALComputePipelineState() = default;

////////////////////////////////////////////////////////////////////////////////

xiiGALRayTracingPipelineState::xiiGALRayTracingPipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALRayTracingPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(std::move(pDevice), creationDescription), m_Description(creationDescription)
{
}

xiiGALRayTracingPipelineState::~xiiGALRayTracingPipelineState() = default;

////////////////////////////////////////////////////////////////////////////////

xiiGALTilePipelineState::xiiGALTilePipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALTilePipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(std::move(pDevice), creationDescription), m_Description(creationDescription)
{
}

xiiGALTilePipelineState::~xiiGALTilePipelineState() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_States_Implementation_PipelineState);
