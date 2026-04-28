/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiGameObjectDocument;

///
class XII_EDITORFRAMEWORK_DLL xiiGameObjectSelectionActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(xiiStringView sMapping);
  static void MapContextMenuActions(xiiStringView sMapping);
  static void MapViewContextMenuActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hSelectionCategory;
  static xiiActionDescriptorHandle s_hShowInScenegraph;
  static xiiActionDescriptorHandle s_hFocusOnSelection;
  static xiiActionDescriptorHandle s_hFocusOnSelectionAllViews;
  static xiiActionDescriptorHandle s_hSnapCameraToObject;
  static xiiActionDescriptorHandle s_hMoveCameraHere;
};

///
class XII_EDITORFRAMEWORK_DLL xiiGameObjectSelectionAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectSelectionAction, xiiButtonAction);

public:
  enum class ActionType
  {
    ShowInScenegraph,
    FocusOnSelection,
    FocusOnSelectionAllViews,
    SnapCameraToObject,
    MoveCameraHere,
  };

  xiiGameObjectSelectionAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiGameObjectSelectionAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);

  void UpdateEnableState();

  xiiGameObjectDocument* m_pSceneDocument;
  ActionType             m_Type;
};
