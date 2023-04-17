#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

void OnLoadPlugin()
{
  // RmlUi
  {
    // Menu Bar
    {
      xiiActionMapManager::RegisterActionMap("RmlUiAssetMenuBar").IgnoreResult();
      xiiStandardMenus::MapActions("RmlUiAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
      xiiProjectActions::MapActions("RmlUiAssetMenuBar");
      xiiDocumentActions::MapActions("RmlUiAssetMenuBar", "Menu.File", false);
      xiiAssetActions::MapMenuActions("RmlUiAssetMenuBar", "Menu.File");
      xiiCommandHistoryActions::MapActions("RmlUiAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      xiiActionMapManager::RegisterActionMap("RmlUiAssetToolBar").IgnoreResult();
      xiiDocumentActions::MapActions("RmlUiAssetToolBar", "", true);
      xiiCommandHistoryActions::MapActions("RmlUiAssetToolBar", "");
      xiiAssetActions::MapToolBarActions("RmlUiAssetToolBar", true);
    }
  }
}

void OnUnloadPlugin() {}

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
