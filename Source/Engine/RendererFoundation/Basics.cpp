#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>

const xiiUInt8 xiiGALIndexType::s_Size[xiiGALIndexType::ENUM_COUNT] = {
  0,                // None
  sizeof(xiiInt16), // UShort
  sizeof(xiiInt32)  // UInt
};

const char* xiiGALShaderStage::Names[ENUM_COUNT] = {
  "VertexShader",
  "HullShader",
  "DomainShader",
  "GeometryShader",
  "PixelShader",
  "ComputeShader",
};

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALMSAASampleCount, 1)
XII_ENUM_CONSTANTS(
  xiiGALMSAASampleCount::None,
  xiiGALMSAASampleCount::TwoSamples,
  xiiGALMSAASampleCount::FourSamples,
  xiiGALMSAASampleCount::EightSamples)
XII_END_STATIC_REFLECTED_ENUM;

XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Basics);
