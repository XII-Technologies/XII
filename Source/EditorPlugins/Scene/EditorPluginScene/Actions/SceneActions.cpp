#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Dialogs/ExportAndRunDlg.moc.h>
#include <EditorPluginScene/Dialogs/ExtractGeometryDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QProcess>
#include <SharedPluginScene/Common/Messages.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiSceneActions::s_hSceneCategory;
xiiActionDescriptorHandle xiiSceneActions::s_hSceneUtilsMenu;
xiiActionDescriptorHandle xiiSceneActions::s_hExportScene;
xiiActionDescriptorHandle xiiSceneActions::s_hGameModeSimulate;
xiiActionDescriptorHandle xiiSceneActions::s_hGameModePlay;
xiiActionDescriptorHandle xiiSceneActions::s_hGameModePlayFromHere;
xiiActionDescriptorHandle xiiSceneActions::s_hGameModeStop;
xiiActionDescriptorHandle xiiSceneActions::s_hGameModePause;
xiiActionDescriptorHandle xiiSceneActions::s_hKeepSimulationChanges;
xiiActionDescriptorHandle xiiSceneActions::s_hUtilExportSceneToOBJ;
xiiActionDescriptorHandle xiiSceneActions::s_hCreateThumbnail;
xiiActionDescriptorHandle xiiSceneActions::s_hFavoriteCamsMenu;
xiiActionDescriptorHandle xiiSceneActions::s_hStoreEditorCamera[10];
xiiActionDescriptorHandle xiiSceneActions::s_hRestoreEditorCamera[10];
xiiActionDescriptorHandle xiiSceneActions::s_hJumpToCamera[10];
xiiActionDescriptorHandle xiiSceneActions::s_hCreateLevelCamera[10];

