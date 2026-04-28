/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

void OnLoadPlugin()
{
  // VisualScript
  {
    // Menu Bar
    {
      xiiActionMapManager::RegisterActionMap("VisualScriptAssetMenuBar", "AssetMenuBar");
      xiiEditActions::MapActions("VisualScriptAssetMenuBar", false, false);
    }

    // Tool Bar
    {
      xiiActionMapManager::RegisterActionMap("VisualScriptAssetToolBar", "AssetToolbar");
    }
  }
}

void OnUnloadPlugin()
{
}

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
