#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

xiiQtMenuBarActionMapView::xiiQtMenuBarActionMapView(QWidget* pParent) :
  QMenuBar(pParent)
{
}

xiiQtMenuBarActionMapView::~xiiQtMenuBarActionMapView()
{
  ClearView();
}

void xiiQtMenuBarActionMapView::SetActionContext(const xiiActionContext& context)
{
  auto pMap = xiiActionMapManager::GetActionMap(context.m_sMapping);

  XII_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping);

  m_pActionMap = pMap;
  m_Context    = context;

  CreateView();
}

void xiiQtMenuBarActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void xiiQtMenuBarActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = m_pActionMap->GetDescriptor(pChild);

    QSharedPointer<xiiQtProxy> pProxy = xiiQtProxy::GetProxy(m_Context, pDesc->m_hAction);
    m_Proxies[pChild->GetGuid()]      = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
      case xiiActionType::Action:
      {
        XII_REPORT_FAILURE("Cannot map actions in a menubar view!");
      }
      break;

      case xiiActionType::Category:
      {
        XII_REPORT_FAILURE("Cannot map category in a menubar view!");
      }
      break;

      case xiiActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<xiiQtMenuProxy*>(pProxy.data())->GetQMenu();
        addMenu(pQtMenu);
        xiiQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;

      case xiiActionType::ActionAndMenu:
      {
        XII_REPORT_FAILURE("Cannot map ActionAndMenu in a menubar view!");
      }
      break;
    }
  }
}