void xiiSceneActions::RegisterActions()
{
  s_hSceneCategory  = XII_REGISTER_CATEGORY("SceneCategory");
  s_hSceneUtilsMenu = XII_REGISTER_MENU_WITH_ICON("Scene.Utils.Menu", "");

  s_hExportScene      = XII_REGISTER_ACTION_1("Scene.ExportAndRun", xiiActionScope::Document, "Scene", "Ctrl+R", xiiSceneAction, xiiSceneAction::ActionType::ExportAndRunScene);
  s_hGameModeSimulate = XII_REGISTER_ACTION_1("Scene.GameMode.Simulate", xiiActionScope::Document, "Scene", "F5", xiiSceneAction, xiiSceneAction::ActionType::StartGameModeSimulate);
  s_hGameModePlay     = XII_REGISTER_ACTION_1("Scene.GameMode.Play", xiiActionScope::Document, "Scene", "Ctrl+F5", xiiSceneAction, xiiSceneAction::ActionType::StartGameModePlay);

  s_hGameModePlayFromHere  = XII_REGISTER_ACTION_1("Scene.GameMode.PlayFromHere", xiiActionScope::Document, "Scene", "F6", xiiSceneAction, xiiSceneAction::ActionType::StartGameModePlayFromHere);
  s_hGameModeStop          = XII_REGISTER_ACTION_1("Scene.GameMode.Stop", xiiActionScope::Document, "Scene", "Shift+F5", xiiSceneAction, xiiSceneAction::ActionType::StopGameMode);
  s_hGameModePause         = XII_REGISTER_ACTION_1("Scene.GameMode.Pause", xiiActionScope::Document, "Scene", "Pause", xiiSceneAction, xiiSceneAction::ActionType::PauseSimulation);
  s_hKeepSimulationChanges = XII_REGISTER_ACTION_1("Scene.KeepSimulationChanges", xiiActionScope::Document, "Scene", "K", xiiSceneAction, xiiSceneAction::ActionType::KeepSimulationChanges);

  s_hUtilExportSceneToOBJ = XII_REGISTER_ACTION_1("Scene.ExportSceneToOBJ", xiiActionScope::Document, "Scene", "", xiiSceneAction, xiiSceneAction::ActionType::ExportSceneToOBJ);
  s_hCreateThumbnail      = XII_REGISTER_ACTION_1("Scene.CreateThumbnail", xiiActionScope::Document, "Scene", "", xiiSceneAction, xiiSceneAction::ActionType::CreateThumbnail);

  // unfortunately the macros use lambdas thus using a loop to generate the strings does not work
  {
    s_hFavoriteCamsMenu = XII_REGISTER_MENU_WITH_ICON("Scene.FavoriteCams.Menu", "");

    s_hStoreEditorCamera[0] = XII_REGISTER_ACTION_1("Scene.Camera.Store.0", xiiActionScope::Document, "Scene - Cameras", "Ctrl+0", xiiSceneAction, xiiSceneAction::ActionType::StoreEditorCamera0);
    s_hStoreEditorCamera[1] = XII_REGISTER_ACTION_1("Scene.Camera.Store.1", xiiActionScope::Document, "Scene - Cameras", "Ctrl+1", xiiSceneAction, xiiSceneAction::ActionType::StoreEditorCamera1);
    s_hStoreEditorCamera[2] = XII_REGISTER_ACTION_1("Scene.Camera.Store.2", xiiActionScope::Document, "Scene - Cameras", "Ctrl+2", xiiSceneAction, xiiSceneAction::ActionType::StoreEditorCamera2);
    s_hStoreEditorCamera[3] = XII_REGISTER_ACTION_1("Scene.Camera.Store.3", xiiActionScope::Document, "Scene - Cameras", "Ctrl+3", xiiSceneAction, xiiSceneAction::ActionType::StoreEditorCamera3);
    s_hStoreEditorCamera[4] = XII_REGISTER_ACTION_1("Scene.Camera.Store.4", xiiActionScope::Document, "Scene - Cameras", "Ctrl+4", xiiSceneAction, xiiSceneAction::ActionType::StoreEditorCamera4);
    s_hStoreEditorCamera[5] = XII_REGISTER_ACTION_1("Scene.Camera.Store.5", xiiActionScope::Document, "Scene - Cameras", "Ctrl+5", xiiSceneAction, xiiSceneAction::ActionType::StoreEditorCamera5);
    s_hStoreEditorCamera[6] = XII_REGISTER_ACTION_1("Scene.Camera.Store.6", xiiActionScope::Document, "Scene - Cameras", "Ctrl+6", xiiSceneAction, xiiSceneAction::ActionType::StoreEditorCamera6);
    s_hStoreEditorCamera[7] = XII_REGISTER_ACTION_1("Scene.Camera.Store.7", xiiActionScope::Document, "Scene - Cameras", "Ctrl+7", xiiSceneAction, xiiSceneAction::ActionType::StoreEditorCamera7);
    s_hStoreEditorCamera[8] = XII_REGISTER_ACTION_1("Scene.Camera.Store.8", xiiActionScope::Document, "Scene - Cameras", "Ctrl+8", xiiSceneAction, xiiSceneAction::ActionType::StoreEditorCamera8);
    s_hStoreEditorCamera[9] = XII_REGISTER_ACTION_1("Scene.Camera.Store.9", xiiActionScope::Document, "Scene - Cameras", "Ctrl+9", xiiSceneAction, xiiSceneAction::ActionType::StoreEditorCamera9);

    s_hRestoreEditorCamera[0] = XII_REGISTER_ACTION_1("Scene.Camera.Restore.0", xiiActionScope::Document, "Scene - Cameras", "0", xiiSceneAction, xiiSceneAction::ActionType::RestoreEditorCamera0);
    s_hRestoreEditorCamera[1] = XII_REGISTER_ACTION_1("Scene.Camera.Restore.1", xiiActionScope::Document, "Scene - Cameras", "1", xiiSceneAction, xiiSceneAction::ActionType::RestoreEditorCamera1);
    s_hRestoreEditorCamera[2] = XII_REGISTER_ACTION_1("Scene.Camera.Restore.2", xiiActionScope::Document, "Scene - Cameras", "2", xiiSceneAction, xiiSceneAction::ActionType::RestoreEditorCamera2);
    s_hRestoreEditorCamera[3] = XII_REGISTER_ACTION_1("Scene.Camera.Restore.3", xiiActionScope::Document, "Scene - Cameras", "3", xiiSceneAction, xiiSceneAction::ActionType::RestoreEditorCamera3);
    s_hRestoreEditorCamera[4] = XII_REGISTER_ACTION_1("Scene.Camera.Restore.4", xiiActionScope::Document, "Scene - Cameras", "4", xiiSceneAction, xiiSceneAction::ActionType::RestoreEditorCamera4);
    s_hRestoreEditorCamera[5] = XII_REGISTER_ACTION_1("Scene.Camera.Restore.5", xiiActionScope::Document, "Scene - Cameras", "5", xiiSceneAction, xiiSceneAction::ActionType::RestoreEditorCamera5);
    s_hRestoreEditorCamera[6] = XII_REGISTER_ACTION_1("Scene.Camera.Restore.6", xiiActionScope::Document, "Scene - Cameras", "6", xiiSceneAction, xiiSceneAction::ActionType::RestoreEditorCamera6);
    s_hRestoreEditorCamera[7] = XII_REGISTER_ACTION_1("Scene.Camera.Restore.7", xiiActionScope::Document, "Scene - Cameras", "7", xiiSceneAction, xiiSceneAction::ActionType::RestoreEditorCamera7);
    s_hRestoreEditorCamera[8] = XII_REGISTER_ACTION_1("Scene.Camera.Restore.8", xiiActionScope::Document, "Scene - Cameras", "8", xiiSceneAction, xiiSceneAction::ActionType::RestoreEditorCamera8);
    s_hRestoreEditorCamera[9] = XII_REGISTER_ACTION_1("Scene.Camera.Restore.9", xiiActionScope::Document, "Scene - Cameras", "9", xiiSceneAction, xiiSceneAction::ActionType::RestoreEditorCamera9);

    s_hJumpToCamera[0] = XII_REGISTER_ACTION_1("Scene.Camera.JumpTo.0", xiiActionScope::Document, "Scene - Cameras", "Alt+0", xiiSceneAction, xiiSceneAction::ActionType::JumpToCamera0);
    s_hJumpToCamera[1] = XII_REGISTER_ACTION_1("Scene.Camera.JumpTo.1", xiiActionScope::Document, "Scene - Cameras", "Alt+1", xiiSceneAction, xiiSceneAction::ActionType::JumpToCamera1);
    s_hJumpToCamera[2] = XII_REGISTER_ACTION_1("Scene.Camera.JumpTo.2", xiiActionScope::Document, "Scene - Cameras", "Alt+2", xiiSceneAction, xiiSceneAction::ActionType::JumpToCamera2);
    s_hJumpToCamera[3] = XII_REGISTER_ACTION_1("Scene.Camera.JumpTo.3", xiiActionScope::Document, "Scene - Cameras", "Alt+3", xiiSceneAction, xiiSceneAction::ActionType::JumpToCamera3);
    s_hJumpToCamera[4] = XII_REGISTER_ACTION_1("Scene.Camera.JumpTo.4", xiiActionScope::Document, "Scene - Cameras", "Alt+4", xiiSceneAction, xiiSceneAction::ActionType::JumpToCamera4);
    s_hJumpToCamera[5] = XII_REGISTER_ACTION_1("Scene.Camera.JumpTo.5", xiiActionScope::Document, "Scene - Cameras", "Alt+5", xiiSceneAction, xiiSceneAction::ActionType::JumpToCamera5);
    s_hJumpToCamera[6] = XII_REGISTER_ACTION_1("Scene.Camera.JumpTo.6", xiiActionScope::Document, "Scene - Cameras", "Alt+6", xiiSceneAction, xiiSceneAction::ActionType::JumpToCamera6);
    s_hJumpToCamera[7] = XII_REGISTER_ACTION_1("Scene.Camera.JumpTo.7", xiiActionScope::Document, "Scene - Cameras", "Alt+7", xiiSceneAction, xiiSceneAction::ActionType::JumpToCamera7);
    s_hJumpToCamera[8] = XII_REGISTER_ACTION_1("Scene.Camera.JumpTo.8", xiiActionScope::Document, "Scene - Cameras", "Alt+8", xiiSceneAction, xiiSceneAction::ActionType::JumpToCamera8);
    s_hJumpToCamera[9] = XII_REGISTER_ACTION_1("Scene.Camera.JumpTo.9", xiiActionScope::Document, "Scene - Cameras", "Alt+9", xiiSceneAction, xiiSceneAction::ActionType::JumpToCamera9);

    s_hCreateLevelCamera[0] = XII_REGISTER_ACTION_1("Scene.Camera.Create.0", xiiActionScope::Document, "Scene - Cameras", "Ctrl+Alt+0", xiiSceneAction, xiiSceneAction::ActionType::CreateLevelCamera0);
    s_hCreateLevelCamera[1] = XII_REGISTER_ACTION_1("Scene.Camera.Create.1", xiiActionScope::Document, "Scene - Cameras", "Ctrl+Alt+1", xiiSceneAction, xiiSceneAction::ActionType::CreateLevelCamera1);
    s_hCreateLevelCamera[2] = XII_REGISTER_ACTION_1("Scene.Camera.Create.2", xiiActionScope::Document, "Scene - Cameras", "Ctrl+Alt+2", xiiSceneAction, xiiSceneAction::ActionType::CreateLevelCamera2);
    s_hCreateLevelCamera[3] = XII_REGISTER_ACTION_1("Scene.Camera.Create.3", xiiActionScope::Document, "Scene - Cameras", "Ctrl+Alt+3", xiiSceneAction, xiiSceneAction::ActionType::CreateLevelCamera3);
    s_hCreateLevelCamera[4] = XII_REGISTER_ACTION_1("Scene.Camera.Create.4", xiiActionScope::Document, "Scene - Cameras", "Ctrl+Alt+4", xiiSceneAction, xiiSceneAction::ActionType::CreateLevelCamera4);
    s_hCreateLevelCamera[5] = XII_REGISTER_ACTION_1("Scene.Camera.Create.5", xiiActionScope::Document, "Scene - Cameras", "Ctrl+Alt+5", xiiSceneAction, xiiSceneAction::ActionType::CreateLevelCamera5);
    s_hCreateLevelCamera[6] = XII_REGISTER_ACTION_1("Scene.Camera.Create.6", xiiActionScope::Document, "Scene - Cameras", "Ctrl+Alt+6", xiiSceneAction, xiiSceneAction::ActionType::CreateLevelCamera6);
    s_hCreateLevelCamera[7] = XII_REGISTER_ACTION_1("Scene.Camera.Create.7", xiiActionScope::Document, "Scene - Cameras", "Ctrl+Alt+7", xiiSceneAction, xiiSceneAction::ActionType::CreateLevelCamera7);
    s_hCreateLevelCamera[8] = XII_REGISTER_ACTION_1("Scene.Camera.Create.8", xiiActionScope::Document, "Scene - Cameras", "Ctrl+Alt+8", xiiSceneAction, xiiSceneAction::ActionType::CreateLevelCamera8);
    s_hCreateLevelCamera[9] = XII_REGISTER_ACTION_1("Scene.Camera.Create.9", xiiActionScope::Document, "Scene - Cameras", "Ctrl+Alt+9", xiiSceneAction, xiiSceneAction::ActionType::CreateLevelCamera9);
  }
}

void xiiSceneActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hSceneCategory);
  xiiActionManager::UnregisterAction(s_hSceneUtilsMenu);
  xiiActionManager::UnregisterAction(s_hExportScene);
  xiiActionManager::UnregisterAction(s_hGameModeSimulate);
  xiiActionManager::UnregisterAction(s_hGameModePlay);
  xiiActionManager::UnregisterAction(s_hGameModePlayFromHere);
  xiiActionManager::UnregisterAction(s_hGameModeStop);
  xiiActionManager::UnregisterAction(s_hGameModePause);
  xiiActionManager::UnregisterAction(s_hKeepSimulationChanges);
  xiiActionManager::UnregisterAction(s_hUtilExportSceneToOBJ);
  xiiActionManager::UnregisterAction(s_hCreateThumbnail);
  xiiActionManager::UnregisterAction(s_hFavoriteCamsMenu);

  for (xiiUInt32 i = 0; i < 10; ++i)
  {
    xiiActionManager::UnregisterAction(s_hStoreEditorCamera[i]);
    xiiActionManager::UnregisterAction(s_hRestoreEditorCamera[i]);
    xiiActionManager::UnregisterAction(s_hJumpToCamera[i]);
    xiiActionManager::UnregisterAction(s_hCreateLevelCamera[i]);
  }
}

