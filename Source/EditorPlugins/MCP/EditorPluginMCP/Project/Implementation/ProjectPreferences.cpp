/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <EditorPluginMCP/Project/ProjectPreferences.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMCPProjectPreferences, 1, xiiRTTIDefaultAllocator<xiiMCPProjectPreferences>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSettingsScopeAttribute(xiiSettingsScope::Project),
    new xiiCategoryAttribute("Integrations"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("AllowEditorChanges", m_bAllowEditorChanges),
    XII_MEMBER_PROPERTY("AllowAssetProcessing", m_bAllowAssetProcessing),
    XII_MEMBER_PROPERTY("AllowCppBuilds", m_bAllowCppBuilds),
    XII_MEMBER_PROPERTY("AllowViewportCapture", m_bAllowViewportCapture),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMCPProjectPreferences::xiiMCPProjectPreferences() :
  xiiEditorSettings(Domain::Project, "MCP")
{
}
