
XII_ALWAYS_INLINE xiiGALTopLevelASInstanceDescription xiiGALTopLevelASVulkan::GetInstanceDescription(xiiStringView sName) const
{
  /// \todo GraphicsVulkan: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALTopLevelASInstanceDescription();
}

XII_ALWAYS_INLINE xiiGALTopLevelASBuildDescription xiiGALTopLevelASVulkan::GetBuildDescription() const
{
  const Diligent::TLASBuildInfo buildInfo = m_pTopLevelAS->GetBuildInfo();

  xiiGALTopLevelASBuildDescription description;
  description.m_uiInstanceCount                    = buildInfo.InstanceCount;
  description.m_uiHitGroupStride                   = buildInfo.HitGroupStride;
  description.m_BindingMode                        = xiiDiligentTypeConversions::GetGALHitGroupBindingMode(buildInfo.BindingMode);
  description.m_uiFirstContributionToHitGroupIndex = buildInfo.FirstContributionToHitGroupIndex;
  description.m_uiLastContributionToHitGroupIndex  = buildInfo.LastContributionToHitGroupIndex;

  return description;
}

XII_ALWAYS_INLINE xiiGALScratchBufferSizeDescription xiiGALTopLevelASVulkan::GetScratchBufferSizeDescription() const
{
  const Diligent::ScratchBufferSizes sizes = m_pTopLevelAS->GetScratchBufferSizes();

  return xiiGALScratchBufferSizeDescription{.m_uiBuild = sizes.Build, .m_uiUpdate = sizes.Update};
}

XII_ALWAYS_INLINE void xiiGALTopLevelASVulkan::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
  Diligent::RESOURCE_STATE requestedStates = xiiDiligentTypeConversions::GetResourceState(stateFlags);

  if (!(m_pTopLevelAS->GetState() & requestedStates))
  {
    m_pTopLevelAS->SetState(requestedStates);
  }
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALTopLevelASVulkan::GetState() const
{
  return xiiDiligentTypeConversions::GetResourceState(m_pTopLevelAS->GetState());
}

XII_ALWAYS_INLINE Diligent::ITopLevelAS* xiiGALTopLevelASVulkan::GetTopLevelAS() const
{
  return m_pTopLevelAS;
}