void xiiSceneActions::MapMenuActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath      = "G.Scene/SceneCategory";
    const char* szUtilsSubPath = "G.Scene/Scene.Utils.Menu";

    pMap->MapAction(s_hSceneUtilsMenu, "G.Scene", 2.0f);
    // pMap->MapAction(s_hCreateThumbnail, szUtilsSubPath, 0.0f); // now available through the export scene dialog
    pMap->MapAction(s_hKeepSimulationChanges, szUtilsSubPath, 1.0f);
    pMap->MapAction(s_hUtilExportSceneToOBJ, szUtilsSubPath, 2.0f);

    pMap->MapAction(s_hFavoriteCamsMenu, "G.Scene", 3.0f);
    const char* szFavCamsSubPath = "G.Scene/Scene.FavoriteCams.Menu";

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      pMap->MapAction(s_hStoreEditorCamera[i], szFavCamsSubPath, 10.0f + i);
      pMap->MapAction(s_hRestoreEditorCamera[i], szFavCamsSubPath, 20.0f + i);
      pMap->MapAction(s_hJumpToCamera[i], szFavCamsSubPath, 30.0f + i);
      pMap->MapAction(s_hCreateLevelCamera[i], szFavCamsSubPath, 40.0f + i);
    }

    pMap->MapAction(s_hSceneCategory, "G.Scene", 4.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 1.0f);
    pMap->MapAction(s_hGameModeStop, szSubPath, 4.0f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 5.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 6.0f);
    pMap->MapAction(s_hGameModePlayFromHere, szSubPath, 7.0f);
    pMap->MapAction(s_hGameModePause, szSubPath, 8.0f);
  }
}

