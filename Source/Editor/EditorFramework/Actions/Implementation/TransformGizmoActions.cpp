#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Dialogs/SnapSettingsDlg.moc.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGizmoAction, 0, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGizmoAction::xiiGizmoAction(const xiiActionContext& context, const char* szName, const xiiRTTI* pGizmoType) :
  xiiButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_pGizmoType          = pGizmoType;
  m_pGameObjectDocument = static_cast<xiiGameObjectDocument*>(context.m_pDocument);
  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiGizmoAction::GameObjectEventHandler, this));

  if (m_pGizmoType)
  {
    xiiStringBuilder sIcon(":/TypeIcons/", m_pGizmoType->GetTypeName());
    SetIconPath(sIcon);
  }
  else
  {
    SetIconPath(":/EditorFramework/Icons/GizmoNone24.png");
  }

  UpdateState();
}

xiiGizmoAction::~xiiGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiGizmoAction::GameObjectEventHandler, this));
}

void xiiGizmoAction::Execute(const xiiVariant& value)
{
  m_pGameObjectDocument->SetActiveEditTool(m_pGizmoType);
  UpdateState();
}

void xiiGizmoAction::UpdateState()
{
  SetChecked(m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType));
}

void xiiGizmoAction::GameObjectEventHandler(const xiiGameObjectEvent& e)
{
  if (e.m_Type == xiiGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

//////////////////////////////////////////////////////////////////////////

xiiToggleWorldSpaceGizmo::xiiToggleWorldSpaceGizmo(const xiiActionContext& context, const char* szName, const xiiRTTI* pGizmoType) :
  xiiGizmoAction(context, szName, pGizmoType)
{
}

void xiiToggleWorldSpaceGizmo::Execute(const xiiVariant& value)
{
  if (m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType))
  {
    // toggle local/world space if the same tool is selected again
    m_pGameObjectDocument->SetGizmoWorldSpace(!m_pGameObjectDocument->GetGizmoWorldSpace());
  }
  else
  {
    xiiGizmoAction::Execute(value);
  }
}

//////////////////////////////////////////////////////////////////////////

xiiActionDescriptorHandle xiiTransformGizmoActions::s_hGizmoCategory;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hGizmoMenu;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hNoGizmo;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hTranslateGizmo;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hRotateGizmo;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hScaleGizmo;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hDragToPositionGizmo;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hWorldSpace;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hMoveParentOnly;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_SnapSettings;

void xiiTransformGizmoActions::RegisterActions()
{
  s_hGizmoCategory  = XII_REGISTER_CATEGORY("GizmoCategory");
  s_hGizmoMenu      = XII_REGISTER_MENU("Gizmo.Menu");
  s_hNoGizmo        = XII_REGISTER_ACTION_1("Gizmo.Mode.Select", xiiActionScope::Document, "Gizmo", "Q", xiiGizmoAction, nullptr);
  s_hTranslateGizmo = XII_REGISTER_ACTION_1(
    "Gizmo.Mode.Translate", xiiActionScope::Document, "Gizmo", "W", xiiToggleWorldSpaceGizmo, xiiGetStaticRTTI<xiiTranslateGizmoEditTool>());
  s_hRotateGizmo = XII_REGISTER_ACTION_1(
    "Gizmo.Mode.Rotate", xiiActionScope::Document, "Gizmo", "E", xiiToggleWorldSpaceGizmo, xiiGetStaticRTTI<xiiRotateGizmoEditTool>());
  s_hScaleGizmo =
    XII_REGISTER_ACTION_1("Gizmo.Mode.Scale", xiiActionScope::Document, "Gizmo", "R", xiiGizmoAction, xiiGetStaticRTTI<xiiScaleGizmoEditTool>());
  s_hDragToPositionGizmo = XII_REGISTER_ACTION_1(
    "Gizmo.Mode.DragToPosition", xiiActionScope::Document, "Gizmo", "T", xiiGizmoAction, xiiGetStaticRTTI<xiiDragToPositionGizmoEditTool>());
  s_hWorldSpace = XII_REGISTER_ACTION_1(
    "Gizmo.TransformSpace", xiiActionScope::Document, "Gizmo", "", xiiTransformGizmoAction, xiiTransformGizmoAction::ActionType::GizmoToggleWorldSpace);
  s_hMoveParentOnly = XII_REGISTER_ACTION_1("Gizmo.MoveParentOnly", xiiActionScope::Document, "Gizmo", "", xiiTransformGizmoAction,
                                            xiiTransformGizmoAction::ActionType::GizmoToggleMoveParentOnly);
  s_SnapSettings    = XII_REGISTER_ACTION_1(
    "Gizmo.SnapSettings", xiiActionScope::Document, "Gizmo", "End", xiiTransformGizmoAction, xiiTransformGizmoAction::ActionType::GizmoSnapSettings);
}

void xiiTransformGizmoActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hGizmoCategory);
  xiiActionManager::UnregisterAction(s_hGizmoMenu);
  xiiActionManager::UnregisterAction(s_hNoGizmo);
  xiiActionManager::UnregisterAction(s_hTranslateGizmo);
  xiiActionManager::UnregisterAction(s_hRotateGizmo);
  xiiActionManager::UnregisterAction(s_hScaleGizmo);
  xiiActionManager::UnregisterAction(s_hDragToPositionGizmo);
  xiiActionManager::UnregisterAction(s_hWorldSpace);
  xiiActionManager::UnregisterAction(s_hMoveParentOnly);
  xiiActionManager::UnregisterAction(s_SnapSettings);
}

