
XII_ALWAYS_INLINE Diligent::RENDER_DEVICE_TYPE xiiDiligentUtils::GetDiligentRenderDeviceType(const xiiGraphicsDeviceType::Enum type)
{
  switch (type)
  {
    case xiiGraphicsDeviceType::OpenGLES:
      return Diligent::RENDER_DEVICE_TYPE_GLES;
    case xiiGraphicsDeviceType::OpenGL:
      return Diligent::RENDER_DEVICE_TYPE_GL;
    case xiiGraphicsDeviceType::D3D11:
      return Diligent::RENDER_DEVICE_TYPE_D3D11;
    case xiiGraphicsDeviceType::D3D12:
      return Diligent::RENDER_DEVICE_TYPE_D3D12;
    case xiiGraphicsDeviceType ::Vulkan:
      return Diligent::RENDER_DEVICE_TYPE_VULKAN;
    case xiiGraphicsDeviceType::Metal:
      return Diligent::RENDER_DEVICE_TYPE_METAL;
  }
  return Diligent::RENDER_DEVICE_TYPE_UNDEFINED;
}

XII_ALWAYS_INLINE xiiUInt8 xiiDiligentUtils::ToDiligentMSAACount(xiiEnum<xiiGALMSAASampleCount> sampleCount)
{
  return static_cast<xiiUInt8>(sampleCount.GetValue());
}

