
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

XII_ALWAYS_INLINE vk::Format xiiVulkanTypeConversions::GetFormat(xiiGALTextureFormat::Enum e)
{
  switch (e)
  {
    case xiiGALTextureFormat::Unknown:
      return vk::Format::eUndefined;

    case xiiGALTextureFormat::RGBA32Typeless:
    case xiiGALTextureFormat::RGBA32Float:
      return vk::Format::eR32G32B32A32Sfloat;
    case xiiGALTextureFormat::RGBA32UInt:
      return vk::Format::eR32G32B32A32Uint;
    case xiiGALTextureFormat::RGBA32SInt:
      return vk::Format::eR32G32B32A32Sint;

    case xiiGALTextureFormat::RGB32Typeless:
    case xiiGALTextureFormat::RGB32Float:
      return vk::Format::eR32G32B32Sfloat;
    case xiiGALTextureFormat::RGB32UInt:
      return vk::Format::eR32G32B32Uint;
    case xiiGALTextureFormat::RGB32SInt:
      return vk::Format::eR32G32B32Sint;

    case xiiGALTextureFormat::RGBA16Typeless:
    case xiiGALTextureFormat::RGBA16Float:
      return vk::Format::eR16G16B16A16Sfloat;
    case xiiGALTextureFormat::RGBA16UNormalized:
      return vk::Format::eR16G16B16A16Unorm;
    case xiiGALTextureFormat::RGBA16UInt:
      return vk::Format::eR16G16B16A16Uint;
    case xiiGALTextureFormat::RGBA16SNormalized:
      return vk::Format::eR16G16B16A16Snorm;
    case xiiGALTextureFormat::RGBA16SInt:
      return vk::Format::eR16G16B16A16Sint;

    case xiiGALTextureFormat::RG32Typeless:
    case xiiGALTextureFormat::RG32Float:
      return vk::Format::eR32G32Sfloat;
    case xiiGALTextureFormat::RG32UInt:
      return vk::Format::eR32G32Uint;
    case xiiGALTextureFormat::RG32SInt:
      return vk::Format::eR32G32Sint;

    case xiiGALTextureFormat::R32G8X24Typeless:
    case xiiGALTextureFormat::D32FloatS8X24UInt:
    case xiiGALTextureFormat::R32FloatX8X24Typeless:
      return vk::Format::eD32SfloatS8Uint;

    case xiiGALTextureFormat::X32TypelessG8X24UInt:
      return vk::Format::eUndefined;

    case xiiGALTextureFormat::RGB10A2Typeless:
    case xiiGALTextureFormat::RGB10A2UNormalized:
      return vk::Format::eA2R10G10B10UnormPack32;
    case xiiGALTextureFormat::RGB10A2UInt:
      return vk::Format::eA2R10G10B10UintPack32;
    case xiiGALTextureFormat::RG11B10Float:
      return vk::Format::eB10G11R11UfloatPack32;

    case xiiGALTextureFormat::RGBA8Typeless:
    case xiiGALTextureFormat::RGBA8UNormalized:
      return vk::Format::eR8G8B8A8Unorm;
    case xiiGALTextureFormat::RGBA8UNormalizedSRGB:
      return vk::Format::eR8G8B8A8Srgb;
    case xiiGALTextureFormat::RGBA8UInt:
      return vk::Format::eR8G8B8A8Uint;
    case xiiGALTextureFormat::RGBA8SNormalized:
      return vk::Format::eR8G8B8A8Snorm;
    case xiiGALTextureFormat::RGBA8SInt:
      return vk::Format::eR8G8B8A8Sint;

    case xiiGALTextureFormat::RG16Typeless:
    case xiiGALTextureFormat::RG16Float:
      return vk::Format::eR16G16Sfloat;
    case xiiGALTextureFormat::RG16UNormalized:
      return vk::Format::eR16G16Unorm;
    case xiiGALTextureFormat::RG16UInt:
      return vk::Format::eR16G16Uint;
    case xiiGALTextureFormat::RG16SNormalized:
      return vk::Format::eR16G16Snorm;
    case xiiGALTextureFormat::RG16SInt:
      return vk::Format::eR16G16Sint;

    case xiiGALTextureFormat::R32Typeless:
      return vk::Format::eR32Sfloat;
    case xiiGALTextureFormat::D32Float:
      return vk::Format::eD32Sfloat;
    case xiiGALTextureFormat::R32Float:
      return vk::Format::eR32Sfloat;
    case xiiGALTextureFormat::R32UInt:
      return vk::Format::eR32Uint;
    case xiiGALTextureFormat::R32SInt:
      return vk::Format::eR32Sint;

    case xiiGALTextureFormat::R24G8Typeless:
    case xiiGALTextureFormat::D24UNormalizedS8UInt:
    case xiiGALTextureFormat::R24UNormalizedX8Typeless:
      return vk::Format::eD24UnormS8Uint;

    case xiiGALTextureFormat::X24TypelessG8UInt:
      return vk::Format::eUndefined;

    case xiiGALTextureFormat::RG8Typeless:
    case xiiGALTextureFormat::RG8UNormalized:
      return vk::Format::eR8G8Unorm;
    case xiiGALTextureFormat::RG8UInt:
      return vk::Format::eR8G8Uint;
    case xiiGALTextureFormat::RG8SNormalized:
      return vk::Format::eR8G8Snorm;
    case xiiGALTextureFormat::RG8SInt:
      return vk::Format::eR8G8Sint;

    case xiiGALTextureFormat::R16Typeless:
    case xiiGALTextureFormat::R16Float:
      return vk::Format::eR16Sfloat;
    case xiiGALTextureFormat::D16UNormalized:
      return vk::Format::eD16Unorm;
    case xiiGALTextureFormat::R16UNormalized:
      return vk::Format::eR16Unorm;
    case xiiGALTextureFormat::R16UInt:
      return vk::Format::eR16Uint;
    case xiiGALTextureFormat::R16SNormalized:
      return vk::Format::eR16Snorm;
    case xiiGALTextureFormat::R16SInt:
      return vk::Format::eR16Sint;

    case xiiGALTextureFormat::R8Typeless:
    case xiiGALTextureFormat::R8UNormalized:
      return vk::Format::eR8Unorm;
    case xiiGALTextureFormat::R8UInt:
      return vk::Format::eR8Uint;
    case xiiGALTextureFormat::R8SNormalized:
      return vk::Format::eR8Snorm;
    case xiiGALTextureFormat::R8SInt:
      return vk::Format::eR8Sint;
    case xiiGALTextureFormat::A8UNormalized:
      // If we use vk::Format::eR8Unorm, to get the same behaviour as xiiGALTextureFormat::A8UNormalized, we have to
      // Swizzle the components appropriately using the image view create info structure.
      return vk::Format::eA8UnormKHR;

    case xiiGALTextureFormat::R1UNormalized:
      return vk::Format::eUndefined;

    case xiiGALTextureFormat::RGB9E5SharedExponent:
      return vk::Format::eE5B9G9R9UfloatPack32;

    case xiiGALTextureFormat::RG8BG8UNormalized:
    case xiiGALTextureFormat::GR8GB8UNormalized:
      return vk::Format::eUndefined;

    case xiiGALTextureFormat::BC1Typeless:
    case xiiGALTextureFormat::BC1UNormalized:
      return vk::Format::eBc1RgbUnormBlock;
    case xiiGALTextureFormat::BC1UNormalizedSRGB:
      return vk::Format::eBc1RgbSrgbBlock;

    case xiiGALTextureFormat::BC2Typeless:
    case xiiGALTextureFormat::BC2UNormalized:
      return vk::Format::eBc2UnormBlock;
    case xiiGALTextureFormat::BC2UNormalizedSRGB:
      return vk::Format::eBc2SrgbBlock;

    case xiiGALTextureFormat::BC3Typeless:
    case xiiGALTextureFormat::BC3UNormalized:
      return vk::Format::eBc3UnormBlock;
    case xiiGALTextureFormat::BC3UNormalizedSRGB:
      return vk::Format::eBc3SrgbBlock;

    case xiiGALTextureFormat::BC4Typeless:
    case xiiGALTextureFormat::BC4UNormalized:
      return vk::Format::eBc4UnormBlock;
    case xiiGALTextureFormat::BC4SNormalized:
      return vk::Format::eBc4SnormBlock;

    case xiiGALTextureFormat::BC5Typeless:
    case xiiGALTextureFormat::BC5UNormalized:
      return vk::Format::eBc5UnormBlock;
    case xiiGALTextureFormat::BC5SNormalized:
      return vk::Format::eBc5SnormBlock;

    case xiiGALTextureFormat::B5G6R5UNormalized:
      return vk::Format::eB5G6R5UnormPack16;
    case xiiGALTextureFormat::B5G5R5A1UNormalized:
      return vk::Format::eB5G5R5A1UnormPack16;
    case xiiGALTextureFormat::BGRA8UNormalized:
    case xiiGALTextureFormat::BGRX8UNormalized:
      return vk::Format::eB8G8R8A8Unorm;

    case xiiGALTextureFormat::R10G10B10XRBiasA2UNormalized:
      return vk::Format::eUndefined;

    case xiiGALTextureFormat::BGRA8Typeless:
      return vk::Format::eB8G8R8A8Unorm;
    case xiiGALTextureFormat::BGRA8UNormalizedSRGB:
      return vk::Format::eB8G8R8A8Srgb;
    case xiiGALTextureFormat::BGRX8Typeless:
      return vk::Format::eB8G8R8A8Unorm;
    case xiiGALTextureFormat::BGRX8UNormalizedSRGB:
      return vk::Format::eB8G8R8A8Srgb;

    case xiiGALTextureFormat::BC6HTypeless:
    case xiiGALTextureFormat::BC6HUF16:
      return vk::Format::eBc6HUfloatBlock;
    case xiiGALTextureFormat::BC6HSF16:
      return vk::Format::eBc6HSfloatBlock;

    case xiiGALTextureFormat::BC7Typeless:
    case xiiGALTextureFormat::BC7UNormalized:
      return vk::Format::eBc7UnormBlock;
    case xiiGALTextureFormat::BC7UNormalizedSRGB:
      return vk::Format::eBc7SrgbBlock;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return vk::Format::eUndefined;
}

XII_ALWAYS_INLINE xiiGALTextureFormat::Enum xiiVulkanTypeConversions::GetGALFormat(vk::Format e)
{
  return xiiGALTextureFormat::Enum();
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

XII_ALWAYS_INLINE xiiBitflags<xiiGALCommandQueueType> xiiVulkanTypeConversions::GetGALCommandQueueType(vk::QueueFlags e)
{
  xiiBitflags<xiiGALCommandQueueType> queueType;

  if (e & vk::QueueFlagBits::eSparseBinding)
    queueType |= xiiGALCommandQueueType::SparseBinding;

  if (e & vk::QueueFlagBits::eGraphics)
    return queueType | xiiGALCommandQueueType::Graphics;

  if (e & vk::QueueFlagBits::eCompute)
    return queueType | xiiGALCommandQueueType::Compute;

  if (e & vk::QueueFlagBits::eTransfer)
    return queueType | xiiGALCommandQueueType::Transfer;

  return xiiGALCommandQueueType::Unknown;
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

XII_ALWAYS_INLINE vk::BorderColor xiiVulkanTypeConversions::GetBorderColor(const xiiColor& c)
{
  vk::BorderColor vkBorderColor = vk::BorderColor::eFloatTransparentBlack;

  if (c.r == 0 && c.g == 0 && c.b == 0 && c.a == 0)
  {
    vkBorderColor = vk::BorderColor::eFloatTransparentBlack;
  }
  else if (c.r == 0 && c.g == 0 && c.b == 0 && c.a == 1)
  {
    vkBorderColor = vk::BorderColor::eFloatOpaqueBlack;
  }
  else if (c.r == 1 && c.g == 1 && c.b == 1 && c.a == 1)
  {
    vkBorderColor = vk::BorderColor::eFloatOpaqueWhite;
  }
  else
  {
    xiiLog::Error("Vulkan samplers only allow transparent black (0,0,0,0), opaque black (0,0,0,1) or opaque white (1,1,1,1) as border colors.");
  }

  return vkBorderColor;
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
  return vk::AttachmentLoadOp();
}

XII_ALWAYS_INLINE vk::AttachmentStoreOp xiiVulkanTypeConversions::GetAttachmentStoreOperation(xiiGALAttachmentStoreOperation::Enum e)
{
  return vk::AttachmentStoreOp();
}

XII_ALWAYS_INLINE vk::ImageLayout xiiVulkanTypeConversions::GetImageLayout(xiiBitflags<xiiGALResourceStateFlags> e, bool bIsInsideRenderPass, bool bFragDensityMapInsteadOfShadingRate)
{
  return vk::ImageLayout();
}

XII_ALWAYS_INLINE vk::PipelineStageFlags xiiVulkanTypeConversions::GetPipelineStageFlags(xiiBitflags<xiiGALPipelineStageFlags> e)
{
  return vk::PipelineStageFlags();
}

XII_ALWAYS_INLINE vk::AccessFlags xiiVulkanTypeConversions::GetAccessFlags(xiiBitflags<xiiGALAccessFlags> e)
{
  return vk::AccessFlags();
}

XII_ALWAYS_INLINE vk::AccessFlags xiiVulkanTypeConversions::GetAccessFlags(xiiBitflags<xiiGALResourceStateFlags> e)
{
  return vk::AccessFlags();
}
