#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>

xiiGraphicsDeviceType::Enum xiiGraphicsDeviceType::Default = xiiGraphicsDeviceType::Undefined;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGraphicsDeviceType, 1)
XII_ENUM_CONSTANTS(
  xiiGraphicsDeviceType::Undefined,
  xiiGraphicsDeviceType::D3D11,
  xiiGraphicsDeviceType::D3D12,
  xiiGraphicsDeviceType::Vulkan)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALPrimitiveTopology, 1)
XII_ENUM_CONSTANTS(
  xiiGALPrimitiveTopology::Undefined,
  xiiGALPrimitiveTopology::Points,
  xiiGALPrimitiveTopology::Lines,
  xiiGALPrimitiveTopology::Triangles)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALIndexType, 1)
XII_ENUM_CONSTANTS(
  xiiGALIndexType::None,
  xiiGALIndexType::UShort,
  xiiGALIndexType::UInt)
XII_END_STATIC_REFLECTED_ENUM;

const xiiUInt8 xiiGALIndexType::s_Size[xiiGALIndexType::ENUM_COUNT] = {
  0u,               // None
  sizeof(xiiInt16), // UShort
  sizeof(xiiInt32)  // UInt
};

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALColorWriteMask, 1)
XII_ENUM_CONSTANTS(
  xiiGALColorWriteMask::None,
  xiiGALColorWriteMask::Red,
  xiiGALColorWriteMask::Green,
  xiiGALColorWriteMask::Blue,
  xiiGALColorWriteMask::Alpha,
  xiiGALColorWriteMask::RG,
  xiiGALColorWriteMask::RGB,
  xiiGALColorWriteMask::RGBA)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShaderStage, 1)
XII_ENUM_CONSTANTS(
  xiiGALShaderStage::None,
  xiiGALShaderStage::VertexShader,
  xiiGALShaderStage::PixelShader,
  xiiGALShaderStage::GeometryShader,
  xiiGALShaderStage::HullShader,
  xiiGALShaderStage::DomainShader,
  xiiGALShaderStage::ComputeShader,
  xiiGALShaderStage::Amplification,
  xiiGALShaderStage::Mesh,
  xiiGALShaderStage::RayGen,
  xiiGALShaderStage::RayMiss,
  xiiGALShaderStage::RayAnyHit,
  xiiGALShaderStage::RayClosestHit,
  xiiGALShaderStage::RayIntersection,
  xiiGALShaderStage::Callable)
XII_END_STATIC_REFLECTED_ENUM;

// clang-format off
const char* xiiGALShaderStage::Names[ENUM_COUNT] = {
  "None",
  "VertexShader",
  "PixelShader",
  "GeometryShader",
  "HullShader",
  "DomainShader",
  "ComputeShader",
  "Amplification",
  "Mesh",
  "RayGen",
  "RayMiss",
  "RayAnyHit",
  "RayClosestHit",
  "RayIntersection",
  "Callable"
};
// clang-format on

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALMSAASampleCount, 1)
XII_ENUM_CONSTANTS(
  xiiGALMSAASampleCount::None,
  xiiGALMSAASampleCount::TwoSamples,
  xiiGALMSAASampleCount::FourSamples,
  xiiGALMSAASampleCount::EightSamples)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALTextureType, 1)
XII_ENUM_CONSTANTS(
  xiiGALTextureType::Texture1D,
  xiiGALTextureType::Texture1DArray,
  xiiGALTextureType::Texture2D,
  xiiGALTextureType::Texture2DArray,
  xiiGALTextureType::TextureCube,
  xiiGALTextureType::TextureCubeArray,
  xiiGALTextureType::Texture3D,
  xiiGALTextureType::Texture2DProxy,
  xiiGALTextureType::Texture2DProxyArray)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBlendFactor, 1)
XII_ENUM_CONSTANTS(
  xiiGALBlendFactor::None,
  xiiGALBlendFactor::Zero,
  xiiGALBlendFactor::SrcColor,
  xiiGALBlendFactor::InvSrcColor,
  xiiGALBlendFactor::SrcAlpha,
  xiiGALBlendFactor::InvSrcAlpha,
  xiiGALBlendFactor::DestAlpha,
  xiiGALBlendFactor::InvDestAlpha,
  xiiGALBlendFactor::DestColor,
  xiiGALBlendFactor::InvDestColor,
  xiiGALBlendFactor::SrcAlphaSaturated,
  xiiGALBlendFactor::BlendFactor,
  xiiGALBlendFactor::InvBlendFactor,
  xiiGALBlendFactor::SrcOneColor,
  xiiGALBlendFactor::InvSrcOneColor,
  xiiGALBlendFactor::SrcOneAlpha,
  xiiGALBlendFactor::InvSrcOneAlpha)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBlendOperation, 1)
XII_ENUM_CONSTANTS(
  xiiGALBlendOperation::None,
  xiiGALBlendOperation::Add,
  xiiGALBlendOperation::Subtract,
  xiiGALBlendOperation::RevSubtract,
  xiiGALBlendOperation::Min,
  xiiGALBlendOperation::Max)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALStencilOperation, 1)
XII_ENUM_CONSTANTS(
  xiiGALStencilOperation::None,
  xiiGALStencilOperation::Keep,
  xiiGALStencilOperation::Zero,
  xiiGALStencilOperation::Replace,
  xiiGALStencilOperation::None,
  xiiGALStencilOperation::IncrementSaturated,
  xiiGALStencilOperation::DecrementSaturated,
  xiiGALStencilOperation::Invert,
  xiiGALStencilOperation::IncrementWrap,
  xiiGALStencilOperation::DecrementWrap)
XII_END_STATIC_REFLECTED_ENUM;


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Basics);
