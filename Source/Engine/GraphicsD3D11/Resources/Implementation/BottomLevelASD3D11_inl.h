
XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASD3D11::GetGeometryDescriptionIndex(xiiStringView sName) const
{
  return m_pBottomLevelAS->GetGeometryDescIndex(sName.GetStartPointer());
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASD3D11::GetGeometryIndex(xiiStringView sName) const
{
  return m_pBottomLevelAS->GetGeometryIndex(sName.GetStartPointer());
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALBottomLevelASD3D11::GetActualGeometryCount() const
{
  return m_pBottomLevelAS->GetActualGeometryCount();
}

XII_ALWAYS_INLINE xiiGALScratchBufferSizeDescription xiiGALBottomLevelASD3D11::GetScratchBufferSizeDescription() const
{
  const Diligent::ScratchBufferSizes sizes = m_pBottomLevelAS->GetScratchBufferSizes();

  return xiiGALScratchBufferSizeDescription{.m_uiBuild = sizes.Build, .m_uiUpdate = sizes.Update};
}

XII_ALWAYS_INLINE void xiiGALBottomLevelASD3D11::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
  Diligent::RESOURCE_STATE requestedStates = xiiDiligentTypeConversions::GetResourceState(stateFlags);

  if (!(m_pBottomLevelAS->GetState() & requestedStates))
  {
    m_pBottomLevelAS->SetState(requestedStates);
  }
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALBottomLevelASD3D11::GetState() const
{
  return xiiDiligentTypeConversions::GetResourceState(m_pBottomLevelAS->GetState());
}

XII_ALWAYS_INLINE Diligent::IBottomLevelAS* xiiGALBottomLevelASD3D11::GetBottomLevelAS() const
{
  return m_pBottomLevelAS;
}