void xiiSceneActions::MapToolbarActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "SceneCategory";

    /// \todo This works incorrectly with value 6.0f -> it places the action inside the snap category
    pMap->MapAction(s_hSceneCategory, "", 11.0f);
    pMap->MapAction(s_hGameModeStop, szSubPath, 1.0f);
    pMap->MapAction(s_hGameModePause, szSubPath, 1.5f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 2.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 3.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 4.0f);
  }
}

void xiiSceneActions::MapViewContextMenuActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hGameModePlayFromHere, "", 0.0f);
}

xiiSceneAction::xiiSceneAction(const xiiActionContext& context, const char* szName, xiiSceneAction::ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pSceneDocument = static_cast<xiiSceneDocument*>(context.m_pDocument);
  m_pSceneDocument->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiSceneAction::SceneEventHandler, this));

  switch (m_Type)
  {
    case ActionType::ExportAndRunScene:
      SetIconPath(":/EditorPluginScene/Icons/SceneExport.svg");
      break;

    case ActionType::StartGameModeSimulate:
      SetIconPath(":/EditorPluginScene/Icons/ScenePlay.svg");
      SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Play);
      break;

    case ActionType::StartGameModePlay:
      SetIconPath(":/EditorPluginScene/Icons/ScenePlayTheGame.svg");
      break;

    case ActionType::StartGameModePlayFromHere:
      SetIconPath(":/EditorPluginScene/Icons/ScenePlayTheGame.svg"); // TODO: icon
      break;

    case ActionType::StopGameMode:
      SetIconPath(":/EditorPluginScene/Icons/SceneStop.svg");
      break;

    case ActionType::PauseSimulation:
      SetIconPath(":/EditorPluginScene/Icons/ScenePause.svg");
      break;

    case ActionType::ExportSceneToOBJ:
      // SetIconPath(":/EditorPluginScene/Icons/SceneStop.svg"); // TODO: icon
      break;

    case ActionType::KeepSimulationChanges:
      SetIconPath(":/EditorPluginScene/Icons/PullObjectState.svg");
      break;

    case ActionType::CreateThumbnail:
      // SetIconPath(":/EditorPluginScene/Icons/PullObjectState.svg"); // TODO: icon
      break;

    case ActionType::JumpToCamera0:
    case ActionType::JumpToCamera1:
    case ActionType::JumpToCamera2:
    case ActionType::JumpToCamera3:
    case ActionType::JumpToCamera4:
    case ActionType::JumpToCamera5:
    case ActionType::JumpToCamera6:
    case ActionType::JumpToCamera7:
    case ActionType::JumpToCamera8:
    case ActionType::JumpToCamera9:
      SetIconPath(":/TypeIcons/xiiCameraComponent.svg");
      break;

    default:
      // no icon
      break;
  }

  UpdateState();
}

