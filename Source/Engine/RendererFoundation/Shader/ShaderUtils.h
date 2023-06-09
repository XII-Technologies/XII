#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

class xiiShaderUtils
{
public:
  XII_ALWAYS_INLINE static xiiUInt32 Float3ToRGB10(xiiVec3 value)
  {
    const xiiVec3 unsignedValue = value * 0.5f + xiiVec3(0.5f);

    const xiiUInt32 r = xiiMath::Clamp(static_cast<xiiUInt32>(unsignedValue.x * 1023.0f + 0.5f), 0u, 1023u);
    const xiiUInt32 g = xiiMath::Clamp(static_cast<xiiUInt32>(unsignedValue.y * 1023.0f + 0.5f), 0u, 1023u);
    const xiiUInt32 b = xiiMath::Clamp(static_cast<xiiUInt32>(unsignedValue.z * 1023.0f + 0.5f), 0u, 1023u);

    return r | (g << 10) | (b << 20);
  }

  XII_ALWAYS_INLINE static xiiUInt32 PackFloat16intoUint(xiiFloat16 x, xiiFloat16 y)
  {
    const xiiUInt32 r = x.GetRawData();
    const xiiUInt32 g = y.GetRawData();

    return r | (g << 16);
  }

  XII_ALWAYS_INLINE static xiiUInt32 Float2ToRG16F(xiiVec2 value)
  {
    const xiiUInt32 r = xiiFloat16(value.x).GetRawData();
    const xiiUInt32 g = xiiFloat16(value.y).GetRawData();

    return r | (g << 16);
  }

  XII_ALWAYS_INLINE static void Float4ToRGBA16F(xiiVec4 value, xiiUInt32& out_uiRG, xiiUInt32& out_uiBA)
  {
    out_uiRG = Float2ToRG16F(xiiVec2(value.x, value.y));
    out_uiBA = Float2ToRG16F(xiiVec2(value.z, value.w));
  }

  enum class xiiBuiltinShaderType
  {
    CopyImage,
    CopyImageArray,
    DownscaleImage,
    DownscaleImageArray,
  };

  struct xiiBuiltinShader
  {
    xiiGALShaderHandle            m_hActiveGALShader;
    xiiGALBlendStateHandle        m_hBlendState;
    xiiGALDepthStencilStateHandle m_hDepthStencilState;
    xiiGALRasterizerStateHandle   m_hRasterizerState;
  };

  XII_RENDERERFOUNDATION_DLL static xiiDelegate<void(xiiBuiltinShaderType type, xiiBuiltinShader& out_shader)> g_RequestBuiltinShaderCallback;

  XII_ALWAYS_INLINE static void RequestBuiltinShader(xiiBuiltinShaderType type, xiiBuiltinShader& out_shader)
  {
    g_RequestBuiltinShaderCallback(type, out_shader);
  }
};
XII_DEFINE_AS_POD_TYPE(xiiShaderUtils::xiiBuiltinShaderType);
