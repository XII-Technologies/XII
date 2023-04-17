#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginJolt/Actions/JoltActions.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>
#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>

void UpdateCollisionLayerDynamicEnumValues();

static void ToolsProjectEventHandler(const xiiToolsProjectEvent& e);

void OnLoadPlugin()
{
  xiiToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Collision Mesh
  {
    xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);

    // Menu Bar
    {
      xiiActionMapManager::RegisterActionMap("JoltCollisionMeshAssetMenuBar").IgnoreResult();
      xiiStandardMenus::MapActions("JoltCollisionMeshAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
      xiiProjectActions::MapActions("JoltCollisionMeshAssetMenuBar");
      xiiDocumentActions::MapActions("JoltCollisionMeshAssetMenuBar", "Menu.File", false);
      xiiAssetActions::MapMenuActions("JoltCollisionMeshAssetMenuBar", "Menu.File");
      xiiCommandHistoryActions::MapActions("JoltCollisionMeshAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      xiiActionMapManager::RegisterActionMap("JoltCollisionMeshAssetToolBar").IgnoreResult();
      xiiDocumentActions::MapActions("JoltCollisionMeshAssetToolBar", "", true);
      xiiCommandHistoryActions::MapActions("JoltCollisionMeshAssetToolBar", "");
      xiiAssetActions::MapToolBarActions("JoltCollisionMeshAssetToolBar", true);
      xiiCommonAssetActions::MapActions("JoltCollisionMeshAssetToolBar", "", xiiCommonAssetUiState::Grid);
    }
  }

  // Scene
  {
    // Menu Bar
    {
      xiiJoltActions::RegisterActions();
      xiiJoltActions::MapMenuActions();
    }

    // Tool Bar
    {
    }
  }
}

void OnUnloadPlugin()
{
  xiiJoltActions::UnregisterActions();
  xiiToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);
}

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void UpdateCollisionLayerDynamicEnumValues()
{
  auto& cfe = xiiDynamicEnum::GetDynamicEnum("PhysicsCollisionLayer");
  cfe.Clear();

  xiiCollisionFilterConfig cfg;
  if (cfg.Load().Failed())
    return;

  // add all names and values that are valid (non-empty)
  for (xiiInt32 i = 0; i < 32; ++i)
  {
    if (!xiiStringUtils::IsNullOrEmpty(cfg.GetGroupName(i)))
    {
      cfe.SetValueAndName(i, cfg.GetGroupName(i));
    }
  }
}

static void ToolsProjectEventHandler(const xiiToolsProjectEvent& e)
{
  if (e.m_Type == xiiToolsProjectEvent::Type::ProjectSaveState)
  {
    xiiQtJoltProjectSettingsDlg::EnsureConfigFileExists();
  }

  if (e.m_Type == xiiToolsProjectEvent::Type::ProjectOpened)
  {
    UpdateCollisionLayerDynamicEnumValues();
  }
}
