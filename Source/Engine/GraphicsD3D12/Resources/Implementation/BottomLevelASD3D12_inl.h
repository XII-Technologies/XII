
XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASD3D12::GetGeometryDescriptionIndex(xiiStringView sName) const
{
  return 0;
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASD3D12::GetGeometryIndex(xiiStringView sName) const
{
  return 0;
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASD3D12::GetActualGeometryCount() const
{
  return 0;
}

XII_ALWAYS_INLINE xiiGALScratchBufferSizeDescription xiiGALBottomLevelASD3D12::GetScratchBufferSizeDescription() const
{

  return xiiGALScratchBufferSizeDescription{.m_uiBuild = 0, .m_uiUpdate = 0};
}

XII_ALWAYS_INLINE void xiiGALBottomLevelASD3D12::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALBottomLevelASD3D12::GetState() const
{
  return {};
}
