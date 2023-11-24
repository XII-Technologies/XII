
// static
XII_ALWAYS_INLINE xiiEnum<xiiGALTextureFormat> xiiMeshNormalPrecision::ToResourceFormatNormal(Enum value)
{
  return value == _10Bit ? xiiGALResourceFormat::RGB10A2UIntNormalized : (value == _16Bit ? xiiGALResourceFormat::RGBAUShortNormalized : xiiGALResourceFormat::XYZFloat);
}

// static
XII_ALWAYS_INLINE xiiEnum<xiiGALTextureFormat> xiiMeshNormalPrecision::ToResourceFormatTangent(Enum value)
{
  return value == _10Bit ? xiiGALResourceFormat::RGB10A2UIntNormalized : (value == _16Bit ? xiiGALResourceFormat::RGBAUShortNormalized : xiiGALResourceFormat::XYZWFloat);
}

//////////////////////////////////////////////////////////////////////////

// static
XII_ALWAYS_INLINE xiiEnum<xiiGALTextureFormat> xiiMeshTexCoordPrecision::ToResourceFormat(Enum value)
{
  return value == _16Bit ? xiiGALResourceFormat::UVHalf : xiiGALResourceFormat::UVFloat;
}

//////////////////////////////////////////////////////////////////////////

// static
XII_ALWAYS_INLINE xiiEnum<xiiGALTextureFormat> xiiMeshBoneWeigthPrecision::ToResourceFormat(Enum value)
{
  switch (value)
  {
    case _8Bit:
      return xiiGALResourceFormat::RGBAUByteNormalized;
    case _10Bit:
      return xiiGALResourceFormat::RGB10A2UIntNormalized;
    case _16Bit:
      return xiiGALResourceFormat::RGBAUShortNormalized;
    case _32Bit:
      return xiiGALResourceFormat::RGBAFloat;
      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiGALResourceFormat::RGBAUByteNormalized;
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
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeNormal(const xiiVec3& vNormal, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALTextureFormat> destFormat)
{
  // we store normals in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec3(vNormal * 0.5f + xiiVec3(0.5f), dest, destFormat);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeTangent(const xiiVec3& vTangent, float fTangentSign, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALTextureFormat> destFormat)
{
  // make sure biTangentSign is either -1 or 1
  fTangentSign = (fTangentSign < 0.0f) ? -1.0f : 1.0f;

  // we store tangents in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec4(vTangent.GetAsVec4(fTangentSign) * 0.5f + xiiVec4(0.5f), dest, destFormat);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeTexCoord(const xiiVec2& vTexCoord, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALTextureFormat> destFormat)
{
  return EncodeFromVec2(vTexCoord, dest, destFormat);
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::EncodeBoneWeights(const xiiVec4& vWeights, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALTextureFormat> destFormat)
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
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeNormal(xiiArrayPtr<const xiiUInt8> source, xiiEnum<xiiGALTextureFormat> sourceFormat, xiiVec3& ref_vDestNormal)
{
  xiiVec3 tempNormal;
  XII_SUCCEED_OR_RETURN(DecodeToVec3(source, sourceFormat, tempNormal));
  ref_vDestNormal = tempNormal * 2.0f - xiiVec3(1.0f);
  return XII_SUCCESS;
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeTangent(xiiArrayPtr<const xiiUInt8> source, xiiEnum<xiiGALTextureFormat> sourceFormat, xiiVec3& ref_vDestTangent, float& ref_fDestBiTangentSign)
{
  xiiVec4 tempTangent;
  XII_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, tempTangent));
  ref_vDestTangent       = tempTangent.GetAsVec3() * 2.0f - xiiVec3(1.0f);
  ref_fDestBiTangentSign = tempTangent.w * 2.0f - 1.0f;
  return XII_SUCCESS;
}

// static
XII_ALWAYS_INLINE xiiResult xiiMeshBufferUtils::DecodeTexCoord(xiiArrayPtr<const xiiUInt8> source, xiiEnum<xiiGALTextureFormat> sourceFormat, xiiVec2& ref_vDestTexCoord)
{
  return DecodeToVec2(source, sourceFormat, ref_vDestTexCoord);
}
