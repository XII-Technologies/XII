#include "MeshBufferUtils.h"

// static
XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormat> xiiMeshNormalPrecision::ToResourceFormatNormal(Enum value)
{
  switch (value)
  {
    case xiiMeshNormalPrecision::_8Bit:
      return xiiGALResourceFormat::RGBA8UNormalized;
    case xiiMeshNormalPrecision::_10Bit:
      return xiiGALResourceFormat::RGB10A2UNormalized;
    case xiiMeshNormalPrecision::_16Bit:
      return xiiGALResourceFormat::RGBA16UNormalized;
    case xiiMeshNormalPrecision::_32Bit:
      return xiiGALResourceFormat::RGB32Float;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALResourceFormat::Unknown;
}

// static
XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormat> xiiMeshNormalPrecision::ToResourceFormatTangent(Enum value)
{
  switch (value)
  {
    case xiiMeshNormalPrecision::_8Bit:
      return xiiGALResourceFormat::RGBA8UNormalized;
    case xiiMeshNormalPrecision::_10Bit:
      return xiiGALResourceFormat::RGB10A2UNormalized;
    case xiiMeshNormalPrecision::_16Bit:
      return xiiGALResourceFormat::RGBA16UNormalized;
    case xiiMeshNormalPrecision::_32Bit:
      return xiiGALResourceFormat::RGBA32Float;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALResourceFormat::Unknown;
}

//////////////////////////////////////////////////////////////////////////

// static
XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormat> xiiMeshTexCoordPrecision::ToResourceFormat(Enum value)
{
  switch (value)
  {
    case xiiMeshTexCoordPrecision::_16Bit:
      return xiiGALResourceFormat::RG16Float;
    case xiiMeshTexCoordPrecision::_32Bit:
      return xiiGALResourceFormat::RG32Float;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALResourceFormat::Unknown;
}

//////////////////////////////////////////////////////////////////////////

// static
XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormat> xiiMeshBoneWeigthPrecision::ToResourceFormat(Enum value)
{
  switch (value)
  {
    case _8Bit:
      return xiiGALResourceFormat::RGBA8UNormalized;
    case _10Bit:
      return xiiGALResourceFormat::RGB10A2UNormalized;
    case _16Bit:
      return xiiGALResourceFormat::RGBA16UNormalized;
    case _32Bit:
      return xiiGALResourceFormat::RGBA32Float;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiGALResourceFormat::RGBA8UNormalized;
}

//////////////////////////////////////////////////////////////////////////

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeNormal(const xiiVec3& vNormal, xiiArrayPtr<xiiUInt8> dest, xiiMeshNormalPrecision::Enum normalPrecision)
{
  return EncodeNormal(vNormal, dest, xiiMeshNormalPrecision::ToResourceFormatNormal(normalPrecision));
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeTangent(const xiiVec3& vTangent, float fTangentSign, xiiArrayPtr<xiiUInt8> dest, xiiMeshNormalPrecision::Enum tangentPrecision)
{
  return EncodeTangent(vTangent, fTangentSign, dest, xiiMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision));
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeTexCoord(const xiiVec2& vTexCoord, xiiArrayPtr<xiiUInt8> dest, xiiMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return EncodeTexCoord(vTexCoord, dest, xiiMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision));
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeBoneWeights(const xiiVec4& vWeights, xiiArrayPtr<xiiUInt8> dest, xiiMeshBoneWeigthPrecision::Enum precision)
{
  return EncodeBoneWeights(vWeights, dest, xiiMeshBoneWeigthPrecision::ToResourceFormat(precision));
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeColor(const xiiVec4& vColor, xiiArrayPtr<xiiUInt8> dest, xiiMeshVertexColorConversion::Enum conversion)
{
  xiiVec4 finalColor;
  if (conversion == xiiMeshVertexColorConversion::LinearToSrgb)
  {
    finalColor = xiiColor::LinearToGamma(vColor.GetAsVec3()).GetAsVec4(vColor.w);
  }
  else if (conversion == xiiMeshVertexColorConversion::SrgbToLinear)
  {
    finalColor = xiiColor::GammaToLinear(vColor.GetAsVec3()).GetAsVec4(vColor.w);
  }
  else
  {
    finalColor = vColor;
  }

  return EncodeFromVec4(finalColor, dest, xiiGALResourceFormat::RGBA8UNormalized);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeNormal(const xiiVec3& vNormal, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALResourceFormat> destFormat)
{
  // we store normals in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec3(vNormal * 0.5f + xiiVec3(0.5f), dest, destFormat);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeTangent(const xiiVec3& vTangent, float fTangentSign, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALResourceFormat> destFormat)
{
  // make sure biTangentSign is either -1 or 1
  fTangentSign = (fTangentSign < 0.0f) ? -1.0f : 1.0f;

  // we store tangents in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec4(vTangent.GetAsVec4(fTangentSign) * 0.5f + xiiVec4(0.5f), dest, destFormat);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeTexCoord(const xiiVec2& vTexCoord, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALResourceFormat> destFormat)
{
  return EncodeFromVec2(vTexCoord, dest, destFormat);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeBoneWeights(const xiiVec4& vWeights, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALResourceFormat> destFormat)
{
  return EncodeFromVec4(vWeights, dest, destFormat);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeNormal(xiiArrayPtr<const xiiUInt8> source, xiiVec3& ref_vDestNormal, xiiMeshNormalPrecision::Enum normalPrecision)
{
  return DecodeNormal(source, xiiMeshNormalPrecision::ToResourceFormatNormal(normalPrecision), ref_vDestNormal);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeTangent(xiiArrayPtr<const xiiUInt8> source, xiiVec3& ref_vDestTangent, float& ref_fDestBiTangentSign, xiiMeshNormalPrecision::Enum tangentPrecision)
{
  return DecodeTangent(source, xiiMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision), ref_vDestTangent, ref_fDestBiTangentSign);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeTexCoord(xiiArrayPtr<const xiiUInt8> source, xiiVec2& ref_vDestTexCoord, xiiMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return DecodeTexCoord(source, xiiMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision), ref_vDestTexCoord);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeNormal(xiiArrayPtr<const xiiUInt8> source, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiVec3& ref_vDestNormal)
{
  xiiVec3 tempNormal;
  XII_SUCCEED_OR_RETURN(DecodeToVec3(source, sourceFormat, tempNormal));
  ref_vDestNormal = tempNormal * 2.0f - xiiVec3(1.0f);
  return XII_SUCCESS;
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeTangent(xiiArrayPtr<const xiiUInt8> source, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiVec3& ref_vDestTangent, float& ref_fDestBiTangentSign)
{
  xiiVec4 tempTangent;
  XII_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, tempTangent));
  ref_vDestTangent       = tempTangent.GetAsVec3() * 2.0f - xiiVec3(1.0f);
  ref_fDestBiTangentSign = tempTangent.w * 2.0f - 1.0f;
  return XII_SUCCESS;
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeTexCoord(xiiArrayPtr<const xiiUInt8> source, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiVec2& ref_vDestTexCoord)
{
  return DecodeToVec2(source, sourceFormat, ref_vDestTexCoord);
}
