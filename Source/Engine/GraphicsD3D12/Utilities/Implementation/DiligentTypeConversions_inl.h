
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

  return resourceState;
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

XII_ALWAYS_INLINE Diligent::RESOURCE_DIMENSION xiiDiligentTypeConversions::GetResourceDimension(xiiEnum<xiiGALResourceDimension> e)
{
  switch (e)
  {
    case xiiGALResourceDimension::Undefined:
      return Diligent::RESOURCE_DIM_UNDEFINED;
    case xiiGALResourceDimension::Buffer:
      return Diligent::RESOURCE_DIM_BUFFER;
    case xiiGALResourceDimension::Texture1D:
      return Diligent::RESOURCE_DIM_TEX_1D;
    case xiiGALResourceDimension::Texture1DArray:
      return Diligent::RESOURCE_DIM_TEX_1D_ARRAY;
    case xiiGALResourceDimension::Texture2D:
      return Diligent::RESOURCE_DIM_TEX_2D;
    case xiiGALResourceDimension::Texture2DArray:
      return Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
    case xiiGALResourceDimension::Texture3D:
      return Diligent::RESOURCE_DIM_TEX_3D;
    case xiiGALResourceDimension::TextureCube:
      return Diligent::RESOURCE_DIM_TEX_CUBE;
    case xiiGALResourceDimension::TextureCubeArray:
      return Diligent::RESOURCE_DIM_TEX_CUBE_ARRAY;
  }
  return Diligent::RESOURCE_DIM_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::TEXTURE_FORMAT xiiDiligentTypeConversions::GetTextureFormat(xiiEnum<xiiGALTextureFormat> e)
{
  switch (e)
  {
    case xiiGALTextureFormat::Unknown:
      return Diligent::TEX_FORMAT_UNKNOWN;
    case xiiGALTextureFormat::RGBA32Typeless:
      return Diligent::TEX_FORMAT_RGBA32_TYPELESS;
    case xiiGALTextureFormat::RGBA32Float:
      return Diligent::TEX_FORMAT_RGBA32_FLOAT;
    case xiiGALTextureFormat::RGBA32UInt:
      return Diligent::TEX_FORMAT_RGBA32_UINT;
    case xiiGALTextureFormat::RGBA32SInt:
      return Diligent::TEX_FORMAT_RGBA32_SINT;
    case xiiGALTextureFormat::RGB32Typeless:
      return Diligent::TEX_FORMAT_RGBA32_TYPELESS;
    case xiiGALTextureFormat::RGB32Float:
      return Diligent::TEX_FORMAT_RGBA32_FLOAT;
    case xiiGALTextureFormat::RGB32UInt:
      return Diligent::TEX_FORMAT_RGBA32_UINT;
    case xiiGALTextureFormat::RGB32SInt:
      return Diligent::TEX_FORMAT_RGBA32_SINT;
    case xiiGALTextureFormat::RGBA16Typeless:
      return Diligent::TEX_FORMAT_RGBA16_TYPELESS;
    case xiiGALTextureFormat::RGBA16Float:
      return Diligent::TEX_FORMAT_RGBA16_FLOAT;
    case xiiGALTextureFormat::RGBA16UNormalized:
      return Diligent::TEX_FORMAT_RGBA16_UNORM;
    case xiiGALTextureFormat::RGBA16UInt:
      return Diligent::TEX_FORMAT_RGBA16_UINT;
    case xiiGALTextureFormat::RGBA16SNormalized:
      return Diligent::TEX_FORMAT_RGBA16_SNORM;
    case xiiGALTextureFormat::RGBA16SInt:
      return Diligent::TEX_FORMAT_RGBA16_SINT;
    case xiiGALTextureFormat::RG32Typeless:
      return Diligent::TEX_FORMAT_RG32_TYPELESS;
    case xiiGALTextureFormat::RG32Float:
      return Diligent::TEX_FORMAT_RG32_FLOAT;
    case xiiGALTextureFormat::RG32UInt:
      return Diligent::TEX_FORMAT_RG32_UINT;
    case xiiGALTextureFormat::RG32SInt:
      return Diligent::TEX_FORMAT_RG32_SINT;
    case xiiGALTextureFormat::R32G8X24Typeless:
      return Diligent::TEX_FORMAT_R32G8X24_TYPELESS;
    case xiiGALTextureFormat::D32FloatS8X24UInt:
      return Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT;
    case xiiGALTextureFormat::R32FloatX8X24Typeless:
      return Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case xiiGALTextureFormat::X32TypelessG8X24UInt:
      return Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT;
    case xiiGALTextureFormat::RGB10A2Typeless:
      return Diligent::TEX_FORMAT_RGB10A2_TYPELESS;
    case xiiGALTextureFormat::RGB10A2UNormalized:
      return Diligent::TEX_FORMAT_RGB10A2_UNORM;
    case xiiGALTextureFormat::RGB10A2UInt:
      return Diligent::TEX_FORMAT_RGB10A2_UINT;
    case xiiGALTextureFormat::RG11B10Float:
      return Diligent::TEX_FORMAT_R11G11B10_FLOAT;
    case xiiGALTextureFormat::RGBA8Typeless:
      return Diligent::TEX_FORMAT_RGBA8_TYPELESS;
    case xiiGALTextureFormat::RGBA8UNormalized:
      return Diligent::TEX_FORMAT_RGBA8_UNORM;
    case xiiGALTextureFormat::RGBA8UNormalizedSRGB:
      return Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
    case xiiGALTextureFormat::RGBA8UInt:
      return Diligent::TEX_FORMAT_RGBA8_UINT;
    case xiiGALTextureFormat::RGBA8SNormalized:
      return Diligent::TEX_FORMAT_RGBA8_SNORM;
    case xiiGALTextureFormat::RGBA8SInt:
      return Diligent::TEX_FORMAT_RGBA8_SINT;
    case xiiGALTextureFormat::RG16Typeless:
      return Diligent::TEX_FORMAT_RG16_TYPELESS;
    case xiiGALTextureFormat::RG16Float:
      return Diligent::TEX_FORMAT_RG16_FLOAT;
    case xiiGALTextureFormat::RG16UNormalized:
      return Diligent::TEX_FORMAT_RG16_UNORM;
    case xiiGALTextureFormat::RG16UInt:
      return Diligent::TEX_FORMAT_RG16_UINT;
    case xiiGALTextureFormat::RG16SNormalized:
      return Diligent::TEX_FORMAT_RG16_SNORM;
    case xiiGALTextureFormat::RG16SInt:
      return Diligent::TEX_FORMAT_RG16_SINT;
    case xiiGALTextureFormat::R32Typeless:
      return Diligent::TEX_FORMAT_R32_TYPELESS;
    case xiiGALTextureFormat::D32Float:
      return Diligent::TEX_FORMAT_D32_FLOAT;
    case xiiGALTextureFormat::R32Float:
      return Diligent::TEX_FORMAT_R32_FLOAT;
    case xiiGALTextureFormat::R32UInt:
      return Diligent::TEX_FORMAT_R32_UINT;
    case xiiGALTextureFormat::R32SInt:
      return Diligent::TEX_FORMAT_R32_SINT;
    case xiiGALTextureFormat::R24G8Typeless:
      return Diligent::TEX_FORMAT_R24G8_TYPELESS;
    case xiiGALTextureFormat::D24UNormalizedS8UInt:
      return Diligent::TEX_FORMAT_D24_UNORM_S8_UINT;
    case xiiGALTextureFormat::R24UNormalizedX8Typeless:
      return Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS;
    case xiiGALTextureFormat::X24TypelessG8UInt:
      return Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT;
    case xiiGALTextureFormat::RG8Typeless:
      return Diligent::TEX_FORMAT_RG8_TYPELESS;
    case xiiGALTextureFormat::RG8UNormalized:
      return Diligent::TEX_FORMAT_RG8_UNORM;
    case xiiGALTextureFormat::RG8UInt:
      return Diligent::TEX_FORMAT_RG8_UINT;
    case xiiGALTextureFormat::RG8SNormalized:
      return Diligent::TEX_FORMAT_RG8_SNORM;
    case xiiGALTextureFormat::RG8SInt:
      return Diligent::TEX_FORMAT_RG8_SINT;
    case xiiGALTextureFormat::R16Typeless:
      return Diligent::TEX_FORMAT_R16_TYPELESS;
    case xiiGALTextureFormat::R16Float:
      return Diligent::TEX_FORMAT_R16_FLOAT;
    case xiiGALTextureFormat::D16UNormalized:
      return Diligent::TEX_FORMAT_D16_UNORM;
    case xiiGALTextureFormat::R16UNormalized:
      return Diligent::TEX_FORMAT_R16_UNORM;
    case xiiGALTextureFormat::R16UInt:
      return Diligent::TEX_FORMAT_R16_UINT;
    case xiiGALTextureFormat::R16SNormalized:
      return Diligent::TEX_FORMAT_R16_SNORM;
    case xiiGALTextureFormat::R16SInt:
      return Diligent::TEX_FORMAT_R16_SINT;
    case xiiGALTextureFormat::R8Typeless:
      return Diligent::TEX_FORMAT_R8_TYPELESS;
    case xiiGALTextureFormat::R8UNormalized:
      return Diligent::TEX_FORMAT_R8_UNORM;
    case xiiGALTextureFormat::R8UInt:
      return Diligent::TEX_FORMAT_R8_UINT;
    case xiiGALTextureFormat::R8SNormalized:
      return Diligent::TEX_FORMAT_R8_SNORM;
    case xiiGALTextureFormat::R8SInt:
      return Diligent::TEX_FORMAT_R8_SINT;
    case xiiGALTextureFormat::A8UNormalized:
      return Diligent::TEX_FORMAT_A8_UNORM;
    case xiiGALTextureFormat::R1UNormalized:
      return Diligent::TEX_FORMAT_R1_UNORM;
    case xiiGALTextureFormat::RGB9E5SharedExponent:
      return Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP;
    case xiiGALTextureFormat::RG8BG8UNormalized:
      return Diligent::TEX_FORMAT_RG8_B8G8_UNORM;
    case xiiGALTextureFormat::GR8GB8UNormalized:
      return Diligent::TEX_FORMAT_G8R8_G8B8_UNORM;
    case xiiGALTextureFormat::BC1Typeless:
      return Diligent::TEX_FORMAT_BC1_TYPELESS;
    case xiiGALTextureFormat::BC1UNormalized:
      return Diligent::TEX_FORMAT_BC1_UNORM;
    case xiiGALTextureFormat::BC1UNormalizedSRGB:
      return Diligent::TEX_FORMAT_BC1_UNORM_SRGB;
    case xiiGALTextureFormat::BC2Typeless:
      return Diligent::TEX_FORMAT_BC2_TYPELESS;
    case xiiGALTextureFormat::BC2UNormalized:
      return Diligent::TEX_FORMAT_BC2_UNORM;
    case xiiGALTextureFormat::BC2UNormalizedSRGB:
      return Diligent::TEX_FORMAT_BC2_UNORM_SRGB;
    case xiiGALTextureFormat::BC3Typeless:
      return Diligent::TEX_FORMAT_BC3_TYPELESS;
    case xiiGALTextureFormat::BC3UNormalized:
      return Diligent::TEX_FORMAT_BC3_UNORM;
    case xiiGALTextureFormat::BC3UNormalizedSRGB:
      return Diligent::TEX_FORMAT_BC3_UNORM_SRGB;
    case xiiGALTextureFormat::BC4Typeless:
      return Diligent::TEX_FORMAT_BC4_TYPELESS;
    case xiiGALTextureFormat::BC4UNormalized:
      return Diligent::TEX_FORMAT_BC4_UNORM;
    case xiiGALTextureFormat::BC4SNormalized:
      return Diligent::TEX_FORMAT_BC4_SNORM;
    case xiiGALTextureFormat::BC5Typeless:
      return Diligent::TEX_FORMAT_BC5_TYPELESS;
    case xiiGALTextureFormat::BC5UNormalized:
      return Diligent::TEX_FORMAT_BC5_UNORM;
    case xiiGALTextureFormat::BC5SNormalized:
      return Diligent::TEX_FORMAT_BC5_SNORM;
    case xiiGALTextureFormat::B5G6R5UNormalized:
      return Diligent::TEX_FORMAT_B5G6R5_UNORM;
    case xiiGALTextureFormat::B5G5R5A1UNormalized:
      return Diligent::TEX_FORMAT_B5G5R5A1_UNORM;
    case xiiGALTextureFormat::BGRA8UNormalized:
      return Diligent::TEX_FORMAT_BGRA8_UNORM;
    case xiiGALTextureFormat::BGRX8UNormalized:
      return Diligent::TEX_FORMAT_BGRX8_UNORM;
    case xiiGALTextureFormat::R10G10B10XRBiasA2UNormalized:
      return Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
    case xiiGALTextureFormat::BGRA8Typeless:
      return Diligent::TEX_FORMAT_BGRA8_TYPELESS;
    case xiiGALTextureFormat::BGRA8UNormalizedSRGB:
      return Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB;
    case xiiGALTextureFormat::BGRX8Typeless:
      return Diligent::TEX_FORMAT_BGRX8_TYPELESS;
    case xiiGALTextureFormat::BGRX8UNormalizedSRGB:
      return Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB;
    case xiiGALTextureFormat::BC6HTypeless:
      return Diligent::TEX_FORMAT_BC6H_TYPELESS;
    case xiiGALTextureFormat::BC6HUF16:
      return Diligent::TEX_FORMAT_BC6H_UF16;
    case xiiGALTextureFormat::BC6HSF16:
      return Diligent::TEX_FORMAT_BC6H_SF16;
    case xiiGALTextureFormat::BC7Typeless:
      return Diligent::TEX_FORMAT_BC7_TYPELESS;
    case xiiGALTextureFormat::BC7UNormalized:
      return Diligent::TEX_FORMAT_BC7_UNORM;
    case xiiGALTextureFormat::BC7UNormalizedSRGB:
      return Diligent::TEX_FORMAT_BC7_UNORM_SRGB;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::TEX_FORMAT_UNKNOWN;
}

XII_ALWAYS_INLINE xiiEnum<xiiGALTextureFormat> xiiDiligentTypeConversions::GetGALTextureFormat(Diligent::TEXTURE_FORMAT e)
{
  switch (e)
  {
    case Diligent::TEX_FORMAT_UNKNOWN:
      return xiiGALTextureFormat::Unknown;
    case Diligent::TEX_FORMAT_RGBA32_TYPELESS:
      return xiiGALTextureFormat::RGBA32Typeless;
    case Diligent::TEX_FORMAT_RGBA32_FLOAT:
      return xiiGALTextureFormat::RGBA32Float;
    case Diligent::TEX_FORMAT_RGBA32_UINT:
      return xiiGALTextureFormat::RGBA32UInt;
    case Diligent::TEX_FORMAT_RGBA32_SINT:
      return xiiGALTextureFormat::RGBA32SInt;
    case Diligent::TEX_FORMAT_RGB32_TYPELESS:
      return xiiGALTextureFormat::RGB32Typeless;
    case Diligent::TEX_FORMAT_RGB32_FLOAT:
      return xiiGALTextureFormat::RGB32Float;
    case Diligent::TEX_FORMAT_RGB32_UINT:
      return xiiGALTextureFormat::RGB32UInt;
    case Diligent::TEX_FORMAT_RGB32_SINT:
      return xiiGALTextureFormat::RGB32SInt;
    case Diligent::TEX_FORMAT_RGBA16_TYPELESS:
      return xiiGALTextureFormat::RGBA16Typeless;
    case Diligent::TEX_FORMAT_RGBA16_FLOAT:
      return xiiGALTextureFormat::RGBA16Float;
    case Diligent::TEX_FORMAT_RGBA16_UNORM:
      return xiiGALTextureFormat::RGBA16UNormalized;
    case Diligent::TEX_FORMAT_RGBA16_UINT:
      return xiiGALTextureFormat::RGBA16UInt;
    case Diligent::TEX_FORMAT_RGBA16_SNORM:
      return xiiGALTextureFormat::RGBA16SNormalized;
    case Diligent::TEX_FORMAT_RGBA16_SINT:
      return xiiGALTextureFormat::RGBA16SInt;
    case Diligent::TEX_FORMAT_RG32_TYPELESS:
      return xiiGALTextureFormat::RG32Typeless;
    case Diligent::TEX_FORMAT_RG32_FLOAT:
      return xiiGALTextureFormat::RG32Float;
    case Diligent::TEX_FORMAT_RG32_UINT:
      return xiiGALTextureFormat::RG32UInt;
    case Diligent::TEX_FORMAT_RG32_SINT:
      return xiiGALTextureFormat::RG32SInt;
    case Diligent::TEX_FORMAT_R32G8X24_TYPELESS:
      return xiiGALTextureFormat::R32G8X24Typeless;
    case Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT:
      return xiiGALTextureFormat::D32FloatS8X24UInt;
    case Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      return xiiGALTextureFormat::R32FloatX8X24Typeless;
    case Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT:
      return xiiGALTextureFormat::X32TypelessG8X24UInt;
    case Diligent::TEX_FORMAT_RGB10A2_TYPELESS:
      return xiiGALTextureFormat::RGB10A2Typeless;
    case Diligent::TEX_FORMAT_RGB10A2_UNORM:
      return xiiGALTextureFormat::RGB10A2UNormalized;
    case Diligent::TEX_FORMAT_RGB10A2_UINT:
      return xiiGALTextureFormat::RGB10A2UInt;
    case Diligent::TEX_FORMAT_R11G11B10_FLOAT:
      return xiiGALTextureFormat::RG11B10Float;
    case Diligent::TEX_FORMAT_RGBA8_TYPELESS:
      return xiiGALTextureFormat::RGBA8Typeless;
    case Diligent::TEX_FORMAT_RGBA8_UNORM:
      return xiiGALTextureFormat::RGBA8UNormalized;
    case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
      return xiiGALTextureFormat::RGBA8UNormalizedSRGB;
    case Diligent::TEX_FORMAT_RGBA8_UINT:
      return xiiGALTextureFormat::RGBA8UInt;
    case Diligent::TEX_FORMAT_RGBA8_SNORM:
      return xiiGALTextureFormat::RGBA8SNormalized;
    case Diligent::TEX_FORMAT_RGBA8_SINT:
      return xiiGALTextureFormat::RGBA8SInt;
    case Diligent::TEX_FORMAT_RG16_TYPELESS:
      return xiiGALTextureFormat::RG16Typeless;
    case Diligent::TEX_FORMAT_RG16_FLOAT:
      return xiiGALTextureFormat::RG16Float;
    case Diligent::TEX_FORMAT_RG16_UNORM:
      return xiiGALTextureFormat::RG16UNormalized;
    case Diligent::TEX_FORMAT_RG16_UINT:
      return xiiGALTextureFormat::RG16UInt;
    case Diligent::TEX_FORMAT_RG16_SNORM:
      return xiiGALTextureFormat::RG16SNormalized;
    case Diligent::TEX_FORMAT_RG16_SINT:
      return xiiGALTextureFormat::RG16SInt;
    case Diligent::TEX_FORMAT_R32_TYPELESS:
      return xiiGALTextureFormat::R32Typeless;
    case Diligent::TEX_FORMAT_D32_FLOAT:
      return xiiGALTextureFormat::D32Float;
    case Diligent::TEX_FORMAT_R32_FLOAT:
      return xiiGALTextureFormat::R32Float;
    case Diligent::TEX_FORMAT_R32_UINT:
      return xiiGALTextureFormat::R32UInt;
    case Diligent::TEX_FORMAT_R32_SINT:
      return xiiGALTextureFormat::R32SInt;
    case Diligent::TEX_FORMAT_R24G8_TYPELESS:
      return xiiGALTextureFormat::R24G8Typeless;
    case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
      return xiiGALTextureFormat::D24UNormalizedS8UInt;
    case Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS:
      return xiiGALTextureFormat::R24UNormalizedX8Typeless;
    case Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT:
      return xiiGALTextureFormat::X24TypelessG8UInt;
    case Diligent::TEX_FORMAT_RG8_TYPELESS:
      return xiiGALTextureFormat::RG8Typeless;
    case Diligent::TEX_FORMAT_RG8_UNORM:
      return xiiGALTextureFormat::RG8UNormalized;
    case Diligent::TEX_FORMAT_RG8_UINT:
      return xiiGALTextureFormat::RG8UInt;
    case Diligent::TEX_FORMAT_RG8_SNORM:
      return xiiGALTextureFormat::RG8SNormalized;
    case Diligent::TEX_FORMAT_RG8_SINT:
      return xiiGALTextureFormat::RG8SInt;
    case Diligent::TEX_FORMAT_R16_TYPELESS:
      return xiiGALTextureFormat::R16Typeless;
    case Diligent::TEX_FORMAT_R16_FLOAT:
      return xiiGALTextureFormat::R16Float;
    case Diligent::TEX_FORMAT_D16_UNORM:
      return xiiGALTextureFormat::D16UNormalized;
    case Diligent::TEX_FORMAT_R16_UNORM:
      return xiiGALTextureFormat::R16UNormalized;
    case Diligent::TEX_FORMAT_R16_UINT:
      return xiiGALTextureFormat::R16UInt;
    case Diligent::TEX_FORMAT_R16_SNORM:
      return xiiGALTextureFormat::R16SNormalized;
    case Diligent::TEX_FORMAT_R16_SINT:
      return xiiGALTextureFormat::R16SInt;
    case Diligent::TEX_FORMAT_R8_TYPELESS:
      return xiiGALTextureFormat::R8Typeless;
    case Diligent::TEX_FORMAT_R8_UNORM:
      return xiiGALTextureFormat::R8UNormalized;
    case Diligent::TEX_FORMAT_R8_UINT:
      return xiiGALTextureFormat::R8UInt;
    case Diligent::TEX_FORMAT_R8_SNORM:
      return xiiGALTextureFormat::R8SNormalized;
    case Diligent::TEX_FORMAT_R8_SINT:
      return xiiGALTextureFormat::R8SInt;
    case Diligent::TEX_FORMAT_A8_UNORM:
      return xiiGALTextureFormat::A8UNormalized;
    case Diligent::TEX_FORMAT_R1_UNORM:
      return xiiGALTextureFormat::R1UNormalized;
    case Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP:
      return xiiGALTextureFormat::RGB9E5SharedExponent;
    case Diligent::TEX_FORMAT_RG8_B8G8_UNORM:
      return xiiGALTextureFormat::RG8BG8UNormalized;
    case Diligent::TEX_FORMAT_G8R8_G8B8_UNORM:
      return xiiGALTextureFormat::GR8GB8UNormalized;
    case Diligent::TEX_FORMAT_BC1_TYPELESS:
      return xiiGALTextureFormat::BC1Typeless;
    case Diligent::TEX_FORMAT_BC1_UNORM:
      return xiiGALTextureFormat::BC1UNormalized;
    case Diligent::TEX_FORMAT_BC1_UNORM_SRGB:
      return xiiGALTextureFormat::BC1UNormalizedSRGB;
    case Diligent::TEX_FORMAT_BC2_TYPELESS:
      return xiiGALTextureFormat::BC2Typeless;
    case Diligent::TEX_FORMAT_BC2_UNORM:
      return xiiGALTextureFormat::BC2UNormalized;
    case Diligent::TEX_FORMAT_BC2_UNORM_SRGB:
      return xiiGALTextureFormat::BC2UNormalizedSRGB;
    case Diligent::TEX_FORMAT_BC3_TYPELESS:
      return xiiGALTextureFormat::BC3Typeless;
    case Diligent::TEX_FORMAT_BC3_UNORM:
      return xiiGALTextureFormat::BC3UNormalized;
    case Diligent::TEX_FORMAT_BC3_UNORM_SRGB:
      return xiiGALTextureFormat::BC3UNormalizedSRGB;
    case Diligent::TEX_FORMAT_BC4_TYPELESS:
      return xiiGALTextureFormat::BC4Typeless;
    case Diligent::TEX_FORMAT_BC4_UNORM:
      return xiiGALTextureFormat::BC4UNormalized;
    case Diligent::TEX_FORMAT_BC4_SNORM:
      return xiiGALTextureFormat::BC4SNormalized;
    case Diligent::TEX_FORMAT_BC5_TYPELESS:
      return xiiGALTextureFormat::BC5Typeless;
    case Diligent::TEX_FORMAT_BC5_UNORM:
      return xiiGALTextureFormat::BC5UNormalized;
    case Diligent::TEX_FORMAT_BC5_SNORM:
      return xiiGALTextureFormat::BC5SNormalized;
    case Diligent::TEX_FORMAT_B5G6R5_UNORM:
      return xiiGALTextureFormat::B5G6R5UNormalized;
    case Diligent::TEX_FORMAT_B5G5R5A1_UNORM:
      return xiiGALTextureFormat::B5G5R5A1UNormalized;
    case Diligent::TEX_FORMAT_BGRA8_UNORM:
      return xiiGALTextureFormat::BGRA8UNormalized;
    case Diligent::TEX_FORMAT_BGRX8_UNORM:
      return xiiGALTextureFormat::BGRX8UNormalized;
    case Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
      return xiiGALTextureFormat::R10G10B10XRBiasA2UNormalized;
    case Diligent::TEX_FORMAT_BGRA8_TYPELESS:
      return xiiGALTextureFormat::BGRA8Typeless;
    case Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB:
      return xiiGALTextureFormat::BGRA8UNormalizedSRGB;
    case Diligent::TEX_FORMAT_BGRX8_TYPELESS:
      return xiiGALTextureFormat::BGRX8Typeless;
    case Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB:
      return xiiGALTextureFormat::BGRX8UNormalizedSRGB;
    case Diligent::TEX_FORMAT_BC6H_TYPELESS:
      return xiiGALTextureFormat::BC6HTypeless;
    case Diligent::TEX_FORMAT_BC6H_UF16:
      return xiiGALTextureFormat::BC6HUF16;
    case Diligent::TEX_FORMAT_BC6H_SF16:
      return xiiGALTextureFormat::BC6HSF16;
    case Diligent::TEX_FORMAT_BC7_TYPELESS:
      return xiiGALTextureFormat::BC7Typeless;
    case Diligent::TEX_FORMAT_BC7_UNORM:
      return xiiGALTextureFormat::BC7UNormalized;
    case Diligent::TEX_FORMAT_BC7_UNORM_SRGB:
      return xiiGALTextureFormat::BC7UNormalizedSRGB;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALTextureFormat::Unknown;
}

XII_ALWAYS_INLINE Diligent::MISC_TEXTURE_FLAGS xiiDiligentTypeConversions::GetMiscTextureFlags(xiiBitflags<xiiGALMiscTextureFlags> e)
{
  if (e.IsNoFlagSet())
    return Diligent::MISC_TEXTURE_FLAG_NONE;

  Diligent::MISC_TEXTURE_FLAGS miscTextureFlags = {};

  if (e.IsSet(xiiGALMiscTextureFlags::None))
    miscTextureFlags |= Diligent::MISC_TEXTURE_FLAG_NONE;
  if (e.IsSet(xiiGALMiscTextureFlags::GenerateMips))
    miscTextureFlags |= Diligent::MISC_TEXTURE_FLAG_GENERATE_MIPS;
  if (e.IsSet(xiiGALMiscTextureFlags::Memoryless))
    miscTextureFlags |= Diligent::MISC_TEXTURE_FLAG_MEMORYLESS;
  if (e.IsSet(xiiGALMiscTextureFlags::SparseAlias))
    miscTextureFlags |= Diligent::MISC_TEXTURE_FLAG_SPARSE_ALIASING;
  if (e.IsSet(xiiGALMiscTextureFlags::Subsampled))
    miscTextureFlags |= Diligent::MISC_TEXTURE_FLAG_SUBSAMPLED;

  return miscTextureFlags;
}

XII_ALWAYS_INLINE Diligent::FENCE_TYPE xiiDiligentTypeConversions::GetFenceType(xiiEnum<xiiGALFenceType> e)
{
  switch (e)
  {
    case xiiGALFenceType::CpuWaitOnly:
      return Diligent::FENCE_TYPE_CPU_WAIT_ONLY;
    case xiiGALFenceType::General:
      return Diligent::FENCE_TYPE_GENERAL;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::FENCE_TYPE_CPU_WAIT_ONLY;
}

XII_ALWAYS_INLINE Diligent::QUERY_TYPE xiiDiligentTypeConversions::GetQueryType(xiiEnum<xiiGALQueryType> e)
{
  switch (e)
  {
    case xiiGALQueryType::Undefined:
      return Diligent::QUERY_TYPE_UNDEFINED;
    case xiiGALQueryType::Occlusion:
      return Diligent::QUERY_TYPE_OCCLUSION;
    case xiiGALQueryType::BinaryOcclusion:
      return Diligent::QUERY_TYPE_BINARY_OCCLUSION;
    case xiiGALQueryType::Timestamp:
      return Diligent::QUERY_TYPE_TIMESTAMP;
    case xiiGALQueryType::PipelineStatistics:
      return Diligent::QUERY_TYPE_PIPELINE_STATISTICS;
    case xiiGALQueryType::Duration:
      return Diligent::QUERY_TYPE_DURATION;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::QUERY_TYPE_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::ATTACHMENT_LOAD_OP xiiDiligentTypeConversions::GetLoadOperation(xiiEnum<xiiGALAttachmentLoadOperation> e)
{
  switch (e)
  {
    case xiiGALAttachmentLoadOperation::Load:
      return Diligent::ATTACHMENT_LOAD_OP_LOAD;
    case xiiGALAttachmentLoadOperation::Clear:
      return Diligent::ATTACHMENT_LOAD_OP_CLEAR;
    case xiiGALAttachmentLoadOperation::Discard:
      return Diligent::ATTACHMENT_LOAD_OP_DISCARD;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::ATTACHMENT_LOAD_OP_LOAD;
}

XII_ALWAYS_INLINE Diligent::ATTACHMENT_STORE_OP xiiDiligentTypeConversions::GetStoreOperation(xiiEnum<xiiGALAttachmentStoreOperation> e)
{
  switch (e)
  {
    case xiiGALAttachmentStoreOperation::Store:
      return Diligent::ATTACHMENT_STORE_OP_STORE;
    case xiiGALAttachmentStoreOperation::Discard:
      return Diligent::ATTACHMENT_STORE_OP_DISCARD;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::ATTACHMENT_STORE_OP_STORE;
}

XII_ALWAYS_INLINE Diligent::BUFFER_VIEW_TYPE xiiDiligentTypeConversions::GetBufferViewType(xiiEnum<xiiGALBufferViewType> e)
{
  switch (e)
  {
    case xiiGALBufferViewType::Undefined:
      return Diligent::BUFFER_VIEW_UNDEFINED;
    case xiiGALBufferViewType::ShaderResource:
      return Diligent::BUFFER_VIEW_SHADER_RESOURCE;
    case xiiGALBufferViewType::UnorderedAccess:
      return Diligent::BUFFER_VIEW_UNORDERED_ACCESS;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::BUFFER_VIEW_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::TEXTURE_VIEW_TYPE xiiDiligentTypeConversions::GetTextureViewType(xiiEnum<xiiGALTextureViewType> e)
{
  switch (e)
  {
    case xiiGALTextureViewType::Undefined:
      return Diligent::TEXTURE_VIEW_UNDEFINED;
    case xiiGALTextureViewType::ShaderResource:
      return Diligent::TEXTURE_VIEW_SHADER_RESOURCE;
    case xiiGALTextureViewType::RenderTarget:
      return Diligent::TEXTURE_VIEW_RENDER_TARGET;
    case xiiGALTextureViewType::DepthStencil:
      return Diligent::TEXTURE_VIEW_DEPTH_STENCIL;
    case xiiGALTextureViewType::ReadOnlyDepthStencil:
      return Diligent::TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL;
    case xiiGALTextureViewType::UnorderedAccess:
      return Diligent::TEXTURE_VIEW_UNORDERED_ACCESS;
    case xiiGALTextureViewType::ShadingRate:
      return Diligent::TEXTURE_VIEW_SHADING_RATE;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::TEXTURE_VIEW_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::TEXTURE_ADDRESS_MODE xiiDiligentTypeConversions::GetTextureAddress(xiiEnum<xiiGALTextureAddressMode> e)
{
  switch (e)
  {
    case xiiGALTextureAddressMode::Unknown:
      return Diligent::TEXTURE_ADDRESS_UNKNOWN;
    case xiiGALTextureAddressMode::Wrap:
      return Diligent::TEXTURE_ADDRESS_WRAP;
    case xiiGALTextureAddressMode::Mirror:
      return Diligent::TEXTURE_ADDRESS_MIRROR;
    case xiiGALTextureAddressMode::Clamp:
      return Diligent::TEXTURE_ADDRESS_CLAMP;
    case xiiGALTextureAddressMode::Border:
      return Diligent::TEXTURE_ADDRESS_BORDER;
    case xiiGALTextureAddressMode::MirrorOnce:
      return Diligent::TEXTURE_ADDRESS_MIRROR_ONCE;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::TEXTURE_ADDRESS_UNKNOWN;
}

XII_ALWAYS_INLINE Diligent::VALUE_TYPE xiiDiligentTypeConversions::GetValueType(xiiEnum<xiiGALValueType> e)
{
  switch (e)
  {
    case xiiGALValueType::Undefined:
      return Diligent::VT_UNDEFINED;
    case xiiGALValueType::Int8:
      return Diligent::VT_INT8;
    case xiiGALValueType::Int16:
      return Diligent::VT_INT16;
    case xiiGALValueType::Int32:
      return Diligent::VT_INT32;
    case xiiGALValueType::UInt8:
      return Diligent::VT_UINT8;
    case xiiGALValueType::UInt16:
      return Diligent::VT_UINT16;
    case xiiGALValueType::UInt32:
      return Diligent::VT_UINT32;
    case xiiGALValueType::Float16:
      return Diligent::VT_FLOAT16;
    case xiiGALValueType::Float32:
      return Diligent::VT_FLOAT32;
    case xiiGALValueType::Float64:
      return Diligent::VT_FLOAT64;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::VT_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::UAV_ACCESS_FLAG xiiDiligentTypeConversions::GetUAVAccessFlags(xiiBitflags<xiiGALUnorderedAccessViewFlags> e)
{
  if (e.IsNoFlagSet())
    return Diligent::UAV_ACCESS_FLAG_READ;

  Diligent::UAV_ACCESS_FLAG uavAccessFlags = {};

  if (e.IsSet(xiiGALUnorderedAccessViewFlags::Read))
    uavAccessFlags |= Diligent::UAV_ACCESS_FLAG_READ;
  if (e.IsSet(xiiGALUnorderedAccessViewFlags::Write))
    uavAccessFlags |= Diligent::UAV_ACCESS_FLAG_WRITE;

  return uavAccessFlags;
}

XII_ALWAYS_INLINE Diligent::TEXTURE_VIEW_FLAGS xiiDiligentTypeConversions::GetTextureViewFlags(xiiBitflags<xiiGALTextureViewFlags> e)
{
  if (e.IsNoFlagSet())
    return Diligent::TEXTURE_VIEW_FLAG_NONE;

  Diligent::TEXTURE_VIEW_FLAGS textureViewFlags = {};

  if (e.IsSet(xiiGALTextureViewFlags::AllowMipGeneration))
    textureViewFlags |= Diligent::TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION;

  return textureViewFlags;
}

XII_ALWAYS_INLINE Diligent::TEXTURE_COMPONENT_SWIZZLE xiiDiligentTypeConversions::GetComponentSwizzle(xiiEnum<xiiGALTextureComponentSwizzle> e)
{
  switch (e)
  {
    case xiiGALTextureComponentSwizzle::Identity:
      return Diligent::TEXTURE_COMPONENT_SWIZZLE_IDENTITY;
    case xiiGALTextureComponentSwizzle::Zero:
      return Diligent::TEXTURE_COMPONENT_SWIZZLE_ZERO;
    case xiiGALTextureComponentSwizzle::One:
      return Diligent::TEXTURE_COMPONENT_SWIZZLE_ONE;
    case xiiGALTextureComponentSwizzle::R:
      return Diligent::TEXTURE_COMPONENT_SWIZZLE_R;
    case xiiGALTextureComponentSwizzle::G:
      return Diligent::TEXTURE_COMPONENT_SWIZZLE_G;
    case xiiGALTextureComponentSwizzle::B:
      return Diligent::TEXTURE_COMPONENT_SWIZZLE_B;
    case xiiGALTextureComponentSwizzle::A:
      return Diligent::TEXTURE_COMPONENT_SWIZZLE_A;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::TEXTURE_COMPONENT_SWIZZLE_IDENTITY;
}

XII_ALWAYS_INLINE Diligent::RAYTRACING_BUILD_AS_FLAGS xiiDiligentTypeConversions::GetRayTracingBuildASFlags(xiiBitflags<xiiGALRayTracingBuildASFlags> e)
{
  if (e.IsNoFlagSet())
    return Diligent::RAYTRACING_BUILD_AS_NONE;

  Diligent::RAYTRACING_BUILD_AS_FLAGS rayTracingBuildASFlags = {};

  if (e.IsSet(xiiGALRayTracingBuildASFlags::AllowUpdate))
    rayTracingBuildASFlags |= Diligent::RAYTRACING_BUILD_AS_NONE;
  if (e.IsSet(xiiGALRayTracingBuildASFlags::AllowCompaction))
    rayTracingBuildASFlags |= Diligent::RAYTRACING_BUILD_AS_ALLOW_COMPACTION;
  if (e.IsSet(xiiGALRayTracingBuildASFlags::PreferFastTrace))
    rayTracingBuildASFlags |= Diligent::RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
  if (e.IsSet(xiiGALRayTracingBuildASFlags::PreferFastBuild))
    rayTracingBuildASFlags |= Diligent::RAYTRACING_BUILD_AS_PREFER_FAST_BUILD;
  if (e.IsSet(xiiGALRayTracingBuildASFlags::LowMemory))
    rayTracingBuildASFlags |= Diligent::RAYTRACING_BUILD_AS_LOW_MEMORY;

  return rayTracingBuildASFlags;
}

XII_ALWAYS_INLINE Diligent::HIT_GROUP_BINDING_MODE xiiDiligentTypeConversions::GetHitGroupBindingMode(xiiEnum<xiiGALHitGroupBindingMode> e)
{
  switch (e)
  {
    case xiiGALHitGroupBindingMode::PerGeometry:
      return Diligent::HIT_GROUP_BINDING_MODE_PER_GEOMETRY;
    case xiiGALHitGroupBindingMode::PerInstance:
      return Diligent::HIT_GROUP_BINDING_MODE_PER_INSTANCE;
    case xiiGALHitGroupBindingMode::PerTopLevelAccelerationStructure:
      return Diligent::HIT_GROUP_BINDING_MODE_PER_TLAS;
    case xiiGALHitGroupBindingMode::UserDefined:
      return Diligent::HIT_GROUP_BINDING_MODE_USER_DEFINED;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::HIT_GROUP_BINDING_MODE_PER_GEOMETRY;
}

XII_ALWAYS_INLINE xiiEnum<xiiGALHitGroupBindingMode> xiiDiligentTypeConversions::GetGALHitGroupBindingMode(Diligent::HIT_GROUP_BINDING_MODE e)
{
  switch (e)
  {
    case Diligent::HIT_GROUP_BINDING_MODE_PER_GEOMETRY:
      return xiiGALHitGroupBindingMode::PerGeometry;
    case Diligent::HIT_GROUP_BINDING_MODE_PER_INSTANCE:
      return xiiGALHitGroupBindingMode::PerInstance;
    case Diligent::HIT_GROUP_BINDING_MODE_PER_TLAS:
      return xiiGALHitGroupBindingMode::PerTopLevelAccelerationStructure;
    case Diligent::HIT_GROUP_BINDING_MODE_USER_DEFINED:
      return xiiGALHitGroupBindingMode::UserDefined;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiEnum<xiiGALHitGroupBindingMode>();
}

XII_ALWAYS_INLINE Diligent::ADAPTER_TYPE xiiDiligentTypeConversions::GetAdapterType(xiiEnum<xiiGALDeviceAdapterType> e)
{
  switch (e)
  {
    case xiiGALDeviceAdapterType::Unknown:
      return Diligent::ADAPTER_TYPE_UNKNOWN;
    case xiiGALDeviceAdapterType::Software:
      return Diligent::ADAPTER_TYPE_SOFTWARE;
    case xiiGALDeviceAdapterType::Integrated:
      return Diligent::ADAPTER_TYPE_INTEGRATED;
    case xiiGALDeviceAdapterType::Discrete:
      return Diligent::ADAPTER_TYPE_DISCRETE;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::ADAPTER_TYPE_UNKNOWN;
}

XII_ALWAYS_INLINE xiiEnum<xiiGALDeviceAdapterType> xiiDiligentTypeConversions::GetGALAdapterType(Diligent::ADAPTER_TYPE e)
{
  switch (e)
  {
    case Diligent::ADAPTER_TYPE_UNKNOWN:
      return xiiGALDeviceAdapterType::Unknown;
    case Diligent::ADAPTER_TYPE_SOFTWARE:
      return xiiGALDeviceAdapterType::Software;
    case Diligent::ADAPTER_TYPE_INTEGRATED:
      return xiiGALDeviceAdapterType::Integrated;
    case Diligent::ADAPTER_TYPE_DISCRETE:
      return xiiGALDeviceAdapterType::Discrete;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiEnum<xiiGALDeviceAdapterType>();
}

XII_ALWAYS_INLINE Diligent::DEVICE_FEATURE_STATE xiiDiligentTypeConversions::GetDeviceFeatureState(xiiEnum<xiiGALDeviceFeatureState> e)
{
  switch (e)
  {
    case xiiGALDeviceFeatureState::Disabled:
      return Diligent::DEVICE_FEATURE_STATE_DISABLED;
    case xiiGALDeviceFeatureState::Enabled:
      return Diligent::DEVICE_FEATURE_STATE_ENABLED;
    case xiiGALDeviceFeatureState::Optional:
      return Diligent::DEVICE_FEATURE_STATE_OPTIONAL;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::DEVICE_FEATURE_STATE_DISABLED;
}

XII_ALWAYS_INLINE Diligent::VALIDATION_LEVEL xiiDiligentTypeConversions::GetDeviceValidationLevel(xiiEnum<xiiGALDeviceValidationLevel> e)
{
  switch (e)
  {
    case xiiGALDeviceValidationLevel::Disabled:
      return Diligent::VALIDATION_LEVEL_DISABLED;
    case xiiGALDeviceValidationLevel::Standard:
      return Diligent::VALIDATION_LEVEL_1;
    case xiiGALDeviceValidationLevel::All:
      return Diligent::VALIDATION_LEVEL_2;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::VALIDATION_LEVEL_DISABLED;
}

XII_ALWAYS_INLINE Diligent::SHADER_TYPE xiiDiligentTypeConversions::GetShaderTypeFlags(xiiBitflags<xiiGALShaderStage> e)
{
  if (e.IsNoFlagSet())
    return Diligent::SHADER_TYPE_UNKNOWN;

  Diligent::SHADER_TYPE shaderTypeFlags = {};

  if (e.IsSet(xiiGALShaderStage::Vertex))
    shaderTypeFlags |= Diligent::SHADER_TYPE_VERTEX;
  if (e.IsSet(xiiGALShaderStage::Pixel))
    shaderTypeFlags |= Diligent::SHADER_TYPE_PIXEL;
  if (e.IsSet(xiiGALShaderStage::Geometry))
    shaderTypeFlags |= Diligent::SHADER_TYPE_GEOMETRY;
  if (e.IsSet(xiiGALShaderStage::Hull))
    shaderTypeFlags |= Diligent::SHADER_TYPE_HULL;
  if (e.IsSet(xiiGALShaderStage::Domain))
    shaderTypeFlags |= Diligent::SHADER_TYPE_DOMAIN;
  if (e.IsSet(xiiGALShaderStage::Compute))
    shaderTypeFlags |= Diligent::SHADER_TYPE_COMPUTE;
  if (e.IsSet(xiiGALShaderStage::Amplification))
    shaderTypeFlags |= Diligent::SHADER_TYPE_AMPLIFICATION;
  if (e.IsSet(xiiGALShaderStage::Mesh))
    shaderTypeFlags |= Diligent::SHADER_TYPE_MESH;
  if (e.IsSet(xiiGALShaderStage::RayGeneration))
    shaderTypeFlags |= Diligent::SHADER_TYPE_RAY_GEN;
  if (e.IsSet(xiiGALShaderStage::RayMiss))
    shaderTypeFlags |= Diligent::SHADER_TYPE_RAY_MISS;
  if (e.IsSet(xiiGALShaderStage::RayClosestHit))
    shaderTypeFlags |= Diligent::SHADER_TYPE_RAY_CLOSEST_HIT;
  if (e.IsSet(xiiGALShaderStage::RayAnyHit))
    shaderTypeFlags |= Diligent::SHADER_TYPE_RAY_ANY_HIT;
  if (e.IsSet(xiiGALShaderStage::RayIntersection))
    shaderTypeFlags |= Diligent::SHADER_TYPE_RAY_INTERSECTION;
  if (e.IsSet(xiiGALShaderStage::Callable))
    shaderTypeFlags |= Diligent::SHADER_TYPE_CALLABLE;
  if (e.IsSet(xiiGALShaderStage::Tile))
    shaderTypeFlags |= Diligent::SHADER_TYPE_TILE;

  return shaderTypeFlags;
}

XII_ALWAYS_INLINE Diligent::INPUT_ELEMENT_FREQUENCY xiiDiligentTypeConversions::GetElementFrequency(xiiEnum<xiiGALInputElementFrequency> e)
{
  switch (e)
  {
    case xiiGALInputElementFrequency::PerVertex:
      return Diligent::INPUT_ELEMENT_FREQUENCY_UNDEFINED;
    case xiiGALInputElementFrequency::PerInstance:
      return Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::INPUT_ELEMENT_FREQUENCY_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::VALUE_TYPE xiiDiligentTypeConversions::GetDiligentValueType(Diligent::TEXTURE_FORMAT e)
{
  switch (e)
  {
    case Diligent::TEX_FORMAT_RGBA32_FLOAT:
    case Diligent::TEX_FORMAT_RGB32_FLOAT:
    case Diligent::TEX_FORMAT_RG32_FLOAT:
    case Diligent::TEX_FORMAT_D32_FLOAT:
    case Diligent::TEX_FORMAT_R32_FLOAT:
    case Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT:
    case Diligent::TEX_FORMAT_R11G11B10_FLOAT:
      return Diligent::VT_FLOAT32;

    // case Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP:
    case Diligent::TEX_FORMAT_RGBA16_FLOAT:
    case Diligent::TEX_FORMAT_RG16_FLOAT:
    case Diligent::TEX_FORMAT_R16_FLOAT:
      return Diligent::VT_FLOAT16;

    case Diligent::TEX_FORMAT_RGBA32_UINT:
    case Diligent::TEX_FORMAT_RGB32_UINT:
    case Diligent::TEX_FORMAT_RG32_UINT:
    case Diligent::TEX_FORMAT_RGB10A2_UNORM:
    case Diligent::TEX_FORMAT_RGB10A2_UINT:
    case Diligent::TEX_FORMAT_R32_UINT:
    case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
      // case Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
      return Diligent::VT_UINT32;

    case Diligent::TEX_FORMAT_RGBA16_UNORM:
    case Diligent::TEX_FORMAT_RGBA16_UINT:
    case Diligent::TEX_FORMAT_RG16_UNORM:
    case Diligent::TEX_FORMAT_RG16_UINT:
    case Diligent::TEX_FORMAT_D16_UNORM:
    case Diligent::TEX_FORMAT_R16_UNORM:
    case Diligent::TEX_FORMAT_R16_UINT:
    case Diligent::TEX_FORMAT_BC2_UNORM:
    case Diligent::TEX_FORMAT_BC2_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BC3_UNORM:
    case Diligent::TEX_FORMAT_BC3_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BC5_UNORM:
    case Diligent::TEX_FORMAT_BC5_SNORM:
    case Diligent::TEX_FORMAT_BC6H_UF16:
    case Diligent::TEX_FORMAT_BC6H_SF16:
    case Diligent::TEX_FORMAT_BC7_UNORM:
    case Diligent::TEX_FORMAT_BC7_UNORM_SRGB:
    case Diligent::TEX_FORMAT_B5G6R5_UNORM:
    case Diligent::TEX_FORMAT_B5G5R5A1_UNORM:
      return Diligent::VT_UINT16;

    case Diligent::TEX_FORMAT_RGBA8_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_RGBA8_UINT:
    case Diligent::TEX_FORMAT_RG8_UNORM:
    case Diligent::TEX_FORMAT_RG8_UINT:
    case Diligent::TEX_FORMAT_R8_UNORM:
    case Diligent::TEX_FORMAT_R8_UINT:
    case Diligent::TEX_FORMAT_A8_UNORM:
    case Diligent::TEX_FORMAT_R1_UNORM:
    case Diligent::TEX_FORMAT_BC1_UNORM:
    case Diligent::TEX_FORMAT_BC1_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BC4_UNORM:
    case Diligent::TEX_FORMAT_BC4_SNORM:
    case Diligent::TEX_FORMAT_RG8_B8G8_UNORM:
    case Diligent::TEX_FORMAT_G8R8_G8B8_UNORM:
      return Diligent::VT_UINT8;

    case Diligent::TEX_FORMAT_RGBA32_SINT:
    case Diligent::TEX_FORMAT_RGB32_SINT:
    case Diligent::TEX_FORMAT_RG32_SINT:
    case Diligent::TEX_FORMAT_R32_SINT:
      return Diligent::VT_INT32;

    case Diligent::TEX_FORMAT_RGBA16_SNORM:
    case Diligent::TEX_FORMAT_RGBA16_SINT:
    case Diligent::TEX_FORMAT_RG16_SNORM:
    case Diligent::TEX_FORMAT_RG16_SINT:
    case Diligent::TEX_FORMAT_R16_SNORM:
    case Diligent::TEX_FORMAT_R16_SINT:
      return Diligent::VT_INT16;

    case Diligent::TEX_FORMAT_RGBA8_SNORM:
    case Diligent::TEX_FORMAT_RGBA8_SINT:
    case Diligent::TEX_FORMAT_RG8_SNORM:
    case Diligent::TEX_FORMAT_RG8_SINT:
    case Diligent::TEX_FORMAT_R8_SNORM:
    case Diligent::TEX_FORMAT_R8_SINT:
    case Diligent::TEX_FORMAT_BGRA8_UNORM:
    case Diligent::TEX_FORMAT_BGRX8_UNORM:
    case Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB:
      return Diligent::VT_INT8;

    case Diligent::TEX_FORMAT_UNKNOWN:
    case Diligent::TEX_FORMAT_RGBA32_TYPELESS:
    case Diligent::TEX_FORMAT_RGB32_TYPELESS:
    case Diligent::TEX_FORMAT_RGBA16_TYPELESS:
    case Diligent::TEX_FORMAT_RG32_TYPELESS:
    case Diligent::TEX_FORMAT_R32G8X24_TYPELESS:
    case Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT:
    case Diligent::TEX_FORMAT_RGB10A2_TYPELESS:
    case Diligent::TEX_FORMAT_RGBA8_TYPELESS:
    case Diligent::TEX_FORMAT_RG16_TYPELESS:
    case Diligent::TEX_FORMAT_R24G8_TYPELESS:
    case Diligent::TEX_FORMAT_R32_TYPELESS:
    case Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT:
    case Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS:
    case Diligent::TEX_FORMAT_RG8_TYPELESS:
    case Diligent::TEX_FORMAT_R16_TYPELESS:
    case Diligent::TEX_FORMAT_R8_TYPELESS:
    case Diligent::TEX_FORMAT_BC1_TYPELESS:
    case Diligent::TEX_FORMAT_BC2_TYPELESS:
    case Diligent::TEX_FORMAT_BC3_TYPELESS:
    case Diligent::TEX_FORMAT_BC4_TYPELESS:
    case Diligent::TEX_FORMAT_BC5_TYPELESS:
    case Diligent::TEX_FORMAT_BGRA8_TYPELESS:
    case Diligent::TEX_FORMAT_BGRX8_TYPELESS:
    case Diligent::TEX_FORMAT_BC6H_TYPELESS:
    case Diligent::TEX_FORMAT_BC7_TYPELESS:

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::VT_UNDEFINED;
}

XII_ALWAYS_INLINE bool xiiDiligentTypeConversions::GetFormatNormalized(Diligent::TEXTURE_FORMAT e)
{
  switch (e)
  {
    case Diligent::TEX_FORMAT_RGBA16_UNORM:
    case Diligent::TEX_FORMAT_RGBA16_SNORM:
    case Diligent::TEX_FORMAT_RGB10A2_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_RG16_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_SNORM:
    case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
    case Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS:
    case Diligent::TEX_FORMAT_RG8_UNORM:
    case Diligent::TEX_FORMAT_D16_UNORM:
    case Diligent::TEX_FORMAT_R16_UNORM:
    case Diligent::TEX_FORMAT_R8_UNORM:
    case Diligent::TEX_FORMAT_A8_UNORM:
    case Diligent::TEX_FORMAT_R1_UNORM:
    case Diligent::TEX_FORMAT_RG8_B8G8_UNORM:
    case Diligent::TEX_FORMAT_G8R8_G8B8_UNORM:
    case Diligent::TEX_FORMAT_BC1_UNORM:
    case Diligent::TEX_FORMAT_BC1_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BC2_UNORM:
    case Diligent::TEX_FORMAT_BC2_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BC3_UNORM:
    case Diligent::TEX_FORMAT_BC3_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BC4_UNORM:
    case Diligent::TEX_FORMAT_BC5_UNORM:
    case Diligent::TEX_FORMAT_B5G6R5_UNORM:
    case Diligent::TEX_FORMAT_B5G5R5A1_UNORM:
    case Diligent::TEX_FORMAT_BGRA8_UNORM:
    case Diligent::TEX_FORMAT_BGRX8_UNORM:
    case Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case Diligent::TEX_FORMAT_BC7_UNORM:
    case Diligent::TEX_FORMAT_BC7_UNORM_SRGB:
    case Diligent::TEX_FORMAT_RG16_SNORM:
    case Diligent::TEX_FORMAT_RG8_SNORM:
    case Diligent::TEX_FORMAT_R16_SNORM:
    case Diligent::TEX_FORMAT_R8_SNORM:
    case Diligent::TEX_FORMAT_BC4_SNORM:
    case Diligent::TEX_FORMAT_BC5_SNORM:
      return true;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return false;
}

XII_ALWAYS_INLINE Diligent::SCALING_MODE xiiDiligentTypeConversions::GetScalingMode(xiiEnum<xiiGALScalingMode> e)
{
  switch (e)
  {
    case xiiGALScalingMode::Unspecified:
      return Diligent::SCALING_MODE_UNSPECIFIED;
    case xiiGALScalingMode::Centered:
      return Diligent::SCALING_MODE_CENTERED;
    case xiiGALScalingMode::Stretched:
      return Diligent::SCALING_MODE_STRETCHED;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::SCALING_MODE_UNSPECIFIED;
}

XII_ALWAYS_INLINE Diligent::SCANLINE_ORDER xiiDiligentTypeConversions::GetScanLineOrder(xiiEnum<xiiGALScanLineOrder> e)
{
  switch (e)
  {
    case xiiGALScanLineOrder::Unspecified:
      return Diligent::SCANLINE_ORDER_UNSPECIFIED;
    case xiiGALScanLineOrder::Progressive:
      return Diligent::SCANLINE_ORDER_PROGRESSIVE;
    case xiiGALScanLineOrder::UpperFieldFirst:
      return Diligent::SCANLINE_ORDER_UPPER_FIELD_FIRST;
    case xiiGALScanLineOrder::LowerFieldFirst:
      return Diligent::SCANLINE_ORDER_LOWER_FIELD_FIRST;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::SCANLINE_ORDER_UNSPECIFIED;
}

XII_ALWAYS_INLINE Diligent::SURFACE_TRANSFORM xiiDiligentTypeConversions::GetSurfaceTransform(xiiEnum<xiiGALSurfaceTransform> e)
{
  switch (e)
  {
    case xiiGALSurfaceTransform::Optimal:
      return Diligent::SURFACE_TRANSFORM_OPTIMAL;
    case xiiGALSurfaceTransform::Identity:
      return Diligent::SURFACE_TRANSFORM_IDENTITY;
    case xiiGALSurfaceTransform::Rotate90:
      return Diligent::SURFACE_TRANSFORM_ROTATE_90;
    case xiiGALSurfaceTransform::Rotate180:
      return Diligent::SURFACE_TRANSFORM_ROTATE_180;
    case xiiGALSurfaceTransform::Rotate270:
      return Diligent::SURFACE_TRANSFORM_ROTATE_270;
    case xiiGALSurfaceTransform::HorizontalMirror:
      return Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR;
    case xiiGALSurfaceTransform::HorizontalMirrorRotate90:
      return Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90;
    case xiiGALSurfaceTransform::HorizontalMirrorRotate180:
      return Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180;
    case xiiGALSurfaceTransform::HorizontalMirrorRotate270:
      return Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270;
  }
  return Diligent::SURFACE_TRANSFORM_OPTIMAL;
}

XII_ALWAYS_INLINE Diligent::SWAP_CHAIN_USAGE_FLAGS xiiDiligentTypeConversions::GetSwapChainUsageFlags(xiiBitflags<xiiGALSwapChainUsageFlags> e)
{
  if (e.IsNoFlagSet())
    return Diligent::SWAP_CHAIN_USAGE_NONE;

  Diligent::SWAP_CHAIN_USAGE_FLAGS swapChainUsageFlags = {};

  if (e.IsSet(xiiGALSwapChainUsageFlags::RenderTarget))
    swapChainUsageFlags |= Diligent::SWAP_CHAIN_USAGE_RENDER_TARGET;
  if (e.IsSet(xiiGALSwapChainUsageFlags::ShaderResource))
    swapChainUsageFlags |= Diligent::SWAP_CHAIN_USAGE_SHADER_RESOURCE;
  if (e.IsSet(xiiGALSwapChainUsageFlags::InputAttachment))
    swapChainUsageFlags |= Diligent::SWAP_CHAIN_USAGE_INPUT_ATTACHMENT;
  if (e.IsSet(xiiGALSwapChainUsageFlags::CopySource))
    swapChainUsageFlags |= Diligent::SWAP_CHAIN_USAGE_COPY_SOURCE;

  return swapChainUsageFlags;
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALBindFlags> xiiDiligentTypeConversions::GetGALBindFlags(Diligent::BIND_FLAGS e)
{
  if (e == Diligent::BIND_NONE)
    return xiiBitflags<xiiGALBindFlags>();

  xiiBitflags<xiiGALBindFlags> bindFlags;

  if (e & Diligent::BIND_VERTEX_BUFFER)
    bindFlags |= xiiGALBindFlags::VertexBuffer;
  if (e & Diligent::BIND_INDEX_BUFFER)
    bindFlags |= xiiGALBindFlags::IndexBuffer;
  if (e & Diligent::BIND_UNIFORM_BUFFER)
    bindFlags |= xiiGALBindFlags::UniformBuffer;
  if (e & Diligent::BIND_SHADER_RESOURCE)
    bindFlags |= xiiGALBindFlags::ShaderResource;
  if (e & Diligent::BIND_STREAM_OUTPUT)
    bindFlags |= xiiGALBindFlags::StreamOutput;
  if (e & Diligent::BIND_RENDER_TARGET)
    bindFlags |= xiiGALBindFlags::RenderTarget;
  if (e & Diligent::BIND_DEPTH_STENCIL)
    bindFlags |= xiiGALBindFlags::DepthStencil;
  if (e & Diligent::BIND_UNORDERED_ACCESS)
    bindFlags |= xiiGALBindFlags::UnorderedAccess;
  if (e & Diligent::BIND_INDIRECT_DRAW_ARGS)
    bindFlags |= xiiGALBindFlags::IndirectDrawArguments;
  if (e & Diligent::BIND_INPUT_ATTACHMENT)
    bindFlags |= xiiGALBindFlags::InputAttachment;
  if (e & Diligent::BIND_RAY_TRACING)
    bindFlags |= xiiGALBindFlags::RayTracing;
  if (e & Diligent::BIND_SHADING_RATE)
    bindFlags |= xiiGALBindFlags::ShadingRate;

  return bindFlags;
}

XII_ALWAYS_INLINE xiiEnum<xiiGALResourceUsage> xiiDiligentTypeConversions::GetGALUsage(Diligent::USAGE e)
{
  switch (e)
  {
    case Diligent::USAGE_IMMUTABLE:
      return xiiGALResourceUsage::Immutable;
    case Diligent::USAGE_DEFAULT:
      return xiiGALResourceUsage::Default;
    case Diligent::USAGE_DYNAMIC:
      return xiiGALResourceUsage::Dynamic;
    case Diligent::USAGE_STAGING:
      return xiiGALResourceUsage::Staging;
    case Diligent::USAGE_UNIFIED:
      return xiiGALResourceUsage::Unified;
    case Diligent::USAGE_SPARSE:
      return xiiGALResourceUsage::Sparse;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiEnum<xiiGALResourceUsage>();
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALCPUAccessFlag> xiiDiligentTypeConversions::GetGALCPUAccessFlags(Diligent::CPU_ACCESS_FLAGS e)
{
  if (e == Diligent::CPU_ACCESS_NONE)
    return xiiBitflags<xiiGALCPUAccessFlag>();

  xiiBitflags<xiiGALCPUAccessFlag> cpuAccessFlags;

  if (e & Diligent::CPU_ACCESS_READ)
    cpuAccessFlags |= xiiGALCPUAccessFlag::Read;
  if (e & Diligent::CPU_ACCESS_WRITE)
    cpuAccessFlags |= xiiGALCPUAccessFlag::Write;

  return cpuAccessFlags;
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALMiscTextureFlags> xiiDiligentTypeConversions::GetGALMiscTextureFlags(Diligent::MISC_TEXTURE_FLAGS e)
{
  if (e == Diligent::MISC_TEXTURE_FLAG_NONE)
    return xiiBitflags<xiiGALMiscTextureFlags>();

  xiiBitflags<xiiGALMiscTextureFlags> miscTextureFlags;

  if (e & Diligent::MISC_TEXTURE_FLAG_MEMORYLESS)
    miscTextureFlags |= xiiGALMiscTextureFlags::Memoryless;
  if (e & Diligent::MISC_TEXTURE_FLAG_GENERATE_MIPS)
    miscTextureFlags |= xiiGALMiscTextureFlags::GenerateMips;
  if (e & Diligent::MISC_TEXTURE_FLAG_SPARSE_ALIASING)
    miscTextureFlags |= xiiGALMiscTextureFlags::SparseAlias;
  if (e & Diligent::MISC_TEXTURE_FLAG_SUBSAMPLED)
    miscTextureFlags |= xiiGALMiscTextureFlags::Subsampled;

  return miscTextureFlags;
}

XII_ALWAYS_INLINE Diligent::MAP_FLAGS xiiDiligentTypeConversions::GetMapFlags(xiiBitflags<xiiGALMapFlags> e)
{
  if (e.IsNoFlagSet())
    return Diligent::MAP_FLAG_NONE;

  Diligent::MAP_FLAGS mapFlags = {};

  if (e.IsSet(xiiGALMapFlags::DoNotWait))
    mapFlags |= Diligent::MAP_FLAG_DO_NOT_WAIT;
  if (e.IsSet(xiiGALMapFlags::Discard))
    mapFlags |= Diligent::MAP_FLAG_DISCARD;
  if (e.IsSet(xiiGALMapFlags::NoOverWrite))
    mapFlags |= Diligent::MAP_FLAG_NO_OVERWRITE;

  return mapFlags;
}

XII_ALWAYS_INLINE Diligent::PRIMITIVE_TOPOLOGY xiiDiligentTypeConversions::GetPrimitiveTopology(xiiEnum<xiiGALPrimitiveTopology> e)
{
  switch (e)
  {
    case xiiGALPrimitiveTopology::Undefined:
      return Diligent::PRIMITIVE_TOPOLOGY_UNDEFINED;
    case xiiGALPrimitiveTopology::TriangleList:
      return Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case xiiGALPrimitiveTopology::TriangleStrip:
      return Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case xiiGALPrimitiveTopology::PointList:
      return Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST;
    case xiiGALPrimitiveTopology::LineList:
      return Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;
    case xiiGALPrimitiveTopology::LineStrip:
      return Diligent::PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case xiiGALPrimitiveTopology::TriangleListAdjacent:
      return Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_ADJ;
    case xiiGALPrimitiveTopology::TrangleStripAdjacent:
      return Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_ADJ;
    case xiiGALPrimitiveTopology::LineListAdjacent:
      return Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST_ADJ;
    case xiiGALPrimitiveTopology::LineStripAdjacent:
      return Diligent::PRIMITIVE_TOPOLOGY_LINE_STRIP_ADJ;
    case xiiGALPrimitiveTopology::ControlPointPatchList1:
      return Diligent::PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList2:
      return Diligent::PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList3:
      return Diligent::PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList4:
      return Diligent::PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList5:
      return Diligent::PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList6:
      return Diligent::PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList7:
      return Diligent::PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList8:
      return Diligent::PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList9:
      return Diligent::PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList10:
      return Diligent::PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList11:
      return Diligent::PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList12:
      return Diligent::PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList13:
      return Diligent::PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList14:
      return Diligent::PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList15:
      return Diligent::PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList16:
      return Diligent::PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList17:
      return Diligent::PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList18:
      return Diligent::PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList19:
      return Diligent::PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList20:
      return Diligent::PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList21:
      return Diligent::PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList22:
      return Diligent::PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList23:
      return Diligent::PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList24:
      return Diligent::PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList25:
      return Diligent::PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList26:
      return Diligent::PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList27:
      return Diligent::PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList28:
      return Diligent::PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList29:
      return Diligent::PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList30:
      return Diligent::PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList31:
      return Diligent::PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList32:
      return Diligent::PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::PRIMITIVE_TOPOLOGY_UNDEFINED;
}

XII_ALWAYS_INLINE xiiEnum<xiiGALDeviceFeatureState> xiiDiligentTypeConversions::GetGALDeviceFeatureState(Diligent::DEVICE_FEATURE_STATE e)
{
  switch (e)
  {
    case Diligent::DEVICE_FEATURE_STATE_DISABLED:
      return xiiGALDeviceFeatureState::Disabled;
    case Diligent::DEVICE_FEATURE_STATE_ENABLED:
      return xiiGALDeviceFeatureState::Enabled;
    case Diligent::DEVICE_FEATURE_STATE_OPTIONAL:
      return xiiGALDeviceFeatureState::Optional;
  }
  return xiiEnum<xiiGALDeviceFeatureState>();
}
