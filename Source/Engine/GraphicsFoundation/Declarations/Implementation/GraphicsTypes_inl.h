
// static
XII_ALWAYS_INLINE xiiUInt32 xiiGALShaderStage::GetStageIndex(xiiBitflags<xiiGALShaderStage> stage)
{
  // \todo Assert that a single shader stage is set.
  XII_ASSERT_DEV(!stage.IsNoFlagSet() && !stage.IsAnyFlagSet(), "Expected a single shader stage.");

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
