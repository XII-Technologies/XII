
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

XII_ALWAYS_INLINE Diligent::BLEND_OPERATION xiiDiligentTypeConversions::GetBlendOperation(xiiEnum<xiiGALBlendOperation> e)
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

XII_ALWAYS_INLINE Diligent::COMPARISON_FUNCTION xiiDiligentTypeConversions::GetComparisonFunction(xiiEnum<xiiGALComparisonFunction> e)
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
}

XII_ALWAYS_INLINE Diligent::STENCIL_OP xiiDiligentTypeConversions::GetStencilOperation(xiiEnum<xiiGALStencilOperation> e)
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
