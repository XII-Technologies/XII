
XII_ALWAYS_INLINE Diligent::RENDER_DEVICE_TYPE xiiDiligentTypeConversions::GetRenderDeviceType(const xiiEnum<xiiGALGraphicsDeviceType> e)
{
  switch (e)
  {
    case xiiGALGraphicsDeviceType::Direct3D12:
      return Diligent::RENDER_DEVICE_TYPE_D3D12;
    case xiiGALGraphicsDeviceType ::Vulkan:
      return Diligent::RENDER_DEVICE_TYPE_VULKAN;
    case xiiGALGraphicsDeviceType::Metal:
      return Diligent::RENDER_DEVICE_TYPE_METAL;
  }
  return Diligent::RENDER_DEVICE_TYPE_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::BLEND_FACTOR xiiDiligentTypeConversions::GetBlendFactor(xiiEnum<xiiGALBlendFactor> e)
{
  switch (e)
  {
    case xiiGALBlendFactor::Undefined:
      return Diligent::BLEND_FACTOR_UNDEFINED;
    case xiiGALBlendFactor::Zero:
      return Diligent::BLEND_FACTOR_ZERO;
    case xiiGALBlendFactor::One:
      return Diligent::BLEND_FACTOR_ONE;
    case xiiGALBlendFactor::SourceColor:
      return Diligent::BLEND_FACTOR_SRC_COLOR;
    case xiiGALBlendFactor::InverseSourceColor:
      return Diligent::BLEND_FACTOR_INV_SRC_COLOR;
    case xiiGALBlendFactor::SourceAlpha:
      return Diligent::BLEND_FACTOR_SRC_ALPHA;
    case xiiGALBlendFactor::InverseSourceAlpha:
      return Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
    case xiiGALBlendFactor::DestinationAlpha:
      return Diligent::BLEND_FACTOR_DEST_ALPHA;
    case xiiGALBlendFactor::InverseDestinationAlpha:
      return Diligent::BLEND_FACTOR_INV_DEST_ALPHA;
    case xiiGALBlendFactor::DestinationColor:
      return Diligent::BLEND_FACTOR_DEST_COLOR;
    case xiiGALBlendFactor::InverseDestinationColor:
      return Diligent::BLEND_FACTOR_INV_DEST_COLOR;
    case xiiGALBlendFactor::SourceAlphaSaturate:
      return Diligent::BLEND_FACTOR_SRC_ALPHA_SAT;
    case xiiGALBlendFactor::BlendFactor:
      return Diligent::BLEND_FACTOR_BLEND_FACTOR;
    case xiiGALBlendFactor::InverseBlendFactor:
      return Diligent::BLEND_FACTOR_INV_BLEND_FACTOR;
    case xiiGALBlendFactor::SourceOneColor:
      return Diligent::BLEND_FACTOR_SRC1_COLOR;
    case xiiGALBlendFactor::InverseSourceOneColor:
      return Diligent::BLEND_FACTOR_INV_SRC1_COLOR;
    case xiiGALBlendFactor::SourceOneAlpha:
      return Diligent::BLEND_FACTOR_SRC1_ALPHA;
    case xiiGALBlendFactor::InverseSourceOneAlpha:
      return Diligent::BLEND_FACTOR_INV_SRC1_ALPHA;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::BLEND_FACTOR_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::BLEND_OPERATION xiiDiligentTypeConversions::GetBlendOp(xiiEnum<xiiGALBlendOperation> e)
{
  switch (e)
  {
    case xiiGALBlendOperation::Undefined:
      return Diligent::BLEND_OPERATION_UNDEFINED;
    case xiiGALBlendOperation::Add:
      return Diligent::BLEND_OPERATION_ADD;
    case xiiGALBlendOperation::Subtract:
      return Diligent::BLEND_OPERATION_SUBTRACT;
    case xiiGALBlendOperation::ReverseSubtract:
      return Diligent::BLEND_OPERATION_REV_SUBTRACT;
    case xiiGALBlendOperation::Min:
      return Diligent::BLEND_OPERATION_MIN;
    case xiiGALBlendOperation::Max:
      return Diligent::BLEND_OPERATION_MAX;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::BLEND_OPERATION_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::COMPARISON_FUNCTION xiiDiligentTypeConversions::GetComparisonFunc(xiiEnum<xiiGALComparisonFunction> e)
{
  switch (e)
  {
    case xiiGALComparisonFunction::Unknown:
      return Diligent::COMPARISON_FUNC_UNKNOWN;
    case xiiGALComparisonFunction::Never:
      return Diligent::COMPARISON_FUNC_NEVER;
    case xiiGALComparisonFunction::Less:
      return Diligent::COMPARISON_FUNC_LESS;
    case xiiGALComparisonFunction::Equal:
      return Diligent::COMPARISON_FUNC_EQUAL;
    case xiiGALComparisonFunction::LessEqual:
      return Diligent::COMPARISON_FUNC_LESS_EQUAL;
    case xiiGALComparisonFunction::Greater:
      return Diligent::COMPARISON_FUNC_GREATER;
    case xiiGALComparisonFunction::NotEqual:
      return Diligent::COMPARISON_FUNC_NOT_EQUAL;
    case xiiGALComparisonFunction::GreaterEqual:
      return Diligent::COMPARISON_FUNC_GREATER_EQUAL;
    case xiiGALComparisonFunction::Always:
      return Diligent::COMPARISON_FUNC_ALWAYS;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::COMPARISON_FUNC_UNKNOWN;
}

XII_ALWAYS_INLINE Diligent::STENCIL_OP xiiDiligentTypeConversions::GetStencilOp(xiiEnum<xiiGALStencilOperation> e)
{
  switch (e)
  {
    case xiiGALStencilOperation::Undefined:
      return Diligent::STENCIL_OP_UNDEFINED;
    case xiiGALStencilOperation::Keep:
      return Diligent::STENCIL_OP_KEEP;
    case xiiGALStencilOperation::Zero:
      return Diligent::STENCIL_OP_ZERO;
    case xiiGALStencilOperation::Replace:
      return Diligent::STENCIL_OP_REPLACE;
    case xiiGALStencilOperation::IncrementSaturate:
      return Diligent::STENCIL_OP_INCR_SAT;
    case xiiGALStencilOperation::DecrementSaturate:
      return Diligent::STENCIL_OP_DECR_SAT;
    case xiiGALStencilOperation::Invert:
      return Diligent::STENCIL_OP_INVERT;
    case xiiGALStencilOperation::IncrementWrap:
      return Diligent::STENCIL_OP_INCR_WRAP;
    case xiiGALStencilOperation::DecrementWrap:
      return Diligent::STENCIL_OP_DECR_WRAP;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::STENCIL_OP::STENCIL_OP_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::FILL_MODE xiiDiligentTypeConversions::GetFillMode(xiiEnum<xiiGALFillMode> e)
{
  switch (e)
  {
    case xiiGALFillMode::Undefined:
      return Diligent::FILL_MODE_UNDEFINED;
    case xiiGALFillMode::Wireframe:
      return Diligent::FILL_MODE_WIREFRAME;
    case xiiGALFillMode::Solid:
      return Diligent::FILL_MODE_SOLID;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::FILL_MODE_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::CULL_MODE xiiDiligentTypeConversions::GetCullMode(xiiEnum<xiiGALCullMode> e)
{
  switch (e)
  {
    case xiiGALCullMode::Undefined:
      return Diligent::CULL_MODE_UNDEFINED;
    case xiiGALCullMode::None:
      return Diligent::CULL_MODE_NONE;
    case xiiGALCullMode::Front:
      return Diligent::CULL_MODE_NONE;
    case xiiGALCullMode::Back:

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::CULL_MODE_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::FILTER_TYPE xiiDiligentTypeConversions::GetFilter(xiiEnum<xiiGALFilterType> e)
{
  switch (e)
  {
    case xiiGALFilterType::Unknown:
      return Diligent::FILTER_TYPE_UNKNOWN;
    case xiiGALFilterType::Point:
      return Diligent::FILTER_TYPE_POINT;
    case xiiGALFilterType::Linear:
      return Diligent::FILTER_TYPE_LINEAR;
    case xiiGALFilterType::Anisotropic:
      return Diligent::FILTER_TYPE_ANISOTROPIC;
    case xiiGALFilterType::ComparisonPoint:
      return Diligent::FILTER_TYPE_COMPARISON_POINT;
    case xiiGALFilterType::ComparisonLinear:
      return Diligent::FILTER_TYPE_COMPARISON_LINEAR;
    case xiiGALFilterType::ComparisonAnisotropic:
      return Diligent::FILTER_TYPE_COMPARISON_ANISOTROPIC;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::FILTER_TYPE_UNKNOWN;
}

XII_ALWAYS_INLINE Diligent::COLOR_MASK xiiDiligentTypeConversions::GetColorMask(xiiBitflags<xiiGALColorMask> mask)
{
  if (mask == xiiGALColorMask::None)
    return Diligent::COLOR_MASK_NONE;

  return Diligent::COLOR_MASK(((mask.IsSet(xiiGALColorMask::Red)) ? Diligent::COLOR_MASK_RED : 0) |
                              ((mask.IsSet(xiiGALColorMask::Green)) ? Diligent::COLOR_MASK_GREEN : 0) |
                              ((mask.IsSet(xiiGALColorMask::Blue)) ? Diligent::COLOR_MASK_BLUE : 0) |
                              ((mask.IsSet(xiiGALColorMask::Alpha)) ? Diligent::COLOR_MASK_ALPHA : 0));
}

XII_ALWAYS_INLINE Diligent::BIND_FLAGS xiiDiligentTypeConversions::GetBindFlags(xiiBitflags<xiiGALBindFlags> e)
{
  if (e.IsNoFlagSet())
    return Diligent::BIND_NONE;

  Diligent::BIND_FLAGS bindFlags = {};

  if (e.IsSet(xiiGALBindFlags::VertexBuffer))
    bindFlags |= Diligent::BIND_VERTEX_BUFFER;
  if (e.IsSet(xiiGALBindFlags::IndexBuffer))
    bindFlags |= Diligent::BIND_INDEX_BUFFER;
  if (e.IsSet(xiiGALBindFlags::UniformBuffer))
    bindFlags |= Diligent::BIND_UNIFORM_BUFFER;
  if (e.IsSet(xiiGALBindFlags::ShaderResource))
    bindFlags |= Diligent::BIND_SHADER_RESOURCE;
  if (e.IsSet(xiiGALBindFlags::StreamOutput))
    bindFlags |= Diligent::BIND_STREAM_OUTPUT;
  if (e.IsSet(xiiGALBindFlags::RenderTarget))
    bindFlags |= Diligent::BIND_RENDER_TARGET;
  if (e.IsSet(xiiGALBindFlags::DepthStencil))
    bindFlags |= Diligent::BIND_DEPTH_STENCIL;
  if (e.IsSet(xiiGALBindFlags::UnorderedAccess))
    bindFlags |= Diligent::BIND_UNORDERED_ACCESS;
  if (e.IsSet(xiiGALBindFlags::IndirectDrawArguments))
    bindFlags |= Diligent::BIND_INDIRECT_DRAW_ARGS;
  if (e.IsSet(xiiGALBindFlags::InputAttachment))
    bindFlags |= Diligent::BIND_INPUT_ATTACHMENT;
  if (e.IsSet(xiiGALBindFlags::RayTracing))
    bindFlags |= Diligent::BIND_RAY_TRACING;
  if (e.IsSet(xiiGALBindFlags::ShadingRate))
    bindFlags |= Diligent::BIND_SHADING_RATE;

  return bindFlags;
}

XII_ALWAYS_INLINE Diligent::CPU_ACCESS_FLAGS xiiDiligentTypeConversions::GetCPUAccessFlags(xiiBitflags<xiiGALCPUAccessFlag> e)
{
  if (e.IsNoFlagSet())
    return Diligent::CPU_ACCESS_NONE;

  Diligent::CPU_ACCESS_FLAGS accessFlags = {};

  if (e.IsSet(xiiGALCPUAccessFlag::Read))
    accessFlags |= Diligent::CPU_ACCESS_READ;
  if (e.IsSet(xiiGALCPUAccessFlag::Write))
    accessFlags |= Diligent::CPU_ACCESS_WRITE;

  return accessFlags;
}

XII_ALWAYS_INLINE Diligent::BUFFER_MODE xiiDiligentTypeConversions::GetBufferMode(xiiEnum<xiiGALBufferMode> e)
{
  switch (e)
  {
    case xiiGALBufferMode::Undefined:
      return Diligent::BUFFER_MODE_UNDEFINED;
    case xiiGALBufferMode::Formatted:
      return Diligent::BUFFER_MODE_FORMATTED;
    case xiiGALBufferMode::Structured:
      return Diligent::BUFFER_MODE_STRUCTURED;
    case xiiGALBufferMode::Raw:
      return Diligent::BUFFER_MODE_RAW;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::BUFFER_MODE_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::USAGE xiiDiligentTypeConversions::GetUsage(xiiEnum<xiiGALResourceUsage> e)
{
  switch (e)
  {
    case xiiGALResourceUsage::Immutable:
      return Diligent::USAGE_IMMUTABLE;
    case xiiGALResourceUsage::Default:
      return Diligent::USAGE_DEFAULT;
    case xiiGALResourceUsage::Dynamic:
      return Diligent::USAGE_DYNAMIC;
    case xiiGALResourceUsage::Staging:
      return Diligent::USAGE_STAGING;
    case xiiGALResourceUsage::Unified:
      return Diligent::USAGE_UNIFIED;
    case xiiGALResourceUsage::Sparse:
      return Diligent::USAGE_SPARSE;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::USAGE_IMMUTABLE;
}

XII_ALWAYS_INLINE Diligent::RESOURCE_STATE xiiDiligentTypeConversions::GetResourceState(xiiBitflags<xiiGALResourceStateFlags> e)
{
  if (e.IsNoFlagSet())
    return Diligent::RESOURCE_STATE_UNKNOWN;

  Diligent::RESOURCE_STATE resourceState = {};

  if (e.IsSet(xiiGALResourceStateFlags::Unknown))
    resourceState |= Diligent::RESOURCE_STATE_UNKNOWN;
  if (e.IsSet(xiiGALResourceStateFlags::Undefined))
    resourceState |= Diligent::RESOURCE_STATE_UNDEFINED;
  if (e.IsSet(xiiGALResourceStateFlags::VertexBuffer))
    resourceState |= Diligent::RESOURCE_STATE_VERTEX_BUFFER;
  if (e.IsSet(xiiGALResourceStateFlags::ConstantBuffer))
    resourceState |= Diligent::RESOURCE_STATE_CONSTANT_BUFFER;
  if (e.IsSet(xiiGALResourceStateFlags::IndexBuffer))
    resourceState |= Diligent::RESOURCE_STATE_INDEX_BUFFER;
  if (e.IsSet(xiiGALResourceStateFlags::RenderTarget))
    resourceState |= Diligent::RESOURCE_STATE_RENDER_TARGET;
  if (e.IsSet(xiiGALResourceStateFlags::UnorderedAccess))
    resourceState |= Diligent::RESOURCE_STATE_UNORDERED_ACCESS;
  if (e.IsSet(xiiGALResourceStateFlags::DepthWrite))
    resourceState |= Diligent::RESOURCE_STATE_DEPTH_WRITE;
  if (e.IsSet(xiiGALResourceStateFlags::DepthRead))
    resourceState |= Diligent::RESOURCE_STATE_DEPTH_READ;
  if (e.IsSet(xiiGALResourceStateFlags::ShaderResource))
    resourceState |= Diligent::RESOURCE_STATE_SHADER_RESOURCE;
  if (e.IsSet(xiiGALResourceStateFlags::StreamOut))
    resourceState |= Diligent::RESOURCE_STATE_STREAM_OUT;
  if (e.IsSet(xiiGALResourceStateFlags::IndirectArgument))
    resourceState |= Diligent::RESOURCE_STATE_INDIRECT_ARGUMENT;
  if (e.IsSet(xiiGALResourceStateFlags::CopyDestination))
    resourceState |= Diligent::RESOURCE_STATE_COPY_DEST;
  if (e.IsSet(xiiGALResourceStateFlags::CopySource))
    resourceState |= Diligent::RESOURCE_STATE_COPY_SOURCE;
  if (e.IsSet(xiiGALResourceStateFlags::ResolveDestination))
    resourceState |= Diligent::RESOURCE_STATE_RESOLVE_DEST;
  if (e.IsSet(xiiGALResourceStateFlags::ResolveSource))
    resourceState |= Diligent::RESOURCE_STATE_RESOLVE_SOURCE;
  if (e.IsSet(xiiGALResourceStateFlags::InputAttachment))
    resourceState |= Diligent::RESOURCE_STATE_INPUT_ATTACHMENT;
  if (e.IsSet(xiiGALResourceStateFlags::Present))
    resourceState |= Diligent::RESOURCE_STATE_PRESENT;
  if (e.IsSet(xiiGALResourceStateFlags::BuildAsRead))
    resourceState |= Diligent::RESOURCE_STATE_BUILD_AS_READ;
  if (e.IsSet(xiiGALResourceStateFlags::BuildAsWrite))
    resourceState |= Diligent::RESOURCE_STATE_BUILD_AS_WRITE;
  if (e.IsSet(xiiGALResourceStateFlags::RayTracing))
    resourceState |= Diligent::RESOURCE_STATE_RAY_TRACING;
  if (e.IsSet(xiiGALResourceStateFlags::Common))
    resourceState |= Diligent::RESOURCE_STATE_COMMON;
  if (e.IsSet(xiiGALResourceStateFlags::ShadingRate))
    resourceState |= Diligent::RESOURCE_STATE_SHADING_RATE;

  return Diligent::RESOURCE_STATE_UNKNOWN;
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiDiligentTypeConversions::GetResourceState(Diligent::RESOURCE_STATE e)
{
  if (e == Diligent::RESOURCE_STATE_UNKNOWN)
    return xiiBitflags<xiiGALResourceStateFlags>();

  xiiBitflags<xiiGALResourceStateFlags> resourceStateFlags;

  if (e & Diligent::RESOURCE_STATE_UNKNOWN)
    resourceStateFlags |= xiiGALResourceStateFlags::Unknown;
  if (e & Diligent::RESOURCE_STATE_UNDEFINED)
    resourceStateFlags |= xiiGALResourceStateFlags::Undefined;
  if (e & Diligent::RESOURCE_STATE_VERTEX_BUFFER)
    resourceStateFlags |= xiiGALResourceStateFlags::VertexBuffer;
  if (e & Diligent::RESOURCE_STATE_CONSTANT_BUFFER)
    resourceStateFlags |= xiiGALResourceStateFlags::ConstantBuffer;
  if (e & Diligent::RESOURCE_STATE_INDEX_BUFFER)
    resourceStateFlags |= xiiGALResourceStateFlags::IndexBuffer;
  if (e & Diligent::RESOURCE_STATE_RENDER_TARGET)
    resourceStateFlags |= xiiGALResourceStateFlags::RenderTarget;
  if (e & Diligent::RESOURCE_STATE_UNORDERED_ACCESS)
    resourceStateFlags |= xiiGALResourceStateFlags::UnorderedAccess;
  if (e & Diligent::RESOURCE_STATE_DEPTH_WRITE)
    resourceStateFlags |= xiiGALResourceStateFlags::DepthWrite;
  if (e & Diligent::RESOURCE_STATE_DEPTH_READ)
    resourceStateFlags |= xiiGALResourceStateFlags::DepthRead;
  if (e & Diligent::RESOURCE_STATE_SHADER_RESOURCE)
    resourceStateFlags |= xiiGALResourceStateFlags::ShaderResource;
  if (e & Diligent::RESOURCE_STATE_STREAM_OUT)
    resourceStateFlags |= xiiGALResourceStateFlags::StreamOut;
  if (e & Diligent::RESOURCE_STATE_INDIRECT_ARGUMENT)
    resourceStateFlags |= xiiGALResourceStateFlags::IndirectArgument;
  if (e & Diligent::RESOURCE_STATE_COPY_DEST)
    resourceStateFlags |= xiiGALResourceStateFlags::CopyDestination;
  if (e & Diligent::RESOURCE_STATE_COPY_SOURCE)
    resourceStateFlags |= xiiGALResourceStateFlags::CopySource;
  if (e & Diligent::RESOURCE_STATE_RESOLVE_DEST)
    resourceStateFlags |= xiiGALResourceStateFlags::ResolveDestination;
  if (e & Diligent::RESOURCE_STATE_RESOLVE_SOURCE)
    resourceStateFlags |= xiiGALResourceStateFlags::ResolveSource;
  if (e & Diligent::RESOURCE_STATE_INPUT_ATTACHMENT)
    resourceStateFlags |= xiiGALResourceStateFlags::InputAttachment;
  if (e & Diligent::RESOURCE_STATE_PRESENT)
    resourceStateFlags |= xiiGALResourceStateFlags::Present;
  if (e & Diligent::RESOURCE_STATE_BUILD_AS_READ)
    resourceStateFlags |= xiiGALResourceStateFlags::BuildAsRead;
  if (e & Diligent::RESOURCE_STATE_BUILD_AS_WRITE)
    resourceStateFlags |= xiiGALResourceStateFlags::BuildAsWrite;
  if (e & Diligent::RESOURCE_STATE_RAY_TRACING)
    resourceStateFlags |= xiiGALResourceStateFlags::RayTracing;
  if (e & Diligent::RESOURCE_STATE_COMMON)
    resourceStateFlags |= xiiGALResourceStateFlags::Common;
  if (e & Diligent::RESOURCE_STATE_SHADING_RATE)
    resourceStateFlags |= xiiGALResourceStateFlags::ShadingRate;

  return resourceStateFlags;
}
