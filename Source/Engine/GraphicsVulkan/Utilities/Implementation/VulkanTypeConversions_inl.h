/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_ALWAYS_INLINE vk::BlendOp xiiVulkanTypeConversions::GetBlendOp(xiiGALBlendOperation::Enum e)
{
  switch (e)
  {
    case xiiGALBlendOperation::Add:
      return vk::BlendOp::eAdd;
    case xiiGALBlendOperation::Subtract:
      return vk::BlendOp::eSubtract;
    case xiiGALBlendOperation::ReverseSubtract:
      return vk::BlendOp::eReverseSubtract;
    case xiiGALBlendOperation::Min:
      return vk::BlendOp::eMin;
    case xiiGALBlendOperation::Max:
      return vk::BlendOp::eMax;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::BlendOp::eAdd;
}

XII_ALWAYS_INLINE vk::BlendFactor xiiVulkanTypeConversions::GetBlendFactor(xiiGALBlendFactor::Enum e)
{
  switch (e)
  {
    case xiiGALBlendFactor::Zero:
      return vk::BlendFactor::eZero;
    case xiiGALBlendFactor::One:
      return vk::BlendFactor::eOne;
    case xiiGALBlendFactor::SourceColor:
      return vk::BlendFactor::eSrcColor;
    case xiiGALBlendFactor::InverseSourceColor:
      return vk::BlendFactor::eOneMinusSrcColor;
    case xiiGALBlendFactor::SourceAlpha:
      return vk::BlendFactor::eSrcAlpha;
    case xiiGALBlendFactor::InverseSourceAlpha:
      return vk::BlendFactor::eOneMinusSrcAlpha;
    case xiiGALBlendFactor::DestinationAlpha:
      return vk::BlendFactor::eDstAlpha;
    case xiiGALBlendFactor::InverseDestinationAlpha:
      return vk::BlendFactor::eOneMinusDstAlpha;
    case xiiGALBlendFactor::DestinationColor:
      return vk::BlendFactor::eDstColor;
    case xiiGALBlendFactor::InverseDestinationColor:
      return vk::BlendFactor::eOneMinusDstColor;
    case xiiGALBlendFactor::SourceAlphaSaturate:
      return vk::BlendFactor::eSrcAlphaSaturate;
    case xiiGALBlendFactor::BlendFactor:
      return vk::BlendFactor::eConstantColor;
    case xiiGALBlendFactor::InverseBlendFactor:
      return vk::BlendFactor::eOneMinusConstantColor;
    case xiiGALBlendFactor::SourceOneColor:
      return vk::BlendFactor::eSrc1Color;
    case xiiGALBlendFactor::InverseSourceOneColor:
      return vk::BlendFactor::eOneMinusSrc1Color;
    case xiiGALBlendFactor::SourceOneAlpha:
      return vk::BlendFactor::eSrc1Alpha;
    case xiiGALBlendFactor::InverseSourceOneAlpha:
      return vk::BlendFactor::eOneMinusSrc1Alpha;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::BlendFactor::eZero;
}

XII_ALWAYS_INLINE vk::LogicOp xiiVulkanTypeConversions::GetLogicOp(xiiGALLogicOperation::Enum e)
{
  switch (e)
  {
    case xiiGALLogicOperation::Clear:
      return vk::LogicOp::eClear;
    case xiiGALLogicOperation::Set:
      return vk::LogicOp::eSet;
    case xiiGALLogicOperation::Copy:
      return vk::LogicOp::eCopy;
    case xiiGALLogicOperation::CopyInverted:
      return vk::LogicOp::eCopyInverted;
    case xiiGALLogicOperation::NoOperation:
      return vk::LogicOp::eNoOp;
    case xiiGALLogicOperation::Invert:
      return vk::LogicOp::eInvert;
    case xiiGALLogicOperation::AND:
      return vk::LogicOp::eAnd;
    case xiiGALLogicOperation::NAND:
      return vk::LogicOp::eNand;
    case xiiGALLogicOperation::OR:
      return vk::LogicOp::eOr;
    case xiiGALLogicOperation::NOR:
      return vk::LogicOp::eNor;
    case xiiGALLogicOperation::XOR:
      return vk::LogicOp::eXor;
    case xiiGALLogicOperation::Equivalent:
      return vk::LogicOp::eEquivalent;
    case xiiGALLogicOperation::AndReversed:
      return vk::LogicOp::eAndReverse;
    case xiiGALLogicOperation::AndInverted:
      return vk::LogicOp::eAndInverted;
    case xiiGALLogicOperation::OrReversed:
      return vk::LogicOp::eOrReverse;
    case xiiGALLogicOperation::OrInverted:
      return vk::LogicOp::eOrInverted;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::LogicOp::eClear;
}

XII_ALWAYS_INLINE vk::CompareOp xiiVulkanTypeConversions::GetCompareOp(xiiGALComparisonFunction::Enum e)
{
  switch (e)
  {
    case xiiGALComparisonFunction::Never:
      return vk::CompareOp::eNever;
    case xiiGALComparisonFunction::Less:
      return vk::CompareOp::eLess;
    case xiiGALComparisonFunction::Equal:
      return vk::CompareOp::eEqual;
    case xiiGALComparisonFunction::LessEqual:
      return vk::CompareOp::eLessOrEqual;
    case xiiGALComparisonFunction::Greater:
      return vk::CompareOp::eGreater;
    case xiiGALComparisonFunction::NotEqual:
      return vk::CompareOp::eNotEqual;
    case xiiGALComparisonFunction::GreaterEqual:
      return vk::CompareOp::eGreaterOrEqual;
    case xiiGALComparisonFunction::Always:
      return vk::CompareOp::eAlways;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::CompareOp::eNever;
}

XII_ALWAYS_INLINE vk::StencilOp xiiVulkanTypeConversions::GetStencilOp(xiiGALStencilOperation::Enum e)
{
  switch (e)
  {
    case xiiGALStencilOperation::Keep:
      return vk::StencilOp::eKeep;
    case xiiGALStencilOperation::Zero:
      return vk::StencilOp::eZero;
    case xiiGALStencilOperation::Replace:
      return vk::StencilOp::eReplace;
    case xiiGALStencilOperation::IncrementSaturate:
      return vk::StencilOp::eIncrementAndClamp;
    case xiiGALStencilOperation::DecrementSaturate:
      return vk::StencilOp::eDecrementAndClamp;
    case xiiGALStencilOperation::Invert:
      return vk::StencilOp::eInvert;
    case xiiGALStencilOperation::IncrementWrap:
      return vk::StencilOp::eIncrementAndWrap;
    case xiiGALStencilOperation::DecrementWrap:
      return vk::StencilOp::eDecrementAndWrap;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::StencilOp::eKeep;
}

XII_ALWAYS_INLINE vk::PolygonMode xiiVulkanTypeConversions::GetPolygonMode(xiiGALFillMode::Enum e)
{
  switch (e)
  {
    case xiiGALFillMode::Wireframe:
      return vk::PolygonMode::eLine;
    case xiiGALFillMode::Solid:
      return vk::PolygonMode::eFill;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::PolygonMode::eFill;
}

XII_ALWAYS_INLINE vk::CullModeFlagBits xiiVulkanTypeConversions::GetCullMode(xiiGALCullMode::Enum e)
{
  switch (e)
  {
    case xiiGALCullMode::None:
      return vk::CullModeFlagBits::eNone;
    case xiiGALCullMode::Front:
      return vk::CullModeFlagBits::eFront;
    case xiiGALCullMode::Back:
      return vk::CullModeFlagBits::eBack;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::CullModeFlagBits::eNone;
}

XII_ALWAYS_INLINE vk::ColorComponentFlags xiiVulkanTypeConversions::GetColorWriteMask(xiiBitflags<xiiGALColorMask> e)
{
  if (e.IsNoFlagSet())
    return vk::ColorComponentFlagBits{};

  vk::ColorComponentFlags colorMask = {};

  if (e.IsSet(xiiGALColorMask::Red))
    colorMask |= vk::ColorComponentFlagBits::eR;
  if (e.IsSet(xiiGALColorMask::Green))
    colorMask |= vk::ColorComponentFlagBits::eG;
  if (e.IsSet(xiiGALColorMask::Blue))
    colorMask |= vk::ColorComponentFlagBits::eB;
  if (e.IsSet(xiiGALColorMask::Alpha))
    colorMask |= vk::ColorComponentFlagBits::eA;

  return colorMask;
}

XII_ALWAYS_INLINE vk::Format xiiVulkanTypeConversions::GetFormat(xiiGALResourceFormat::Enum e)
{
  switch (e)
  {
    case xiiGALResourceFormat::Unknown:
      return vk::Format::eUndefined;

    case xiiGALResourceFormat::RGBA32Typeless:
    case xiiGALResourceFormat::RGBA32Float:
      return vk::Format::eR32G32B32A32Sfloat;
    case xiiGALResourceFormat::RGBA32UInt:
      return vk::Format::eR32G32B32A32Uint;
    case xiiGALResourceFormat::RGBA32SInt:
      return vk::Format::eR32G32B32A32Sint;

    case xiiGALResourceFormat::RGB32Typeless:
    case xiiGALResourceFormat::RGB32Float:
      return vk::Format::eR32G32B32Sfloat;
    case xiiGALResourceFormat::RGB32UInt:
      return vk::Format::eR32G32B32Uint;
    case xiiGALResourceFormat::RGB32SInt:
      return vk::Format::eR32G32B32Sint;

    case xiiGALResourceFormat::RGBA16Typeless:
    case xiiGALResourceFormat::RGBA16Float:
      return vk::Format::eR16G16B16A16Sfloat;
    case xiiGALResourceFormat::RGBA16UNormalized:
      return vk::Format::eR16G16B16A16Unorm;
    case xiiGALResourceFormat::RGBA16UInt:
      return vk::Format::eR16G16B16A16Uint;
    case xiiGALResourceFormat::RGBA16SNormalized:
      return vk::Format::eR16G16B16A16Snorm;
    case xiiGALResourceFormat::RGBA16SInt:
      return vk::Format::eR16G16B16A16Sint;

    case xiiGALResourceFormat::RG32Typeless:
    case xiiGALResourceFormat::RG32Float:
      return vk::Format::eR32G32Sfloat;
    case xiiGALResourceFormat::RG32UInt:
      return vk::Format::eR32G32Uint;
    case xiiGALResourceFormat::RG32SInt:
      return vk::Format::eR32G32Sint;

    case xiiGALResourceFormat::R32G8X24Typeless:
    case xiiGALResourceFormat::D32FloatS8X24UInt:
    case xiiGALResourceFormat::R32FloatX8X24Typeless:
      return vk::Format::eD32SfloatS8Uint;

    case xiiGALResourceFormat::X32TypelessG8X24UInt:
      return vk::Format::eUndefined;

    case xiiGALResourceFormat::RGB10A2Typeless:
    case xiiGALResourceFormat::RGB10A2UNormalized:
      return vk::Format::eA2R10G10B10UnormPack32;
    case xiiGALResourceFormat::RGB10A2UInt:
      return vk::Format::eA2R10G10B10UintPack32;
    case xiiGALResourceFormat::RG11B10Float:
      return vk::Format::eB10G11R11UfloatPack32;

    case xiiGALResourceFormat::RGBA8Typeless:
    case xiiGALResourceFormat::RGBA8UNormalized:
      return vk::Format::eR8G8B8A8Unorm;
    case xiiGALResourceFormat::RGBA8UNormalizedSRGB:
      return vk::Format::eR8G8B8A8Srgb;
    case xiiGALResourceFormat::RGBA8UInt:
      return vk::Format::eR8G8B8A8Uint;
    case xiiGALResourceFormat::RGBA8SNormalized:
      return vk::Format::eR8G8B8A8Snorm;
    case xiiGALResourceFormat::RGBA8SInt:
      return vk::Format::eR8G8B8A8Sint;

    case xiiGALResourceFormat::RG16Typeless:
    case xiiGALResourceFormat::RG16Float:
      return vk::Format::eR16G16Sfloat;
    case xiiGALResourceFormat::RG16UNormalized:
      return vk::Format::eR16G16Unorm;
    case xiiGALResourceFormat::RG16UInt:
      return vk::Format::eR16G16Uint;
    case xiiGALResourceFormat::RG16SNormalized:
      return vk::Format::eR16G16Snorm;
    case xiiGALResourceFormat::RG16SInt:
      return vk::Format::eR16G16Sint;

    case xiiGALResourceFormat::R32Typeless:
      return vk::Format::eR32Sfloat;
    case xiiGALResourceFormat::D32Float:
      return vk::Format::eD32Sfloat;
    case xiiGALResourceFormat::R32Float:
      return vk::Format::eR32Sfloat;
    case xiiGALResourceFormat::R32UInt:
      return vk::Format::eR32Uint;
    case xiiGALResourceFormat::R32SInt:
      return vk::Format::eR32Sint;

    case xiiGALResourceFormat::R24G8Typeless:
    case xiiGALResourceFormat::D24UNormalizedS8UInt:
    case xiiGALResourceFormat::R24UNormalizedX8Typeless:
      return vk::Format::eD24UnormS8Uint;

    case xiiGALResourceFormat::X24TypelessG8UInt:
      return vk::Format::eUndefined;

    case xiiGALResourceFormat::RG8Typeless:
    case xiiGALResourceFormat::RG8UNormalized:
      return vk::Format::eR8G8Unorm;
    case xiiGALResourceFormat::RG8UInt:
      return vk::Format::eR8G8Uint;
    case xiiGALResourceFormat::RG8SNormalized:
      return vk::Format::eR8G8Snorm;
    case xiiGALResourceFormat::RG8SInt:
      return vk::Format::eR8G8Sint;

    case xiiGALResourceFormat::R16Typeless:
    case xiiGALResourceFormat::R16Float:
      return vk::Format::eR16Sfloat;
    case xiiGALResourceFormat::D16UNormalized:
      return vk::Format::eD16Unorm;
    case xiiGALResourceFormat::R16UNormalized:
      return vk::Format::eR16Unorm;
    case xiiGALResourceFormat::R16UInt:
      return vk::Format::eR16Uint;
    case xiiGALResourceFormat::R16SNormalized:
      return vk::Format::eR16Snorm;
    case xiiGALResourceFormat::R16SInt:
      return vk::Format::eR16Sint;

    case xiiGALResourceFormat::R8Typeless:
    case xiiGALResourceFormat::R8UNormalized:
      return vk::Format::eR8Unorm;
    case xiiGALResourceFormat::R8UInt:
      return vk::Format::eR8Uint;
    case xiiGALResourceFormat::R8SNormalized:
      return vk::Format::eR8Snorm;
    case xiiGALResourceFormat::R8SInt:
      return vk::Format::eR8Sint;
    case xiiGALResourceFormat::A8UNormalized:
      // If we use vk::Format::eR8Unorm, to get the same behaviour as xiiGALResourceFormat::A8UNormalized, we have to
      // Swizzle the components appropriately using the image view create info structure.
      return vk::Format::eA8UnormKHR;

    case xiiGALResourceFormat::R1UNormalized:
      return vk::Format::eUndefined;

    case xiiGALResourceFormat::RGB9E5SharedExponent:
      return vk::Format::eE5B9G9R9UfloatPack32;

    case xiiGALResourceFormat::RG8BG8UNormalized:
    case xiiGALResourceFormat::GR8GB8UNormalized:
      return vk::Format::eUndefined;

    case xiiGALResourceFormat::BC1Typeless:
    case xiiGALResourceFormat::BC1UNormalized:
      return vk::Format::eBc1RgbUnormBlock;
    case xiiGALResourceFormat::BC1UNormalizedSRGB:
      return vk::Format::eBc1RgbSrgbBlock;

    case xiiGALResourceFormat::BC2Typeless:
    case xiiGALResourceFormat::BC2UNormalized:
      return vk::Format::eBc2UnormBlock;
    case xiiGALResourceFormat::BC2UNormalizedSRGB:
      return vk::Format::eBc2SrgbBlock;

    case xiiGALResourceFormat::BC3Typeless:
    case xiiGALResourceFormat::BC3UNormalized:
      return vk::Format::eBc3UnormBlock;
    case xiiGALResourceFormat::BC3UNormalizedSRGB:
      return vk::Format::eBc3SrgbBlock;

    case xiiGALResourceFormat::BC4Typeless:
    case xiiGALResourceFormat::BC4UNormalized:
      return vk::Format::eBc4UnormBlock;
    case xiiGALResourceFormat::BC4SNormalized:
      return vk::Format::eBc4SnormBlock;

    case xiiGALResourceFormat::BC5Typeless:
    case xiiGALResourceFormat::BC5UNormalized:
      return vk::Format::eBc5UnormBlock;
    case xiiGALResourceFormat::BC5SNormalized:
      return vk::Format::eBc5SnormBlock;

    case xiiGALResourceFormat::B5G6R5UNormalized:
      return vk::Format::eB5G6R5UnormPack16;
    case xiiGALResourceFormat::B5G5R5A1UNormalized:
      return vk::Format::eB5G5R5A1UnormPack16;
    case xiiGALResourceFormat::BGRA8UNormalized:
    case xiiGALResourceFormat::BGRX8UNormalized:
      return vk::Format::eB8G8R8A8Unorm;

    case xiiGALResourceFormat::R10G10B10XRBiasA2UNormalized:
      return vk::Format::eUndefined;

    case xiiGALResourceFormat::BGRA8Typeless:
      return vk::Format::eB8G8R8A8Unorm;
    case xiiGALResourceFormat::BGRA8UNormalizedSRGB:
      return vk::Format::eB8G8R8A8Srgb;
    case xiiGALResourceFormat::BGRX8Typeless:
      return vk::Format::eB8G8R8A8Unorm;
    case xiiGALResourceFormat::BGRX8UNormalizedSRGB:
      return vk::Format::eB8G8R8A8Srgb;

    case xiiGALResourceFormat::BC6HTypeless:
    case xiiGALResourceFormat::BC6HUF16:
      return vk::Format::eBc6HUfloatBlock;
    case xiiGALResourceFormat::BC6HSF16:
      return vk::Format::eBc6HSfloatBlock;

    case xiiGALResourceFormat::BC7Typeless:
    case xiiGALResourceFormat::BC7UNormalized:
      return vk::Format::eBc7UnormBlock;
    case xiiGALResourceFormat::BC7UNormalizedSRGB:
      return vk::Format::eBc7SrgbBlock;

    case xiiGALResourceFormat::NV12:
      return vk::Format::eG8B8R82Plane420Unorm;
    case xiiGALResourceFormat::P010:
      return vk::Format::eG10X6B10X6R10X62Plane420Unorm3Pack16;
    case xiiGALResourceFormat::P016:
      return vk::Format::eG16B16R162Plane420Unorm;
    case xiiGALResourceFormat::YUY2:
      return vk::Format::eG8B8G8R8422Unorm;
    case xiiGALResourceFormat::AYUV:
      return vk::Format::eG8B8R83Plane444Unorm;
    case xiiGALResourceFormat::P216:
      return vk::Format::eG16B16R163Plane422Unorm;
    case xiiGALResourceFormat::P416:
      return vk::Format::eG16B16R163Plane444Unorm;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::Format::eUndefined;
}

XII_ALWAYS_INLINE xiiGALResourceFormat::Enum xiiVulkanTypeConversions::GetGALResourceFormat(vk::Format e)
{
  switch (e)
  {
    case vk::Format::eUndefined:
      return xiiGALResourceFormat::Unknown;

    case vk::Format::eR32G32B32A32Sfloat:
      return xiiGALResourceFormat::RGBA32Typeless;
    case vk::Format::eR32G32B32A32Uint:
      return xiiGALResourceFormat::RGBA32UInt;
    case vk::Format::eR32G32B32A32Sint:
      return xiiGALResourceFormat::RGBA32SInt;

    case vk::Format::eR32G32B32Sfloat:
      return xiiGALResourceFormat::RGB32Typeless;
    case vk::Format::eR32G32B32Uint:
      return xiiGALResourceFormat::RGB32UInt;
    case vk::Format::eR32G32B32Sint:
      return xiiGALResourceFormat::RGB32SInt;

    case vk::Format::eR16G16B16A16Sfloat:
      return xiiGALResourceFormat::RGBA16Typeless;
    case vk::Format::eR16G16B16A16Unorm:
      return xiiGALResourceFormat::RGBA16UNormalized;
    case vk::Format::eR16G16B16A16Uint:
      return xiiGALResourceFormat::RGBA16UInt;
    case vk::Format::eR16G16B16A16Snorm:
      return xiiGALResourceFormat::RGBA16SNormalized;
    case vk::Format::eR16G16B16A16Sint:
      return xiiGALResourceFormat::RGBA16SInt;

    case vk::Format::eR32G32Sfloat:
      return xiiGALResourceFormat::RG32Typeless;
    case vk::Format::eR32G32Uint:
      return xiiGALResourceFormat::RG32UInt;
    case vk::Format::eR32G32Sint:
      return xiiGALResourceFormat::RG32SInt;

    case vk::Format::eD32SfloatS8Uint:
      return xiiGALResourceFormat::R32G8X24Typeless;

    case vk::Format::eA2R10G10B10UnormPack32:
      return xiiGALResourceFormat::RGB10A2UNormalized;
    case vk::Format::eA2R10G10B10UintPack32:
      return xiiGALResourceFormat::RGB10A2UInt;
    case vk::Format::eB10G11R11UfloatPack32:
      return xiiGALResourceFormat::RG11B10Float;

    case vk::Format::eR8G8B8A8Unorm:
      return xiiGALResourceFormat::RGBA8UNormalized;
    case vk::Format::eR8G8B8A8Srgb:
      return xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    case vk::Format::eR8G8B8A8Uint:
      return xiiGALResourceFormat::RGBA8UInt;
    case vk::Format::eR8G8B8A8Snorm:
      return xiiGALResourceFormat::RGBA8SNormalized;
    case vk::Format::eR8G8B8A8Sint:
      return xiiGALResourceFormat::RGBA8SInt;

    case vk::Format::eR16G16Sfloat:
      return xiiGALResourceFormat::RG16Typeless;
    case vk::Format::eR16G16Unorm:
      return xiiGALResourceFormat::RG16UNormalized;
    case vk::Format::eR16G16Uint:
      return xiiGALResourceFormat::RG16UInt;
    case vk::Format::eR16G16Snorm:
      return xiiGALResourceFormat::RG16SNormalized;
    case vk::Format::eR16G16Sint:
      return xiiGALResourceFormat::RG16SInt;

    case vk::Format::eR32Sfloat:
      return xiiGALResourceFormat::R32Typeless;
    case vk::Format::eD32Sfloat:
      return xiiGALResourceFormat::D32Float;
    case vk::Format::eR32Uint:
      return xiiGALResourceFormat::R32UInt;
    case vk::Format::eR32Sint:
      return xiiGALResourceFormat::R32SInt;

    case vk::Format::eD24UnormS8Uint:
      return xiiGALResourceFormat::R24G8Typeless;

    case vk::Format::eR8G8Unorm:
      return xiiGALResourceFormat::RG8UNormalized;
    case vk::Format::eR8G8Uint:
      return xiiGALResourceFormat::RG8UInt;
    case vk::Format::eR8G8Snorm:
      return xiiGALResourceFormat::RG8SNormalized;
    case vk::Format::eR8G8Sint:
      return xiiGALResourceFormat::RG8SInt;

    case vk::Format::eR16Sfloat:
      return xiiGALResourceFormat::R16Typeless;
    case vk::Format::eD16Unorm:
      return xiiGALResourceFormat::D16UNormalized;
    case vk::Format::eR16Unorm:
      return xiiGALResourceFormat::R16UNormalized;
    case vk::Format::eR16Uint:
      return xiiGALResourceFormat::R16UInt;
    case vk::Format::eR16Snorm:
      return xiiGALResourceFormat::R16SNormalized;
    case vk::Format::eR16Sint:
      return xiiGALResourceFormat::R16SInt;

    case vk::Format::eR8Unorm:
      return xiiGALResourceFormat::R8UNormalized;
    case vk::Format::eR8Uint:
      return xiiGALResourceFormat::R8UInt;
    case vk::Format::eR8Snorm:
      return xiiGALResourceFormat::R8SNormalized;
    case vk::Format::eR8Sint:
      return xiiGALResourceFormat::R8SInt;
    case vk::Format::eA8UnormKHR:
      return xiiGALResourceFormat::A8UNormalized;

    case vk::Format::eE5B9G9R9UfloatPack32:
      return xiiGALResourceFormat::RGB9E5SharedExponent;

    case vk::Format::eBc1RgbUnormBlock:
      return xiiGALResourceFormat::BC1UNormalized;
    case vk::Format::eBc1RgbSrgbBlock:
      return xiiGALResourceFormat::BC1UNormalizedSRGB;

    case vk::Format::eBc2UnormBlock:
      return xiiGALResourceFormat::BC2UNormalized;
    case vk::Format::eBc2SrgbBlock:
      return xiiGALResourceFormat::BC2UNormalizedSRGB;

    case vk::Format::eBc3UnormBlock:
      return xiiGALResourceFormat::BC3UNormalized;
    case vk::Format::eBc3SrgbBlock:
      return xiiGALResourceFormat::BC3UNormalizedSRGB;

    case vk::Format::eBc4UnormBlock:
      return xiiGALResourceFormat::BC4UNormalized;
    case vk::Format::eBc4SnormBlock:
      return xiiGALResourceFormat::BC4SNormalized;

    case vk::Format::eBc5UnormBlock:
      return xiiGALResourceFormat::BC5UNormalized;
    case vk::Format::eBc5SnormBlock:
      return xiiGALResourceFormat::BC5SNormalized;

    case vk::Format::eB5G6R5UnormPack16:
      return xiiGALResourceFormat::B5G6R5UNormalized;
    case vk::Format::eB5G5R5A1UnormPack16:
      return xiiGALResourceFormat::B5G5R5A1UNormalized;
    case vk::Format::eB8G8R8A8Unorm:
      return xiiGALResourceFormat::BGRA8UNormalized;
    case vk::Format::eB8G8R8A8Srgb:
      return xiiGALResourceFormat::BGRA8UNormalizedSRGB;

    case vk::Format::eBc6HUfloatBlock:
      return xiiGALResourceFormat::BC6HUF16;
    case vk::Format::eBc6HSfloatBlock:
      return xiiGALResourceFormat::BC6HSF16;

    case vk::Format::eBc7UnormBlock:
      return xiiGALResourceFormat::BC7UNormalized;
    case vk::Format::eBc7SrgbBlock:
      return xiiGALResourceFormat::BC7UNormalizedSRGB;

    case vk::Format::eG8B8R82Plane420Unorm:
      return xiiGALResourceFormat::NV12;
    case vk::Format::eG10X6B10X6R10X62Plane420Unorm3Pack16:
      return xiiGALResourceFormat::P010;
    case vk::Format::eG16B16R162Plane420Unorm:
      return xiiGALResourceFormat::P016;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALResourceFormat::Unknown;
}

XII_ALWAYS_INLINE vk::ShaderStageFlags xiiVulkanTypeConversions::GetShaderStageFlags(xiiBitflags<xiiGALShaderType> e)
{
  if (e == xiiGALShaderType::AllGraphics)
  {
    return vk::ShaderStageFlagBits::eAllGraphics;
  }
  else if (e == xiiGALShaderType::All)
  {
    return vk::ShaderStageFlagBits::eAll;
  }

  vk::ShaderStageFlags shaderStage = {};

  if (e.IsSet(xiiGALShaderType::Vertex))
    shaderStage |= vk::ShaderStageFlagBits::eVertex;
  if (e.IsSet(xiiGALShaderType::Pixel))
    shaderStage |= vk::ShaderStageFlagBits::eFragment;
  if (e.IsSet(xiiGALShaderType::Geometry))
    shaderStage |= vk::ShaderStageFlagBits::eGeometry;
  if (e.IsSet(xiiGALShaderType::Hull))
    shaderStage |= vk::ShaderStageFlagBits::eTessellationControl;
  if (e.IsSet(xiiGALShaderType::Domain))
    shaderStage |= vk::ShaderStageFlagBits::eTessellationEvaluation;
  if (e.IsSet(xiiGALShaderType::Compute))
    shaderStage |= vk::ShaderStageFlagBits::eCompute;
  if (e.IsSet(xiiGALShaderType::Amplification))
    shaderStage |= vk::ShaderStageFlagBits::eTaskEXT;
  if (e.IsSet(xiiGALShaderType::Mesh))
    shaderStage |= vk::ShaderStageFlagBits::eMeshEXT;
  if (e.IsSet(xiiGALShaderType::RayGeneration))
    shaderStage |= vk::ShaderStageFlagBits::eRaygenKHR;
  if (e.IsSet(xiiGALShaderType::RayMiss))
    shaderStage |= vk::ShaderStageFlagBits::eMissKHR;
  if (e.IsSet(xiiGALShaderType::RayClosestHit))
    shaderStage |= vk::ShaderStageFlagBits::eClosestHitKHR;
  if (e.IsSet(xiiGALShaderType::RayAnyHit))
    shaderStage |= vk::ShaderStageFlagBits::eAnyHitKHR;
  if (e.IsSet(xiiGALShaderType::RayIntersection))
    shaderStage |= vk::ShaderStageFlagBits::eIntersectionKHR;
  if (e.IsSet(xiiGALShaderType::Callable))
    shaderStage |= vk::ShaderStageFlagBits::eCallableKHR;

  return shaderStage;
}

XII_ALWAYS_INLINE vk::ShaderStageFlagBits xiiVulkanTypeConversions::GetShaderStageFlagBits(xiiGALShaderType::Enum e)
{
  switch (e)
  {
    case xiiGALShaderType::Vertex:
      return vk::ShaderStageFlagBits::eVertex;
    case xiiGALShaderType::Pixel:
      return vk::ShaderStageFlagBits::eFragment;
    case xiiGALShaderType::Geometry:
      return vk::ShaderStageFlagBits::eGeometry;
    case xiiGALShaderType::Hull:
      return vk::ShaderStageFlagBits::eTessellationControl;
    case xiiGALShaderType::Domain:
      return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case xiiGALShaderType::Compute:
      return vk::ShaderStageFlagBits::eCompute;
    case xiiGALShaderType::Amplification:
      return vk::ShaderStageFlagBits::eTaskEXT;
    case xiiGALShaderType::Mesh:
      return vk::ShaderStageFlagBits::eMeshEXT;
    case xiiGALShaderType::RayGeneration:
      return vk::ShaderStageFlagBits::eRaygenKHR;
    case xiiGALShaderType::RayMiss:
      return vk::ShaderStageFlagBits::eMissKHR;
    case xiiGALShaderType::RayClosestHit:
      return vk::ShaderStageFlagBits::eClosestHitKHR;
    case xiiGALShaderType::RayAnyHit:
      return vk::ShaderStageFlagBits::eAnyHitKHR;
    case xiiGALShaderType::RayIntersection:
      return vk::ShaderStageFlagBits::eIntersectionKHR;
    case xiiGALShaderType::Callable:
      return vk::ShaderStageFlagBits::eCallableKHR;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::ShaderStageFlagBits();
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALShaderType> xiiVulkanTypeConversions::GetGALShaderStageFlags(vk::ShaderStageFlags e)
{
  if (e == vk::ShaderStageFlagBits::eAllGraphics)
  {
    return xiiGALShaderType::AllGraphics;
  }
  else if (e == vk::ShaderStageFlagBits::eAll)
  {
    return xiiGALShaderType::All;
  }

  xiiBitflags<xiiGALShaderType> shaderType;

  if (e & vk::ShaderStageFlagBits::eVertex)
    shaderType |= xiiGALShaderType::Vertex;
  if (e & vk::ShaderStageFlagBits::eTessellationControl)
    shaderType |= xiiGALShaderType::Hull;
  if (e & vk::ShaderStageFlagBits::eTessellationEvaluation)
    shaderType |= xiiGALShaderType::Domain;
  if (e & vk::ShaderStageFlagBits::eGeometry)
    shaderType |= xiiGALShaderType::Geometry;
  if (e & vk::ShaderStageFlagBits::eFragment)
    shaderType |= xiiGALShaderType::Pixel;
  if (e & vk::ShaderStageFlagBits::eCompute)
    shaderType |= xiiGALShaderType::Compute;
  if (e & vk::ShaderStageFlagBits::eRaygenKHR)
    shaderType |= xiiGALShaderType::RayGeneration;
  if (e & vk::ShaderStageFlagBits::eAnyHitKHR)
    shaderType |= xiiGALShaderType::RayAnyHit;
  if (e & vk::ShaderStageFlagBits::eClosestHitKHR)
    shaderType |= xiiGALShaderType::RayClosestHit;
  if (e & vk::ShaderStageFlagBits::eMissKHR)
    shaderType |= xiiGALShaderType::RayMiss;
  if (e & vk::ShaderStageFlagBits::eIntersectionKHR)
    shaderType |= xiiGALShaderType::RayIntersection;
  if (e & vk::ShaderStageFlagBits::eCallableKHR)
    shaderType |= xiiGALShaderType::Callable;
  if (e & vk::ShaderStageFlagBits::eTaskEXT)
    shaderType |= xiiGALShaderType::Amplification;
  if (e & vk::ShaderStageFlagBits::eMeshEXT)
    shaderType |= xiiGALShaderType::Mesh;

  return shaderType;
}

XII_ALWAYS_INLINE vk::Extent2D xiiVulkanTypeConversions::ShadingRateToFragmentSize(xiiBitflags<xiiGALShadingRateFlags> e)
{
  vk::Extent2D extent = {};
  extent.width        = XII_BIT((e.GetValue() >> XII_GAL_SHADING_RATE_X_SHIFT) & 0x3);
  extent.height       = XII_BIT(e.GetValue() & 0x3);

  XII_ASSERT_DEV(extent.width > 0U && extent.height > 0U, "");
  XII_ASSERT_DEV(extent.width <= XII_BIT(xiiGALShadingRateAxis::X4) && extent.height <= XII_BIT(xiiGALShadingRateAxis::X4), "");
  XII_ASSERT_DEV(xiiMath::IsPowerOf2(extent.width) && xiiMath::IsPowerOf2(extent.height), "");

  return extent;
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALShadingRateFlags> xiiVulkanTypeConversions::FragmentSizeToShadingRate(vk::Extent2D e)
{
  XII_ASSERT_DEV(xiiMath::IsPowerOf2(e.width) && xiiMath::IsPowerOf2(e.height), "");

  xiiUInt32 x = xiiMath::FirstBitHigh(e.width);
  xiiUInt32 y = xiiMath::FirstBitHigh(e.height);

  XII_ASSERT_DEV(XII_BIT(x) == e.width, "");
  XII_ASSERT_DEV(XII_BIT(y) == e.height, "");

  return static_cast<xiiGALShadingRateFlags::Enum>((x << XII_GAL_SHADING_RATE_X_SHIFT) | y);
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALCommandQueueFlags> xiiVulkanTypeConversions::GetGALCommandQueueFlags(vk::QueueFlags e)
{
  xiiBitflags<xiiGALCommandQueueFlags> queueFlags;

  if (e & vk::QueueFlagBits::eSparseBinding)
    queueFlags |= xiiGALCommandQueueFlags::SparseBinding;

  if (e & vk::QueueFlagBits::eGraphics)
    return queueFlags | xiiGALCommandQueueFlags::Graphics;

  if (e & vk::QueueFlagBits::eCompute)
    return queueFlags | xiiGALCommandQueueFlags::Compute;

  if (e & vk::QueueFlagBits::eTransfer)
    return queueFlags | xiiGALCommandQueueFlags::Transfer;

  return xiiGALCommandQueueFlags::None;
}

XII_ALWAYS_INLINE vk::SurfaceTransformFlagsKHR xiiVulkanTypeConversions::GetSurfaceTransform(xiiGALSurfaceTransform::Enum e)
{
  switch (e)
  {
    case xiiGALSurfaceTransform::Optimal:
      XII_ASSERT_DEV(false, "No Vulkan equivalent of xiiGALSurfaceTransform::Optimal.");
      return vk::SurfaceTransformFlagBitsKHR::eIdentity;
    case xiiGALSurfaceTransform::Identity:
      return vk::SurfaceTransformFlagBitsKHR::eIdentity;
    case xiiGALSurfaceTransform::Rotate90:
      return vk::SurfaceTransformFlagBitsKHR::eRotate90;
    case xiiGALSurfaceTransform::Rotate180:
      return vk::SurfaceTransformFlagBitsKHR::eRotate180;
    case xiiGALSurfaceTransform::Rotate270:
      return vk::SurfaceTransformFlagBitsKHR::eRotate270;
    case xiiGALSurfaceTransform::HorizontalMirror:
      return vk::SurfaceTransformFlagBitsKHR::eHorizontalMirror;
    case xiiGALSurfaceTransform::HorizontalMirrorRotate90:
      return vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90;
    case xiiGALSurfaceTransform::HorizontalMirrorRotate180:
      return vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180;
    case xiiGALSurfaceTransform::HorizontalMirrorRotate270:
      return vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::SurfaceTransformFlagBitsKHR::eIdentity;
}

XII_ALWAYS_INLINE xiiGALSurfaceTransform::Enum xiiVulkanTypeConversions::GetGALSurfaceTransform(vk::SurfaceTransformFlagsKHR e)
{
  XII_ASSERT_DEV(xiiMath::IsPowerOf2(xiiVulkanTypeConversions::GetUnderlyingFlagsValue(e)), "Expected a single bit transform.");

  if (e & vk::SurfaceTransformFlagBitsKHR::eIdentity)
    return xiiGALSurfaceTransform::Identity;
  if (e & vk::SurfaceTransformFlagBitsKHR::eRotate90)
    return xiiGALSurfaceTransform::Rotate90;
  if (e & vk::SurfaceTransformFlagBitsKHR::eRotate180)
    return xiiGALSurfaceTransform::Rotate180;
  if (e & vk::SurfaceTransformFlagBitsKHR::eRotate270)
    return xiiGALSurfaceTransform::Rotate270;
  if (e & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirror)
    return xiiGALSurfaceTransform::HorizontalMirror;
  if (e & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90)
    return xiiGALSurfaceTransform::HorizontalMirrorRotate90;
  if (e & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180)
    return xiiGALSurfaceTransform::HorizontalMirrorRotate180;
  if (e & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270)
    return xiiGALSurfaceTransform::HorizontalMirrorRotate270;

  XII_ASSERT_DEV(false, "Unexpected surface transform flag.");

  return xiiGALSurfaceTransform::Identity;
}

XII_ALWAYS_INLINE vk::Filter xiiVulkanTypeConversions::GetFilter(xiiGALFilterType::Enum e)
{
  switch (e)
  {
    case xiiGALFilterType::Point:
    case xiiGALFilterType::ComparisonPoint:
    case xiiGALFilterType::MinimumPoint:
    case xiiGALFilterType::MaximumPoint:
      return vk::Filter::eNearest;

    case xiiGALFilterType::Linear:
    case xiiGALFilterType::Anisotropic:
    case xiiGALFilterType::ComparisonLinear:
    case xiiGALFilterType::ComparisonAnisotropic:
    case xiiGALFilterType::MinimumAnisotropic:
    case xiiGALFilterType::MinimumLinear:
    case xiiGALFilterType::MaximumLinear:
    case xiiGALFilterType::MaximumAnisotropic:
      return vk::Filter::eLinear;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::Filter::eNearest;
}

XII_ALWAYS_INLINE vk::SamplerMipmapMode xiiVulkanTypeConversions::GetSamplerMipMapMode(xiiGALFilterType::Enum e)
{
  switch (e)
  {
    case xiiGALFilterType::Point:
    case xiiGALFilterType::ComparisonPoint:
    case xiiGALFilterType::MinimumPoint:
    case xiiGALFilterType::MaximumPoint:
      return vk::SamplerMipmapMode::eNearest;

    case xiiGALFilterType::Linear:
    case xiiGALFilterType::Anisotropic:
    case xiiGALFilterType::ComparisonLinear:
    case xiiGALFilterType::ComparisonAnisotropic:
    case xiiGALFilterType::MinimumLinear:
    case xiiGALFilterType::MinimumAnisotropic:
    case xiiGALFilterType::MaximumLinear:
    case xiiGALFilterType::MaximumAnisotropic:
      return vk::SamplerMipmapMode::eLinear;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::SamplerMipmapMode::eNearest;
}

XII_ALWAYS_INLINE vk::SamplerAddressMode xiiVulkanTypeConversions::GetSamplerAddressMode(xiiGALTextureAddressMode::Enum e)
{
  switch (e)
  {
    case xiiGALTextureAddressMode::Wrap:
      return vk::SamplerAddressMode::eRepeat;
    case xiiGALTextureAddressMode::Mirror:
      return vk::SamplerAddressMode::eMirroredRepeat;
    case xiiGALTextureAddressMode::Clamp:
      return vk::SamplerAddressMode::eClampToEdge;
    case xiiGALTextureAddressMode::Border:
      return vk::SamplerAddressMode::eClampToBorder;
    case xiiGALTextureAddressMode::MirrorOnce:
      return vk::SamplerAddressMode::eMirrorClampToEdge;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::SamplerAddressMode::eClampToEdge;
}

XII_ALWAYS_INLINE vk::VertexInputRate xiiVulkanTypeConversions::GetFrequency(xiiGALInputElementFrequency::Enum e)
{
  switch (e)
  {
    case xiiGALInputElementFrequency::PerVertex:
      return vk::VertexInputRate::eVertex;
    case xiiGALInputElementFrequency::PerInstance:
      return vk::VertexInputRate::eInstance;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::VertexInputRate::eVertex;
}

XII_ALWAYS_INLINE vk::AttachmentLoadOp xiiVulkanTypeConversions::GetAttachmentLoadOperation(xiiGALAttachmentLoadOperation::Enum e)
{
  switch (e)
  {
    case xiiGALAttachmentLoadOperation::Load:
      return vk::AttachmentLoadOp::eLoad;
    case xiiGALAttachmentLoadOperation::Clear:
      return vk::AttachmentLoadOp::eClear;
    case xiiGALAttachmentLoadOperation::Discard:
      return vk::AttachmentLoadOp::eDontCare;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::AttachmentLoadOp::eLoad;
}

XII_ALWAYS_INLINE vk::AttachmentStoreOp xiiVulkanTypeConversions::GetAttachmentStoreOperation(xiiGALAttachmentStoreOperation::Enum e)
{
  switch (e)
  {
    case xiiGALAttachmentStoreOperation::Store:
      return vk::AttachmentStoreOp::eStore;
    case xiiGALAttachmentStoreOperation::Discard:
      return vk::AttachmentStoreOp::eDontCare;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::AttachmentStoreOp::eStore;
}

XII_ALWAYS_INLINE vk::ImageLayout xiiVulkanTypeConversions::GetImageLayout(xiiBitflags<xiiGALResourceStateFlags> e, bool bIsInsideRenderPass, bool bFragDensityMapInsteadOfShadingRate)
{
  if (e == xiiGALResourceStateFlags::Unknown)
    return vk::ImageLayout::eUndefined;

  // Currently not used:
  // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
  // VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
  // VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV
  // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
  // VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,

  XII_ASSERT_DEV((e.GetValue() & (e.GetValue() - 1)) == 0, "Expected a single bit set.");

  switch (e.GetValue())
  {
    case xiiGALResourceStateFlags::Undefined:
      return vk::ImageLayout::eUndefined;
    case xiiGALResourceStateFlags::VertexBuffer:
      XII_ASSERT_DEV(false, "Invalid resource state!");
      return vk::ImageLayout::eUndefined;
    case xiiGALResourceStateFlags::ConstantBuffer:
      XII_ASSERT_DEV(false, "Invalid resource state!");
      return vk::ImageLayout::eUndefined;
    case xiiGALResourceStateFlags::IndexBuffer:
      XII_ASSERT_DEV(false, "Invalid resource state!");
      return vk::ImageLayout::eUndefined;
    case xiiGALResourceStateFlags::RenderTarget:
      return vk::ImageLayout::eColorAttachmentOptimal;
    case xiiGALResourceStateFlags::UnorderedAccess:
      return vk::ImageLayout::eGeneral;
    case xiiGALResourceStateFlags::DepthWrite:
      return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case xiiGALResourceStateFlags::DepthRead:
      return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    case xiiGALResourceStateFlags::ShaderResource:
      return vk::ImageLayout::eShaderReadOnlyOptimal;
    case xiiGALResourceStateFlags::StreamOut:
      XII_ASSERT_DEV(false, "Invalid resource state!");
      return vk::ImageLayout::eUndefined;
    case xiiGALResourceStateFlags::IndirectArgument:
      XII_ASSERT_DEV(false, "Invalid resource state!");
      return vk::ImageLayout::eUndefined;
    case xiiGALResourceStateFlags::CopyDestination:
      return vk::ImageLayout::eTransferDstOptimal;
    case xiiGALResourceStateFlags::CopySource:
      return vk::ImageLayout::eTransferSrcOptimal;
    case xiiGALResourceStateFlags::ResolveDestination:
      return bIsInsideRenderPass ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::eTransferDstOptimal;
    case xiiGALResourceStateFlags::ResolveSource:
      return vk::ImageLayout::eTransferSrcOptimal;
    case xiiGALResourceStateFlags::InputAttachment:
      return vk::ImageLayout::eShaderReadOnlyOptimal;
    case xiiGALResourceStateFlags::Present:
      return vk::ImageLayout::ePresentSrcKHR;
    case xiiGALResourceStateFlags::BuildASRead:
      XII_ASSERT_DEV(false, "Invalid resource state!");
      return vk::ImageLayout::eUndefined;
    case xiiGALResourceStateFlags::BuildASWrite:
      XII_ASSERT_DEV(false, "Invalid resource state!");
      return vk::ImageLayout::eUndefined;
    case xiiGALResourceStateFlags::RayTracing:
      XII_ASSERT_DEV(false, "Invalid resource state!");
      return vk::ImageLayout::eUndefined;
    case xiiGALResourceStateFlags::Common:
      return vk::ImageLayout::eGeneral;
    case xiiGALResourceStateFlags::ShadingRate:
      return bFragDensityMapInsteadOfShadingRate ? vk::ImageLayout::eFragmentDensityMapOptimalEXT : vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::ImageLayout::eUndefined;
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiVulkanTypeConversions::GetResourceState(vk::ImageLayout e)
{
  switch (e)
  {
    case vk::ImageLayout::eUndefined:
      return xiiGALResourceStateFlags::Undefined;
    case vk::ImageLayout::eGeneral:
      return xiiGALResourceStateFlags::UnorderedAccess;
    case vk::ImageLayout::eColorAttachmentOptimal:
      return xiiGALResourceStateFlags::RenderTarget;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
      return xiiGALResourceStateFlags::DepthWrite;
    case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
      return xiiGALResourceStateFlags::DepthRead;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
      return xiiGALResourceStateFlags::ShaderResource;
    case vk::ImageLayout::eTransferSrcOptimal:
      return xiiGALResourceStateFlags::CopySource;
    case vk::ImageLayout::eTransferDstOptimal:
      return xiiGALResourceStateFlags::CopyDestination;
    case vk::ImageLayout::ePreinitialized:
      XII_REPORT_FAILURE("vk::ImageLayout::ePreinitialized is not supported.");
      return xiiGALResourceStateFlags::Undefined;
    case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
      XII_REPORT_FAILURE("vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal is not supported.");
      return xiiGALResourceStateFlags::Undefined;
    case vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal:
      XII_REPORT_FAILURE("vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal is not supported.");
      return xiiGALResourceStateFlags::Undefined;
    case vk::ImageLayout::ePresentSrcKHR:
      return xiiGALResourceStateFlags::Present;
    case vk::ImageLayout::eSharedPresentKHR:
      XII_REPORT_FAILURE("vk::ImageLayout::eSharedPresentKHR is not supported.");
      return xiiGALResourceStateFlags::Undefined;
    case vk::ImageLayout::eFragmentDensityMapOptimalEXT:
    case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
      return xiiGALResourceStateFlags::ShadingRate;

    default:
      XII_REPORT_FAILURE("Unknown Vulkan image layout ({}).", (xiiUInt32)e);
  }
  return xiiGALResourceStateFlags::Undefined;
}

XII_ALWAYS_INLINE vk::PipelineStageFlags xiiVulkanTypeConversions::GetPipelineStageFlags(xiiBitflags<xiiGALPipelineStageFlags> e)
{
  vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eNone;

  if (e.IsSet(xiiGALPipelineStageFlags::TopOfPipeline))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eTopOfPipe;
  if (e.IsSet(xiiGALPipelineStageFlags::DrawIndirect))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eDrawIndirect;
  if (e.IsSet(xiiGALPipelineStageFlags::VertexInput))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eVertexInput;
  if (e.IsSet(xiiGALPipelineStageFlags::VertexShader))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eVertexShader;
  if (e.IsSet(xiiGALPipelineStageFlags::HullShader))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eTessellationControlShader;
  if (e.IsSet(xiiGALPipelineStageFlags::DomainShader))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eTessellationEvaluationShader;
  if (e.IsSet(xiiGALPipelineStageFlags::GeometryShader))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eGeometryShader;
  if (e.IsSet(xiiGALPipelineStageFlags::PixelShader))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eFragmentShader;
  if (e.IsSet(xiiGALPipelineStageFlags::EarlyFragmentTests))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eEarlyFragmentTests;
  if (e.IsSet(xiiGALPipelineStageFlags::LateFragmentTests))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eLateFragmentTests;
  if (e.IsSet(xiiGALPipelineStageFlags::RenderTarget))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
  if (e.IsSet(xiiGALPipelineStageFlags::ComputeShader))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eComputeShader;
  if (e.IsSet(xiiGALPipelineStageFlags::Transfer))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eTransfer;
  if (e.IsSet(xiiGALPipelineStageFlags::BottomOfPipeline))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eBottomOfPipe;
  if (e.IsSet(xiiGALPipelineStageFlags::Host))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eHost;
  if (e.IsSet(xiiGALPipelineStageFlags::ConditionalRendering))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eConditionalRenderingEXT;
  if (e.IsSet(xiiGALPipelineStageFlags::ShadingRateTexture))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eShadingRateImageNV;
  if (e.IsSet(xiiGALPipelineStageFlags::RayTracingShader))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eRayTracingShaderNV;
  if (e.IsSet(xiiGALPipelineStageFlags::AccelerationStructureBuild))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eAccelerationStructureBuildNV;
  if (e.IsSet(xiiGALPipelineStageFlags::TaskShader))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eTaskShaderEXT;
  if (e.IsSet(xiiGALPipelineStageFlags::MeshShader))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eMeshShaderEXT;
  if (e.IsSet(xiiGALPipelineStageFlags::FragmentDensityProcess))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eFragmentDensityProcessEXT;

  return pipelineStageFlags;
}

XII_ALWAYS_INLINE vk::PipelineStageFlags xiiVulkanTypeConversions::GetPipelineStageFlags(xiiBitflags<xiiGALResourceStateFlags> e)
{
  vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eNone;

  if (e.IsSet(xiiGALResourceStateFlags::Undefined))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eTopOfPipe;
  if (e.IsSet(xiiGALResourceStateFlags::VertexBuffer))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eVertexInput;
  if (e.IsSet(xiiGALResourceStateFlags::ConstantBuffer))
    pipelineStageFlags |= VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
  if (e.IsSet(xiiGALResourceStateFlags::IndexBuffer))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eVertexInput;
  if (e.IsSet(xiiGALResourceStateFlags::RenderTarget))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
  if (e.IsSet(xiiGALResourceStateFlags::UnorderedAccess))
    pipelineStageFlags |= VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
  if (e.IsSet(xiiGALResourceStateFlags::DepthWrite))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
  if (e.IsSet(xiiGALResourceStateFlags::DepthRead))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
  if (e.IsSet(xiiGALResourceStateFlags::ShaderResource))
    pipelineStageFlags |= VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
  if (e.IsSet(xiiGALResourceStateFlags::IndirectArgument))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eDrawIndirect;
  if (e.IsSet(xiiGALResourceStateFlags::CopyDestination))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eTransfer;
  if (e.IsSet(xiiGALResourceStateFlags::CopySource))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eTransfer;
  if (e.IsSet(xiiGALResourceStateFlags::ResolveDestination))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eTransfer;
  if (e.IsSet(xiiGALResourceStateFlags::ResolveSource))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eTransfer;
  if (e.IsSet(xiiGALResourceStateFlags::InputAttachment))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eFragmentShader;
  if (e.IsSet(xiiGALResourceStateFlags::Present))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eBottomOfPipe;
  if (e.IsSet(xiiGALResourceStateFlags::BuildASRead))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR;
  if (e.IsSet(xiiGALResourceStateFlags::BuildASWrite))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR;
  if (e.IsSet(xiiGALResourceStateFlags::RayTracing))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eRayTracingShaderKHR;
  if (e.IsSet(xiiGALResourceStateFlags::Common))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eAllCommands;
  if (e.IsSet(xiiGALResourceStateFlags::ShadingRate))
    pipelineStageFlags |= vk::PipelineStageFlagBits::eFragmentDensityProcessEXT | vk::PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR;

  // No flag to set for the following.
  // if (e.IsSet(xiiGALResourceStateFlags::StreamOut))
  //   pipelineStageFlags |= vk::PipelineStageFlagBits::eStreamOutput;

  return pipelineStageFlags;
}

XII_ALWAYS_INLINE vk::AccessFlags xiiVulkanTypeConversions::GetAccessFlags(xiiBitflags<xiiGALAccessFlags> e)
{
  vk::AccessFlags vkAccessFlags = vk::AccessFlagBits::eNone;

  if (e.IsSet(xiiGALAccessFlags::IndirectCommandRead))
    vkAccessFlags |= vk::AccessFlagBits::eIndirectCommandRead;
  if (e.IsSet(xiiGALAccessFlags::IndexRead))
    vkAccessFlags |= vk::AccessFlagBits::eIndexRead;
  if (e.IsSet(xiiGALAccessFlags::VertexRead))
    vkAccessFlags |= vk::AccessFlagBits::eVertexAttributeRead;
  if (e.IsSet(xiiGALAccessFlags::UniformRead))
    vkAccessFlags |= vk::AccessFlagBits::eUniformRead;
  if (e.IsSet(xiiGALAccessFlags::InputAttachmentRead))
    vkAccessFlags |= vk::AccessFlagBits::eInputAttachmentRead;
  if (e.IsSet(xiiGALAccessFlags::ShaderRead))
    vkAccessFlags |= vk::AccessFlagBits::eShaderRead;
  if (e.IsSet(xiiGALAccessFlags::ShaderWrite))
    vkAccessFlags |= vk::AccessFlagBits::eShaderWrite;
  if (e.IsSet(xiiGALAccessFlags::RenderTargetRead))
    vkAccessFlags |= vk::AccessFlagBits::eColorAttachmentRead;
  if (e.IsSet(xiiGALAccessFlags::RenderTargetWrite))
    vkAccessFlags |= vk::AccessFlagBits::eColorAttachmentWrite;
  if (e.IsSet(xiiGALAccessFlags::DepthStencilRead))
    vkAccessFlags |= vk::AccessFlagBits::eDepthStencilAttachmentRead;
  if (e.IsSet(xiiGALAccessFlags::DepthStencilWrite))
    vkAccessFlags |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
  if (e.IsSet(xiiGALAccessFlags::CopySource))
    vkAccessFlags |= vk::AccessFlagBits::eTransferRead;
  if (e.IsSet(xiiGALAccessFlags::CopyDestination))
    vkAccessFlags |= vk::AccessFlagBits::eTransferWrite;
  if (e.IsSet(xiiGALAccessFlags::HostRead))
    vkAccessFlags |= vk::AccessFlagBits::eHostRead;
  if (e.IsSet(xiiGALAccessFlags::HostWrite))
    vkAccessFlags |= vk::AccessFlagBits::eHostWrite;
  if (e.IsSet(xiiGALAccessFlags::MemoryRead))
    vkAccessFlags |= vk::AccessFlagBits::eMemoryRead;
  if (e.IsSet(xiiGALAccessFlags::MemoryWrite))
    vkAccessFlags |= vk::AccessFlagBits::eMemoryWrite;
  if (e.IsSet(xiiGALAccessFlags::ConditionalRenderingRead))
    vkAccessFlags |= vk::AccessFlagBits::eConditionalRenderingReadEXT;
  if (e.IsSet(xiiGALAccessFlags::ShadingRateTextureRead))
    vkAccessFlags |= vk::AccessFlagBits::eShadingRateImageReadNV;
  if (e.IsSet(xiiGALAccessFlags::AccelerationStructureRead))
    vkAccessFlags |= vk::AccessFlagBits::eAccelerationStructureReadNV;
  if (e.IsSet(xiiGALAccessFlags::AccelerationStructureWrite))
    vkAccessFlags |= vk::AccessFlagBits::eAccelerationStructureWriteNV;
  if (e.IsSet(xiiGALAccessFlags::FragmentDensityMapRead))
    vkAccessFlags |= vk::AccessFlagBits::eFragmentDensityMapReadEXT;

  return vkAccessFlags;
}

XII_ALWAYS_INLINE vk::AccessFlags xiiVulkanTypeConversions::GetAccessFlags(xiiBitflags<xiiGALResourceStateFlags> e)
{
  vk::AccessFlags vkAccessFlags = vk::AccessFlagBits::eNone;

  if (e.IsSet(xiiGALResourceStateFlags::Undefined))
    vkAccessFlags |= static_cast<vk::AccessFlagBits>(0);
  if (e.IsSet(xiiGALResourceStateFlags::VertexBuffer))
    vkAccessFlags |= vk::AccessFlagBits::eVertexAttributeRead;
  if (e.IsSet(xiiGALResourceStateFlags::ConstantBuffer))
    vkAccessFlags |= vk::AccessFlagBits::eUniformRead;
  if (e.IsSet(xiiGALResourceStateFlags::IndexBuffer))
    vkAccessFlags |= vk::AccessFlagBits::eIndexRead;
  if (e.IsSet(xiiGALResourceStateFlags::RenderTarget))
    vkAccessFlags |= vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
  if (e.IsSet(xiiGALResourceStateFlags::UnorderedAccess))
    vkAccessFlags |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
  if (e.IsSet(xiiGALResourceStateFlags::DepthWrite))
    vkAccessFlags |= vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
  if (e.IsSet(xiiGALResourceStateFlags::DepthRead))
    vkAccessFlags |= vk::AccessFlagBits::eDepthStencilAttachmentRead;
  if (e.IsSet(xiiGALResourceStateFlags::ShaderResource))
    vkAccessFlags |= vk::AccessFlagBits::eShaderRead;
  if (e.IsSet(xiiGALResourceStateFlags::StreamOut))
    vkAccessFlags |= vk::AccessFlagBits::eTransformFeedbackCounterWriteEXT;
  if (e.IsSet(xiiGALResourceStateFlags::IndirectArgument))
    vkAccessFlags |= vk::AccessFlagBits::eIndirectCommandRead;
  if (e.IsSet(xiiGALResourceStateFlags::CopyDestination))
    vkAccessFlags |= vk::AccessFlagBits::eTransferWrite;
  if (e.IsSet(xiiGALResourceStateFlags::CopySource))
    vkAccessFlags |= vk::AccessFlagBits::eTransferRead;
  if (e.IsSet(xiiGALResourceStateFlags::ResolveDestination))
    vkAccessFlags |= vk::AccessFlagBits::eTransferWrite;
  if (e.IsSet(xiiGALResourceStateFlags::ResolveSource))
    vkAccessFlags |= vk::AccessFlagBits::eTransferRead;
  if (e.IsSet(xiiGALResourceStateFlags::InputAttachment))
    vkAccessFlags |= vk::AccessFlagBits::eInputAttachmentRead;
  if (e.IsSet(xiiGALResourceStateFlags::Present))
    vkAccessFlags |= static_cast<vk::AccessFlagBits>(0);
  if (e.IsSet(xiiGALResourceStateFlags::BuildASRead))
    vkAccessFlags |= vk::AccessFlagBits::eShaderRead;
  if (e.IsSet(xiiGALResourceStateFlags::BuildASWrite))                                                                       // for vertex, index, transform, AABB, instance buffers
    vkAccessFlags |= vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eAccelerationStructureWriteKHR; // for scratch buffer
  if (e.IsSet(xiiGALResourceStateFlags::RayTracing))
    vkAccessFlags |= vk::AccessFlagBits::eShaderRead; // for SBT
  if (e.IsSet(xiiGALResourceStateFlags::Common))
    vkAccessFlags |= static_cast<vk::AccessFlagBits>(0); // COMMON state must be used for queue to queue transition (like in D3D12), queue to queue synchronization via semaphore creates a memory dependency
  if (e.IsSet(xiiGALResourceStateFlags::ShadingRate))
    vkAccessFlags |= vk::AccessFlagBits::eFragmentDensityMapReadEXT | vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;

  return vkAccessFlags;
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiVulkanTypeConversions::GetResourceState(vk::AccessFlags e)
{
  xiiBitflags<xiiGALResourceStateFlags> resourceStateFlags = xiiGALResourceStateFlags::Unknown;

  if (e == vk::AccessFlags{})
  {
    resourceStateFlags |= xiiGALResourceStateFlags::Common;
  }

  if ((e & vk::AccessFlagBits::eVertexAttributeRead) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::VertexBuffer;
  if ((e & vk::AccessFlagBits::eUniformRead) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::ConstantBuffer;
  if ((e & vk::AccessFlagBits::eIndexRead) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::IndexBuffer;
  if ((e & (vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::RenderTarget;
  if ((e & (vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite)) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::DepthWrite;
  if ((e & vk::AccessFlagBits::eInputAttachmentRead) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::InputAttachment;
  if ((e & vk::AccessFlagBits::eIndirectCommandRead) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::IndirectArgument;
  if ((e & vk::AccessFlagBits::eTransferRead) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::CopySource | xiiGALResourceStateFlags::ResolveSource;
  if ((e & vk::AccessFlagBits::eTransferWrite) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::CopyDestination | xiiGALResourceStateFlags::ResolveDestination;
  if ((e & vk::AccessFlagBits::eShaderRead) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::ShaderResource;
  if ((e & vk::AccessFlagBits::eShaderWrite) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::UnorderedAccess;
  if ((e & (vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eAccelerationStructureReadNV)) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::BuildASRead;
  if ((e & (vk::AccessFlagBits::eAccelerationStructureWriteKHR | vk::AccessFlagBits::eAccelerationStructureWriteNV)) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::BuildASWrite;
  if ((e & (vk::AccessFlagBits::eFragmentDensityMapReadEXT | vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR)) != vk::AccessFlags{})
    resourceStateFlags |= xiiGALResourceStateFlags::ShadingRate;

  if (resourceStateFlags == xiiGALResourceStateFlags::Unknown)
  {
    resourceStateFlags |= xiiGALResourceStateFlags::Common;
  }

  return resourceStateFlags;
}

XII_ALWAYS_INLINE void xiiVulkanTypeConversions::GetPermittedStagesAndAccessFlags(xiiBitflags<xiiGALBindFlags> e, vk::PipelineStageFlags& vkStageFlags, vk::AccessFlags& vkAccessFlags)
{
  vkStageFlags  = vk::PipelineStageFlagBits::eTransfer;
  vkAccessFlags = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite;

  for (auto v : e)
  {
    switch (v)
    {
      case xiiGALBindFlags::VertexBuffer:
      {
        vkStageFlags |= vk::PipelineStageFlagBits::eVertexInput;
        vkAccessFlags |= vk::AccessFlagBits::eVertexAttributeRead;
      }
      break;
      case xiiGALBindFlags::IndexBuffer:
      {
        vkStageFlags |= vk::PipelineStageFlagBits::eVertexInput;
        vkAccessFlags |= vk::AccessFlagBits::eIndexRead;
      }
      break;
      case xiiGALBindFlags::UniformBuffer:
      {
        vkStageFlags |= VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
        vkAccessFlags |= vk::AccessFlagBits::eUniformRead;
      }
      break;
      case xiiGALBindFlags::ShaderResource:
      {
        vkStageFlags |= VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
        vkAccessFlags |= vk::AccessFlagBits::eShaderRead;
      }
      break;
      case xiiGALBindFlags::RenderTarget:
      {
        vkStageFlags |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vkAccessFlags |= vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
      }
      break;
      case xiiGALBindFlags::DepthStencil:
      {
        vkStageFlags |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
        vkAccessFlags |= vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      }
      break;
      case xiiGALBindFlags::UnorderedAccess:
      {
        vkStageFlags |= VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
        vkAccessFlags |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
      }
      break;
      case xiiGALBindFlags::IndirectDrawArguments:
      {
        vkStageFlags |= vk::PipelineStageFlagBits::eDrawIndirect;
        vkAccessFlags |= vk::AccessFlagBits::eIndirectCommandRead;
      }
      break;
      case xiiGALBindFlags::InputAttachment:
      {
        vkStageFlags |= vk::PipelineStageFlagBits::eFragmentShader;
        vkAccessFlags |= vk::AccessFlagBits::eInputAttachmentRead;
      }
      break;
      case xiiGALBindFlags::RayTracing:
      {
        vkStageFlags |= vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR | vk::PipelineStageFlagBits::eRayTracingShaderKHR;
        vkAccessFlags |= vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eAccelerationStructureWriteKHR;
      }
      break;
      case xiiGALBindFlags::ShadingRate:
      {
        vkStageFlags |= vk::PipelineStageFlagBits::eFragmentDensityProcessEXT | vk::PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR;
        vkAccessFlags |= vk::AccessFlagBits::eFragmentDensityMapReadEXT | vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
      }
      break;

      case xiiGALBindFlags::None:
      case xiiGALBindFlags::StreamOutput:
      default:
        XII_REPORT_FAILURE("Unexpected bind flag.");
        break;
    }
  }
}

XII_ALWAYS_INLINE vk::ComponentSwizzle xiiVulkanTypeConversions::GetComponentSwizzle(xiiGALTextureComponentSwizzle::Enum e)
{
  switch (e)
  {
    case xiiGALTextureComponentSwizzle::Identity:
      return vk::ComponentSwizzle::eIdentity;
    case xiiGALTextureComponentSwizzle::Zero:
      return vk::ComponentSwizzle::eZero;
    case xiiGALTextureComponentSwizzle::One:
      return vk::ComponentSwizzle::eOne;
    case xiiGALTextureComponentSwizzle::R:
      return vk::ComponentSwizzle::eR;
    case xiiGALTextureComponentSwizzle::G:
      return vk::ComponentSwizzle::eG;
    case xiiGALTextureComponentSwizzle::B:
      return vk::ComponentSwizzle::eB;
    case xiiGALTextureComponentSwizzle::A:
      return vk::ComponentSwizzle::eA;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::ComponentSwizzle::eIdentity;
}

XII_ALWAYS_INLINE vk::ComponentMapping xiiVulkanTypeConversions::GetComponentMapping(const xiiGALTextureComponentMapping& mapping)
{
  return vk::ComponentMapping{xiiVulkanTypeConversions::GetComponentSwizzle(mapping.m_R), xiiVulkanTypeConversions::GetComponentSwizzle(mapping.m_G), xiiVulkanTypeConversions::GetComponentSwizzle(mapping.m_B), xiiVulkanTypeConversions::GetComponentSwizzle(mapping.m_A)};
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALSparseTextureFlags> xiiVulkanTypeConversions::GetSparseTextureFlags(vk::SparseImageFormatFlags e)
{
  xiiBitflags<xiiGALSparseTextureFlags> sparseTextureFlags = xiiGALSparseTextureFlags::None;

  if (e & vk::SparseImageFormatFlagBits::eSingleMiptail)
    sparseTextureFlags |= xiiGALSparseTextureFlags::SingleMipTail;
  if (e & vk::SparseImageFormatFlagBits::eAlignedMipSize)
    sparseTextureFlags |= xiiGALSparseTextureFlags::AlignedMipSize;
  if (e & vk::SparseImageFormatFlagBits::eNonstandardBlockSize)
    sparseTextureFlags |= xiiGALSparseTextureFlags::NonStandardBlockSize;

  return sparseTextureFlags;
}

XII_ALWAYS_INLINE vk::ImageUsageFlags xiiVulkanTypeConversions::GetImageUsageFlags(xiiBitflags<xiiGALBindFlags> bindFlags, bool bIsMemoryless, bool bFragmentDensityMapInsteadOfShadingRate)
{
  vk::ImageUsageFlags vkImageUsageFlags = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

  if (bindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    vkImageUsageFlags |= vk::ImageUsageFlagBits::eSampled;
  if (bindFlags.IsSet(xiiGALBindFlags::RenderTarget)) // VK_IMAGE_USAGE_TRANSFER_DST_BIT is required for vkCmdClearColorImage
    vkImageUsageFlags |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  if (bindFlags.IsSet(xiiGALBindFlags::DepthStencil)) // VK_IMAGE_USAGE_TRANSFER_DST_BIT is required for vkCmdClearDepthStencilImage()
    vkImageUsageFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferDst;
  if (bindFlags.IsSet(xiiGALBindFlags::UnorderedAccess))
    vkImageUsageFlags |= vk::ImageUsageFlagBits::eStorage;
  if (bindFlags.IsSet(xiiGALBindFlags::InputAttachment))
    vkImageUsageFlags |= vk::ImageUsageFlagBits::eInputAttachment;
  if (bindFlags.IsSet(xiiGALBindFlags::ShadingRate))
    vkImageUsageFlags |= (bFragmentDensityMapInsteadOfShadingRate ? vk::ImageUsageFlagBits::eFragmentDensityMapEXT : vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR);

  if (bIsMemoryless)
  {
    vkImageUsageFlags &= (vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment);
    vkImageUsageFlags |= vk::ImageUsageFlagBits::eTransientAttachment;
  }
  return vkImageUsageFlags;
}

XII_ALWAYS_INLINE xiiGALDescriporTypeVulkan xiiVulkanTypeConversions::GetDescriptorType(const xiiGALPipelineResourceDescription& resourceDescription)
{
  XII_ASSERT_DEV(resourceDescription.m_PipelineResourceFlags.IsStrictlyAnySet(xiiGALGraphicsUtilities::GetValidPipelineResourceFlags(resourceDescription.m_ResourceType)) || resourceDescription.m_PipelineResourceFlags.IsNoFlagSet(), "Invalid resource flags, implementation error!");

  const bool bWithDynamicOffset      = !resourceDescription.m_PipelineResourceFlags.IsSet(xiiGALPipelineResourceFlags::NoDynamicBuffers);
  const bool bCombinedSampler        = resourceDescription.m_PipelineResourceFlags.IsSet(xiiGALPipelineResourceFlags::CombinedSampler);
  const bool bUseTexelBuffer         = resourceDescription.m_PipelineResourceFlags.IsSet(xiiGALPipelineResourceFlags::Formattedbuffer);
  const bool bGeneralInputAttachment = resourceDescription.m_PipelineResourceFlags.IsSet(xiiGALPipelineResourceFlags::GeneralInputAttachment);

  switch (resourceDescription.m_ResourceType)
  {
    case xiiGALShaderResourceType::ConstantBuffer:
      return bWithDynamicOffset ? xiiGALDescriporTypeVulkan::UniformBufferDynamic : xiiGALDescriporTypeVulkan::UniformBuffer;
    case xiiGALShaderResourceType::TextureSRV:
      return bCombinedSampler ? xiiGALDescriporTypeVulkan::CombinedImageSampler : xiiGALDescriporTypeVulkan::SeparateImage;
    case xiiGALShaderResourceType::BufferSRV:
      return bUseTexelBuffer ? xiiGALDescriporTypeVulkan::UniformTexelBuffer : (bWithDynamicOffset ? xiiGALDescriporTypeVulkan::StorageBufferDynamicReadOnly : xiiGALDescriporTypeVulkan::StorageBufferReadOnly);
    case xiiGALShaderResourceType::TextureUAV:
      return xiiGALDescriporTypeVulkan::StorageImage;
    case xiiGALShaderResourceType::BufferUAV:
      return bUseTexelBuffer ? xiiGALDescriporTypeVulkan::StorageTexelBuffer : (bWithDynamicOffset ? xiiGALDescriporTypeVulkan::StorageBufferDynamic : xiiGALDescriporTypeVulkan::StorageBuffer);
    case xiiGALShaderResourceType::Sampler:
      return xiiGALDescriporTypeVulkan::Sampler;
    case xiiGALShaderResourceType::InputAttachment:
      return bGeneralInputAttachment ? xiiGALDescriporTypeVulkan::InputAttachmentGeneral : xiiGALDescriporTypeVulkan::InputAttachment;
    case xiiGALShaderResourceType::AccelerationStructure:
      return xiiGALDescriporTypeVulkan::AccelerationStructure;
    case xiiGALShaderResourceType::TextureAndSampler:
      return xiiGALDescriporTypeVulkan::CombinedImageSampler;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALDescriporTypeVulkan::ENUM_COUNT;
}

XII_ALWAYS_INLINE void xiiVulkanTypeConversions::GetPrimitiveTopologyAndControlPatchPointsCount(xiiGALPrimitiveTopology::Enum e, vk::PrimitiveTopology& out_vkPrimitiveTopology, xiiUInt32& out_uiPatchControlPoints)
{
  out_uiPatchControlPoints = 0U;

  switch (e)
  {
    case xiiGALPrimitiveTopology::Undefined:
    {
      XII_REPORT_FAILURE("Unexpected primitive topology undefined.");

      out_vkPrimitiveTopology = vk::PrimitiveTopology::eTriangleList;
      return;
    }
    case xiiGALPrimitiveTopology::PointList:
      out_vkPrimitiveTopology = vk::PrimitiveTopology::ePointList;
      return;
    case xiiGALPrimitiveTopology::LineList:
      out_vkPrimitiveTopology = vk::PrimitiveTopology::eLineList;
      return;
    case xiiGALPrimitiveTopology::TriangleList:
      out_vkPrimitiveTopology = vk::PrimitiveTopology::eTriangleList;
      return;
    case xiiGALPrimitiveTopology::TriangleStrip:
      out_vkPrimitiveTopology = vk::PrimitiveTopology::eTriangleStrip;
      return;
    case xiiGALPrimitiveTopology::LineStrip:
      out_vkPrimitiveTopology = vk::PrimitiveTopology::eLineStrip;
      return;
    case xiiGALPrimitiveTopology::TriangleListAdjacent:
      out_vkPrimitiveTopology = vk::PrimitiveTopology::eTriangleListWithAdjacency;
      return;
    case xiiGALPrimitiveTopology::TriangleStripAdjacent:
      out_vkPrimitiveTopology = vk::PrimitiveTopology::eTriangleStripWithAdjacency;
      return;
    case xiiGALPrimitiveTopology::LineListAdjacent:
      out_vkPrimitiveTopology = vk::PrimitiveTopology::eLineListWithAdjacency;
      return;
    case xiiGALPrimitiveTopology::LineStripAdjacent:
      out_vkPrimitiveTopology = vk::PrimitiveTopology::eLineStripWithAdjacency;
      return;
    default:
    {
      XII_ASSERT_DEV(e >= xiiGALPrimitiveTopology::ControlPointPatchList1 && e <= xiiGALPrimitiveTopology::ControlPointPatchList32, "");

      out_vkPrimitiveTopology  = vk::PrimitiveTopology::ePatchList;
      out_uiPatchControlPoints = static_cast<xiiUInt32>(e) - static_cast<xiiUInt32>(xiiGALPrimitiveTopology::ControlPointPatchList1) + 1U;
      return;
    }
  }
}

XII_ALWAYS_INLINE vk::DescriptorType xiiVulkanTypeConversions::GetDescriptorType(xiiGALDescriporTypeVulkan e)
{
  switch (e)
  {
    case xiiGALDescriporTypeVulkan::Sampler:
      return vk::DescriptorType::eSampler;
    case xiiGALDescriporTypeVulkan::CombinedImageSampler:
      return vk::DescriptorType::eCombinedImageSampler;
    case xiiGALDescriporTypeVulkan::SeparateImage:
      return vk::DescriptorType::eSampledImage;
    case xiiGALDescriporTypeVulkan::StorageImage:
      return vk::DescriptorType::eStorageImage;
    case xiiGALDescriporTypeVulkan::UniformTexelBuffer:
      return vk::DescriptorType::eUniformTexelBuffer;
    case xiiGALDescriporTypeVulkan::StorageTexelBuffer:
      return vk::DescriptorType::eStorageTexelBuffer;
    case xiiGALDescriporTypeVulkan::StorageTexelBufferReadOnly:
      return vk::DescriptorType::eStorageTexelBuffer;
    case xiiGALDescriporTypeVulkan::UniformBuffer:
      return vk::DescriptorType::eUniformBuffer;
    case xiiGALDescriporTypeVulkan::UniformBufferDynamic:
      return vk::DescriptorType::eUniformBufferDynamic;
    case xiiGALDescriporTypeVulkan::StorageBuffer:
      return vk::DescriptorType::eStorageBuffer;
    case xiiGALDescriporTypeVulkan::StorageBufferReadOnly:
      return vk::DescriptorType::eStorageBuffer;
    case xiiGALDescriporTypeVulkan::StorageBufferDynamic:
      return vk::DescriptorType::eStorageBufferDynamic;
    case xiiGALDescriporTypeVulkan::StorageBufferDynamicReadOnly:
      return vk::DescriptorType::eStorageBufferDynamic;
    case xiiGALDescriporTypeVulkan::InputAttachment:
      return vk::DescriptorType::eInputAttachment;
    case xiiGALDescriporTypeVulkan::InputAttachmentGeneral:
      return vk::DescriptorType::eInputAttachment;
    case xiiGALDescriporTypeVulkan::AccelerationStructure:
      return vk::DescriptorType::eAccelerationStructureKHR;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return (vk::DescriptorType)VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

XII_ALWAYS_INLINE vk::IndexType xiiVulkanTypeConversions::GetIndexType(xiiGALValueType::Enum indexType)
{
  switch (indexType)
  {
    case xiiGALValueType::Undefined:
      return vk::IndexType::eNoneKHR; // Valid only in Ray Tracing.
    case xiiGALValueType::UInt16:
      return vk::IndexType::eUint16;
    case xiiGALValueType::UInt32:
      return vk::IndexType::eUint32;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::IndexType::eUint32;
}

XII_ALWAYS_INLINE vk::ResolveModeFlagBits xiiVulkanTypeConversions::GetDepthResolveMode(xiiEnum<xiiGALDepthResolveMode> mode)
{
  switch (mode)
  {
    case xiiGALDepthResolveMode::Min:
      return vk::ResolveModeFlagBits::eMin;
    case xiiGALDepthResolveMode::Max:
      return vk::ResolveModeFlagBits::eMax;
    case xiiGALDepthResolveMode::Average:
      return vk::ResolveModeFlagBits::eAverage;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::ResolveModeFlagBits::eNone;
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiVulkanTypeConversions::GetResourceStateFromBindFlags(xiiBitflags<xiiGALBindFlags> bindFlags)
{
  // UAV is an exclusive write state. A buffer may advertise both SRV and UAV bind flags, but it
  // cannot simultaneously be in ShaderResource and UnorderedAccess state. Prefer the writable
  // state for newly-created UAV-capable buffers; the state tracker will transition it on demand.
  if (bindFlags.IsSet(xiiGALBindFlags::UnorderedAccess))
  {
    return xiiGALResourceStateFlags::UnorderedAccess;
  }

  xiiBitflags<xiiGALResourceStateFlags> resourceStates = xiiGALResourceStateFlags::Unknown;

  for (auto v : bindFlags)
  {
    switch (v)
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
      case xiiGALBindFlags::RenderTarget:
        resourceStates |= xiiGALResourceStateFlags::RenderTarget;
        break;
      case xiiGALBindFlags::DepthStencil:
        resourceStates |= xiiGALResourceStateFlags::DepthRead | xiiGALResourceStateFlags::DepthWrite;
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
      case xiiGALBindFlags::StreamOutput:
      default:
        XII_REPORT_FAILURE("Unexpected bind flag.");
        break;
    }
  }
  return resourceStates;
}

XII_ALWAYS_INLINE xiiUInt32 xiiVulkanTypeConversions::RankDeviceType(vk::PhysicalDeviceType type)
{
  switch (type)
  {
    case vk::PhysicalDeviceType::eDiscreteGpu: return 5;
    case vk::PhysicalDeviceType::eIntegratedGpu: return 4;
    case vk::PhysicalDeviceType::eVirtualGpu: return 3;
    case vk::PhysicalDeviceType::eCpu: return 2;
    case vk::PhysicalDeviceType::eOther: return 1;
    default: return 0;
  }
}

XII_ALWAYS_INLINE vk::FragmentShadingRateCombinerOpKHR xiiVulkanTypeConversions::GetFragmentShadingRateCombinerOp(xiiBitflags<xiiGALShadingRateCombinerFlags> e)
{
  XII_ASSERT_DEV(xiiMath::IsPowerOf2(e.GetValue()), "Expected a single combiner flag.");

  switch (e.GetValue())
  {
    case xiiGALShadingRateCombinerFlags::PassThrough:
      return vk::FragmentShadingRateCombinerOpKHR::eKeep;
    case xiiGALShadingRateCombinerFlags::CombinerOverride:
      return vk::FragmentShadingRateCombinerOpKHR::eReplace;
    case xiiGALShadingRateCombinerFlags::CombinerMin:
      return vk::FragmentShadingRateCombinerOpKHR::eMin;
    case xiiGALShadingRateCombinerFlags::CombinerMax:
      return vk::FragmentShadingRateCombinerOpKHR::eMax;
    case xiiGALShadingRateCombinerFlags::CombinerSum:
    case xiiGALShadingRateCombinerFlags::CombinerMul:
      return vk::FragmentShadingRateCombinerOpKHR::eMul;
    default:
      XII_REPORT_FAILURE("Unknown shading rate combiner flag.");
      return vk::FragmentShadingRateCombinerOpKHR();
  }
}

XII_ALWAYS_INLINE vk::BuildAccelerationStructureFlagsKHR xiiVulkanTypeConversions::GetAccelerationStructureFlags(xiiBitflags<xiiGALRayTracingBuildASFlags> flags)
{
  vk::BuildAccelerationStructureFlagsKHR vkFlags = {};

  if (flags.IsSet(xiiGALRayTracingBuildASFlags::AllowUpdate))
    vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
  if (flags.IsSet(xiiGALRayTracingBuildASFlags::AllowCompaction))
    vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction;
  if (flags.IsSet(xiiGALRayTracingBuildASFlags::PreferFastTrace))
    vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
  if (flags.IsSet(xiiGALRayTracingBuildASFlags::PreferFastBuild))
    vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild;
  if (flags.IsSet(xiiGALRayTracingBuildASFlags::LowMemory))
    vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eLowMemory;

  return vkFlags;
}

XII_ALWAYS_INLINE vk::Format xiiVulkanTypeConversions::GetTriangleVertexFormat(const xiiGALBLASTriangleDescription& triangle)
{
  if (triangle.m_VertexValueType == xiiGALValueType::Float32)
  {
    return triangle.m_uiVertexComponentCount == 2U ? vk::Format::eR32G32Sfloat : vk::Format::eR32G32B32Sfloat;
  }
  if (triangle.m_VertexValueType == xiiGALValueType::Float16)
  {
    return triangle.m_uiVertexComponentCount == 2U ? vk::Format::eR16G16Sfloat : vk::Format::eR16G16B16Sfloat;
  }
  if (triangle.m_VertexValueType == xiiGALValueType::Int32)
  {
    return triangle.m_uiVertexComponentCount == 2U ? vk::Format::eR32G32Sint : vk::Format::eR32G32B32Sint;
  }
  return vk::Format::eUndefined;
}