xiiSceneAction::~xiiSceneAction()
{
  m_pSceneDocument->m_GameObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiSceneAction::SceneEventHandler, this));
}

void xiiSceneAction::Execute(const xiiVariant& value)
{
  switch (m_Type)
  {
    case ActionType::ExportAndRunScene:
    {
      xiiStringBuilder sCmd;
      GetPlayerCommandLine(sCmd);

      xiiQtExportAndRunDlg dlg(nullptr);
      dlg.m_sCmdLine               = sCmd;
      dlg.s_bUpdateThumbnail       = false;
      dlg.m_bShowThumbnailCheckbox = !m_pSceneDocument->IsPrefab();

      if (dlg.exec() != QDialog::Accepted)
        return;

      xiiProgressRange range("Export and Run", 4, true);

      range.BeginNextStep("Build C++");
      if (dlg.s_bCompileCpp)
      {
        if (xiiCppProject::EnsureCppPluginReady().Failed())
          return;
      }

      bool bDidTransformAll = false;

      range.BeginNextStep("Transform Assets");
      if (dlg.s_bTransformAll)
      {
        if (xiiAssetCurator::GetSingleton()->TransformAllAssets(xiiTransformFlags::None).Succeeded())
        {
          // once all assets have been transformed, disable it for the next export
          dlg.s_bTransformAll = false;
          bDidTransformAll    = true;
        }
      }

      bool bCreateThumbnail = dlg.s_bUpdateThumbnail;

      range.BeginNextStep("Create Thumbnail");
      if (!m_pSceneDocument->IsPrefab() && !bCreateThumbnail)
      {
        // if the thumbnail doesn't exist, or is very old, update it anyway

        xiiStringBuilder sThumbnailPath = m_pSceneDocument->GetAssetDocumentManager()->GenerateResourceThumbnailPath(m_pSceneDocument->GetDocumentPath());

        xiiFileStats stat;
        if (xiiOSFile::GetFileStats(sThumbnailPath, stat).Failed())
        {
          bCreateThumbnail = true;
        }
        else
        {
          auto tNow  = xiiTimestamp::CurrentTimestamp();
          auto tComp = stat.m_LastModificationTime + xiiTime::MakeFromHours(24) * 7;

          if (tComp.GetInt64(xiiSIUnitOfTime::Second) < tNow.GetInt64(xiiSIUnitOfTime::Second))
          {
            bCreateThumbnail = true;
          }
        }
      }


      // Convert collections
      if (!bDidTransformAll)
      {
        xiiAssetCurator* pCurator = xiiAssetCurator::GetSingleton();
        pCurator->TransformAssetsForSceneExport(pCurator->GetActiveAssetProfile());
      }

      dlg.s_bUpdateThumbnail = false;

      range.BeginNextStep("Export Scene");
      if (m_pSceneDocument->ExportScene(bCreateThumbnail).Failed())
      {
        xiiQtUiServices::GetSingleton()->MessageBoxWarning("Scene export failed.");
        return;
      }

      // send event, so that 3rd party code can hook into this
      {
        xiiGameObjectDocumentEvent e;
        e.m_Type      = xiiGameObjectDocumentEvent::Type::GameMode_StartingExternal;
        e.m_pDocument = m_pSceneDocument;
        m_pSceneDocument->s_GameObjectDocumentEvents.Broadcast(e);
      }

      if (dlg.m_bRunAfterExport)
      {
        LaunchPlayer(dlg.m_sApplication);
      }

      return;
    }

    case ActionType::StartGameModePlay:
      m_pSceneDocument->TriggerGameModePlay(false);
      return;

    case ActionType::StartGameModePlayFromHere:
      m_pSceneDocument->TriggerGameModePlay(true);
      return;

    case ActionType::StartGameModeSimulate:
    {
      if (m_pSceneDocument->GetPauseSimulation())
      {
        m_pSceneDocument->SetPauseSimulation(false);
      }
      else
      {
        m_pSceneDocument->StartSimulateWorld();
      }
      return;
    }

    case ActionType::StopGameMode:
      m_pSceneDocument->StopGameMode();
      return;

    case ActionType::PauseSimulation:
    {
      if (m_pSceneDocument->GetPauseSimulation())
        m_pSceneDocument->StepSimulation();
      else
        m_pSceneDocument->PauseSimulation();
      return;
    }

    case ActionType::ExportSceneToOBJ:
    {
      xiiQtExtractGeometryDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        m_pSceneDocument->ExportSceneGeometry(dlg.s_sDestinationFile.toUtf8().data(), dlg.s_bOnlySelection, dlg.s_iExtractionMode, dlg.GetCoordinateSystemTransform());
      }
      return;
    }

    case ActionType::KeepSimulationChanges:
    {
      xiiPullObjectStateMsgToEngine msg;
      m_pSceneDocument->SendMessageToEngine(&msg);
      return;
    }

    case ActionType::CreateThumbnail:
      m_pSceneDocument->ExportScene(true);
      return;

    case ActionType::StoreEditorCamera0:
    case ActionType::StoreEditorCamera1:
    case ActionType::StoreEditorCamera2:
    case ActionType::StoreEditorCamera3:
    case ActionType::StoreEditorCamera4:
    case ActionType::StoreEditorCamera5:
    case ActionType::StoreEditorCamera6:
    case ActionType::StoreEditorCamera7:
    case ActionType::StoreEditorCamera8:
    case ActionType::StoreEditorCamera9:
    {
      const xiiInt32 iCamIdx = (int)m_Type - (int)ActionType::StoreEditorCamera0;

      m_pSceneDocument->StoreFavoriteCamera(iCamIdx);
      m_pSceneDocument->ShowDocumentStatus(xiiFmt("Stored favorite camera position {0}", iCamIdx));

      return;
    }

    case ActionType::RestoreEditorCamera0:
    case ActionType::RestoreEditorCamera1:
    case ActionType::RestoreEditorCamera2:
    case ActionType::RestoreEditorCamera3:
    case ActionType::RestoreEditorCamera4:
    case ActionType::RestoreEditorCamera5:
    case ActionType::RestoreEditorCamera6:
    case ActionType::RestoreEditorCamera7:
    case ActionType::RestoreEditorCamera8:
    case ActionType::RestoreEditorCamera9:
    {
      const xiiInt32 iCamIdx = (int)m_Type - (int)ActionType::RestoreEditorCamera0;

      m_pSceneDocument->RestoreFavoriteCamera(iCamIdx);
      m_pSceneDocument->ShowDocumentStatus(xiiFmt("Restored favorite camera position {0}", iCamIdx));

      return;
    }

    case ActionType::JumpToCamera0:
    case ActionType::JumpToCamera1:
    case ActionType::JumpToCamera2:
    case ActionType::JumpToCamera3:
    case ActionType::JumpToCamera4:
    case ActionType::JumpToCamera5:
    case ActionType::JumpToCamera6:
    case ActionType::JumpToCamera7:
    case ActionType::JumpToCamera8:
    case ActionType::JumpToCamera9:
    {
      const xiiInt32 iCamIdx = (int)m_Type - (int)ActionType::JumpToCamera0;

      const bool bImmediate = value.IsA<bool>() ? value.Get<bool>() : false;
      if (m_pSceneDocument->JumpToLevelCamera(iCamIdx, bImmediate).Failed())
      {
        m_pSceneDocument->ShowDocumentStatus(xiiFmt("No Camera Component found with shortcut set to '{0}'", iCamIdx));
      }
      return;
    }

    case ActionType::CreateLevelCamera0:
    case ActionType::CreateLevelCamera1:
    case ActionType::CreateLevelCamera2:
    case ActionType::CreateLevelCamera3:
    case ActionType::CreateLevelCamera4:
    case ActionType::CreateLevelCamera5:
    case ActionType::CreateLevelCamera6:
    case ActionType::CreateLevelCamera7:
    case ActionType::CreateLevelCamera8:
    case ActionType::CreateLevelCamera9:
    {
      const xiiInt32 iCamIdx = (int)m_Type - (int)ActionType::CreateLevelCamera0;

      if (auto pView = xiiQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;
          pView == nullptr || pView->m_pViewConfig->m_Perspective != xiiSceneViewPerspective::Perspective)
      {
        m_pSceneDocument->ShowDocumentStatus("Note: Level cameras cannot be created in orthographic views.");
        return;
      }

      if (m_pSceneDocument->CreateLevelCamera(iCamIdx).Succeeded())
      {
        m_pSceneDocument->ShowDocumentStatus(xiiFmt("Create level camera with shortcut set to '{0}'", iCamIdx));
      }
      else
      {
        m_pSceneDocument->ShowDocumentStatus(xiiFmt("Could not create level camera '{}'.", iCamIdx));
      }
      return;
    }
  }
}

