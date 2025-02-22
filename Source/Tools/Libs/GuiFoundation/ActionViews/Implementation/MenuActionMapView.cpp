#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

xiiQtMenuActionMapView::xiiQtMenuActionMapView(QWidget* pParent)
{
  setToolTipsVisible(true);
}

xiiQtMenuActionMapView::~xiiQtMenuActionMapView()
{
  ClearView();
}

void xiiQtMenuActionMapView::SetActionContext(const xiiActionContext& context)
{
  auto pMap = xiiActionMapManager::GetActionMap(context.m_sMapping);

  XII_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping);

  m_pActionMap = pMap;
  m_Context    = context;

  CreateView();
}

void xiiQtMenuActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void xiiQtMenuActionMapView::AddDocumentObjectToMenu(xiiHashTable<xiiUuid, QSharedPointer<xiiQtProxy>>& ref_proxies, xiiActionContext& ref_context, xiiActionMap* pActionMap, QMenu* pCurrentRoot, const xiiActionMap::TreeNode* pObject)
{
  if (pObject == nullptr)
    return;

  for (auto pChild : pObject->GetChildren())
  {
    auto                       pDesc  = pActionMap->GetDescriptor(pChild);
    QSharedPointer<xiiQtProxy> pProxy = xiiQtProxy::GetProxy(ref_context, pDesc->m_hAction);
    ref_proxies[pChild->GetGuid()]    = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
      case xiiActionType::Action:
      {
        QAction* pQtAction = static_cast<xiiQtActionProxy*>(pProxy.data())->GetQAction();
        pCurrentRoot->addAction(pQtAction);
      }
      break;

      case xiiActionType::Category:
      {
        pCurrentRoot->addSeparator();

        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pCurrentRoot, pChild);

        pCurrentRoot->addSeparator();
      }
      break;

      case xiiActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<xiiQtMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pQtMenu, pChild);
      }
      break;

      case xiiActionType::ActionAndMenu:
      {
        QAction* pQtAction = static_cast<xiiQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQAction();
        QMenu*   pQtMenu   = static_cast<xiiQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addAction(pQtAction);
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}

void xiiQtMenuActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->BuildActionTree();

  AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, this, pObject);
}
