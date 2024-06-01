
XII_ALWAYS_INLINE xiiGALTopLevelASInstanceDescription xiiGALTopLevelASD3D12::GetInstanceDescription(xiiStringView sName) const
{
  /// \todo GraphicsD3D12: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALTopLevelASInstanceDescription();
}

XII_ALWAYS_INLINE xiiGALTopLevelASBuildDescription xiiGALTopLevelASD3D12::GetBuildDescription() const
{
  return {};
}

XII_ALWAYS_INLINE xiiGALScratchBufferSizeDescription xiiGALTopLevelASD3D12::GetScratchBufferSizeDescription() const
{
  return xiiGALScratchBufferSizeDescription{.m_uiBuild = 0, .m_uiUpdate = 0};
}
