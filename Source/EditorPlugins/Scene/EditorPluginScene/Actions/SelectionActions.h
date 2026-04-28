/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class XII_EDITORPLUGINSCENE_DLL xiiSelectionActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(xiiStringView sMapping);
  static void MapPrefabActions(xiiStringView sMapping, float fPriority);
  static void MapContextMenuActions(xiiStringView sMapping);
  static void MapViewContextMenuActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hGroupSelectedItems;
  static xiiActionDescriptorHandle s_hCreateEmptyChildObject;
  static xiiActionDescriptorHandle s_hCreateEmptyObjectAtPosition;
  static xiiActionDescriptorHandle s_hHideSelectedObjects;
  static xiiActionDescriptorHandle s_hHideUnselectedObjects;
  static xiiActionDescriptorHandle s_hShowHiddenObjects;
  static xiiActionDescriptorHandle s_hPrefabMenu;
  static xiiActionDescriptorHandle s_hCreatePrefab;
  static xiiActionDescriptorHandle s_hRevertPrefab;
  static xiiActionDescriptorHandle s_hUnlinkFromPrefab;
  static xiiActionDescriptorHandle s_hOpenPrefabDocument;
  static xiiActionDescriptorHandle s_hDuplicateSpecial;
  static xiiActionDescriptorHandle s_hDeltaTransform;
  static xiiActionDescriptorHandle s_hSnapObjectToCamera;
  static xiiActionDescriptorHandle s_hAttachToObject;
  static xiiActionDescriptorHandle s_hDetachFromParent;
  static xiiActionDescriptorHandle s_hConvertToEnginePrefab;
  static xiiActionDescriptorHandle s_hConvertToEditorPrefab;
  static xiiActionDescriptorHandle s_hCopyReference;
  static xiiActionDescriptorHandle s_hSelectParent;
  static xiiActionDescriptorHandle s_hSetActiveParent;
  static xiiActionDescriptorHandle s_hClearActiveParent;
  static xiiActionDescriptorHandle s_hUndoSelection;
};

///
class XII_EDITORPLUGINSCENE_DLL xiiSelectionAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSelectionAction, xiiButtonAction);

public:
  enum class ActionType
  {
    GroupSelectedItems,
    CreateEmptyChildObject,
    CreateEmptyObjectAtPosition,
    HideSelectedObjects,
    HideUnselectedObjects,
    ShowHiddenObjects,

    CreatePrefab,
    RevertPrefab,
    UnlinkFromPrefab,
    OpenPrefabDocument,
    ConvertToEnginePrefab,
    ConvertToEditorPrefab,

    DuplicateSpecial,
    DeltaTransform,
    SnapObjectToCamera,
    AttachToObject,
    DetachFromParent,
    CopyReference,
    SelectParent,

    SetActiveParent,
    ClearActiveParent,

    UndoSelection,
  };

  xiiSelectionAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiSelectionAction();

  virtual void Execute(const xiiVariant& value) override;

  void OpenPrefabDocument();

  void CreatePrefab();

private:
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);

  void UpdateEnableState();

  xiiSceneDocument* m_pSceneDocument;
  ActionType        m_Type;
};
