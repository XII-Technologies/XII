
XII_ALWAYS_INLINE D3D12_BLEND xiiD3D12TypeConversions::GetD3D12BlendFactor(xiiEnum<xiiGALBlendFactor> e)
{
  switch (e)
  {
    case xiiGALBlendFactor::Zero:
      return D3D12_BLEND::D3D12_BLEND_ZERO;
    case xiiGALBlendFactor::One:
      return D3D12_BLEND::D3D12_BLEND_ONE;
    case xiiGALBlendFactor::SourceColor:
      return D3D12_BLEND::D3D12_BLEND_SRC_COLOR;
    case xiiGALBlendFactor::InverseSourceColor:
      return D3D12_BLEND::D3D12_BLEND_INV_SRC_COLOR;
    case xiiGALBlendFactor::SourceAlpha:
      return D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
    case xiiGALBlendFactor::InverseSourceAlpha:
      return D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
    case xiiGALBlendFactor::DestinationAlpha:
      return D3D12_BLEND::D3D12_BLEND_DEST_ALPHA;
    case xiiGALBlendFactor::InverseDestinationAlpha:
      return D3D12_BLEND::D3D12_BLEND_INV_DEST_ALPHA;
    case xiiGALBlendFactor::DestinationColor:
      return D3D12_BLEND::D3D12_BLEND_DEST_COLOR;
    case xiiGALBlendFactor::InverseDestinationColor:
      return D3D12_BLEND::D3D12_BLEND_INV_DEST_COLOR;
    case xiiGALBlendFactor::SourceAlphaSaturate:
      return D3D12_BLEND::D3D12_BLEND_SRC_ALPHA_SAT;
    case xiiGALBlendFactor::BlendFactor:
      return D3D12_BLEND::D3D12_BLEND_BLEND_FACTOR;
    case xiiGALBlendFactor::InverseBlendFactor:
      return D3D12_BLEND::D3D12_BLEND_INV_BLEND_FACTOR;
    case xiiGALBlendFactor::SourceOneColor:
      return D3D12_BLEND::D3D12_BLEND_SRC1_COLOR;
    case xiiGALBlendFactor::InverseSourceOneColor:
      return D3D12_BLEND::D3D12_BLEND_INV_SRC1_COLOR;
    case xiiGALBlendFactor::SourceOneAlpha:
      return D3D12_BLEND::D3D12_BLEND_SRC1_ALPHA;
    case xiiGALBlendFactor::InverseSourceOneAlpha:
      return D3D12_BLEND::D3D12_BLEND_INV_SRC1_ALPHA;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return D3D12_BLEND::D3D12_BLEND_ZERO;
}

XII_ALWAYS_INLINE D3D12_BLEND_OP xiiD3D12TypeConversions::GetD3D12BlendOp(xiiEnum<xiiGALBlendOperation> e)
{
  switch (e)
  {
    case xiiGALBlendOperation::Add:
      return D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
    case xiiGALBlendOperation::Subtract:
      return D3D12_BLEND_OP::D3D12_BLEND_OP_SUBTRACT;
    case xiiGALBlendOperation::ReverseSubtract:
      return D3D12_BLEND_OP::D3D12_BLEND_OP_REV_SUBTRACT;
    case xiiGALBlendOperation::Min:
      return D3D12_BLEND_OP::D3D12_BLEND_OP_MIN;
    case xiiGALBlendOperation::Max:
      return D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
}

XII_ALWAYS_INLINE D3D12_COMPARISON_FUNC xiiD3D12TypeConversions::GetD3D12ComparisonFunc(xiiEnum<xiiGALComparisonFunction> e)
{
  switch (e)
  {
    case xiiGALComparisonFunction::Never:
      return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NEVER;
    case xiiGALComparisonFunction::Less:
      return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
    case xiiGALComparisonFunction::Equal:
      return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
    case xiiGALComparisonFunction::LessEqual:
      return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case xiiGALComparisonFunction::Greater:
      return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
    case xiiGALComparisonFunction::NotEqual:
      return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case xiiGALComparisonFunction::GreaterEqual:
      return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case xiiGALComparisonFunction::Always:
      return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NEVER;
}

XII_ALWAYS_INLINE D3D12_STENCIL_OP xiiD3D12TypeConversions::GetD3D12StencilOp(xiiEnum<xiiGALStencilOperation> e)
{
  switch (e)
  {
    case xiiGALStencilOperation::Keep:
      return D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
    case xiiGALStencilOperation::Zero:
      return D3D12_STENCIL_OP::D3D12_STENCIL_OP_ZERO;
    case xiiGALStencilOperation::Replace:
      return D3D12_STENCIL_OP::D3D12_STENCIL_OP_REPLACE;
    case xiiGALStencilOperation::IncrementSaturate:
      return D3D12_STENCIL_OP::D3D12_STENCIL_OP_INCR_SAT;
    case xiiGALStencilOperation::DecrementSaturate:
      return D3D12_STENCIL_OP::D3D12_STENCIL_OP_DECR_SAT;
    case xiiGALStencilOperation::Invert:
      return D3D12_STENCIL_OP::D3D12_STENCIL_OP_INVERT;
    case xiiGALStencilOperation::IncrementWrap:
      return D3D12_STENCIL_OP::D3D12_STENCIL_OP_INCR;
    case xiiGALStencilOperation::DecrementWrap:
      return D3D12_STENCIL_OP::D3D12_STENCIL_OP_DECR;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
}

XII_ALWAYS_INLINE D3D12_FILL_MODE xiiD3D12TypeConversions::GetD3D12FillMode(xiiEnum<xiiGALFillMode> e)
{
  switch (e)
  {
    case xiiGALFillMode::Wireframe:
      return D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;
    case xiiGALFillMode::Solid:
      return D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;
}

XII_ALWAYS_INLINE D3D12_CULL_MODE xiiD3D12TypeConversions::GetD3D12CullMode(xiiEnum<xiiGALCullMode> e)
{
  switch (e)
  {
    case xiiGALCullMode::None:
      return D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
    case xiiGALCullMode::Front:
      return D3D12_CULL_MODE::D3D12_CULL_MODE_FRONT;
    case xiiGALCullMode::Back:
      return D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
}
