#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginTypeScript/Actions/TypeScriptActions.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

void OnLoadPlugin()
{
  xiiTypeScriptActions::RegisterActions();

  // TypeScript
  {
    // Menu Bar
    {
      xiiActionMapManager::RegisterActionMap("TypeScriptAssetMenuBar").IgnoreResult();
      xiiStandardMenus::MapActions("TypeScriptAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
      xiiProjectActions::MapActions("TypeScriptAssetMenuBar");
      xiiDocumentActions::MapActions("TypeScriptAssetMenuBar", "Menu.File", false);
      xiiCommandHistoryActions::MapActions("TypeScriptAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      xiiActionMapManager::RegisterActionMap("TypeScriptAssetToolBar").IgnoreResult();
      xiiDocumentActions::MapActions("TypeScriptAssetToolBar", "", true);
      xiiCommandHistoryActions::MapActions("TypeScriptAssetToolBar", "");
      xiiAssetActions::MapActions("TypeScriptAssetToolBar", true);
      xiiTypeScriptActions::MapActions("TypeScriptAssetToolBar", "");
    }
  }
}

void OnUnloadPlugin()
{
  xiiTypeScriptActions::UnregisterActions();
}

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
