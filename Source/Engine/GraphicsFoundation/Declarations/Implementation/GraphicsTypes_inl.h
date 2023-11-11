
// static
XII_ALWAYS_INLINE xiiUInt32 xiiGALValueType::GetSize(const xiiGALValueType::Enum type)
{
  switch (type)
  {
    case xiiGALValueType::Int8:
      return 1U;
    case xiiGALValueType::Int16:
      return 2U;
    case xiiGALValueType::Int32:
      return 4U;
    case xiiGALValueType::UInt8:
      return 1U;
    case xiiGALValueType::UInt16:
      return 2U;
    case xiiGALValueType::UInt32:
      return 4U;
    case xiiGALValueType::Float16:
      return 2U;
    case xiiGALValueType::Float32:
      return 4U;
    case xiiGALValueType::Float64:
      return 8U;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0U;
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiGALShaderStage::GetStageIndex(xiiBitflags<xiiGALShaderStage> stage)
{
  // \todo Assert that a single shader stage is set.

  switch (stage.GetValue())
  {
    case xiiGALShaderStage::Vertex:
      return 0U;
    case xiiGALShaderStage::Pixel:
      return 1U;
    case xiiGALShaderStage::Geometry:
      return 2U;
    case xiiGALShaderStage::Hull:
      return 3U;
    case xiiGALShaderStage::Domain:
      return 4U;
    case xiiGALShaderStage::Compute:
      return 5U;
    case xiiGALShaderStage::Amplification:
      return 6U;
    case xiiGALShaderStage::Mesh:
      return 7U;
    case xiiGALShaderStage::RayGeneration:
      return 8U;
    case xiiGALShaderStage::RayMiss:
      return 9U;
    case xiiGALShaderStage::RayClosestHit:
      return 10U;
    case xiiGALShaderStage::RayAnyHit:
      return 11U;
    case xiiGALShaderStage::RayIntersection:
      return 12U;
    case xiiGALShaderStage::Callable:
      return 13U;
    case xiiGALShaderStage::Tile:
      return 14U;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiInvalidIndex;
}

// static
XII_ALWAYS_INLINE xiiGALShaderStage::Enum xiiGALShaderStage::GetStageFlag(xiiUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0U:
      return xiiGALShaderStage::Vertex;
    case 1U:
      return xiiGALShaderStage::Pixel;
    case 2U:
      return xiiGALShaderStage::Geometry;
    case 3U:
      return xiiGALShaderStage::Hull;
    case 4U:
      return xiiGALShaderStage::Domain;
    case 5U:
      return xiiGALShaderStage::Compute;
    case 6U:
      return xiiGALShaderStage::Amplification;
    case 7U:
      return xiiGALShaderStage::Mesh;
    case 8U:
      return xiiGALShaderStage::RayGeneration;
    case 9U:
      return xiiGALShaderStage::RayMiss;
    case 10U:
      return xiiGALShaderStage::RayClosestHit;
    case 11U:
      return xiiGALShaderStage::RayAnyHit;
    case 12U:
      return xiiGALShaderStage::RayIntersection;
    case 13U:
      return xiiGALShaderStage::Callable;
    case 14U:
      return xiiGALShaderStage::Tile;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALShaderStage::Unknown;
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiGALTextureFormat::GetBitsPerElement(xiiEnum<xiiGALTextureFormat> format)
{
  return s_BitsPerElement[format];
}

// static
XII_ALWAYS_INLINE xiiUInt8 xiiGALTextureFormat::GetChannelCount(xiiEnum<xiiGALTextureFormat> format)
{
  return s_ChannelCount[format];
}

// static
XII_FORCE_INLINE bool xiiGALTextureFormat::IsDepthFormat(xiiEnum<xiiGALTextureFormat> format)
{
  return format == D16UNormalized || format == D24UNormalizedS8UInt || format == D32Float || format == D32FloatS8X24UInt;
}

// static
XII_FORCE_INLINE bool xiiGALTextureFormat::IsStencilFormat(xiiEnum<xiiGALTextureFormat> format)
{
  return format == D24UNormalizedS8UInt || format == D32FloatS8X24UInt;
}

// static
XII_FORCE_INLINE bool xiiGALTextureFormat::IsSrgb(xiiEnum<xiiGALTextureFormat> format)
{
  return format == RGBA8UNormalizedSRGB || format == BGRX8UNormalizedSRGB || format == BGRA8UNormalizedSRGB || format == BC1UNormalizedSRGB || format == BC2UNormalizedSRGB || format == BC3UNormalizedSRGB || format == BC7UNormalizedSRGB;
}

// static
XII_FORCE_INLINE bool xiiGALFilterType::IsComparisonFilter(xiiEnum<xiiGALFilterType> e)
{
  return e == ComparisonPoint || e == ComparisonLinear || e == ComparisonAnisotropic;
}

// static
XII_FORCE_INLINE bool xiiGALFilterType::IsAnisotropicFilter(xiiEnum<xiiGALFilterType> e)
{
  return e == Anisotropic || e == ComparisonAnisotropic || e == MinimumAnisotropic || e == MaximumAnisotropic;
}
