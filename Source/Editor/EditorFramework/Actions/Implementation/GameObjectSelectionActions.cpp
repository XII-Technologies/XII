#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Document/GameObjectDocument.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectSelectionAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiActionDescriptorHandle xiiGameObjectSelectionActions::s_hSelectionCategory;
xiiActionDescriptorHandle xiiGameObjectSelectionActions::s_hShowInScenegraph;
xiiActionDescriptorHandle xiiGameObjectSelectionActions::s_hFocusOnSelection;
xiiActionDescriptorHandle xiiGameObjectSelectionActions::s_hFocusOnSelectionAllViews;
xiiActionDescriptorHandle xiiGameObjectSelectionActions::s_hSnapCameraToObject;
xiiActionDescriptorHandle xiiGameObjectSelectionActions::s_hMoveCameraHere;
xiiActionDescriptorHandle xiiGameObjectSelectionActions::s_hCreateEmptyGameObjectHere;

void xiiGameObjectSelectionActions::RegisterActions()
{
  s_hSelectionCategory        = XII_REGISTER_CATEGORY("G.Selection");
  s_hShowInScenegraph         = XII_REGISTER_ACTION_1("Selection.ShowInScenegraph", xiiActionScope::Document, "Scene - Selection", "Ctrl+T",
                                              xiiGameObjectSelectionAction, xiiGameObjectSelectionAction::ActionType::ShowInScenegraph);
  s_hFocusOnSelection         = XII_REGISTER_ACTION_1("Selection.FocusSingleView", xiiActionScope::Document, "Scene - Selection", "F",
                                              xiiGameObjectSelectionAction, xiiGameObjectSelectionAction::ActionType::FocusOnSelection);
  s_hFocusOnSelectionAllViews = XII_REGISTER_ACTION_1("Selection.FocusAllViews", xiiActionScope::Document, "Scene - Selection", "Shift+F",
                                                      xiiGameObjectSelectionAction, xiiGameObjectSelectionAction::ActionType::FocusOnSelectionAllViews);
  s_hSnapCameraToObject       = XII_REGISTER_ACTION_1("Scene.Camera.SnapCameraToObject", xiiActionScope::Document, "Camera", "", xiiGameObjectSelectionAction,
                                                xiiGameObjectSelectionAction::ActionType::SnapCameraToObject);
  s_hMoveCameraHere           = XII_REGISTER_ACTION_1("Scene.Camera.MoveCameraHere", xiiActionScope::Document, "Camera", "C", xiiGameObjectSelectionAction,
                                            xiiGameObjectSelectionAction::ActionType::MoveCameraHere);

  s_hCreateEmptyGameObjectHere = XII_REGISTER_ACTION_1("Scene.GameObject.CreateEmptyHere", xiiActionScope::Document, "Scene", "",
                                                       xiiGameObjectSelectionAction, xiiGameObjectSelectionAction::ActionType::CreateGameObjectHere);
}

void xiiGameObjectSelectionActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hSelectionCategory);
  xiiActionManager::UnregisterAction(s_hShowInScenegraph);
  xiiActionManager::UnregisterAction(s_hFocusOnSelection);
  xiiActionManager::UnregisterAction(s_hFocusOnSelectionAllViews);
  xiiActionManager::UnregisterAction(s_hSnapCameraToObject);
  xiiActionManager::UnregisterAction(s_hMoveCameraHere);
  xiiActionManager::UnregisterAction(s_hCreateEmptyGameObjectHere);
}

void xiiGameObjectSelectionActions::MapActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectionCategory, "G.Edit", 5.0f);

  pMap->MapAction(s_hShowInScenegraph, "G.Selection", 2.0f);
  pMap->MapAction(s_hFocusOnSelection, "G.Selection", 3.0f);
  pMap->MapAction(s_hFocusOnSelectionAllViews, "G.Selection", 3.5f);
  pMap->MapAction(s_hSnapCameraToObject, "G.Selection", 8.0f);
  pMap->MapAction(s_hMoveCameraHere, "G.Selection", 10.0f);
}

void xiiGameObjectSelectionActions::MapContextMenuActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectionCategory, "", 5.0f);

  pMap->MapAction(s_hFocusOnSelectionAllViews, "G.Selection", 1.0f);
}


void xiiGameObjectSelectionActions::MapViewContextMenuActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectionCategory, "", 5.0f);

  pMap->MapAction(s_hFocusOnSelectionAllViews, "G.Selection", 1.0f);
  pMap->MapAction(s_hSnapCameraToObject, "G.Selection", 4.0f);
  pMap->MapAction(s_hMoveCameraHere, "G.Selection", 6.0f);
  pMap->MapAction(s_hCreateEmptyGameObjectHere, "G.Selection", 1.0f);
}

xiiGameObjectSelectionAction::xiiGameObjectSelectionAction(
  const xiiActionContext&                  context,
  const char*                              szName,
  xiiGameObjectSelectionAction::ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<xiiGameObjectDocument*>(static_cast<const xiiGameObjectDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::ShowInScenegraph:
      SetIconPath(":/EditorFramework/Icons/Scenegraph.svg");
      break;
    case ActionType::FocusOnSelection:
      SetIconPath(":/EditorFramework/Icons/FocusOnSelection.svg");
      break;
    case ActionType::FocusOnSelectionAllViews:
      SetIconPath(":/EditorFramework/Icons/FocusOnSelectionAllViews.svg");
      break;
    case ActionType::SnapCameraToObject:
      // SetIconPath(":/EditorFramework/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::MoveCameraHere:
      // SetIconPath(":/EditorFramework/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::CreateGameObjectHere:
      SetIconPath(":/EditorFramework/Icons/CreateEmpty.svg");
      break;
  }

  UpdateEnableState();

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiGameObjectSelectionAction::SelectionEventHandler, this));
}


xiiGameObjectSelectionAction::~xiiGameObjectSelectionAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(
    xiiMakeDelegate(&xiiGameObjectSelectionAction::SelectionEventHandler, this));
}

void xiiGameObjectSelectionAction::Execute(const xiiVariant& value)
{
  switch (m_Type)
  {
    case ActionType::ShowInScenegraph:
      m_pSceneDocument->TriggerShowSelectionInScenegraph();
      return;
    case ActionType::FocusOnSelection:
      m_pSceneDocument->TriggerFocusOnSelection(false);
      return;
    case ActionType::FocusOnSelectionAllViews:
      m_pSceneDocument->TriggerFocusOnSelection(true);
      return;
    case ActionType::SnapCameraToObject:
      m_pSceneDocument->SnapCameraToObject();
      break;
    case ActionType::MoveCameraHere:
      m_pSceneDocument->MoveCameraHere();
      break;
    case ActionType::CreateGameObjectHere:
    {
      auto res = m_pSceneDocument->CreateGameObjectHere();
      xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Create empty object at picked position failed.");
    }
    break;
  }
}

void xiiGameObjectSelectionAction::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  UpdateEnableState();
}

void xiiGameObjectSelectionAction::UpdateEnableState()
{
  if (m_Type == ActionType::FocusOnSelection || m_Type == ActionType::FocusOnSelectionAllViews || m_Type == ActionType::ShowInScenegraph)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }

  if (m_Type == ActionType::SnapCameraToObject)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() == 1);
  }
}
