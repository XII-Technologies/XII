/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/AntiAliasingRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAntiAliasingRenderSettings, 1, xiiRTTIDefaultAllocator<xiiAntiAliasingRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiTemporalAntiAliasingPass", "Enabled", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History), new xiiRenderSettingsBindingAttribute("xiiTemporalCameraPass", "Enabled", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("DepthThreshold", m_fDepthThreshold)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiTemporalAntiAliasingPass", "DepthThreshold", xiiRenderSettingsChange::History), new xiiClampValueAttribute(0, 1)),
    XII_MEMBER_PROPERTY("BaseBlendAlpha", m_fBaseBlendAlpha)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiTemporalAntiAliasingPass", "BaseBlendAlpha", 0), new xiiClampValueAttribute(0, 1)),
    XII_MEMBER_PROPERTY("MotionBlendScale", m_fMotionBlendScale)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiTemporalAntiAliasingPass", "MotionBlendScale", 0), new xiiClampValueAttribute(0, 100)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Anti-Aliasing/Temporal"),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_AntiAliasingRenderSettings);
