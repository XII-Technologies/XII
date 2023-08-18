
XII_ALWAYS_INLINE vk::BlendOp xiiVulkanTypeConversions::GetVkBlendOp(xiiEnum<xiiGALBlendOperation> e)
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

XII_ALWAYS_INLINE vk::BlendFactor xiiVulkanTypeConversions::GetVkBlendFactor(xiiEnum<xiiGALBlendFactor> e)
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

XII_ALWAYS_INLINE vk::LogicOp xiiVulkanTypeConversions::GetVkLogicOp(xiiEnum<xiiGALLogicOperation> e)
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

XII_ALWAYS_INLINE vk::CompareOp xiiVulkanTypeConversions::GetVkCompareOp(xiiEnum<xiiGALComparisonFunction> e)
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

XII_ALWAYS_INLINE vk::StencilOp xiiVulkanTypeConversions::GetVkStencilOp(xiiEnum<xiiGALStencilOperation> e)
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

XII_ALWAYS_INLINE vk::PolygonMode xiiVulkanTypeConversions::GetVkPolygonMode(xiiEnum<xiiGALFillMode> e)
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

XII_ALWAYS_INLINE vk::CullModeFlagBits xiiVulkanTypeConversions::GetVkCullMode(xiiEnum<xiiGALCullMode> e)
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