void xiiSceneAction::LaunchPlayer(const char* szPlayerApp)
{
  xiiStringBuilder sCmd;
  QStringList      arguments = GetPlayerCommandLine(sCmd);

  xiiLog::Info("Running: {} {}", szPlayerApp, sCmd);
  m_pSceneDocument->ShowDocumentStatus(xiiFmt("Running: {} {}", szPlayerApp, sCmd));

  QProcess proc;
  proc.startDetached(QString::fromUtf8(szPlayerApp), arguments);
}

QStringList xiiSceneAction::GetPlayerCommandLine(xiiStringBuilder& out_sSingleLine) const
{
  QStringList arguments;
  arguments << "-project";
  arguments << xiiToolsProject::GetSingleton()->GetProjectDirectory().GetData();

  {
    arguments << "-scene";

    xiiStringBuilder sAssetDataDir = xiiAssetCurator::GetSingleton()->FindDataDirectoryForAsset(m_pSceneDocument->GetDocumentPath());

    xiiStringBuilder sRelativePath = m_pSceneDocument->GetAssetDocumentManager()->GetAbsoluteOutputFileName(m_pSceneDocument->GetAssetDocumentTypeDescriptor(), m_pSceneDocument->GetDocumentPath(), "");

    sRelativePath.MakeRelativeTo(sAssetDataDir).AssertSuccess();
    sRelativePath.MakeCleanPath();

    arguments << sRelativePath.GetData();
  }

  xiiStringBuilder sWndCfgPath = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sWndCfgPath.AppendPath("RuntimeConfigs/Window.ddl");

  if (xiiOSFile::ExistsFile(sWndCfgPath))
  {
    arguments << "-wnd";
    arguments << QString::fromUtf8(sWndCfgPath, sWndCfgPath.GetElementCount());
  }

  arguments << "-profile";
  arguments << xiiString(xiiAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName()).GetData();

  if (xiiCommandLineUtils::GetGlobalInstance()->HasOption("-renderer"))
  {
    xiiStringBuilder sRenderer = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer");
    arguments << "-renderer";
    arguments << sRenderer.GetData();
  }

  for (QString s : arguments)
  {
    if (s.contains(" "))
      out_sSingleLine.AppendFormat(" \"{}\"", s.toUtf8().data());
    else
      out_sSingleLine.AppendFormat(" {}", s.toUtf8().data());
  }

  return arguments;
}

