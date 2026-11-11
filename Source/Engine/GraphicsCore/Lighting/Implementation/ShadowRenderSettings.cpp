/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/ShadowRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShadowRenderSettings, 1, xiiRTTIDefaultAllocator<xiiShadowRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiRenderSettingsBindingAttribute("", "", xiiRenderSettingsChange::Pipeline)),
    XII_MEMBER_PROPERTY("Cascades", m_Cascades),
    XII_MEMBER_PROPERTY("SpotResolution", m_uiSpotResolution)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiSpotShadowPass", "Resolution", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 16384U)),
    XII_MEMBER_PROPERTY("MaximumSpotLights", m_uiMaximumSpotLights)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiSpotShadowPass", "MaximumLights", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 1024U)),
    XII_MEMBER_PROPERTY("SpotNearPlane", m_fSpotNearPlane)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiSpotShadowPass", "NearPlane", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0.001, 1000)),
    XII_MEMBER_PROPERTY("DepthBias", m_fDepthBias)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiSpotShadowPass", "DepthBias", xiiRenderSettingsChange::History), new xiiRenderSettingsBindingAttribute("xiiDirectionalShadowVisibilityPass", "DepthBias", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 1)),
    XII_MEMBER_PROPERTY("NormalBias", m_fNormalBias)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiSpotShadowPass", "NormalBias", xiiRenderSettingsChange::History), new xiiRenderSettingsBindingAttribute("xiiDirectionalShadowVisibilityPass", "NormalBias", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 1000)),
    XII_MEMBER_PROPERTY("NormalBiasTexels", m_fNormalBiasTexels)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiSpotShadowPass", "NormalBiasTexels", xiiRenderSettingsChange::History), new xiiRenderSettingsBindingAttribute("xiiDirectionalShadowVisibilityPass", "NormalBiasTexels", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 1000)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Shadows"),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_ShadowRenderSettings);
