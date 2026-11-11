/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/GlobalIlluminationRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGlobalIlluminationRenderSettings, 1, xiiRTTIDefaultAllocator<xiiGlobalIlluminationRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Technique", xiiRenderGlobalIllumination, m_Technique)->AddAttributes(new xiiRenderSettingsBindingAttribute("", "", xiiRenderSettingsChange::Pipeline)),
    XII_MEMBER_PROPERTY("ScreenRayLength", m_fScreenRayLength)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenIrradianceTracePass", "RayLength", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 100000)),
    XII_MEMBER_PROPERTY("ScreenThickness", m_fScreenThickness)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenIrradianceTracePass", "Thickness", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0.001, 10000)),
    XII_MEMBER_PROPERTY("ScreenSurfaceBias", m_fScreenSurfaceBias)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenIrradianceTracePass", "SurfaceBias", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("ScreenSampleCount", m_uiScreenSampleCount)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenIrradianceTracePass", "SampleCount", xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 32U)),
    XII_MEMBER_PROPERTY("ScreenStepCount", m_uiScreenStepCount)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenIrradianceTracePass", "StepCount", xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 64U)),
    XII_MEMBER_PROPERTY("ScreenAnimateSamples", m_bScreenAnimateSamples)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiScreenIrradianceTracePass", "AnimateSamples", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("RayLength", m_fRayLength)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "RayLength", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0.001, 100000)),
    XII_MEMBER_PROPERTY("RaySurfaceBias", m_fRaySurfaceBias)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "SurfaceBias", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("RaySampleCount", m_uiRaySampleCount)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "SampleCount", xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 32U)),
    XII_MEMBER_PROPERTY("RayMaximumHits", m_uiRayMaximumHits)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "MaximumHits", xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 8U)),
    XII_MEMBER_PROPERTY("AnalyticLightSamples", m_uiAnalyticLightSamples)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "AnalyticLightSamples", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0U, 64U)),
    XII_MEMBER_PROPERTY("EmissiveLightSamples", m_uiEmissiveLightSamples)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "EmissiveLightSamples", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0U, 64U)),
    XII_MEMBER_PROPERTY("EnvironmentLightSamples", m_uiEnvironmentLightSamples)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "EnvironmentLightSamples", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0U, 64U)),
    XII_MEMBER_PROPERTY("PowerWeightedLights", m_bPowerWeightedLights)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "PowerWeightedLights", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("RayAnimateSamples", m_bRayAnimateSamples)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "AnimateSamples", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("PathReservoirs", m_bPathReservoirs)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "UsePathReservoirs", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Global Illumination"),
    new xiiRenderSettingsBindingAttribute("xiiScreenIrradianceTracePass", "", 0),
    new xiiRenderSettingsBindingAttribute("xiiRayTracedIrradiancePass", "", 0),
    new xiiRenderSettingsBindingAttribute("xiiDDGIProbeSchedulePass", "", 0),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiGlobalIlluminationRenderSettings::Validate() const
{
  XII_SUCCEED_OR_RETURN(SUPER::Validate());
  return m_Technique >= xiiRenderGlobalIllumination::PipelineDefault && m_Technique < xiiRenderGlobalIllumination::ENUM_COUNT ? XII_SUCCESS : XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_GlobalIlluminationRenderSettings);