XII_ALWAYS_INLINE xiiEnum<xiiGALMSAASampleCount> xiiDiligentUtils::ToGALMSAASampleCount(xiiUInt32 uiSampleCount)
{
  xiiEnum<xiiGALMSAASampleCount> result;
  switch (uiSampleCount)
  {
    case 1:
      result = xiiGALMSAASampleCount::None;
      break;
    case 2:
      result = xiiGALMSAASampleCount::TwoSamples;
      break;
    case 4:
      result = xiiGALMSAASampleCount::FourSamples;
      break;
    case 8:
      result = xiiGALMSAASampleCount::EightSamples;
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return result;
}

XII_ALWAYS_INLINE Diligent::FILTER_TYPE xiiDiligentUtils ::ToDiligentFilter(xiiGALTextureFilterMode::Enum e)
{
  switch (e)
  {
    case xiiGALTextureFilterMode::Undefined:
      return Diligent::FILTER_TYPE_UNKNOWN;
    case xiiGALTextureFilterMode::Point:
      return Diligent::FILTER_TYPE_POINT;
    case xiiGALTextureFilterMode::Linear:
      return Diligent::FILTER_TYPE_LINEAR;
    case xiiGALTextureFilterMode::Anisotropic:
      return Diligent::FILTER_TYPE_ANISOTROPIC;
    case xiiGALTextureFilterMode::ComparisonPoint:
      return Diligent::FILTER_TYPE_COMPARISON_POINT;
    case xiiGALTextureFilterMode::ComparisonLinear:
      return Diligent::FILTER_TYPE_COMPARISON_LINEAR;
    case xiiGALTextureFilterMode::ComparisonAnisotropic:
      return Diligent::FILTER_TYPE_COMPARISON_ANISOTROPIC;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::FILTER_TYPE_UNKNOWN;
}

XII_ALWAYS_INLINE Diligent::STENCIL_OP xiiDiligentUtils::ToDiligentStencilOperation(xiiGALStencilOperation::Enum e)
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
    case xiiGALStencilOperation::IncrementSaturated:
      return Diligent::STENCIL_OP_INCR_SAT;
    case xiiGALStencilOperation::DecrementSaturated:
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

XII_ALWAYS_INLINE Diligent::BLEND_OPERATION xiiDiligentUtils::ToDiligentBlendOperation(xiiGALBlendOperation::Enum e)
{
  switch (e)
  {
    case xiiGALBlendOperation::Undefined:
      return Diligent::BLEND_OPERATION_UNDEFINED;
    case xiiGALBlendOperation::Add:
      return Diligent::BLEND_OPERATION_ADD;
    case xiiGALBlendOperation::Subtract:
      return Diligent::BLEND_OPERATION_SUBTRACT;
    case xiiGALBlendOperation::RevSubtract:
      return Diligent::BLEND_OPERATION_REV_SUBTRACT;
    case xiiGALBlendOperation::Min:
      return Diligent::BLEND_OPERATION_MIN;
    case xiiGALBlendOperation::Max:
      return Diligent::BLEND_OPERATION_MAX;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::BLEND_OPERATION_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::BLEND_FACTOR xiiDiligentUtils::ToDiligentBlendFactor(xiiGALBlendFactor::Enum e)
{
  switch (e)
  {
    case xiiGALBlendFactor::Undefined:
      return Diligent::BLEND_FACTOR_UNDEFINED;
    case xiiGALBlendFactor::Zero:
      return Diligent::BLEND_FACTOR_ZERO;
    case xiiGALBlendFactor::One:
      return Diligent::BLEND_FACTOR_ONE;
    case xiiGALBlendFactor::SrcColor:
      return Diligent::BLEND_FACTOR_SRC_COLOR;
    case xiiGALBlendFactor::InvSrcColor:
      return Diligent::BLEND_FACTOR_INV_SRC_COLOR;
    case xiiGALBlendFactor::SrcAlpha:
      return Diligent::BLEND_FACTOR_SRC_ALPHA;
    case xiiGALBlendFactor::InvSrcAlpha:
      return Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
    case xiiGALBlendFactor::DestAlpha:
      return Diligent::BLEND_FACTOR_DEST_ALPHA;
    case xiiGALBlendFactor::InvDestAlpha:
      return Diligent::BLEND_FACTOR_INV_DEST_ALPHA;
    case xiiGALBlendFactor::DestColor:
      return Diligent::BLEND_FACTOR_DEST_COLOR;
    case xiiGALBlendFactor::InvDestColor:
      return Diligent::BLEND_FACTOR_INV_DEST_COLOR;
    case xiiGALBlendFactor::SrcAlphaSaturated:
      return Diligent::BLEND_FACTOR_SRC_ALPHA_SAT;
    case xiiGALBlendFactor::BlendFactor:
      return Diligent::BLEND_FACTOR_BLEND_FACTOR;
    case xiiGALBlendFactor::InvBlendFactor:
      return Diligent::BLEND_FACTOR_INV_BLEND_FACTOR;
    case xiiGALBlendFactor::SrcOneColor:
      return Diligent::BLEND_FACTOR_SRC1_COLOR;
    case xiiGALBlendFactor::InvSrcOneColor:
      return Diligent::BLEND_FACTOR_INV_SRC1_COLOR;
    case xiiGALBlendFactor::SrcOneAlpha:
      return Diligent::BLEND_FACTOR_SRC1_ALPHA;
    case xiiGALBlendFactor::InvSrcOneAlpha:
      return Diligent::BLEND_FACTOR_INV_SRC1_ALPHA;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::BLEND_FACTOR_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::VALUE_TYPE xiiDiligentUtils::GALNumBitsToDiligentValueType(xiiUInt32 value)
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return Diligent::VT_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::VALUE_TYPE xiiDiligentUtils::GALToDiligentFormat(Diligent::TEXTURE_FORMAT format)
{
  switch (format)
  {
    case Diligent::TEX_FORMAT_RGBA32_FLOAT:
    case Diligent::TEX_FORMAT_RGB32_FLOAT:
    case Diligent::TEX_FORMAT_RG32_FLOAT:
    case Diligent::TEX_FORMAT_D32_FLOAT:
    case Diligent::TEX_FORMAT_R32_FLOAT:
    case Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT:
      return Diligent::VT_FLOAT32;

    // case Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP:
    case Diligent::TEX_FORMAT_RGBA16_FLOAT:
    case Diligent::TEX_FORMAT_R11G11B10_FLOAT:
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

XII_ALWAYS_INLINE bool xiiDiligentUtils::GALIsFormatNormalized(Diligent::TEXTURE_FORMAT format)
{
  switch (format)
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

    case Diligent::TEX_FORMAT_UNKNOWN:
    case Diligent::TEX_FORMAT_RGBA32_TYPELESS:
    case Diligent::TEX_FORMAT_RGBA32_FLOAT:
    case Diligent::TEX_FORMAT_RGBA32_UINT:
    case Diligent::TEX_FORMAT_RGBA32_SINT:
    case Diligent::TEX_FORMAT_RGB32_TYPELESS:
    case Diligent::TEX_FORMAT_RGB32_FLOAT:
    case Diligent::TEX_FORMAT_RGB32_UINT:
    case Diligent::TEX_FORMAT_RGB32_SINT:
    case Diligent::TEX_FORMAT_RGBA16_TYPELESS:
    case Diligent::TEX_FORMAT_RGBA16_FLOAT:
    case Diligent::TEX_FORMAT_RGBA16_UINT:
    case Diligent::TEX_FORMAT_RGBA16_SINT:
    case Diligent::TEX_FORMAT_RG32_TYPELESS:
    case Diligent::TEX_FORMAT_RG32_FLOAT:
    case Diligent::TEX_FORMAT_RG32_UINT:
    case Diligent::TEX_FORMAT_RG32_SINT:
    case Diligent::TEX_FORMAT_R32G8X24_TYPELESS:
    case Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT:
    case Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT:
    case Diligent::TEX_FORMAT_RGB10A2_TYPELESS:
    case Diligent::TEX_FORMAT_RGB10A2_UINT:
    case Diligent::TEX_FORMAT_R11G11B10_FLOAT:
    case Diligent::TEX_FORMAT_RGBA8_TYPELESS:
    case Diligent::TEX_FORMAT_RGBA8_UINT:
    case Diligent::TEX_FORMAT_RGBA8_SINT:
    case Diligent::TEX_FORMAT_RG16_TYPELESS:
    case Diligent::TEX_FORMAT_RG16_FLOAT:
    case Diligent::TEX_FORMAT_RG16_UINT:
    case Diligent::TEX_FORMAT_RG16_SINT:
    case Diligent::TEX_FORMAT_R32_TYPELESS:
    case Diligent::TEX_FORMAT_D32_FLOAT:
    case Diligent::TEX_FORMAT_R32_FLOAT:
    case Diligent::TEX_FORMAT_R32_UINT:
    case Diligent::TEX_FORMAT_R32_SINT:
    case Diligent::TEX_FORMAT_R24G8_TYPELESS:
    case Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT:
    case Diligent::TEX_FORMAT_RG8_TYPELESS:
    case Diligent::TEX_FORMAT_RG8_UINT:
    case Diligent::TEX_FORMAT_RG8_SINT:
    case Diligent::TEX_FORMAT_R16_TYPELESS:
    case Diligent::TEX_FORMAT_R16_FLOAT:
    case Diligent::TEX_FORMAT_R16_UINT:
    case Diligent::TEX_FORMAT_R16_SINT:
    case Diligent::TEX_FORMAT_R8_TYPELESS:
    case Diligent::TEX_FORMAT_R8_UINT:
    case Diligent::TEX_FORMAT_R8_SINT:
    case Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP:
    case Diligent::TEX_FORMAT_BC1_TYPELESS:
    case Diligent::TEX_FORMAT_BC2_TYPELESS:
    case Diligent::TEX_FORMAT_BC3_TYPELESS:
    case Diligent::TEX_FORMAT_BC4_TYPELESS:
    case Diligent::TEX_FORMAT_BC5_TYPELESS:
    case Diligent::TEX_FORMAT_BGRA8_TYPELESS:
    case Diligent::TEX_FORMAT_BGRX8_TYPELESS:
    case Diligent::TEX_FORMAT_BC6H_TYPELESS:
    case Diligent::TEX_FORMAT_BC6H_UF16:
    case Diligent::TEX_FORMAT_BC6H_SF16:
    case Diligent::TEX_FORMAT_BC7_TYPELESS:
    case Diligent::TEX_FORMAT_NUM_FORMATS:
      return false;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return false;
}

XII_ALWAYS_INLINE xiiUInt8 xiiDiligentUtils::GALToDiligentNumComponent(Diligent::TEXTURE_FORMAT format)
{
  switch (format)
  {
    case Diligent::TEX_FORMAT_D32_FLOAT:
    case Diligent::TEX_FORMAT_R32_FLOAT:
    case Diligent::TEX_FORMAT_R16_FLOAT:
    case Diligent::TEX_FORMAT_R32_UINT:
    case Diligent::TEX_FORMAT_D16_UNORM:
    case Diligent::TEX_FORMAT_R16_UNORM:
    case Diligent::TEX_FORMAT_R16_UINT:
    case Diligent::TEX_FORMAT_BC6H_SF16:
    case Diligent::TEX_FORMAT_R8_UNORM:
    case Diligent::TEX_FORMAT_R8_UINT:
    case Diligent::TEX_FORMAT_A8_UNORM:
    case Diligent::TEX_FORMAT_R1_UNORM:
    case Diligent::TEX_FORMAT_BC4_UNORM:
    case Diligent::TEX_FORMAT_BC4_SNORM:
    case Diligent::TEX_FORMAT_R32_SINT:
    case Diligent::TEX_FORMAT_R16_SNORM:
    case Diligent::TEX_FORMAT_R16_SINT:
    case Diligent::TEX_FORMAT_R8_SNORM:
    case Diligent::TEX_FORMAT_R8_SINT:
      return 1;

    case Diligent::TEX_FORMAT_RG32_FLOAT:
    case Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT:
    case Diligent::TEX_FORMAT_RG16_FLOAT:
    case Diligent::TEX_FORMAT_RG32_UINT:
    case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
    case Diligent::TEX_FORMAT_RG16_UNORM:
    case Diligent::TEX_FORMAT_RG16_UINT:
    case Diligent::TEX_FORMAT_BC5_UNORM:
    case Diligent::TEX_FORMAT_BC5_SNORM:
    case Diligent::TEX_FORMAT_BC6H_UF16:
    case Diligent::TEX_FORMAT_RG8_UNORM:
    case Diligent::TEX_FORMAT_RG8_UINT:
    case Diligent::TEX_FORMAT_RG32_SINT:
    case Diligent::TEX_FORMAT_RG16_SNORM:
    case Diligent::TEX_FORMAT_RG16_SINT:
    case Diligent::TEX_FORMAT_RG8_SNORM:
    case Diligent::TEX_FORMAT_RG8_SINT:
      return 2;

    case Diligent::TEX_FORMAT_RGB32_FLOAT:
    case Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP:
    case Diligent::TEX_FORMAT_R11G11B10_FLOAT:
    case Diligent::TEX_FORMAT_RGB32_UINT:
    case Diligent::TEX_FORMAT_BC7_UNORM:
    case Diligent::TEX_FORMAT_BC7_UNORM_SRGB:
    case Diligent::TEX_FORMAT_B5G6R5_UNORM:
    case Diligent::TEX_FORMAT_RGB32_SINT:
      return 3;

    case Diligent::TEX_FORMAT_RGBA32_FLOAT:
    case Diligent::TEX_FORMAT_RGBA16_FLOAT:
    case Diligent::TEX_FORMAT_RGBA32_UINT:
    case Diligent::TEX_FORMAT_RGB10A2_UNORM:
    case Diligent::TEX_FORMAT_RGB10A2_UINT:
    case Diligent::TEX_FORMAT_RGBA8_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_RGBA8_UINT:
    case Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case Diligent::TEX_FORMAT_RGBA16_UNORM:
    case Diligent::TEX_FORMAT_RGBA16_UINT:
    case Diligent::TEX_FORMAT_BC2_UNORM:
    case Diligent::TEX_FORMAT_BC2_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BC3_UNORM:
    case Diligent::TEX_FORMAT_BC3_UNORM_SRGB:
    case Diligent::TEX_FORMAT_B5G5R5A1_UNORM:
    case Diligent::TEX_FORMAT_BC1_UNORM:
    case Diligent::TEX_FORMAT_BC1_UNORM_SRGB:
    case Diligent::TEX_FORMAT_RG8_B8G8_UNORM:
    case Diligent::TEX_FORMAT_G8R8_G8B8_UNORM:
    case Diligent::TEX_FORMAT_RGBA32_SINT:
    case Diligent::TEX_FORMAT_RGBA8_SNORM:
    case Diligent::TEX_FORMAT_RGBA8_SINT:
    case Diligent::TEX_FORMAT_RGBA16_SNORM:
    case Diligent::TEX_FORMAT_RGBA16_SINT:
    case Diligent::TEX_FORMAT_BGRA8_UNORM:
    case Diligent::TEX_FORMAT_BGRX8_UNORM:
    case Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB:
      return 4;

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

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }
  return 0;
}

XII_ALWAYS_INLINE Diligent::SHADER_TYPE xiiDiligentUtils::GALToDiligentShaderStage(xiiGALShaderStage::Enum e)
{
  switch (e)
  {
    case xiiGALShaderStage::VertexShader:
      return Diligent::SHADER_TYPE_VERTEX;
    case xiiGALShaderStage::PixelShader:
      return Diligent::SHADER_TYPE_PIXEL;
    case xiiGALShaderStage::GeometryShader:
      return Diligent::SHADER_TYPE_GEOMETRY;
    case xiiGALShaderStage::HullShader:
      return Diligent::SHADER_TYPE_HULL;
    case xiiGALShaderStage::DomainShader:
      return Diligent::SHADER_TYPE_DOMAIN;
    case xiiGALShaderStage::ComputeShader:
      return Diligent::SHADER_TYPE_COMPUTE;
    case xiiGALShaderStage::AmplificationShader:
      return Diligent::SHADER_TYPE_AMPLIFICATION;
    case xiiGALShaderStage::MeshShader:
      return Diligent::SHADER_TYPE_MESH;
    case xiiGALShaderStage::RayGenShader:
      return Diligent::SHADER_TYPE_RAY_GEN;
    case xiiGALShaderStage::RayMissShader:
      return Diligent::SHADER_TYPE_RAY_MISS;
    case xiiGALShaderStage::RayAnyHitShader:
      return Diligent::SHADER_TYPE_RAY_ANY_HIT;
    case xiiGALShaderStage::RayClosestHitShader:
      return Diligent::SHADER_TYPE_RAY_CLOSEST_HIT;
    case xiiGALShaderStage::RayIntersectionShader:
      return Diligent::SHADER_TYPE_RAY_INTERSECTION;
    case xiiGALShaderStage::CallableShader:
      return Diligent::SHADER_TYPE_CALLABLE;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return Diligent::SHADER_TYPE::SHADER_TYPE_UNKNOWN;
}

XII_ALWAYS_INLINE Diligent::COLOR_MASK xiiDiligentUtils::ToDiligentColorWriteMask(xiiGALColorWriteMask::Enum mask)
{
  if (mask == xiiGALColorWriteMask::None)
    return Diligent::COLOR_MASK_NONE;

  return Diligent::COLOR_MASK(((mask & xiiGALColorWriteMask::Red) ? Diligent::COLOR_MASK_RED : 0) |
                              ((mask & xiiGALColorWriteMask::Green) ? Diligent::COLOR_MASK_GREEN : 0) |
                              ((mask & xiiGALColorWriteMask::Blue) ? Diligent::COLOR_MASK_BLUE : 0) |
                              ((mask & xiiGALColorWriteMask::Alpha) ? Diligent::COLOR_MASK_ALPHA : 0));
}

XII_ALWAYS_INLINE bool xiiDiligentUtils::IsDepthFormat(Diligent::TEXTURE_FORMAT format)
{
  switch (format)
  {
    // Case D16 Unorm S8 Uint not supported
    case Diligent::TEX_FORMAT_D16_UNORM:
    case Diligent::TEX_FORMAT_D32_FLOAT:
    case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
    case Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT:
      return true;
  }
  return false;
}

XII_ALWAYS_INLINE Diligent::RESOURCE_DIMENSION xiiDiligentUtils::GetResourceDimension(xiiGALTextureType::Enum type)
{
  switch (type)
  {
    case xiiGALTextureType::Texture1D:
      return Diligent::RESOURCE_DIM_TEX_1D;
    case xiiGALTextureType::Texture1DArray:
      return Diligent::RESOURCE_DIM_TEX_1D_ARRAY;
    case xiiGALTextureType::Texture2D:
      return Diligent::RESOURCE_DIM_TEX_2D;
    case xiiGALTextureType::Texture2DArray:
      return Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
    case xiiGALTextureType::TextureCube:
      return Diligent::RESOURCE_DIM_TEX_CUBE;
    case xiiGALTextureType::TextureCubeArray:
      return Diligent::RESOURCE_DIM_TEX_CUBE_ARRAY;
    case xiiGALTextureType::Texture3D:
      return Diligent::RESOURCE_DIM_TEX_3D;
    case xiiGALTextureType::Texture2DProxy:
      return Diligent::RESOURCE_DIM_TEX_2D;
    case xiiGALTextureType::Texture2DProxyArray:
      return Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
  }
  return Diligent::RESOURCE_DIM_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::RESOURCE_STATE xiiDiligentUtils::GetDefaultResourceState(Diligent::IBuffer* pBuffer)
{
  Diligent::RESOURCE_STATE defaultSourceResourceState = {};

  const Diligent::BIND_FLAGS& bindFlags = pBuffer->GetDesc().BindFlags;

  if (bindFlags & Diligent::BIND_UNIFORM_BUFFER)
  {
    defaultSourceResourceState |= Diligent::RESOURCE_STATE_CONSTANT_BUFFER;
    return defaultSourceResourceState;
  }

  if (bindFlags & Diligent::BIND_VERTEX_BUFFER)
    defaultSourceResourceState |= Diligent::RESOURCE_STATE_VERTEX_BUFFER;
  if (bindFlags & Diligent::BIND_INDEX_BUFFER)
    defaultSourceResourceState |= Diligent::RESOURCE_STATE_INDEX_BUFFER;
  if (bindFlags & Diligent::BIND_SHADER_RESOURCE)
    defaultSourceResourceState |= Diligent::RESOURCE_STATE_SHADER_RESOURCE;
  if (bindFlags & Diligent::BIND_STREAM_OUTPUT)
    defaultSourceResourceState |= Diligent::RESOURCE_STATE_STREAM_OUT;
  if (bindFlags & Diligent::BIND_UNORDERED_ACCESS)
    defaultSourceResourceState |= Diligent::RESOURCE_STATE_UNORDERED_ACCESS;
  if (bindFlags & Diligent::BIND_INDIRECT_DRAW_ARGS)
    defaultSourceResourceState |= Diligent::RESOURCE_STATE_INDIRECT_ARGUMENT;
  if (bindFlags & Diligent::BIND_RAY_TRACING)
    defaultSourceResourceState |= Diligent::RESOURCE_STATE_RAY_TRACING;

  return defaultSourceResourceState;
}

XII_ALWAYS_INLINE Diligent::RESOURCE_STATE xiiDiligentUtils::GetDefaultResourceState(Diligent::ITexture* pTexture)
{
  Diligent::RESOURCE_STATE defaultResourceState = {};

  const Diligent::BIND_FLAGS& bindFlags = pTexture->GetDesc().BindFlags;

// Some of these are exclusive, use with caution.
  if (bindFlags & Diligent::BIND_RENDER_TARGET)
    defaultResourceState |= Diligent::RESOURCE_STATE_RENDER_TARGET;
  if (bindFlags & Diligent::BIND_DEPTH_STENCIL)
  {
    if (pTexture->GetDesc().Usage == Diligent::USAGE_IMMUTABLE)
      defaultResourceState |= Diligent::RESOURCE_STATE_DEPTH_READ;
    else
      defaultResourceState |= Diligent::RESOURCE_STATE_DEPTH_WRITE;
  }
  if (bindFlags & Diligent::BIND_SHADER_RESOURCE)
    defaultResourceState |= Diligent::RESOURCE_STATE_SHADER_RESOURCE;
  if (bindFlags & Diligent::BIND_UNORDERED_ACCESS)
    defaultResourceState |= Diligent::RESOURCE_STATE_UNORDERED_ACCESS;
  if (bindFlags & Diligent::BIND_INPUT_ATTACHMENT)
    defaultResourceState |= Diligent::RESOURCE_STATE_INPUT_ATTACHMENT;
  if (bindFlags & Diligent::BIND_SHADING_RATE)
    defaultResourceState |= Diligent::RESOURCE_STATE_SHADING_RATE;

  return defaultResourceState;
}
