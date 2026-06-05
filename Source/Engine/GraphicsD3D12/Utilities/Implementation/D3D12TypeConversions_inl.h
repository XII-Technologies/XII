/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_ALWAYS_INLINE D3D12_BLEND xiiD3D12TypeConversions::GetBlendFactor(xiiGALBlendFactor::Enum e)
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

XII_ALWAYS_INLINE D3D12_BLEND_OP xiiD3D12TypeConversions::GetBlendOp(xiiGALBlendOperation::Enum e)
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

XII_ALWAYS_INLINE D3D12_COMPARISON_FUNC xiiD3D12TypeConversions::GetComparisonFunc(xiiGALComparisonFunction::Enum e)
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

XII_ALWAYS_INLINE D3D12_STENCIL_OP xiiD3D12TypeConversions::GetStencilOp(xiiGALStencilOperation::Enum e)
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

XII_ALWAYS_INLINE D3D12_FILL_MODE xiiD3D12TypeConversions::GetFillMode(xiiGALFillMode::Enum e)
{
  switch (e)
  {
    case xiiGALFillMode::Wireframe:
      return D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;
    case xiiGALFillMode::Solid:
      return D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;
}

XII_ALWAYS_INLINE D3D12_CULL_MODE xiiD3D12TypeConversions::GetCullMode(xiiGALCullMode::Enum e)
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

XII_ALWAYS_INLINE xiiUInt8 xiiD3D12TypeConversions::GetColorWriteMask(xiiBitflags<xiiGALColorMask> e)
{
  if (e.IsNoFlagSet())
    return 0U;

  xiiUInt8 uiColorMask = 0U;
  if (e.IsSet(xiiGALColorMask::Red))
    uiColorMask |= D3D12_COLOR_WRITE_ENABLE_RED;
  if (e.IsSet(xiiGALColorMask::Green))
    uiColorMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
  if (e.IsSet(xiiGALColorMask::Blue))
    uiColorMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
  if (e.IsSet(xiiGALColorMask::Alpha))
    uiColorMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;

  return uiColorMask;
}

XII_ALWAYS_INLINE DXGI_FORMAT xiiD3D12TypeConversions::GetFormat(xiiGALResourceFormat::Enum e)
{
  switch (e)
  {
    case xiiGALResourceFormat::Unknown:
      return DXGI_FORMAT_UNKNOWN;
    case xiiGALResourceFormat::RGBA32Typeless:
      return DXGI_FORMAT_R32G32B32A32_TYPELESS;
    case xiiGALResourceFormat::RGBA32Float:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case xiiGALResourceFormat::RGBA32UInt:
      return DXGI_FORMAT_R32G32B32A32_UINT;
    case xiiGALResourceFormat::RGBA32SInt:
      return DXGI_FORMAT_R32G32B32A32_SINT;
    case xiiGALResourceFormat::RGB32Typeless:
      return DXGI_FORMAT_R32G32B32_TYPELESS;
    case xiiGALResourceFormat::RGB32Float:
      return DXGI_FORMAT_R32G32B32_FLOAT;
    case xiiGALResourceFormat::RGB32UInt:
      return DXGI_FORMAT_R32G32B32_UINT;
    case xiiGALResourceFormat::RGB32SInt:
      return DXGI_FORMAT_R32G32B32_SINT;
    case xiiGALResourceFormat::RGBA16Typeless:
      return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    case xiiGALResourceFormat::RGBA16Float:
      return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case xiiGALResourceFormat::RGBA16UNormalized:
      return DXGI_FORMAT_R16G16B16A16_UNORM;
    case xiiGALResourceFormat::RGBA16UInt:
      return DXGI_FORMAT_R16G16B16A16_UINT;
    case xiiGALResourceFormat::RGBA16SNormalized:
      return DXGI_FORMAT_R16G16B16A16_SNORM;
    case xiiGALResourceFormat::RGBA16SInt:
      return DXGI_FORMAT_R16G16B16A16_SINT;
    case xiiGALResourceFormat::RG32Typeless:
      return DXGI_FORMAT_R32G32_TYPELESS;
    case xiiGALResourceFormat::RG32Float:
      return DXGI_FORMAT_R32G32_FLOAT;
    case xiiGALResourceFormat::RG32UInt:
      return DXGI_FORMAT_R32G32_UINT;
    case xiiGALResourceFormat::RG32SInt:
      return DXGI_FORMAT_R32G32_SINT;
    case xiiGALResourceFormat::R32G8X24Typeless:
      return DXGI_FORMAT_R32G8X24_TYPELESS;
    case xiiGALResourceFormat::D32FloatS8X24UInt:
      return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    case xiiGALResourceFormat::R32FloatX8X24Typeless:
      return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case xiiGALResourceFormat::X32TypelessG8X24UInt:
      return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
    case xiiGALResourceFormat::RGB10A2Typeless:
      return DXGI_FORMAT_R10G10B10A2_TYPELESS;
    case xiiGALResourceFormat::RGB10A2UNormalized:
      return DXGI_FORMAT_R10G10B10A2_UNORM;
    case xiiGALResourceFormat::RGB10A2UInt:
      return DXGI_FORMAT_R10G10B10A2_UINT;
    case xiiGALResourceFormat::RG11B10Float:
      return DXGI_FORMAT_R11G11B10_FLOAT;
    case xiiGALResourceFormat::RGBA8Typeless:
      return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case xiiGALResourceFormat::RGBA8UNormalized:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    case xiiGALResourceFormat::RGBA8UNormalizedSRGB:
      return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case xiiGALResourceFormat::RGBA8UInt:
      return DXGI_FORMAT_R8G8B8A8_UINT;
    case xiiGALResourceFormat::RGBA8SNormalized:
      return DXGI_FORMAT_R8G8B8A8_SNORM;
    case xiiGALResourceFormat::RGBA8SInt:
      return DXGI_FORMAT_R8G8B8A8_SINT;
    case xiiGALResourceFormat::RG16Typeless:
      return DXGI_FORMAT_R16G16_TYPELESS;
    case xiiGALResourceFormat::RG16Float:
      return DXGI_FORMAT_R16G16_FLOAT;
    case xiiGALResourceFormat::RG16UNormalized:
      return DXGI_FORMAT_R16G16_UNORM;
    case xiiGALResourceFormat::RG16UInt:
      return DXGI_FORMAT_R16G16_UINT;
    case xiiGALResourceFormat::RG16SNormalized:
      return DXGI_FORMAT_R16G16_SNORM;
    case xiiGALResourceFormat::RG16SInt:
      return DXGI_FORMAT_R16G16_SINT;
    case xiiGALResourceFormat::R32Typeless:
      return DXGI_FORMAT_R32_TYPELESS;
    case xiiGALResourceFormat::D32Float:
      return DXGI_FORMAT_D32_FLOAT;
    case xiiGALResourceFormat::R32Float:
      return DXGI_FORMAT_R32_FLOAT;
    case xiiGALResourceFormat::R32UInt:
      return DXGI_FORMAT_R32_UINT;
    case xiiGALResourceFormat::R32SInt:
      return DXGI_FORMAT_R32_SINT;
    case xiiGALResourceFormat::R24G8Typeless:
      return DXGI_FORMAT_R24G8_TYPELESS;
    case xiiGALResourceFormat::D24UNormalizedS8UInt:
      return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case xiiGALResourceFormat::R24UNormalizedX8Typeless:
      return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case xiiGALResourceFormat::X24TypelessG8UInt:
      return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    case xiiGALResourceFormat::RG8Typeless:
      return DXGI_FORMAT_R8G8_TYPELESS;
    case xiiGALResourceFormat::RG8UNormalized:
      return DXGI_FORMAT_R8G8_UNORM;
    case xiiGALResourceFormat::RG8UInt:
      return DXGI_FORMAT_R8G8_UINT;
    case xiiGALResourceFormat::RG8SNormalized:
      return DXGI_FORMAT_R8G8_SNORM;
    case xiiGALResourceFormat::RG8SInt:
      return DXGI_FORMAT_R8G8_SINT;
    case xiiGALResourceFormat::R16Typeless:
      return DXGI_FORMAT_R16_TYPELESS;
    case xiiGALResourceFormat::R16Float:
      return DXGI_FORMAT_R16_FLOAT;
    case xiiGALResourceFormat::D16UNormalized:
      return DXGI_FORMAT_D16_UNORM;
    case xiiGALResourceFormat::R16UNormalized:
      return DXGI_FORMAT_R16_UNORM;
    case xiiGALResourceFormat::R16UInt:
      return DXGI_FORMAT_R16_UINT;
    case xiiGALResourceFormat::R16SNormalized:
      return DXGI_FORMAT_R16_SNORM;
    case xiiGALResourceFormat::R16SInt:
      return DXGI_FORMAT_R16_SINT;
    case xiiGALResourceFormat::R8Typeless:
      return DXGI_FORMAT_R8_TYPELESS;
    case xiiGALResourceFormat::R8UNormalized:
      return DXGI_FORMAT_R8_UNORM;
    case xiiGALResourceFormat::R8UInt:
      return DXGI_FORMAT_R8_UINT;
    case xiiGALResourceFormat::R8SNormalized:
      return DXGI_FORMAT_R8_SNORM;
    case xiiGALResourceFormat::R8SInt:
      return DXGI_FORMAT_R8_SINT;
    case xiiGALResourceFormat::A8UNormalized:
      return DXGI_FORMAT_A8_UNORM;
    case xiiGALResourceFormat::R1UNormalized:
      return DXGI_FORMAT_R1_UNORM;
    case xiiGALResourceFormat::RGB9E5SharedExponent:
      return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
    case xiiGALResourceFormat::RG8BG8UNormalized:
      return DXGI_FORMAT_R8G8_B8G8_UNORM;
    case xiiGALResourceFormat::GR8GB8UNormalized:
      return DXGI_FORMAT_G8R8_G8B8_UNORM;
    case xiiGALResourceFormat::BC1Typeless:
      return DXGI_FORMAT_BC1_TYPELESS;
    case xiiGALResourceFormat::BC1UNormalized:
      return DXGI_FORMAT_BC1_UNORM;
    case xiiGALResourceFormat::BC1UNormalizedSRGB:
      return DXGI_FORMAT_BC1_UNORM_SRGB;
    case xiiGALResourceFormat::BC2Typeless:
      return DXGI_FORMAT_BC2_TYPELESS;
    case xiiGALResourceFormat::BC2UNormalized:
      return DXGI_FORMAT_BC2_UNORM;
    case xiiGALResourceFormat::BC2UNormalizedSRGB:
      return DXGI_FORMAT_BC2_UNORM_SRGB;
    case xiiGALResourceFormat::BC3Typeless:
      return DXGI_FORMAT_BC3_TYPELESS;
    case xiiGALResourceFormat::BC3UNormalized:
      return DXGI_FORMAT_BC3_UNORM;
    case xiiGALResourceFormat::BC3UNormalizedSRGB:
      return DXGI_FORMAT_BC3_UNORM_SRGB;
    case xiiGALResourceFormat::BC4Typeless:
      return DXGI_FORMAT_BC4_TYPELESS;
    case xiiGALResourceFormat::BC4UNormalized:
      return DXGI_FORMAT_BC4_UNORM;
    case xiiGALResourceFormat::BC4SNormalized:
      return DXGI_FORMAT_BC4_SNORM;
    case xiiGALResourceFormat::BC5Typeless:
      return DXGI_FORMAT_BC5_TYPELESS;
    case xiiGALResourceFormat::BC5UNormalized:
      return DXGI_FORMAT_BC5_UNORM;
    case xiiGALResourceFormat::BC5SNormalized:
      return DXGI_FORMAT_BC5_SNORM;
    case xiiGALResourceFormat::B5G6R5UNormalized:
      return DXGI_FORMAT_B5G6R5_UNORM;
    case xiiGALResourceFormat::B5G5R5A1UNormalized:
      return DXGI_FORMAT_B5G5R5A1_UNORM;
    case xiiGALResourceFormat::BGRA8UNormalized:
      return DXGI_FORMAT_B8G8R8A8_UNORM;
    case xiiGALResourceFormat::BGRX8UNormalized:
      return DXGI_FORMAT_B8G8R8X8_UNORM;
    case xiiGALResourceFormat::R10G10B10XRBiasA2UNormalized:
      return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
    case xiiGALResourceFormat::BGRA8Typeless:
      return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case xiiGALResourceFormat::BGRA8UNormalizedSRGB:
      return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case xiiGALResourceFormat::BGRX8Typeless:
      return DXGI_FORMAT_B8G8R8X8_TYPELESS;
    case xiiGALResourceFormat::BGRX8UNormalizedSRGB:
      return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    case xiiGALResourceFormat::BC6HTypeless:
      return DXGI_FORMAT_BC6H_TYPELESS;
    case xiiGALResourceFormat::BC6HUF16:
      return DXGI_FORMAT_BC6H_UF16;
    case xiiGALResourceFormat::BC6HSF16:
      return DXGI_FORMAT_BC6H_SF16;
    case xiiGALResourceFormat::BC7Typeless:
      return DXGI_FORMAT_BC7_TYPELESS;
    case xiiGALResourceFormat::BC7UNormalized:
      return DXGI_FORMAT_BC7_UNORM;
    case xiiGALResourceFormat::BC7UNormalizedSRGB:
      return DXGI_FORMAT_BC7_UNORM_SRGB;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return DXGI_FORMAT_UNKNOWN;
}

XII_ALWAYS_INLINE xiiGALResourceFormat::Enum xiiD3D12TypeConversions::GetGALFormat(DXGI_FORMAT e)
{
  switch (e)
  {
    case DXGI_FORMAT_UNKNOWN:
      return xiiGALResourceFormat::Unknown;
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
      return xiiGALResourceFormat::RGBA32Typeless;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return xiiGALResourceFormat::RGBA32Float;
    case DXGI_FORMAT_R32G32B32A32_UINT:
      return xiiGALResourceFormat::RGBA32UInt;
    case DXGI_FORMAT_R32G32B32A32_SINT:
      return xiiGALResourceFormat::RGBA32SInt;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
      return xiiGALResourceFormat::RGB32Typeless;
    case DXGI_FORMAT_R32G32B32_FLOAT:
      return xiiGALResourceFormat::RGB32Float;
    case DXGI_FORMAT_R32G32B32_UINT:
      return xiiGALResourceFormat::RGB32UInt;
    case DXGI_FORMAT_R32G32B32_SINT:
      return xiiGALResourceFormat::RGB32SInt;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
      return xiiGALResourceFormat::RGBA16Typeless;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
      return xiiGALResourceFormat::RGBA16Float;
    case DXGI_FORMAT_R16G16B16A16_UNORM:
      return xiiGALResourceFormat::RGBA16UNormalized;
    case DXGI_FORMAT_R16G16B16A16_UINT:
      return xiiGALResourceFormat::RGBA16UInt;
    case DXGI_FORMAT_R16G16B16A16_SNORM:
      return xiiGALResourceFormat::RGBA16SNormalized;
    case DXGI_FORMAT_R16G16B16A16_SINT:
      return xiiGALResourceFormat::RGBA16SInt;
    case DXGI_FORMAT_R32G32_TYPELESS:
      return xiiGALResourceFormat::RG32Typeless;
    case DXGI_FORMAT_R32G32_FLOAT:
      return xiiGALResourceFormat::RG32Float;
    case DXGI_FORMAT_R32G32_UINT:
      return xiiGALResourceFormat::RG32UInt;
    case DXGI_FORMAT_R32G32_SINT:
      return xiiGALResourceFormat::RG32SInt;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
      return xiiGALResourceFormat::R32G8X24Typeless;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      return xiiGALResourceFormat::D32FloatS8X24UInt;
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      return xiiGALResourceFormat::R32FloatX8X24Typeless;
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
      return xiiGALResourceFormat::X32TypelessG8X24UInt;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
      return xiiGALResourceFormat::RGB10A2Typeless;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
      return xiiGALResourceFormat::RGB10A2UNormalized;
    case DXGI_FORMAT_R10G10B10A2_UINT:
      return xiiGALResourceFormat::RGB10A2UInt;
    case DXGI_FORMAT_R11G11B10_FLOAT:
      return xiiGALResourceFormat::RG11B10Float;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      return xiiGALResourceFormat::RGBA8Typeless;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
      return xiiGALResourceFormat::RGBA8UNormalized;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      return xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    case DXGI_FORMAT_R8G8B8A8_UINT:
      return xiiGALResourceFormat::RGBA8UInt;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
      return xiiGALResourceFormat::RGBA8SNormalized;
    case DXGI_FORMAT_R8G8B8A8_SINT:
      return xiiGALResourceFormat::RGBA8SInt;
    case DXGI_FORMAT_R16G16_TYPELESS:
      return xiiGALResourceFormat::RG16Typeless;
    case DXGI_FORMAT_R16G16_FLOAT:
      return xiiGALResourceFormat::RG16Float;
    case DXGI_FORMAT_R16G16_UNORM:
      return xiiGALResourceFormat::RG16UNormalized;
    case DXGI_FORMAT_R16G16_UINT:
      return xiiGALResourceFormat::RG16UInt;
    case DXGI_FORMAT_R16G16_SNORM:
      return xiiGALResourceFormat::RG16SNormalized;
    case DXGI_FORMAT_R16G16_SINT:
      return xiiGALResourceFormat::RG16SInt;
    case DXGI_FORMAT_R32_TYPELESS:
      return xiiGALResourceFormat::R32Typeless;
    case DXGI_FORMAT_D32_FLOAT:
      return xiiGALResourceFormat::D32Float;
    case DXGI_FORMAT_R32_FLOAT:
      return xiiGALResourceFormat::R32Float;
    case DXGI_FORMAT_R32_UINT:
      return xiiGALResourceFormat::R32UInt;
    case DXGI_FORMAT_R32_SINT:
      return xiiGALResourceFormat::R32SInt;
    case DXGI_FORMAT_R24G8_TYPELESS:
      return xiiGALResourceFormat::R24G8Typeless;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return xiiGALResourceFormat::D24UNormalizedS8UInt;
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      return xiiGALResourceFormat::R24UNormalizedX8Typeless;
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
      return xiiGALResourceFormat::X24TypelessG8UInt;
    case DXGI_FORMAT_R8G8_TYPELESS:
      return xiiGALResourceFormat::RG8Typeless;
    case DXGI_FORMAT_R8G8_UNORM:
      return xiiGALResourceFormat::RG8UNormalized;
    case DXGI_FORMAT_R8G8_UINT:
      return xiiGALResourceFormat::RG8UInt;
    case DXGI_FORMAT_R8G8_SNORM:
      return xiiGALResourceFormat::RG8SNormalized;
    case DXGI_FORMAT_R8G8_SINT:
      return xiiGALResourceFormat::RG8SInt;
    case DXGI_FORMAT_R16_TYPELESS:
      return xiiGALResourceFormat::R16Typeless;
    case DXGI_FORMAT_R16_FLOAT:
      return xiiGALResourceFormat::R16Float;
    case DXGI_FORMAT_D16_UNORM:
      return xiiGALResourceFormat::D16UNormalized;
    case DXGI_FORMAT_R16_UNORM:
      return xiiGALResourceFormat::R16UNormalized;
    case DXGI_FORMAT_R16_UINT:
      return xiiGALResourceFormat::R16UInt;
    case DXGI_FORMAT_R16_SNORM:
      return xiiGALResourceFormat::R16SNormalized;
    case DXGI_FORMAT_R16_SINT:
      return xiiGALResourceFormat::R16SInt;
    case DXGI_FORMAT_R8_TYPELESS:
      return xiiGALResourceFormat::R8Typeless;
    case DXGI_FORMAT_R8_UNORM:
      return xiiGALResourceFormat::R8UNormalized;
    case DXGI_FORMAT_R8_UINT:
      return xiiGALResourceFormat::R8UInt;
    case DXGI_FORMAT_R8_SNORM:
      return xiiGALResourceFormat::R8SNormalized;
    case DXGI_FORMAT_R8_SINT:
      return xiiGALResourceFormat::R8SInt;
    case DXGI_FORMAT_A8_UNORM:
      return xiiGALResourceFormat::A8UNormalized;
    case DXGI_FORMAT_R1_UNORM:
      return xiiGALResourceFormat::R1UNormalized;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
      return xiiGALResourceFormat::RGB9E5SharedExponent;
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
      return xiiGALResourceFormat::RG8BG8UNormalized;
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
      return xiiGALResourceFormat::GR8GB8UNormalized;
    case DXGI_FORMAT_BC1_TYPELESS:
      return xiiGALResourceFormat::BC1Typeless;
    case DXGI_FORMAT_BC1_UNORM:
      return xiiGALResourceFormat::BC1UNormalized;
    case DXGI_FORMAT_BC1_UNORM_SRGB:
      return xiiGALResourceFormat::BC1UNormalizedSRGB;
    case DXGI_FORMAT_BC2_TYPELESS:
      return xiiGALResourceFormat::BC2Typeless;
    case DXGI_FORMAT_BC2_UNORM:
      return xiiGALResourceFormat::BC2UNormalized;
    case DXGI_FORMAT_BC2_UNORM_SRGB:
      return xiiGALResourceFormat::BC2UNormalizedSRGB;
    case DXGI_FORMAT_BC3_TYPELESS:
      return xiiGALResourceFormat::BC3Typeless;
    case DXGI_FORMAT_BC3_UNORM:
      return xiiGALResourceFormat::BC3UNormalized;
    case DXGI_FORMAT_BC3_UNORM_SRGB:
      return xiiGALResourceFormat::BC3UNormalizedSRGB;
    case DXGI_FORMAT_BC4_TYPELESS:
      return xiiGALResourceFormat::BC4Typeless;
    case DXGI_FORMAT_BC4_UNORM:
      return xiiGALResourceFormat::BC4UNormalized;
    case DXGI_FORMAT_BC4_SNORM:
      return xiiGALResourceFormat::BC4SNormalized;
    case DXGI_FORMAT_BC5_TYPELESS:
      return xiiGALResourceFormat::BC5Typeless;
    case DXGI_FORMAT_BC5_UNORM:
      return xiiGALResourceFormat::BC5UNormalized;
    case DXGI_FORMAT_BC5_SNORM:
      return xiiGALResourceFormat::BC5SNormalized;
    case DXGI_FORMAT_B5G6R5_UNORM:
      return xiiGALResourceFormat::B5G6R5UNormalized;
    case DXGI_FORMAT_B5G5R5A1_UNORM:
      return xiiGALResourceFormat::B5G5R5A1UNormalized;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
      return xiiGALResourceFormat::BGRA8UNormalized;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
      return xiiGALResourceFormat::BGRX8UNormalized;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
      return xiiGALResourceFormat::R10G10B10XRBiasA2UNormalized;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      return xiiGALResourceFormat::BGRA8Typeless;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      return xiiGALResourceFormat::BGRA8UNormalizedSRGB;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
      return xiiGALResourceFormat::BGRX8Typeless;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return xiiGALResourceFormat::BGRX8UNormalizedSRGB;
    case DXGI_FORMAT_BC6H_TYPELESS:
      return xiiGALResourceFormat::BC6HTypeless;
    case DXGI_FORMAT_BC6H_UF16:
      return xiiGALResourceFormat::BC6HUF16;
    case DXGI_FORMAT_BC6H_SF16:
      return xiiGALResourceFormat::BC6HSF16;
    case DXGI_FORMAT_BC7_TYPELESS:
      return xiiGALResourceFormat::BC7Typeless;
    case DXGI_FORMAT_BC7_UNORM:
      return xiiGALResourceFormat::BC7UNormalized;
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      return xiiGALResourceFormat::BC7UNormalizedSRGB;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALResourceFormat::Unknown;
}

XII_ALWAYS_INLINE D3D12_FILTER xiiD3D12TypeConversions::GetFilter(xiiGALFilterType::Enum minFilter, xiiGALFilterType::Enum magFilter, xiiGALFilterType::Enum mipFilter)
{
  switch (minFilter)
  {
    case xiiGALFilterType::Unknown:
    {
      xiiLog::Error("Filter type is not defined.");
    }
    break;

    // Regular filters
    case xiiGALFilterType::Point:
    {
      if (magFilter == xiiGALFilterType::Point)
      {
        if (mipFilter == xiiGALFilterType::Point)
          return D3D12_FILTER_MIN_MAG_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::Linear)
          return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
      }
      else if (magFilter == xiiGALFilterType::Linear)
      {
        if (mipFilter == xiiGALFilterType::Point)
          return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::Linear)
          return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
      }
    }
    break;

    case xiiGALFilterType::Linear:
    {
      if (magFilter == xiiGALFilterType::Point)
      {
        if (mipFilter == xiiGALFilterType::Point)
          return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::Linear)
          return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
      }
      else if (magFilter == xiiGALFilterType::Linear)
      {
        if (mipFilter == xiiGALFilterType::Point)
          return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::Linear)
          return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
      }
    }
    break;

    case xiiGALFilterType::Anisotropic:
    {
      XII_ASSERT_DEV(magFilter == xiiGALFilterType::Anisotropic && mipFilter == xiiGALFilterType::Anisotropic, "For anisotropic filtering, all filters must be anisotropic.");
      return D3D12_FILTER_ANISOTROPIC;
    }

    // Comparison filters
    case xiiGALFilterType::ComparisonPoint:
    {
      if (magFilter == xiiGALFilterType::ComparisonPoint)
      {
        if (mipFilter == xiiGALFilterType::ComparisonPoint)
          return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::ComparisonLinear)
          return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
      }
      else if (magFilter == xiiGALFilterType::ComparisonLinear)
      {
        if (mipFilter == xiiGALFilterType::ComparisonPoint)
          return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::ComparisonLinear)
          return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
      }
    }
    break;

    case xiiGALFilterType::ComparisonLinear:
    {
      if (magFilter == xiiGALFilterType::ComparisonPoint)
      {
        if (mipFilter == xiiGALFilterType::ComparisonPoint)
          return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::ComparisonLinear)
          return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
      }
      else if (magFilter == xiiGALFilterType::ComparisonLinear)
      {
        if (mipFilter == xiiGALFilterType::ComparisonPoint)
          return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::ComparisonLinear)
          return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
      }
    }
    break;

    case xiiGALFilterType::ComparisonAnisotropic:
    {
      XII_ASSERT_DEV(magFilter == xiiGALFilterType::ComparisonAnisotropic && mipFilter == xiiGALFilterType::ComparisonAnisotropic, "For comparison anisotropic filtering, all filters must be anisotropic");
      return D3D12_FILTER_COMPARISON_ANISOTROPIC;
    }

    // Minimum filters
    case xiiGALFilterType::MinimumPoint:
    {
      if (magFilter == xiiGALFilterType::MinimumPoint)
      {
        if (mipFilter == xiiGALFilterType::MinimumPoint)
          return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::MinimumLinear)
          return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
      }
      else if (magFilter == xiiGALFilterType::MinimumLinear)
      {
        if (mipFilter == xiiGALFilterType::MinimumPoint)
          return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::MinimumLinear)
          return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
      }
    }
    break;

    case xiiGALFilterType::MinimumLinear:
    {
      if (magFilter == xiiGALFilterType::MinimumPoint)
      {
        if (mipFilter == xiiGALFilterType::MinimumPoint)
          return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::MinimumLinear)
          return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
      }
      else if (magFilter == xiiGALFilterType::MinimumLinear)
      {
        if (mipFilter == xiiGALFilterType::MinimumPoint)
          return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::MinimumLinear)
          return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
      }
    }
    break;

    case xiiGALFilterType::MinimumAnisotropic:
    {
      XII_ASSERT_DEV(magFilter == xiiGALFilterType::MinimumAnisotropic && mipFilter == xiiGALFilterType::MinimumAnisotropic, "For minimum anisotropic filtering, all filters must be anisotropic");
      return D3D12_FILTER_MINIMUM_ANISOTROPIC;
    }

    // Maximum filters
    case xiiGALFilterType::MaximumPoint:
    {
      if (magFilter == xiiGALFilterType::MaximumPoint)
      {
        if (mipFilter == xiiGALFilterType::MaximumPoint)
          return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::MaximumLinear)
          return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
      }
      else if (magFilter == xiiGALFilterType::MaximumLinear)
      {
        if (mipFilter == xiiGALFilterType::MaximumPoint)
          return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::MaximumLinear)
          return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
      }
    }
    break;

    case xiiGALFilterType::MaximumLinear:
    {
      if (magFilter == xiiGALFilterType::MaximumPoint)
      {
        if (mipFilter == xiiGALFilterType::MaximumPoint)
          return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::MaximumLinear)
          return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
      }
      else if (magFilter == xiiGALFilterType::MaximumLinear)
      {
        if (mipFilter == xiiGALFilterType::MaximumPoint)
          return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
        else if (mipFilter == xiiGALFilterType::MaximumLinear)
          return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
      }
    }
    break;

    case xiiGALFilterType::MaximumAnisotropic:
    {
      XII_ASSERT_DEV(magFilter == xiiGALFilterType::MaximumAnisotropic && mipFilter == xiiGALFilterType::MaximumAnisotropic, "For maximum anisotropic filtering, all filters must be anisotropic");
      return D3D12_FILTER_MAXIMUM_ANISOTROPIC;
    }

    case xiiGALFilterType::ENUM_COUNT:
    {
      xiiLog::Error("This value does not define a valid filter type");
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  XII_REPORT_FAILURE("Unsupported filter combination");
  return D3D12_FILTER_MIN_MAG_MIP_POINT;
}

