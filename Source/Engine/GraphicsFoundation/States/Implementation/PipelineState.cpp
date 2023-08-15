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

// clang-format on

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_States_Implementation_PipelineState);
