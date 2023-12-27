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
      xiiActionMapManager::RegisterActionMap("VisualScriptAssetMenuBar").IgnoreResult();
      xiiStandardMenus::MapActions("VisualScriptAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
      xiiProjectActions::MapActions("VisualScriptAssetMenuBar");
      xiiDocumentActions::MapMenuActions("VisualScriptAssetMenuBar");
      xiiAssetActions::MapMenuActions("VisualScriptAssetMenuBar");
      xiiCommandHistoryActions::MapActions("VisualScriptAssetMenuBar");
      xiiEditActions::MapActions("VisualScriptAssetMenuBar", false, false);
    }

    // Tool Bar
    {
      xiiActionMapManager::RegisterActionMap("VisualScriptAssetToolBar").IgnoreResult();
      xiiDocumentActions::MapToolbarActions("VisualScriptAssetToolBar");
      xiiCommandHistoryActions::MapActions("VisualScriptAssetToolBar", "");
      xiiAssetActions::MapToolBarActions("VisualScriptAssetToolBar", true);
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