XII_ALWAYS_INLINE D3D12_TEXTURE_ADDRESS_MODE xiiD3D12TypeConversions::GetTextureAddressMode(xiiGALTextureAddressMode::Enum e)
{
  switch (e)
  {
    case xiiGALTextureAddressMode::Wrap:
      return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case xiiGALTextureAddressMode::Mirror:
      return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case xiiGALTextureAddressMode::Clamp:
      return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case xiiGALTextureAddressMode::Border:
      return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case xiiGALTextureAddressMode::MirrorOnce:
      return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

XII_ALWAYS_INLINE D3D12_SHADER_VISIBILITY xiiD3D12TypeConversions::GetShaderVisibility(xiiBitflags<xiiGALShaderType> shaderStages)
{
  if (shaderStages == xiiGALShaderType::Vertex)
    return D3D12_SHADER_VISIBILITY_VERTEX;
  if (shaderStages == xiiGALShaderType::Hull)
    return D3D12_SHADER_VISIBILITY_HULL;
  if (shaderStages == xiiGALShaderType::Domain)
    return D3D12_SHADER_VISIBILITY_DOMAIN;
  if (shaderStages == xiiGALShaderType::Geometry)
    return D3D12_SHADER_VISIBILITY_GEOMETRY;
  if (shaderStages == xiiGALShaderType::Pixel)
    return D3D12_SHADER_VISIBILITY_PIXEL;

  return D3D12_SHADER_VISIBILITY_ALL;
}

XII_ALWAYS_INLINE bool xiiD3D12TypeConversions::TryGetDescriptorRangeType(xiiEnum<xiiGALShaderResourceType> resourceType, D3D12_DESCRIPTOR_RANGE_TYPE& out_rangeType)
{
  switch (resourceType)
  {
    case xiiGALShaderResourceType::ConstantBuffer:
      out_rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
      return true;

    case xiiGALShaderResourceType::TextureSRV:
    case xiiGALShaderResourceType::BufferSRV:
    case xiiGALShaderResourceType::InputAttachment:
    case xiiGALShaderResourceType::AccelerationStructure:
    case xiiGALShaderResourceType::TextureAndSampler:
      out_rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      return true;

    case xiiGALShaderResourceType::TextureUAV:
    case xiiGALShaderResourceType::BufferUAV:
      out_rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      return true;

    case xiiGALShaderResourceType::Sampler:
      out_rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
      return true;

    default:
      break;
  }

  return false;
}

XII_ALWAYS_INLINE D3D12_STATIC_BORDER_COLOR xiiD3D12TypeConversions::GetStaticBorderColor(const xiiColor& color)
{
  if (color.a <= 0.0f)
    return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;

  if (color.r <= 0.0f && color.g <= 0.0f && color.b <= 0.0f)
    return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

  return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
}

XII_ALWAYS_INLINE D3D12_QUERY_HEAP_TYPE xiiD3D12TypeConversions::GetQueryType(xiiGALQueryType::Enum e)
{
  switch (e)
  {
    case xiiGALQueryType::Occlusion:
    case xiiGALQueryType::BinaryOcclusion:
      return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
    case xiiGALQueryType::Duration:
    case xiiGALQueryType::Timestamp:
      return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    case xiiGALQueryType::PipelineStatistics:
      return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
}

XII_ALWAYS_INLINE D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS xiiD3D12TypeConversions::GetAccelerationStructureBuildFlags(xiiBitflags<xiiGALRayTracingBuildASFlags> flags)
{
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS d3d12Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

  if (flags.IsSet(xiiGALRayTracingBuildASFlags::AllowUpdate))
    d3d12Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
  if (flags.IsSet(xiiGALRayTracingBuildASFlags::AllowCompaction))
    d3d12Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
  if (flags.IsSet(xiiGALRayTracingBuildASFlags::PreferFastTrace))
    d3d12Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  if (flags.IsSet(xiiGALRayTracingBuildASFlags::PreferFastBuild))
    d3d12Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
  if (flags.IsSet(xiiGALRayTracingBuildASFlags::LowMemory))
    d3d12Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;

  return d3d12Flags;
}

XII_ALWAYS_INLINE DXGI_FORMAT xiiD3D12TypeConversions::GetBLASTriangleVertexFormat(const xiiGALBLASTriangleDescription& triangle)
{
  if (triangle.m_VertexValueType == xiiGALValueType::Float32)
  {
    return triangle.m_uiVertexComponentCount == 2U ? DXGI_FORMAT_R32G32_FLOAT : DXGI_FORMAT_R32G32B32_FLOAT;
  }
  if (triangle.m_VertexValueType == xiiGALValueType::Float16)
  {
    return triangle.m_uiVertexComponentCount == 2U ? DXGI_FORMAT_R16G16_FLOAT : DXGI_FORMAT_R16G16B16A16_FLOAT;
  }
  if (triangle.m_VertexValueType == xiiGALValueType::Int32)
  {
    return triangle.m_uiVertexComponentCount == 2U ? DXGI_FORMAT_R32G32_SINT : DXGI_FORMAT_R32G32B32_SINT;
  }

  XII_REPORT_FAILURE("Unsupported BLAS triangle vertex value type: {}.", xiiArgEnum(triangle.m_VertexValueType));
  return DXGI_FORMAT_UNKNOWN;
}

XII_ALWAYS_INLINE DXGI_FORMAT xiiD3D12TypeConversions::GetBLASIndexFormat(xiiEnum<xiiGALValueType> indexType)
{
  switch (indexType)
  {
    case xiiGALValueType::UInt16:
      return DXGI_FORMAT_R16_UINT;
    case xiiGALValueType::UInt32:
      return DXGI_FORMAT_R32_UINT;
    case xiiGALValueType::Undefined:
      return DXGI_FORMAT_UNKNOWN;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return DXGI_FORMAT_UNKNOWN;
}

XII_ALWAYS_INLINE xiiUInt32 xiiD3D12TypeConversions::GetBLASTriangleVertexStride(const xiiGALBLASTriangleDescription& triangle)
{
  if (triangle.m_VertexValueType == xiiGALValueType::Float16 && triangle.m_uiVertexComponentCount == 3U)
  {
    // D3D12 does not expose a packed 3x16-bit float triangle format for DXR geometry descriptors.
    // Use 4-component alignment for descriptor validation and expect padded vertex data.
    return sizeof(xiiUInt16) * 4U;
  }

  xiiUInt32 uiComponentSize = 0U;
  switch (triangle.m_VertexValueType)
  {
    case xiiGALValueType::Float16:
      uiComponentSize = sizeof(xiiUInt16);
      break;
    case xiiGALValueType::Float32:
    case xiiGALValueType::Int32:
      uiComponentSize = sizeof(xiiUInt32);
      break;
    default:
      XII_REPORT_FAILURE("Unsupported BLAS triangle vertex value type for stride: {}.", xiiArgEnum(triangle.m_VertexValueType));
      return 0U;
  }

  return triangle.m_uiVertexComponentCount * uiComponentSize;
}

XII_ALWAYS_INLINE DXGI_FORMAT xiiD3D12TypeConversions::GetDXGIFormatFromType(xiiGALValueType::Enum e, xiiUInt32 uiComponentCount, bool bIsNormalized)
{
  switch (e)
  {
    case xiiGALValueType::Int8:
    {
      if (bIsNormalized)
      {
        switch (uiComponentCount)
        {
          case 1:
            return DXGI_FORMAT_R8_SNORM;
          case 2:
            return DXGI_FORMAT_R8G8_SNORM;
          case 4:
            return DXGI_FORMAT_R8G8B8A8_SNORM;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
      else
      {
        switch (uiComponentCount)
        {
          case 1:
            return DXGI_FORMAT_R8_SINT;
          case 2:
            return DXGI_FORMAT_R8G8_SINT;
          case 4:
            return DXGI_FORMAT_R8G8B8A8_SINT;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    break;
    case xiiGALValueType::Int16:
    {
      if (bIsNormalized)
      {
        switch (uiComponentCount)
        {
          case 1:
            return DXGI_FORMAT_R16_SNORM;
          case 2:
            return DXGI_FORMAT_R16G16_SNORM;
          case 4:
            return DXGI_FORMAT_R16G16B16A16_SNORM;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
      else
      {
        switch (uiComponentCount)
        {
          case 1:
            return DXGI_FORMAT_R16_SINT;
          case 2:
            return DXGI_FORMAT_R16G16_SINT;
          case 4:
            return DXGI_FORMAT_R16G16B16A16_SINT;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    break;
    case xiiGALValueType::Int32:
    {
      XII_ASSERT_DEV(bIsNormalized, "32-bit signed normalized formats are unsupported. Use xiiGALResourceFormat::R32Float instead.");

      switch (uiComponentCount)
      {
        case 1:
          return DXGI_FORMAT_R32_SINT;
        case 2:
          return DXGI_FORMAT_R32G32_SINT;
        case 3:
          return DXGI_FORMAT_R32G32B32_SINT;
        case 4:
          return DXGI_FORMAT_R32G32B32A32_SINT;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    break;
    case xiiGALValueType::UInt8:
    {
      if (bIsNormalized)
      {
        switch (uiComponentCount)
        {
          case 1:
            return DXGI_FORMAT_R8_UNORM;
          case 2:
            return DXGI_FORMAT_R8G8_UNORM;
          case 4:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
      else
      {
        switch (uiComponentCount)
        {
          case 1:
            return DXGI_FORMAT_R8_UINT;
          case 2:
            return DXGI_FORMAT_R8G8_UINT;
          case 4:
            return DXGI_FORMAT_R8G8B8A8_UINT;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    break;
    case xiiGALValueType::UInt16:
    {
      if (bIsNormalized)
      {
        switch (uiComponentCount)
        {
          case 1:
            return DXGI_FORMAT_R16_UNORM;
          case 2:
            return DXGI_FORMAT_R16G16_UNORM;
          case 4:
            return DXGI_FORMAT_R16G16B16A16_UNORM;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
      else
      {
        switch (uiComponentCount)
        {
          case 1:
            return DXGI_FORMAT_R16_UINT;
          case 2:
            return DXGI_FORMAT_R16G16_UINT;
          case 4:
            return DXGI_FORMAT_R16G16B16A16_UINT;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    break;
    case xiiGALValueType::UInt32:
    {
      XII_ASSERT_DEV(bIsNormalized, "32-bit unsigned normalized formats are unsupported. Use xiiGALResourceFormat::R32Float instead.");

      switch (uiComponentCount)
      {
        case 1:
          return DXGI_FORMAT_R32_UINT;
        case 2:
          return DXGI_FORMAT_R32G32_UINT;
        case 3:
          return DXGI_FORMAT_R32G32B32_UINT;
        case 4:
          return DXGI_FORMAT_R32G32B32A32_UINT;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    break;
    case xiiGALValueType::Float16:
    {
      XII_ASSERT_DEV(!bIsNormalized, "Floating point formats can not be normalized.");

      switch (uiComponentCount)
      {
        case 1:
          return DXGI_FORMAT_R16_FLOAT;
        case 2:
          return DXGI_FORMAT_R16G16_FLOAT;
        case 4:
          return DXGI_FORMAT_R16G16B16A16_FLOAT;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    break;
    case xiiGALValueType::Float32:
    {
      XII_ASSERT_DEV(!bIsNormalized, "Floating point formats can not be normalized.");

      switch (uiComponentCount)
      {
        case 1:
          return DXGI_FORMAT_R32_FLOAT;
        case 2:
          return DXGI_FORMAT_R32G32_FLOAT;
        case 3:
          return DXGI_FORMAT_R32G32B32_FLOAT;
        case 4:
          return DXGI_FORMAT_R32G32B32A32_FLOAT;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    break;
    case xiiGALValueType::Float64:
    {
      xiiLog::Error("Float64 vertex formats are unsupported in D3D12.");
      return DXGI_FORMAT_UNKNOWN;
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return DXGI_FORMAT_UNKNOWN;
}

XII_ALWAYS_INLINE D3D_PRIMITIVE_TOPOLOGY xiiD3D12TypeConversions::GetPrimitiveTopology(xiiGALPrimitiveTopology::Enum e)
{
  switch (e)
  {
    case xiiGALPrimitiveTopology::PointList:
      return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case xiiGALPrimitiveTopology::LineList:
      return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case xiiGALPrimitiveTopology::TriangleList:
      return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case xiiGALPrimitiveTopology::TriangleStrip:
      return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case xiiGALPrimitiveTopology::LineStrip:
      return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case xiiGALPrimitiveTopology::TriangleListAdjacent:
      return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
    case xiiGALPrimitiveTopology::TriangleStripAdjacent:
      return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
    case xiiGALPrimitiveTopology::LineListAdjacent:
      return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
    case xiiGALPrimitiveTopology::LineStripAdjacent:
      return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
    case xiiGALPrimitiveTopology::ControlPointPatchList1:
      return D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList2:
      return D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList3:
      return D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList4:
      return D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList5:
      return D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList6:
      return D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList7:
      return D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList8:
      return D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList9:
      return D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList10:
      return D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList11:
      return D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList12:
      return D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList13:
      return D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList14:
      return D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList15:
      return D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList16:
      return D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList17:
      return D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList18:
      return D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList19:
      return D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList20:
      return D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList21:
      return D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList22:
      return D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList23:
      return D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList24:
      return D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList25:
      return D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList26:
      return D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList27:
      return D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList28:
      return D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList29:
      return D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList30:
      return D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList31:
      return D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST;
    case xiiGALPrimitiveTopology::ControlPointPatchList32:
      return D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

XII_ALWAYS_INLINE D3D12_PRIMITIVE_TOPOLOGY_TYPE xiiD3D12TypeConversions::GetPrimitiveTopologyType(xiiEnum<xiiGALPrimitiveTopology> primitiveTopology)
{
  if (primitiveTopology >= xiiGALPrimitiveTopology::ControlPointPatchList1 && primitiveTopology <= xiiGALPrimitiveTopology::ControlPointPatchList32)
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;

  switch (primitiveTopology)
  {
    case xiiGALPrimitiveTopology::PointList:
      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

    case xiiGALPrimitiveTopology::LineList:
    case xiiGALPrimitiveTopology::LineStrip:
    case xiiGALPrimitiveTopology::LineListAdjacent:
    case xiiGALPrimitiveTopology::LineStripAdjacent:
      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

    case xiiGALPrimitiveTopology::TriangleList:
    case xiiGALPrimitiveTopology::TriangleStrip:
    case xiiGALPrimitiveTopology::TriangleListAdjacent:
    case xiiGALPrimitiveTopology::TriangleStripAdjacent:
      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    default:
      break;
  }

  return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

XII_ALWAYS_INLINE D3D12_INPUT_CLASSIFICATION xiiD3D12TypeConversions::GetElementFrequency(xiiGALInputElementFrequency::Enum e)
{
  switch (e)
  {
    case xiiGALInputElementFrequency::PerVertex:
      return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    case xiiGALInputElementFrequency::PerInstance:
      return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
}

XII_ALWAYS_INLINE xiiUInt32 xiiD3D12TypeConversions::CalculateSubResourceIndex(xiiUInt32 uiMipSlice, xiiUInt32 uiArraySlice, xiiUInt32 uiMipLevelCount)
{
  return uiMipSlice + (uiArraySlice * uiMipLevelCount);
}

XII_ALWAYS_INLINE xiiUInt32 xiiD3D12TypeConversions::CalculateSubResourceIndex(xiiUInt32 uiMipSlice, xiiUInt32 uiArraySlice, xiiUInt32 uiPlaneSlice, xiiUInt32 uiMipLevelCount, xiiUInt32 uiArraySize)
{
  return uiMipSlice + (uiArraySlice * uiMipLevelCount) + (uiPlaneSlice * uiMipLevelCount * uiArraySize);
}

XII_ALWAYS_INLINE D3D12_RESOURCE_STATES xiiD3D12TypeConversions::GetResourceState(xiiBitflags<xiiGALResourceStateFlags> e)
{
  D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_COMMON;

  if (e.IsSet(xiiGALResourceStateFlags::Undefined))
    resourceStates |= static_cast<D3D12_RESOURCE_STATES>(0);
  if (e.IsSet(xiiGALResourceStateFlags::VertexBuffer))
    resourceStates |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  if (e.IsSet(xiiGALResourceStateFlags::ConstantBuffer))
    resourceStates |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  if (e.IsSet(xiiGALResourceStateFlags::IndexBuffer))
    resourceStates |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
  if (e.IsSet(xiiGALResourceStateFlags::RenderTarget))
    resourceStates |= D3D12_RESOURCE_STATE_RENDER_TARGET;
  if (e.IsSet(xiiGALResourceStateFlags::UnorderedAccess))
    resourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  if (e.IsSet(xiiGALResourceStateFlags::DepthWrite))
    resourceStates |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
  if (e.IsSet(xiiGALResourceStateFlags::DepthRead))
    resourceStates |= D3D12_RESOURCE_STATE_DEPTH_READ;
  if (e.IsSet(xiiGALResourceStateFlags::ShaderResource))
    resourceStates |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  if (e.IsSet(xiiGALResourceStateFlags::StreamOut))
    resourceStates |= D3D12_RESOURCE_STATE_STREAM_OUT;
  if (e.IsSet(xiiGALResourceStateFlags::IndirectArgument))
    resourceStates |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
  if (e.IsSet(xiiGALResourceStateFlags::CopyDestination))
    resourceStates |= D3D12_RESOURCE_STATE_COPY_DEST;
  if (e.IsSet(xiiGALResourceStateFlags::CopySource))
    resourceStates |= D3D12_RESOURCE_STATE_COPY_SOURCE;
  if (e.IsSet(xiiGALResourceStateFlags::ResolveDestination))
    resourceStates |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
  if (e.IsSet(xiiGALResourceStateFlags::ResolveSource))
    resourceStates |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
  if (e.IsSet(xiiGALResourceStateFlags::InputAttachment))
    resourceStates |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  if (e.IsSet(xiiGALResourceStateFlags::Present))
    resourceStates |= D3D12_RESOURCE_STATE_PRESENT;
  if (e.IsSet(xiiGALResourceStateFlags::BuildASRead))
    resourceStates |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
  if (e.IsSet(xiiGALResourceStateFlags::BuildASWrite))
    resourceStates |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
  if (e.IsSet(xiiGALResourceStateFlags::RayTracing))
    resourceStates |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
  if (e.IsSet(xiiGALResourceStateFlags::Common))
    resourceStates |= D3D12_RESOURCE_STATE_COMMON;
  if (e.IsSet(xiiGALResourceStateFlags::ShadingRate))
    resourceStates |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

  return resourceStates;
}

XII_ALWAYS_INLINE D3D12_SHADING_RATE xiiD3D12TypeConversions::GetShadingRate(xiiBitflags<xiiGALShadingRateFlags> e)
{
  switch (e.GetValue())
  {
    case xiiGALShadingRateFlags::_1X1:
      return D3D12_SHADING_RATE_1X1;
    case xiiGALShadingRateFlags::_1X2:
      return D3D12_SHADING_RATE_1X2;
    case xiiGALShadingRateFlags::_2X1:
      return D3D12_SHADING_RATE_2X1;
    case xiiGALShadingRateFlags::_2X2:
      return D3D12_SHADING_RATE_2X2;
    case xiiGALShadingRateFlags::_2X4:
      return D3D12_SHADING_RATE_2X4;
    case xiiGALShadingRateFlags::_4X2:
      return D3D12_SHADING_RATE_4X2;
    case xiiGALShadingRateFlags::_4X4:
      return D3D12_SHADING_RATE_4X4;

    case xiiGALShadingRateFlags::_1X4:
    case xiiGALShadingRateFlags::_4X1:
      xiiLog::Error("Shading rate '{}' is unsupported by Direct3D12.", xiiArgEnum(e));
      return D3D12_SHADING_RATE_1X1;

    default:
      XII_REPORT_FAILURE("Unknown shading rate value.");
      return D3D12_SHADING_RATE_1X1;
  }
}

XII_ALWAYS_INLINE D3D12_SHADING_RATE_COMBINER xiiD3D12TypeConversions::GetShadingRateCombiner(xiiBitflags<xiiGALShadingRateCombinerFlags> e)
{
  XII_ASSERT_DEV(xiiMath::IsPowerOf2(e.GetValue()), "Expected a single combiner flag.");

  switch (e.GetValue())
  {
    case xiiGALShadingRateCombinerFlags::PassThrough:
      return D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
    case xiiGALShadingRateCombinerFlags::CombinerOverride:
      return D3D12_SHADING_RATE_COMBINER_OVERRIDE;
    case xiiGALShadingRateCombinerFlags::CombinerMin:
      return D3D12_SHADING_RATE_COMBINER_MIN;
    case xiiGALShadingRateCombinerFlags::CombinerMax:
      return D3D12_SHADING_RATE_COMBINER_MAX;
    case xiiGALShadingRateCombinerFlags::CombinerSum:
      return D3D12_SHADING_RATE_COMBINER_SUM;
    case xiiGALShadingRateCombinerFlags::CombinerMul:
      xiiLog::Error("Shading rate combiner '{}' is unsupported by Direct3D12.", xiiArgEnum(e));
      return D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
    default:
      XII_REPORT_FAILURE("Unknown shading rate combiner.");
      return D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
  }
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiD3D12TypeConversions::GetResourceStateFromBindFlags(xiiBitflags<xiiGALBindFlags> bindFlags)
{
  xiiBitflags<xiiGALResourceStateFlags> resourceStates = xiiGALResourceStateFlags::Unknown;

  for (xiiUInt32 uiBit : bindFlags)
  {
    switch (uiBit)
    {
      case xiiGALBindFlags::VertexBuffer:
        resourceStates |= xiiGALResourceStateFlags::VertexBuffer;
        break;
      case xiiGALBindFlags::IndexBuffer:
        resourceStates |= xiiGALResourceStateFlags::IndexBuffer;
        break;
      case xiiGALBindFlags::UniformBuffer:
        resourceStates |= xiiGALResourceStateFlags::ConstantBuffer;
        break;
      case xiiGALBindFlags::ShaderResource:
        resourceStates |= xiiGALResourceStateFlags::ShaderResource;
        break;
      case xiiGALBindFlags::StreamOutput:
        resourceStates |= xiiGALResourceStateFlags::StreamOut;
        break;
      case xiiGALBindFlags::RenderTarget:
        resourceStates |= xiiGALResourceStateFlags::RenderTarget;
        break;
      case xiiGALBindFlags::DepthStencil:
        resourceStates |= xiiGALResourceStateFlags::DepthWrite;
        break;
      case xiiGALBindFlags::UnorderedAccess:
        resourceStates |= xiiGALResourceStateFlags::UnorderedAccess;
        break;
      case xiiGALBindFlags::IndirectDrawArguments:
        resourceStates |= xiiGALResourceStateFlags::IndirectArgument;
        break;
      case xiiGALBindFlags::InputAttachment:
        resourceStates |= xiiGALResourceStateFlags::InputAttachment;
        break;
      case xiiGALBindFlags::RayTracing:
        resourceStates |= xiiGALResourceStateFlags::RayTracing;
        break;
      case xiiGALBindFlags::ShadingRate:
        resourceStates |= xiiGALResourceStateFlags::ShadingRate;
        break;
      case xiiGALBindFlags::None:
        break;
      default:
        XII_REPORT_FAILURE("Unexpected bind flag while mapping to D3D12 resource state.");
        break;
    }
  }

  return resourceStates;
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiD3D12TypeConversions::GetDynamicBufferState()
{
  return xiiGALResourceStateFlags::VertexBuffer | xiiGALResourceStateFlags::IndexBuffer | xiiGALResourceStateFlags::ConstantBuffer | xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::CopySource | xiiGALResourceStateFlags::IndirectArgument;
}

XII_ALWAYS_INLINE D3D12_RESOURCE_FLAGS xiiD3D12TypeConversions::GetBufferResourceFlagsFromBindFlags(xiiBitflags<xiiGALBindFlags> bindFlags)
{
  D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

  if (bindFlags.IsAnySet(xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::RayTracing))
  {
    resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }

  if (!bindFlags.IsSet(xiiGALBindFlags::ShaderResource) && !bindFlags.IsSet(xiiGALBindFlags::RayTracing))
  {
    resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  }

  return resourceFlags;
}

XII_ALWAYS_INLINE D3D12_RESOURCE_FLAGS xiiD3D12TypeConversions::GetTextureResourceFlagsFromBindFlags(xiiBitflags<xiiGALBindFlags> bindFlags)
{
  D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

  if (bindFlags.IsSet(xiiGALBindFlags::RenderTarget))
  {
    resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  }

  if (bindFlags.IsSet(xiiGALBindFlags::DepthStencil))
  {
    resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  }

  if (bindFlags.IsSet(xiiGALBindFlags::UnorderedAccess))
  {
    resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }

  if (!bindFlags.IsAnySet(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::InputAttachment | xiiGALBindFlags::ShadingRate))
  {
    resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  }

  return resourceFlags;
}

XII_ALWAYS_INLINE D3D12_RESOURCE_DIMENSION xiiD3D12TypeConversions::GetResourceDimension(xiiEnum<xiiGALResourceDimension> dimension)
{
  switch (dimension)
  {
    case xiiGALResourceDimension::Texture1D:
    case xiiGALResourceDimension::Texture1DArray:
      return D3D12_RESOURCE_DIMENSION_TEXTURE1D;

    case xiiGALResourceDimension::Texture2D:
    case xiiGALResourceDimension::Texture2DArray:
    case xiiGALResourceDimension::TextureCube:
    case xiiGALResourceDimension::TextureCubeArray:
      return D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    case xiiGALResourceDimension::Texture3D:
      return D3D12_RESOURCE_DIMENSION_TEXTURE3D;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return D3D12_RESOURCE_DIMENSION_UNKNOWN;
}

XII_ALWAYS_INLINE UINT xiiD3D12TypeConversions::GetShaderComponentMapping(const xiiGALTextureComponentMapping& componentMapping)
{
  auto ConvertSwizzle = [](xiiGALTextureComponentSwizzle::Enum swizzle, UINT uiIdentity) -> UINT {
    switch (swizzle)
    {
      case xiiGALTextureComponentSwizzle::Identity:
        return uiIdentity;
      case xiiGALTextureComponentSwizzle::R:
        return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0;
      case xiiGALTextureComponentSwizzle::G:
        return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1;
      case xiiGALTextureComponentSwizzle::B:
        return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2;
      case xiiGALTextureComponentSwizzle::A:
        return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3;
      case xiiGALTextureComponentSwizzle::Zero:
        return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
      case xiiGALTextureComponentSwizzle::One:
        return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;

      default:
        XII_REPORT_FAILURE("Unexpected texture component swizzle value.");
        return uiIdentity;
    }
  };

  const UINT uiRed   = ConvertSwizzle(componentMapping.m_R, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
  const UINT uiGreen = ConvertSwizzle(componentMapping.m_G, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1);
  const UINT uiBlue  = ConvertSwizzle(componentMapping.m_B, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2);
  const UINT uiAlpha = ConvertSwizzle(componentMapping.m_A, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3);

  return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(uiRed, uiGreen, uiBlue, uiAlpha);
}

XII_ALWAYS_INLINE D3D12_CLEAR_VALUE xiiD3D12TypeConversions::GetClearValue(const xiiGALOptimizedClearValue& clearValue)
{
  D3D12_CLEAR_VALUE                      optimizedClearValue = {};
  const xiiGALResourceFormatDescription& formatDescription   = xiiGALTextureUtilities::GetResourceFormatProperties(clearValue.m_ResourceFormat);

  if (formatDescription.m_ComponentType == xiiGALResourceFormatComponentType::Depth || formatDescription.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
  {
    optimizedClearValue.DepthStencil.Depth   = clearValue.m_DepthStencil.m_fDepth;
    optimizedClearValue.DepthStencil.Stencil = clearValue.m_DepthStencil.m_uiStencil;
  }
  else
  {
    optimizedClearValue.Color[0] = clearValue.m_ClearColour.r;
    optimizedClearValue.Color[1] = clearValue.m_ClearColour.g;
    optimizedClearValue.Color[2] = clearValue.m_ClearColour.b;
    optimizedClearValue.Color[3] = clearValue.m_ClearColour.a;
  }

  return optimizedClearValue;
}

XII_ALWAYS_INLINE D3D12_RESOURCE_STATES xiiD3D12TypeConversions::GetSupportedD3D12ResourceStatesForCommandList(xiiBitflags<xiiGALCommandQueueFlags> queueFlags)
{
  constexpr D3D12_RESOURCE_STATES transferResourceStates = D3D12_RESOURCE_STATE_COMMON | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE;
  constexpr D3D12_RESOURCE_STATES computeResourceStates  = transferResourceStates | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT | D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
  constexpr D3D12_RESOURCE_STATES graphicsResourceStates = computeResourceStates | D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_RENDER_TARGET | D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_STREAM_OUT | D3D12_RESOURCE_STATE_RESOLVE_DEST | D3D12_RESOURCE_STATE_RESOLVE_SOURCE | D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

  if (queueFlags == xiiGALCommandQueueFlags::Graphics)
    return graphicsResourceStates;
  else if (queueFlags == xiiGALCommandQueueFlags::Compute)
    return computeResourceStates;
  else if (queueFlags == xiiGALCommandQueueFlags::Transfer)
    return transferResourceStates;

  XII_REPORT_FAILURE("Unexpected command queue type.");

  return D3D12_RESOURCE_STATE_COMMON;
}

XII_ALWAYS_INLINE D3D12_RESOURCE_BARRIER_FLAGS xiiD3D12TypeConversions::GetResourceBarrierFlags(xiiEnum<xiiGALStateTransitionType> type)
{
  switch (type)
  {
    case xiiGALStateTransitionType::Immediate:
      return D3D12_RESOURCE_BARRIER_FLAG_NONE;
    case xiiGALStateTransitionType::Begin:
      return D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
    case xiiGALStateTransitionType::End:
      return D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
    default:
      XII_REPORT_FAILURE("Unexpected state transition type.");
      return D3D12_RESOURCE_BARRIER_FLAG_NONE;
  }
}
