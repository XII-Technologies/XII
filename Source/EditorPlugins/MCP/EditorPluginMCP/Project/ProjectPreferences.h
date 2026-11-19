/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginMCP/EditorPluginMCPDLL.h>

#include <EditorFramework/Settings/EditorSettings.h>

/// Per-user permissions for the current project and same-user native MCP transport.
class xiiMCPProjectPreferences final : public xiiEditorSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMCPProjectPreferences, xiiEditorSettings);

public:
  xiiMCPProjectPreferences();

  bool m_bAllowEditorChanges   = false;
  bool m_bAllowAssetProcessing = false;
};
