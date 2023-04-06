#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

class XII_RENDERERDILIGENT_DLL xiiDiligentUtils
{
public:
  static Diligent::RENDER_DEVICE_TYPE GetDiligentRenderDeviceType(const xiiGraphicsDeviceType::Enum type);

  static xiiEnum<xiiGALMSAASampleCount> ToGALMSAASampleCount(xiiUInt32 uiSampleCount);
  static xiiUInt32                      ToDiligentMSAACount(xiiEnum<xiiGALMSAASampleCount> sampleCount);

  static Diligent::BLEND_FACTOR    ToDiligentBlendFactor(xiiGALBlendFactor::Enum e);
  static Diligent::BLEND_OPERATION ToDiligentBlendOperation(xiiGALBlendOperation::Enum e);
  static Diligent::FILTER_TYPE     ToDiligentFilter(xiiGALTextureFilterMode::Enum e);
  static Diligent::STENCIL_OP      ToDiligentStencilOperation(xiiGALStencilOperation::Enum e);

  static Diligent::VALUE_TYPE  GALToDiligentFormat(Diligent::TEXTURE_FORMAT format);
  static Diligent::SHADER_TYPE GALToDiligentShaderStage(xiiGALShaderStage::Enum e);
  static Diligent::VALUE_TYPE  GALNumBitsToDiligentValueType(xiiUInt32 value);
  static bool                  GALIsFormatNormalized(Diligent::TEXTURE_FORMAT format);
  static xiiInt32              GALToDiligentNumComponent(Diligent::TEXTURE_FORMAT format);

  static Diligent::COLOR_MASK ToDiligentColorWriteMask(xiiGALColorWriteMask::Enum mask);

  static bool IsDepthFormat(Diligent::TEXTURE_FORMAT format);
};

#include <RendererDiligent/Utilities/Implementation/DiligentConversions_inl.h>
