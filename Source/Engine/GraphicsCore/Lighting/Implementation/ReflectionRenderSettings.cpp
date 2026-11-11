/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/ReflectionRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectionRenderSettings, 1, xiiRTTIDefaultAllocator<xiiReflectionRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiRenderSettingsBindingAttribute("", "", xiiRenderSettingsChange::Pipeline)),
    XII_MEMBER_PROPERTY("MaximumDistance", m_fMaximumDistance)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsPass", "MaxDistance", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0.001, 100000)),
    XII_MEMBER_PROPERTY("SurfaceBias", m_fSurfaceBias)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsPass", "SurfaceBias", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 100)),
    XII_MEMBER_PROPERTY("MaximumSteps", m_uiMaximumSteps)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsPass", "MaxSteps", xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 4096U)),
    XII_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsPass", "Thickness", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0.00001, 100)),
    XII_MEMBER_PROPERTY("MaximumRoughness", m_fMaximumRoughness)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsPass", "MaxRoughness", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 1)),
    XII_MEMBER_PROPERTY("StrideDepthCutoff", m_fStrideDepthCutoff)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsPass", "StrideZCutoff", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 100)),
    XII_MEMBER_PROPERTY("TemporalEnabled", m_bTemporalEnabled)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsTemporalPass", "Enabled", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("HistoryWeight", m_fHistoryWeight)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsTemporalPass", "HistoryWeight", 0), new xiiClampValueAttribute(0, 0.9999)),
    XII_MEMBER_PROPERTY("ConfidenceDecay", m_fConfidenceDecay)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsTemporalPass", "ConfidenceDecay", 0), new xiiClampValueAttribute(0, 1)),
    XII_MEMBER_PROPERTY("DepthThreshold", m_fDepthThreshold)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsTemporalPass", "DepthThreshold", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 1)),
    XII_MEMBER_PROPERTY("NormalThreshold", m_fNormalThreshold)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenSpaceReflectionsTemporalPass", "NormalThreshold", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 1)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Reflections/Screen Space"),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_ReflectionRenderSettings);
