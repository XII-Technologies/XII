
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
