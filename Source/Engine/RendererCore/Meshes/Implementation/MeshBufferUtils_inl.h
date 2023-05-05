
// static
XII_ALWAYS_INLINE xiiGALResourceFormat::Enum xiiMeshNormalPrecision::ToResourceFormatNormal(Enum value)
{
  return value == _10Bit ? xiiGALResourceFormat::RGB10A2UIntNormalized : (value == _16Bit ? xiiGALResourceFormat::RGBAUShortNormalized : xiiGALResourceFormat::XYZFloat);
}

// static
XII_ALWAYS_INLINE xiiGALResourceFormat::Enum xiiMeshNormalPrecision::ToResourceFormatTangent(Enum value)
{
  return value == _10Bit ? xiiGALResourceFormat::RGB10A2UIntNormalized : (value == _16Bit ? xiiGALResourceFormat::RGBAUShortNormalized : xiiGALResourceFormat::XYZWFloat);
}

//////////////////////////////////////////////////////////////////////////

// static
XII_ALWAYS_INLINE xiiGALResourceFormat::Enum xiiMeshTexCoordPrecision::ToResourceFormat(Enum value)
{
  return value == _16Bit ? xiiGALResourceFormat::UVHalf : xiiGALResourceFormat::UVFloat;
}

//////////////////////////////////////////////////////////////////////////

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeNormal(const xiiVec3& normal, xiiArrayPtr<xiiUInt8> dest, xiiMeshNormalPrecision::Enum normalPrecision)
{
  return EncodeNormal(normal, dest, xiiMeshNormalPrecision::ToResourceFormatNormal(normalPrecision));
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeTangent(const xiiVec3& tangent, float biTangentSign, xiiArrayPtr<xiiUInt8> dest, xiiMeshNormalPrecision::Enum tangentPrecision)
{
  return EncodeTangent(tangent, biTangentSign, dest, xiiMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision));
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeTexCoord(const xiiVec2& texCoord, xiiArrayPtr<xiiUInt8> dest, xiiMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return EncodeTexCoord(texCoord, dest, xiiMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision));
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeNormal(const xiiVec3& normal, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat)
{
  // we store normals in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec3(normal * 0.5f + xiiVec3(0.5f), dest, destFormat);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeTangent(const xiiVec3& tangent, float biTangentSign, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat)
{
  // make sure biTangentSign is either -1 or 1
  biTangentSign = (biTangentSign < 0.0f) ? -1.0f : 1.0f;

  // we store tangents in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec4(tangent.GetAsVec4(biTangentSign) * 0.5f + xiiVec4(0.5f), dest, destFormat);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeTexCoord(const xiiVec2& texCoord, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat)
{
  return EncodeFromVec2(texCoord, dest, destFormat);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeNormal(xiiArrayPtr<const xiiUInt8> source, xiiVec3& destNormal, xiiMeshNormalPrecision::Enum normalPrecision)
{
  return DecodeNormal(source, xiiMeshNormalPrecision::ToResourceFormatNormal(normalPrecision), destNormal);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeTangent(xiiArrayPtr<const xiiUInt8> source, xiiVec3& destTangent, float& destBiTangentSign, xiiMeshNormalPrecision::Enum tangentPrecision)
{
  return DecodeTangent(source, xiiMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision), destTangent, destBiTangentSign);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeTexCoord(xiiArrayPtr<const xiiUInt8> source, xiiVec2& destTexCoord, xiiMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return DecodeTexCoord(source, xiiMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision), destTexCoord);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeNormal(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec3& destNormal)
{
  xiiVec3 tempNormal;
  XII_SUCCEED_OR_RETURN(DecodeToVec3(source, sourceFormat, tempNormal));
  destNormal = tempNormal * 2.0f - xiiVec3(1.0f);
  return XII_SUCCESS;
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeTangent(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec3& destTangent, float& destBiTangentSign)
{
  xiiVec4 tempTangent;
  XII_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, tempTangent));
  destTangent       = tempTangent.GetAsVec3() * 2.0f - xiiVec3(1.0f);
  destBiTangentSign = tempTangent.w * 2.0f - 1.0f;
  return XII_SUCCESS;
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeTexCoord(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec2& destTexCoord)
{
  return DecodeToVec2(source, sourceFormat, destTexCoord);
}
