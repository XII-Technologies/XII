#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

xiiActionDescriptorHandle xiiQuadViewActions::s_hToggleViews;
xiiActionDescriptorHandle xiiQuadViewActions::s_hSpawnView;


void xiiQuadViewActions::RegisterActions()
{
  s_hToggleViews =
    XII_REGISTER_ACTION_1("Scene.View.Toggle", xiiActionScope::Window, "Scene", "", xiiQuadViewAction, xiiQuadViewAction::ButtonType::ToggleViews);
  s_hSpawnView =
    XII_REGISTER_ACTION_1("Scene.View.Span", xiiActionScope::Window, "Scene", "", xiiQuadViewAction, xiiQuadViewAction::ButtonType::SpawnView);
}

void xiiQuadViewActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hToggleViews);
  xiiActionManager::UnregisterAction(s_hSpawnView);
}

void xiiQuadViewActions::MapActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hToggleViews, szPath, 3.0f);
  // pMap->MapAction(s_hSpawnView, szPath, 4.0f);
}

////////////////////////////////////////////////////////////////////////
// xiiSceneViewAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiQuadViewAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiQuadViewAction::xiiQuadViewAction(const xiiActionContext& context, const char* szName, ButtonType button) :
  xiiButtonAction(context, szName, false, "")
{
  m_ButtonType                 = button;
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(context.m_pWindow);
  XII_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'xiiQtGameObjectViewWidget'!");
  switch (m_ButtonType)
  {
    case ButtonType::ToggleViews:
      SetIconPath(":/EditorFramework/Icons/ToggleViews16.png");
      break;
    case ButtonType::SpawnView:
      SetIconPath(":/EditorFramework/Icons/SpawnView16.png");
      break;
  }
}

xiiQuadViewAction::~xiiQuadViewAction() {}

void xiiQuadViewAction::Execute(const xiiVariant& value)
{
  xiiQtEngineViewWidget*     pView   = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);
  xiiQtEngineDocumentWindow* pWindow = static_cast<xiiQtEngineDocumentWindow*>(pView->GetDocumentWindow());

  switch (m_ButtonType)
  {
    case ButtonType::ToggleViews:
      // Duck-typing to the rescue!
      QMetaObject::invokeMethod(pWindow, "ToggleViews", Qt::ConnectionType::QueuedConnection, Q_ARG(QWidget*, pView));
      break;
    case ButtonType::SpawnView:
      break;
  }
}
