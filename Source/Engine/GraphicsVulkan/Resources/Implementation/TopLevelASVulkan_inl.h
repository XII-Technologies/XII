
XII_ALWAYS_INLINE xiiGALTopLevelASInstanceDescription xiiGALTopLevelASVulkan::GetInstanceDescription(xiiStringView sName) const
{
  /// \todo GraphicsVulkan: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALTopLevelASInstanceDescription();
}

XII_ALWAYS_INLINE xiiGALTopLevelASBuildDescription xiiGALTopLevelASVulkan::GetBuildDescription() const
{
  xiiGALTopLevelASBuildDescription description;

  return description;
}

XII_ALWAYS_INLINE xiiGALScratchBufferSizeDescription xiiGALTopLevelASVulkan::GetScratchBufferSizeDescription() const
{
  return xiiGALScratchBufferSizeDescription{.m_uiBuild = 0, .m_uiUpdate = 0};
}
