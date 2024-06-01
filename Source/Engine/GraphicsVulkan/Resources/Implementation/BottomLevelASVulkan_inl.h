
XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASVulkan::GetGeometryDescriptionIndex(xiiStringView sName) const
{
  return xiiInvalidIndex;
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASVulkan::GetGeometryIndex(xiiStringView sName) const
{
  return xiiInvalidIndex;
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASVulkan::GetActualGeometryCount() const
{
  return 0;
}

XII_ALWAYS_INLINE xiiGALScratchBufferSizeDescription xiiGALBottomLevelASVulkan::GetScratchBufferSizeDescription() const
{
  return xiiGALScratchBufferSizeDescription{.m_uiBuild = 0U, .m_uiUpdate = 0U};
}