void xiiSceneAction::SceneEventHandler(const xiiGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGameObjectEvent::Type::GameModeChanged:
    case xiiGameObjectEvent::Type::SimulationSpeedChanged:
      UpdateState();
      break;

    default:
      break;
  }
}

void xiiSceneAction::UpdateState()
{
  if (m_Type == ActionType::StartGameModeSimulate)
  {
    if (m_pSceneDocument->GetGameMode() == GameMode::Off)
    {
      SetEnabled(true);
    }
    else if (m_pSceneDocument->GetPauseSimulation() && (m_pSceneDocument->GetGameMode() == GameMode::Simulate || m_pSceneDocument->GetGameMode() == GameMode::Play))
    {
      SetEnabled(true);
    }
    else
    {
      SetEnabled(false);
    }
  }

  if (m_Type == ActionType::ExportAndRunScene || m_Type == ActionType::StartGameModePlay)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() == GameMode::Off);
  }

  if (m_Type == ActionType::StopGameMode)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off);
  }

  if (m_Type == ActionType::PauseSimulation)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off);

    if (m_pSceneDocument->GetPauseSimulation())
    {
      SetIconPath(":/EditorPluginScene/Icons/SceneStep.svg");
    }
    else
    {
      SetIconPath(":/EditorPluginScene/Icons/ScenePause.svg");
    }

    TriggerUpdate();
  }

  if (m_Type == ActionType::KeepSimulationChanges)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off && !m_pSceneDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}
