
XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASNull::GetGeometryDescriptionIndex(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
  return xiiInvalidIndex;
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASNull::GetGeometryIndex(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
  return xiiInvalidIndex;
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASNull::GetActualGeometryCount() const
{
  return 0U;
}

XII_ALWAYS_INLINE xiiGALScratchBufferSizeDescription xiiGALBottomLevelASNull::GetScratchBufferSizeDescription() const
{
  return xiiGALScratchBufferSizeDescription{.m_uiBuild = 0U, .m_uiUpdate = 0U};
}
