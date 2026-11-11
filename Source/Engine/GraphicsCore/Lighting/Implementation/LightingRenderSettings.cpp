/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/LightingRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLightingRenderSettings, 1, xiiRTTIDefaultAllocator<xiiLightingRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TileSize", m_uiTileSize)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiLightClusterViewPass", "TileSize", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 1024U)),
    XII_MEMBER_PROPERTY("DepthSlices", m_uiDepthSlices)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiLightClusterViewPass", "DepthSlices", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 256U)),
    XII_MEMBER_PROPERTY("MaximumLightsPerCluster", m_uiMaximumLightsPerCluster)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiLightClusterViewPass", "MaxLightsPerCluster", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 4096U)),
    XII_MEMBER_PROPERTY("LogarithmicDepth", m_bLogarithmicDepth)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiLightClusterViewPass", "LogarithmicDepth", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("ReflectionProbes", m_bReflectionProbes)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiWorldReflectionProbesPass", "Enabled", xiiRenderSettingsChange::History)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Lighting/Clustered"),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_LightingRenderSettings);
