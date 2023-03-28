#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiPreferences;

///
class XII_EDITORPLUGINSCENE_DLL xiiSceneActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(const char* szMapping);
  static void MapToolbarActions(const char* szMapping);
  static void MapViewContextMenuActions(const char* szMapping, const char* szPath);

  static xiiActionDescriptorHandle s_hSceneCategory;
  static xiiActionDescriptorHandle s_hSceneUtilsMenu;
  static xiiActionDescriptorHandle s_hExportScene;
  static xiiActionDescriptorHandle s_hGameModeSimulate;
  static xiiActionDescriptorHandle s_hGameModePlay;
  static xiiActionDescriptorHandle s_hGameModePlayFromHere;
  static xiiActionDescriptorHandle s_hGameModeStop;
  static xiiActionDescriptorHandle s_hUtilExportSceneToOBJ;
  static xiiActionDescriptorHandle s_hKeepSimulationChanges;
  static xiiActionDescriptorHandle s_hCreateThumbnail;
  static xiiActionDescriptorHandle s_hFavoriteCamsMenu;
  static xiiActionDescriptorHandle s_hStoreEditorCamera[10];
  static xiiActionDescriptorHandle s_hRestoreEditorCamera[10];
  static xiiActionDescriptorHandle s_hJumpToCamera[10];
  static xiiActionDescriptorHandle s_hCreateLevelCamera[10];
};

///
class XII_EDITORPLUGINSCENE_DLL xiiSceneAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneAction, xiiButtonAction);

public:
  enum class ActionType
  {
    ExportAndRunScene,
    StartGameModeSimulate,
    StartGameModePlay,
    StartGameModePlayFromHere,
    StopGameMode,
    ExportSceneToOBJ,
    KeepSimulationChanges,
    CreateThumbnail,

    StoreEditorCamera0,
    StoreEditorCamera1,
    StoreEditorCamera2,
    StoreEditorCamera3,
    StoreEditorCamera4,
    StoreEditorCamera5,
    StoreEditorCamera6,
    StoreEditorCamera7,
    StoreEditorCamera8,
    StoreEditorCamera9,

    RestoreEditorCamera0,
    RestoreEditorCamera1,
    RestoreEditorCamera2,
    RestoreEditorCamera3,
    RestoreEditorCamera4,
    RestoreEditorCamera5,
    RestoreEditorCamera6,
    RestoreEditorCamera7,
    RestoreEditorCamera8,
    RestoreEditorCamera9,

    JumpToCamera0,
    JumpToCamera1,
    JumpToCamera2,
    JumpToCamera3,
    JumpToCamera4,
    JumpToCamera5,
    JumpToCamera6,
    JumpToCamera7,
    JumpToCamera8,
    JumpToCamera9,

    CreateLevelCamera0,
    CreateLevelCamera1,
    CreateLevelCamera2,
    CreateLevelCamera3,
    CreateLevelCamera4,
    CreateLevelCamera5,
    CreateLevelCamera6,
    CreateLevelCamera7,
    CreateLevelCamera8,
    CreateLevelCamera9,
  };

  xiiSceneAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiSceneAction();

  virtual void Execute(const xiiVariant& value) override;

  void        LaunchPlayer(const char* szPlayerApp);
  QStringList GetPlayerCommandLine(xiiStringBuilder& out_sSingleLine) const;

private:
  void SceneEventHandler(const xiiGameObjectEvent& e);
  void UpdateState();

  xiiSceneDocument* m_pSceneDocument;
  ActionType        m_Type;
};
