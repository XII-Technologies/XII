/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Debug/DebugRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDebugRenderSettings, 1, xiiRTTIDefaultAllocator<xiiDebugRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Overlays", m_bOverlays)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiDebugOverlayPass", "Enabled", 0)),
    XII_MEMBER_PROPERTY("FrameStatistics", m_bFrameStatistics),
    XII_ENUM_MEMBER_PROPERTY("Visualization", xiiViewRenderMode, m_Visualization),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Debugging"),
    new xiiViewRenderSettingsAttribute(),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Debug_Implementation_DebugRenderSettings);
