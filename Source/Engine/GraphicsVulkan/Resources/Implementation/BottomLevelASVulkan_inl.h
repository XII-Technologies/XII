
XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASVulkan::GetGeometryDescriptionIndex(xiiStringView sName) const
{
  return m_pBottomLevelAS->GetGeometryDescIndex(sName.GetStartPointer());
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASVulkan::GetGeometryIndex(xiiStringView sName) const
{
  return m_pBottomLevelAS->GetGeometryIndex(sName.GetStartPointer());
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASVulkan::GetActualGeometryCount() const
{
  return m_pBottomLevelAS->GetActualGeometryCount();
}

XII_ALWAYS_INLINE xiiGALScratchBufferSizeDescription xiiGALBottomLevelASVulkan::GetScratchBufferSizeDescription() const
{
  const Diligent::ScratchBufferSizes sizes = m_pBottomLevelAS->GetScratchBufferSizes();

  return xiiGALScratchBufferSizeDescription{.m_uiBuild = sizes.Build, .m_uiUpdate = sizes.Update};
}

XII_ALWAYS_INLINE void xiiGALBottomLevelASVulkan::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
  Diligent::RESOURCE_STATE requestedStates = xiiDiligentTypeConversions::GetResourceState(stateFlags);

  if (!(m_pBottomLevelAS->GetState() & requestedStates))
  {
    m_pBottomLevelAS->SetState(requestedStates);
  }
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALBottomLevelASVulkan::GetState() const
{
  return xiiDiligentTypeConversions::GetResourceState(m_pBottomLevelAS->GetState());
}

XII_ALWAYS_INLINE const Diligent::IBottomLevelAS* xiiGALBottomLevelASVulkan::GetBottomLevelAS() const
{
  return m_pBottomLevelAS;
}
