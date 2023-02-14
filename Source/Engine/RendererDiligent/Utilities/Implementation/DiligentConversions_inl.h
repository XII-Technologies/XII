
XII_ALWAYS_INLINE Diligent::RENDER_DEVICE_TYPE xiiDiligentUtils::GetDiligentRenderDeviceType()
{
  switch (xiiGraphicsDevice::Default)
  {
    case xiiGraphicsDevice::OpenGL:
      return Diligent::RENDER_DEVICE_TYPE::RENDER_DEVICE_TYPE_GL;

    case xiiGraphicsDevice::D3D11:
      return Diligent::RENDER_DEVICE_TYPE::RENDER_DEVICE_TYPE_D3D11;

    case xiiGraphicsDevice::D3D12:
      return Diligent::RENDER_DEVICE_TYPE::RENDER_DEVICE_TYPE_D3D12;

    case xiiGraphicsDevice::Vulkan:
      return Diligent::RENDER_DEVICE_TYPE::RENDER_DEVICE_TYPE_VULKAN;

    case xiiGraphicsDevice::Metal:
      return Diligent::RENDER_DEVICE_TYPE::RENDER_DEVICE_TYPE_METAL;
  }
  return Diligent::RENDER_DEVICE_TYPE::RENDER_DEVICE_TYPE_UNDEFINED;
}

XII_ALWAYS_INLINE xiiUInt32 xiiDiligentUtils::ToDiligentMSAACount(xiiEnum<xiiGALMSAASampleCount> sampleCount)
{
  return static_cast<xiiUInt32>(sampleCount.GetValue());
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
    case xiiGALTextureFilterMode::Point:
      return Diligent::FILTER_TYPE_POINT;
    case xiiGALTextureFilterMode::Linear:
      return Diligent::FILTER_TYPE_LINEAR;
    case xiiGALTextureFilterMode::Anisotropic:
      return Diligent::FILTER_TYPE_ANISOTROPIC;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }
  return Diligent::FILTER_TYPE_UNKNOWN;
}

