
// static
XII_ALWAYS_INLINE xiiUInt32 xiiGALTextureFormat::GetBitsPerElement(xiiGALTextureFormat::Enum format)
{
  return s_BitsPerElement[format];
}

// static
XII_ALWAYS_INLINE xiiUInt8 xiiGALTextureFormat::GetChannelCount(xiiGALTextureFormat::Enum format)
{
  return s_ChannelCount[format];
}

// static
XII_FORCE_INLINE bool xiiGALTextureFormat::IsDepthFormat(xiiGALTextureFormat::Enum format)
{
  return format == D16UNormalized || format == D24UNormalizedS8UInt || format == D32Float || format == D32FloatS8X24UInt;
}

// static
XII_FORCE_INLINE bool xiiGALTextureFormat::IsStencilFormat(Enum format)
{
  return format == D24UNormalizedS8UInt || format == D32FloatS8X24UInt;
}

// static
XII_FORCE_INLINE bool xiiGALTextureFormat::IsSrgb(xiiGALTextureFormat::Enum format)
{
  return format == RGBA8UNormalizedSRGB || format == BGRX8UNormalizedSRGB || format == BGRA8UNormalizedSRGB || format == BC1UNormalizedSRGB || format == BC2UNormalizedSRGB || format == BC3UNormalizedSRGB || format == BC7UNormalizedSRGB;
}
