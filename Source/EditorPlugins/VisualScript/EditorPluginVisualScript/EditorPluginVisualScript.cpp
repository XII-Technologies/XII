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
      xiiStandardMenus::MapActions("VisualScriptAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
      xiiProjectActions::MapActions("VisualScriptAssetMenuBar");
      xiiDocumentActions::MapActions("VisualScriptAssetMenuBar", "Menu.File", false);
      xiiAssetActions::MapMenuActions("VisualScriptAssetMenuBar", "Menu.File");
      xiiCommandHistoryActions::MapActions("VisualScriptAssetMenuBar", "Menu.Edit");
      xiiEditActions::MapActions("VisualScriptAssetMenuBar", "Menu.Edit", false, false);
    }

    // Tool Bar
    {
      xiiActionMapManager::RegisterActionMap("VisualScriptAssetToolBar").IgnoreResult();
      xiiDocumentActions::MapActions("VisualScriptAssetToolBar", "", true);
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
