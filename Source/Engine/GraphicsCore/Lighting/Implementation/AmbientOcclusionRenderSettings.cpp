/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/AmbientOcclusionRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAmbientOcclusionRenderSettings, 1, xiiRTTIDefaultAllocator<xiiAmbientOcclusionRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "Enabled", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("UseRayTracing", m_bUseRayTracing)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "UseRayTracing", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("AllowScreenSpace", m_bAllowScreenSpace)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "AllowScreenSpace", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "Radius", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0.01, 10000)),
    XII_MEMBER_PROPERTY("RayOriginBias", m_fRayOriginBias)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "RayOriginBias", 0), new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("RayMinimumDistance", m_fRayMinimumDistance)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "RayTMin", 0), new xiiClampValueAttribute(0.00001, 100)),
    XII_MEMBER_PROPERTY("NormalBias", m_fNormalBias)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "NormalBias", 0), new xiiClampValueAttribute(0, 1)),
    XII_MEMBER_PROPERTY("PixelStride", m_fPixelStride)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "PixelStride", 0), new xiiClampValueAttribute(0.1, 128)),
    XII_MEMBER_PROPERTY("MinimumSampleDistance", m_fMinimumSampleDistance)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "MinSampleDistance", 0), new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("DirectionCount", m_uiDirectionCount)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "DirectionCount", 0), new xiiClampValueAttribute(1U, 64U)),
    XII_MEMBER_PROPERTY("StepCount", m_uiStepCount)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAmbientOcclusionPass", "StepCount", 0), new xiiClampValueAttribute(1U, 32U)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Ambient Occlusion"),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiAmbientOcclusionRenderSettings::Validate() const
{
  XII_SUCCEED_OR_RETURN(SUPER::Validate());
  if ((!m_bUseRayTracing && !m_bAllowScreenSpace) || m_fRayMinimumDistance >= m_fRadius || m_fMinimumSampleDistance >= m_fRadius)
    return XII_FAILURE;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_AmbientOcclusionRenderSettings);
