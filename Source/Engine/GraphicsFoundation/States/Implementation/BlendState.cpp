#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/States/BlendState.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBlendFactor, 1)
  XII_ENUM_CONSTANT(xiiGALBlendFactor::Undefined),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::Zero),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::One),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::SourceColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseSourceColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::SourceAlpha),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseSourceAlpha),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::DestinationAlpha),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseDestinationAlpha),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::DestinationColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseDestinationColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::SourceAlphaSaturate),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::BlendFactor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseBlendFactor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::SourceOneColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseSourceOneColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::SourceOneAlpha),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseSourceOneAlpha),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBlendOperation, 1)
  XII_ENUM_CONSTANT(xiiGALBlendOperation::Undefined),
  XII_ENUM_CONSTANT(xiiGALBlendOperation::Add),
  XII_ENUM_CONSTANT(xiiGALBlendOperation::Subtract),
  XII_ENUM_CONSTANT(xiiGALBlendOperation::ReverseSubtract),
  XII_ENUM_CONSTANT(xiiGALBlendOperation::Min),
  XII_ENUM_CONSTANT(xiiGALBlendOperation::Max),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALColorMask, 1)
  XII_ENUM_CONSTANT(xiiGALColorMask::None),
  XII_ENUM_CONSTANT(xiiGALColorMask::Red),
  XII_ENUM_CONSTANT(xiiGALColorMask::Green),
  XII_ENUM_CONSTANT(xiiGALColorMask::Blue),
  XII_ENUM_CONSTANT(xiiGALColorMask::Alpha),
  XII_ENUM_CONSTANT(xiiGALColorMask::RG),
  XII_ENUM_CONSTANT(xiiGALColorMask::RGB),
  XII_ENUM_CONSTANT(xiiGALColorMask::RGBA),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on