void xiiTransformGizmoActions::MapMenuActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  xiiStringBuilder sSubPath(szPath, "/Gizmo.Menu");

  pMap->MapAction(s_hGizmoMenu, szPath, 4.0f);
  pMap->MapAction(s_hNoGizmo, sSubPath, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sSubPath, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sSubPath, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sSubPath, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sSubPath, 4.0f);
  pMap->MapAction(s_hWorldSpace, sSubPath, 6.0f);
  pMap->MapAction(s_hMoveParentOnly, sSubPath, 7.0f);
  pMap->MapAction(s_SnapSettings, sSubPath, 8.0f);
}

void xiiTransformGizmoActions::MapToolbarActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  xiiStringBuilder sSubPath(szPath, "/GizmoCategory");

  pMap->MapAction(s_hGizmoCategory, szPath, 4.0f);
  pMap->MapAction(s_hNoGizmo, sSubPath, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sSubPath, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sSubPath, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sSubPath, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sSubPath, 4.0f);
  pMap->MapAction(s_hWorldSpace, sSubPath, 6.0f);
  pMap->MapAction(s_SnapSettings, sSubPath, 7.0f);
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransformGizmoAction, 0, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiTransformGizmoAction::xiiTransformGizmoAction(const xiiActionContext& context, const char* szName, ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type                = type;
  m_pGameObjectDocument = static_cast<xiiGameObjectDocument*>(context.m_pDocument);

  switch (m_Type)
  {
    case ActionType::GizmoToggleWorldSpace:
      SetIconPath(":/EditorFramework/Icons/WorldSpace16.png");
      break;
    case ActionType::GizmoToggleMoveParentOnly:
      SetIconPath(":/EditorFramework/Icons/TransformParent16.png");
      break;
    case ActionType::GizmoSnapSettings:
      SetCheckable(false);
      SetIconPath(":/EditorFramework/Icons/SnapSettings16.png");
      break;
  }

  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiTransformGizmoAction::GameObjectEventHandler, this));
  UpdateState();
}

xiiTransformGizmoAction::~xiiTransformGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiTransformGizmoAction::GameObjectEventHandler, this));
}

