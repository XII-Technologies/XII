#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

void OnLoadPlugin()
{
  // Kraut Tree
  {
    // Menu Bar
    {
      xiiActionMapManager::RegisterActionMap("KrautTreeAssetMenuBar").IgnoreResult();
      xiiStandardMenus::MapActions("KrautTreeAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
      xiiProjectActions::MapActions("KrautTreeAssetMenuBar");
      xiiDocumentActions::MapActions("KrautTreeAssetMenuBar", "Menu.File", false);
      xiiCommandHistoryActions::MapActions("KrautTreeAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      xiiActionMapManager::RegisterActionMap("KrautTreeAssetToolBar").IgnoreResult();
      xiiDocumentActions::MapActions("KrautTreeAssetToolBar", "", true);
      xiiCommandHistoryActions::MapActions("KrautTreeAssetToolBar", "");
      xiiAssetActions::MapActions("KrautTreeAssetToolBar", true);
    }
  }
}

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}
