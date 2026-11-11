/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/DDGIRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDDGIRenderSettings, 1, xiiRTTIDefaultAllocator<xiiDDGIRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Probes", m_Probes),
    XII_MEMBER_PROPERTY("DirectionalAtlas", m_bDirectionalAtlas)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiDDGIProbeSchedulePass", "DirectionalAtlas", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("ForwardOffset", m_fForwardOffset)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiDDGIProbeSchedulePass", "ForwardOffset", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 100000)),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiDDGIProbeGatherPass", "Intensity", 0), new xiiClampValueAttribute(0, 10000)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Global Illumination/DDGI"),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiDDGIRenderSettings::Validate() const
{
  XII_SUCCEED_OR_RETURN(SUPER::Validate());
  const auto& p = m_Probes;
  if (!p.m_uiProbeCountX || !p.m_uiProbeCountY || !p.m_uiProbeCountZ || p.m_uiProbeCountX > 256 || p.m_uiProbeCountY > 256 || p.m_uiProbeCountZ > 256 || xiiUInt64(p.m_uiProbeCountX) * p.m_uiProbeCountY * p.m_uiProbeCountZ > 1048576 || !p.m_uiProbeUpdateBudget || p.m_uiProbeUpdateBudget > 1048576 || !xiiMath::IsFinite(p.m_fProbeSpacing) || p.m_fProbeSpacing <= 0 || !xiiMath::IsFinite(p.m_fProbeSpacing * p.m_fProbeSpacing * 16) || !xiiMath::IsFinite(p.m_fTemporalHysteresis) || p.m_fTemporalHysteresis < 0 || p.m_fTemporalHysteresis >= 1 || !xiiMath::IsFinite(p.m_fMaximumRelocationDistance) || p.m_fMaximumRelocationDistance < 0)
    return XII_FAILURE;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_DDGIRenderSettings);
