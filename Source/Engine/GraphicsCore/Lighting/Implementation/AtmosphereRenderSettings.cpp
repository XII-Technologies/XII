/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/AtmosphereRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtmosphereRenderSettings, 1, xiiRTTIDefaultAllocator<xiiAtmosphereRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAtmosphereConstantsPass", "Enabled", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("CacheLookups", m_bCacheLookups)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAtmosphereConstantsPass", "CacheLookups", 0)),
    XII_MEMBER_PROPERTY("TransmittanceWidth", m_uiTransmittanceWidth)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAtmosphereTransmittancePass", "Width", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 4096U)),
    XII_MEMBER_PROPERTY("TransmittanceHeight", m_uiTransmittanceHeight)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAtmosphereTransmittancePass", "Height", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 4096U)),
    XII_MEMBER_PROPERTY("MultiScatterWidth", m_uiMultiScatterWidth)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAtmosphereMultiScatterPass", "Width", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 1024U)),
    XII_MEMBER_PROPERTY("MultiScatterHeight", m_uiMultiScatterHeight)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiAtmosphereMultiScatterPass", "Height", xiiRenderSettingsChange::Resources | xiiRenderSettingsChange::History), new xiiClampValueAttribute(1U, 1024U)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Atmosphere"),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_AtmosphereRenderSettings);
