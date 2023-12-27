#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

enum class ActiveGizmo;
class xiiGameObjectDocument;
struct xiiSnapProviderEvent;
struct xiiGameObjectEvent;

///
class XII_EDITORFRAMEWORK_DLL xiiTransformGizmoActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(xiiStringView sMapping);
  static void MapToolbarActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hGizmoCategory;
  static xiiActionDescriptorHandle s_hGizmoMenu;
  static xiiActionDescriptorHandle s_hNoGizmo;
  static xiiActionDescriptorHandle s_hTranslateGizmo;
  static xiiActionDescriptorHandle s_hRotateGizmo;
  static xiiActionDescriptorHandle s_hScaleGizmo;
  static xiiActionDescriptorHandle s_hDragToPositionGizmo;
  static xiiActionDescriptorHandle s_hWorldSpace;
  static xiiActionDescriptorHandle s_hMoveParentOnly;
  static xiiActionDescriptorHandle s_SnapSettings;
};

///
class XII_EDITORFRAMEWORK_DLL xiiGizmoAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGizmoAction, xiiButtonAction);

public:
  xiiGizmoAction(const xiiActionContext& context, const char* szName, const xiiRTTI* pGizmoType);
  ~xiiGizmoAction();

  virtual void Execute(const xiiVariant& value) override;

protected:
  void UpdateState();
  void GameObjectEventHandler(const xiiGameObjectEvent& e);

  xiiGameObjectDocument* m_pGameObjectDocument = nullptr;
  const xiiRTTI*         m_pGizmoType          = nullptr;
};

///
class XII_EDITORFRAMEWORK_DLL xiiToggleWorldSpaceGizmo : public xiiGizmoAction
{
public:
  xiiToggleWorldSpaceGizmo(const xiiActionContext& context, const char* szName, const xiiRTTI* pGizmoType);
  virtual void Execute(const xiiVariant& value) override;
};

///
class XII_EDITORFRAMEWORK_DLL xiiTransformGizmoAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTransformGizmoAction, xiiButtonAction);

public:
  enum class ActionType
  {
    GizmoToggleWorldSpace,
    GizmoToggleMoveParentOnly,
    GizmoSnapSettings,
  };

  xiiTransformGizmoAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiTransformGizmoAction();

  virtual void Execute(const xiiVariant& value) override;
  void         GameObjectEventHandler(const xiiGameObjectEvent& e);

private:
  void UpdateState();

  xiiGameObjectDocument* m_pGameObjectDocument = nullptr;
  ActionType             m_Type;
};

///
class XII_EDITORFRAMEWORK_DLL xiiTranslateGizmoAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTranslateGizmoAction, xiiButtonAction);

public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(xiiStringView sMapping);

private:
  static xiiActionDescriptorHandle s_hSnappingValueMenu;
  static xiiActionDescriptorHandle s_hSnapPivotToGrid;
  static xiiActionDescriptorHandle s_hSnapObjectsToGrid;

public:
  enum class ActionType
  {
    SnapSelectionPivotToGrid,
    SnapEachSelectedObjectToGrid,
  };

  xiiTranslateGizmoAction(const xiiActionContext& context, const char* szName, ActionType type);

  virtual void Execute(const xiiVariant& value) override;

private:
  const xiiGameObjectDocument* m_pSceneDocument = nullptr;
  ActionType                   m_Type;
};
