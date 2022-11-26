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

  static void MapActions(const char* szMapping, const char* szPath);
  static void MapContextMenuActions(const char* szMapping, const char* szPath);
  static void MapViewContextMenuActions(const char* szMapping, const char* szPath);

  static xiiActionDescriptorHandle s_hSelectionCategory;
  static xiiActionDescriptorHandle s_hShowInScenegraph;
  static xiiActionDescriptorHandle s_hFocusOnSelection;
  static xiiActionDescriptorHandle s_hFocusOnSelectionAllViews;
  static xiiActionDescriptorHandle s_hSnapCameraToObject;
  static xiiActionDescriptorHandle s_hMoveCameraHere;
  static xiiActionDescriptorHandle s_hCreateEmptyGameObjectHere;
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
    CreateGameObjectHere,
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
