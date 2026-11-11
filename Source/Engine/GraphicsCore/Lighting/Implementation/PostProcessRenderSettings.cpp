/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/PostProcessRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPostProcessRenderSettings, 1, xiiRTTIDefaultAllocator<xiiPostProcessRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Display", m_Display),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Post-Processing and Output"),
    new xiiRenderSettingsBindingAttribute("xiiToneMappingPass", "", 0),
    new xiiRenderSettingsBindingAttribute("xiiPresentPass", "", 0),
    new xiiRenderSettingsBindingAttribute("xiiBloomPass", "", 0),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiPostProcessRenderSettings::Validate() const
{
  XII_SUCCEED_OR_RETURN(SUPER::Validate());
  if (!xiiDisplayOutputManager::IsValid(m_Display))
    return XII_FAILURE;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_PostProcessRenderSettings);
