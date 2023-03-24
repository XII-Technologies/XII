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

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALMSAASampleCount, 1)
XII_ENUM_CONSTANTS(
  xiiGALMSAASampleCount::None,
  xiiGALMSAASampleCount::TwoSamples,
  xiiGALMSAASampleCount::FourSamples,
  xiiGALMSAASampleCount::EightSamples)
XII_END_STATIC_REFLECTED_ENUM;


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Basics);
