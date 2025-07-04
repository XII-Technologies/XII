#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSourceFormat, 1)
  XII_ENUM_CONSTANT(xiiSourceFormat::Color4Channel8BitNormalized_sRGB),
  XII_ENUM_CONSTANT(xiiSourceFormat::Color4Channel8BitNormalized),
  XII_ENUM_CONSTANT(xiiSourceFormat::Color2Channel16BitFloat),
  XII_ENUM_CONSTANT(xiiSourceFormat::Color4Channel16BitFloat),
  XII_ENUM_CONSTANT(xiiSourceFormat::Color2Channel32BitFloat),
  XII_ENUM_CONSTANT(xiiSourceFormat::Color3Channel32BitFloat),
  XII_ENUM_CONSTANT(xiiSourceFormat::Color4Channel32BitFloat),
  XII_ENUM_CONSTANT(xiiSourceFormat::Color3Channel11_11_10BitFloat),
  XII_ENUM_CONSTANT(xiiSourceFormat::Depth16Bit),
  XII_ENUM_CONSTANT(xiiSourceFormat::Depth24BitStencil8Bit),
  XII_ENUM_CONSTANT(xiiSourceFormat::Depth32BitFloat),
  XII_ENUM_CONSTANT(xiiSourceFormat::BC1_RGB_DXT1),
  XII_ENUM_CONSTANT(xiiSourceFormat::BC2_RGBA_DXT3),
  XII_ENUM_CONSTANT(xiiSourceFormat::BC3_RGBA_DXT5),
  XII_ENUM_CONSTANT(xiiSourceFormat::BC4_R_Grey_DXT5A),
  XII_ENUM_CONSTANT(xiiSourceFormat::BC5_RG_Grey_DXT5A),
  XII_ENUM_CONSTANT(xiiSourceFormat::BC6H_RGB_Float),
  XII_ENUM_CONSTANT(xiiSourceFormat::BC7_RGBA),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
xiiGALResourceFormat::Enum xiiSourceFormat::GetGALResourceFormat(xiiSourceFormat::Enum format, bool bFlipColorChannels /*= false*/)
{
  switch (format)
  {
    case xiiSourceFormat::Color4Channel8BitNormalized_sRGB:
      return bFlipColorChannels ? xiiGALResourceFormat::BGRA8UNormalizedSRGB : xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    case xiiSourceFormat::Color4Channel8BitNormalized:
      return bFlipColorChannels ? xiiGALResourceFormat::BGRA8UNormalized : xiiGALResourceFormat::RGBA8UNormalized;
    case xiiSourceFormat::Color2Channel16BitFloat:
      return xiiGALResourceFormat::RG16Float;
    case xiiSourceFormat::Color4Channel16BitFloat:
      return xiiGALResourceFormat::RGBA16Float;
    case xiiSourceFormat::Color2Channel32BitFloat:
      return xiiGALResourceFormat::RG32Float;
    case xiiSourceFormat::Color3Channel32BitFloat:
      return xiiGALResourceFormat::RGB32Float;
    case xiiSourceFormat::Color4Channel32BitFloat:
      return xiiGALResourceFormat::RGBA32Float;
    case xiiSourceFormat::Color3Channel11_11_10BitFloat:
      return xiiGALResourceFormat::RG11B10Float;
    case xiiSourceFormat::Depth16Bit:
      return xiiGALResourceFormat::D16UNormalized;
    case xiiSourceFormat::Depth24BitStencil8Bit:
      return xiiGALResourceFormat::D24UNormalizedS8UInt;
    case xiiSourceFormat::Depth32BitFloat:
      return xiiGALResourceFormat::D32Float;
    case xiiSourceFormat::BC1_RGB_DXT1:
      return xiiGALResourceFormat::BC1UNormalized;
    case xiiSourceFormat::BC2_RGBA_DXT3:
      return xiiGALResourceFormat::BC2UNormalized;
    case xiiSourceFormat::BC3_RGBA_DXT5:
      return xiiGALResourceFormat::BC3UNormalized;
    case xiiSourceFormat::BC4_R_Grey_DXT5A:
      return xiiGALResourceFormat::BC4UNormalized;
    case xiiSourceFormat::BC5_RG_Grey_DXT5A:
      return xiiGALResourceFormat::BC5UNormalized;
    case xiiSourceFormat::BC6H_RGB_Float:
      return xiiGALResourceFormat::BC6HUF16;
    case xiiSourceFormat::BC7_RGBA:
      return xiiGALResourceFormat::BC7UNormalized;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALResourceFormat::Unknown;
}

void xiiRenderViewContext::SetShaderPermutationVariable(const char* szName, const xiiTempHashedString& sTempValue) const
{
  xiiTempHashedString sHashedName(szName);

  xiiHashedString sName, sValue;
  if (xiiGALShaderManager::IsPermutationValueAllowed(szName, sHashedName, sTempValue, sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void xiiRenderViewContext::SetShaderPermutationVariable(xiiStringView sName, const xiiTempHashedString& sTempValue) const
{
  xiiTempHashedString sHashedName(sName);

  xiiHashedString sName0, sValue;
  if (xiiGALShaderManager::IsPermutationValueAllowed(sName, sHashedName, sTempValue, sName0, sValue))
  {
    SetShaderPermutationVariableInternal(sName0, sValue);
  }
}

void xiiRenderViewContext::SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue) const
{
  if (xiiGALShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void xiiRenderViewContext::SetShaderPermutationVariableInternal(const xiiHashedString& sName, const xiiHashedString& sValue) const
{
  xiiHashedString* pOldValue = nullptr;
  m_PermutationVariables.TryGetValue(sName, pOldValue);

  if (pOldValue == nullptr || *pOldValue != sValue)
  {
    m_PermutationVariables.Insert(sName, sValue);
  }
}
