#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ToolsProjectEventHandler(const xiiToolsProjectEvent& e);

void OnLoadPlugin()
{
  // Asset
  {
    // Menu Bar
    {
      const char* szMenuBar = "ProcGenAssetMenuBar";

  xiiActionMapManager::RegisterActionMap(szMenuBar).IgnoreResult();
  xiiStandardMenus::MapActions(szMenuBar, xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
  xiiProjectActions::MapActions(szMenuBar);
  xiiDocumentActions::MapActions(szMenuBar, "Menu.File", false);
  xiiAssetActions::MapMenuActions(szMenuBar, "Menu.File");
  xiiCommandHistoryActions::MapActions(szMenuBar, "Menu.Edit");

  xiiEditActions::MapActions("ProcGenAssetMenuBar", "Menu.Edit", false, false);
}

// Tool Bar
{
  const char* szToolBar = "ProcGenAssetToolBar";
  xiiActionMapManager::RegisterActionMap(szToolBar).IgnoreResult();
  xiiDocumentActions::MapActions(szToolBar, "", true);
  xiiCommandHistoryActions::MapActions(szToolBar, "");
  xiiAssetActions::MapToolBarActions(szToolBar, true);
}
}

// Scene
{
  // Menu Bar
  {
    xiiProcGenActions::RegisterActions();
    xiiProcGenActions::MapMenuActions();
  }

  // Tool Bar
  {
  }
}
}

void OnUnloadPlugin()
{
  xiiProcGenActions::UnregisterActions();
}

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