void xiiTransformGizmoAction::Execute(const xiiVariant& value)
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    m_pGameObjectDocument->SetGizmoWorldSpace(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    m_pGameObjectDocument->SetGizmoMoveParentOnly(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoSnapSettings)
  {
    xiiQtSnapSettingsDlg dlg(nullptr);
    dlg.exec();
  }

  UpdateState();
}

void xiiTransformGizmoAction::GameObjectEventHandler(const xiiGameObjectEvent& e)
{
  if (e.m_Type == xiiGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

void xiiTransformGizmoAction::UpdateState()
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    xiiGameObjectEditTool* pTool = m_pGameObjectDocument->GetActiveEditTool();
    SetEnabled(pTool != nullptr && pTool->GetSupportedSpaces() == xiiEditToolSupportedSpaces::LocalAndWorldSpace);

    if (pTool != nullptr)
    {
      switch (pTool->GetSupportedSpaces())
      {
        case xiiEditToolSupportedSpaces::LocalSpaceOnly:
          SetChecked(false);
          break;
        case xiiEditToolSupportedSpaces::WorldSpaceOnly:
          SetChecked(true);
          break;
        case xiiEditToolSupportedSpaces::LocalAndWorldSpace:
          SetChecked(m_pGameObjectDocument->GetGizmoWorldSpace());
          break;
      }
    }
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    xiiGameObjectEditTool* pTool      = m_pGameObjectDocument->GetActiveEditTool();
    const bool             bSupported = pTool != nullptr && pTool->GetSupportsMoveParentOnly();

    SetEnabled(bSupported);
    SetChecked(bSupported && m_pGameObjectDocument->GetGizmoMoveParentOnly());
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTranslateGizmoAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiTranslateGizmoAction::s_hSnappingValueMenu;
xiiActionDescriptorHandle xiiTranslateGizmoAction::s_hSnapPivotToGrid;
xiiActionDescriptorHandle xiiTranslateGizmoAction::s_hSnapObjectsToGrid;

void xiiTranslateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = XII_REGISTER_CATEGORY("Gizmo.Translate.Snap.Menu");
  s_hSnapPivotToGrid   = XII_REGISTER_ACTION_1("Gizmo.Translate.Snap.PivotToGrid", xiiActionScope::Document, "Gizmo - Position Snap", "Ctrl+End",
                                             xiiTranslateGizmoAction, xiiTranslateGizmoAction::ActionType::SnapSelectionPivotToGrid);
  s_hSnapObjectsToGrid = XII_REGISTER_ACTION_1("Gizmo.Translate.Snap.ObjectsToGrid", xiiActionScope::Document, "Gizmo - Position Snap", "",
                                               xiiTranslateGizmoAction, xiiTranslateGizmoAction::ActionType::SnapEachSelectedObjectToGrid);
}

void xiiTranslateGizmoAction::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hSnappingValueMenu);
  xiiActionManager::UnregisterAction(s_hSnapPivotToGrid);
  xiiActionManager::UnregisterAction(s_hSnapObjectsToGrid);
}

void xiiTranslateGizmoAction::MapActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  xiiStringBuilder sSubPath(szPath, "/Gizmo.Translate.Snap.Menu");

  pMap->MapAction(s_hSnappingValueMenu, szPath, 8.0f);

  pMap->MapAction(s_hSnapPivotToGrid, sSubPath, 0.0f);
  pMap->MapAction(s_hSnapObjectsToGrid, sSubPath, 1.0f);
}

xiiTranslateGizmoAction::xiiTranslateGizmoAction(const xiiActionContext& context, const char* szName, ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  m_pSceneDocument = static_cast<const xiiGameObjectDocument*>(context.m_pDocument);
  m_Type           = type;
}

void xiiTranslateGizmoAction::Execute(const xiiVariant& value)
{
  if (m_Type == ActionType::SnapSelectionPivotToGrid)
    m_pSceneDocument->TriggerSnapPivotToGrid();

  if (m_Type == ActionType::SnapEachSelectedObjectToGrid)
    m_pSceneDocument->TriggerSnapEachObjectToGrid();
}