XII_ALWAYS_INLINE Diligent::BLEND_OPERATION xiiDiligentUtils::ToDiligentBlendOperation(xiiGALBlendOp::Enum e)
{
  switch (e)
  {
    case xiiGALBlendOp::Add:
      return Diligent::BLEND_OPERATION::BLEND_OPERATION_ADD;
    case xiiGALBlendOp::Subtract:
      return Diligent::BLEND_OPERATION::BLEND_OPERATION_SUBTRACT;
    case xiiGALBlendOp::RevSubtract:
      return Diligent::BLEND_OPERATION::BLEND_OPERATION_REV_SUBTRACT;
    case xiiGALBlendOp::Min:
      return Diligent::BLEND_OPERATION::BLEND_OPERATION_MIN;
    case xiiGALBlendOp::Max:
      return Diligent::BLEND_OPERATION::BLEND_OPERATION_MAX;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::BLEND_OPERATION_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::BLEND_FACTOR xiiDiligentUtils::ToDiligentBlendFactor(xiiGALBlend::Enum e)
{
  switch (e)
  {
    case xiiGALBlend::Zero:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_ZERO;
    case xiiGALBlend::One:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_ONE;
    case xiiGALBlend::SrcColor:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_SRC_COLOR;
    case xiiGALBlend::InvSrcColor:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_INV_SRC_COLOR;
    case xiiGALBlend::SrcAlpha:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_SRC_ALPHA;
    case xiiGALBlend::InvSrcAlpha:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_INV_SRC_ALPHA;
    case xiiGALBlend::DestAlpha:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_DEST_ALPHA;
    case xiiGALBlend::InvDestAlpha:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_INV_DEST_ALPHA;
    case xiiGALBlend::DestColor:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_DEST_COLOR;
    case xiiGALBlend::InvDestColor:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_INV_DEST_COLOR;
    case xiiGALBlend::SrcAlphaSaturated:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_SRC_ALPHA_SAT;
    case xiiGALBlend::BlendFactor:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_BLEND_FACTOR;
    case xiiGALBlend::InvBlendFactor:
      return Diligent::BLEND_FACTOR::BLEND_FACTOR_INV_BLEND_FACTOR;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return Diligent::BLEND_FACTOR_UNDEFINED;
}

XII_ALWAYS_INLINE Diligent::VALUE_TYPE xiiDiligentUtils::GALToDiligentFormat(Diligent::TEXTURE_FORMAT format)
{
  switch (format)
  {
    case Diligent::TEX_FORMAT_RGBA32_FLOAT:
    case Diligent::TEX_FORMAT_RGB32_FLOAT:
    case Diligent::TEX_FORMAT_RG32_FLOAT:
    case Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT:
    case Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP:
      return Diligent::VALUE_TYPE::VT_FLOAT32;

    case Diligent::TEX_FORMAT_RGBA16_FLOAT:
    case Diligent::TEX_FORMAT_R11G11B10_FLOAT:
    case Diligent::TEX_FORMAT_RG16_FLOAT:
    case Diligent::TEX_FORMAT_D32_FLOAT:
    case Diligent::TEX_FORMAT_R32_FLOAT:
    case Diligent::TEX_FORMAT_R16_FLOAT:
      return Diligent::VALUE_TYPE::VT_FLOAT16;

    case Diligent::TEX_FORMAT_RGBA32_UINT:
    case Diligent::TEX_FORMAT_RGB32_UINT:
    case Diligent::TEX_FORMAT_RG32_UINT:
    case Diligent::TEX_FORMAT_RGB10A2_UNORM:
    case Diligent::TEX_FORMAT_RGB10A2_UINT:
    case Diligent::TEX_FORMAT_RGBA8_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_RGBA8_UINT:
    case Diligent::TEX_FORMAT_R32_UINT:
    case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
    case Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
      return Diligent::VALUE_TYPE::VT_UINT32;

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
      return Diligent::VALUE_TYPE::VT_UINT16;

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
      return Diligent::VALUE_TYPE::VT_UINT8;

    case Diligent::TEX_FORMAT_RGBA32_SINT:
    case Diligent::TEX_FORMAT_RGB32_SINT:
    case Diligent::TEX_FORMAT_RG32_SINT:
    case Diligent::TEX_FORMAT_RGBA8_SNORM:
    case Diligent::TEX_FORMAT_RGBA8_SINT:
    case Diligent::TEX_FORMAT_R32_SINT:
      return Diligent::VALUE_TYPE::VT_INT32;

    case Diligent::TEX_FORMAT_RGBA16_SNORM:
    case Diligent::TEX_FORMAT_RGBA16_SINT:
    case Diligent::TEX_FORMAT_RG16_SNORM:
    case Diligent::TEX_FORMAT_RG16_SINT:
    case Diligent::TEX_FORMAT_R16_SNORM:
    case Diligent::TEX_FORMAT_R16_SINT:
      return Diligent::VALUE_TYPE::VT_INT16;

    case Diligent::TEX_FORMAT_RG8_SNORM:
    case Diligent::TEX_FORMAT_RG8_SINT:
    case Diligent::TEX_FORMAT_R8_SNORM:
    case Diligent::TEX_FORMAT_R8_SINT:
    case Diligent::TEX_FORMAT_BGRA8_UNORM:
    case Diligent::TEX_FORMAT_BGRX8_UNORM:
    case Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB:
      return Diligent::VALUE_TYPE::VT_INT8;

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

XII_ALWAYS_INLINE xiiInt32 xiiDiligentUtils::GALToDiligentNumComponent(Diligent::TEXTURE_FORMAT format)
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
      return Diligent::SHADER_TYPE::SHADER_TYPE_VERTEX;
    case xiiGALShaderStage::HullShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_HULL;
    case xiiGALShaderStage::DomainShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_DOMAIN;
    case xiiGALShaderStage::GeometryShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_GEOMETRY;
    case xiiGALShaderStage::PixelShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_PIXEL;
    case xiiGALShaderStage::ComputeShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_COMPUTE;

      XII_ASSERT_NOT_IMPLEMENTED;
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
