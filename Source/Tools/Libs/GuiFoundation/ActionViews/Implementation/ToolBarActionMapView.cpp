/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMenu>
#include <QToolButton>

xiiQtToolBarActionMapView::xiiQtToolBarActionMapView(QString sTitle, QWidget* pParent) :
  QToolBar(sTitle, pParent)
{
  setIconSize(QSize(16, 16));
  setFloatable(false);

  toggleViewAction()->setEnabled(false);
}

xiiQtToolBarActionMapView::~xiiQtToolBarActionMapView()
{
  ClearView();
}

void xiiQtToolBarActionMapView::SetActionContext(const xiiActionContext& context)
{
  auto pMap = xiiActionMapManager::GetActionMap(context.m_sMapping);

  XII_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping);

  m_pActionMap = pMap;
  m_Context    = context;

  CreateView();
}

void xiiQtToolBarActionMapView::setVisible(bool bVisible)
{
  QToolBar::setVisible(true);
}

void xiiQtToolBarActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void xiiQtToolBarActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->BuildActionTree();

  CreateView(pObject);

  if (!actions().isEmpty() && actions().back()->isSeparator())
  {
    QAction* pAction = actions().back();
    removeAction(pAction);
    pAction->deleteLater();
  }
}

void xiiQtToolBarActionMapView::CreateView(const xiiActionMap::TreeNode* pObject)
{
  for (auto pChild : pObject->GetChildren())
  {
    auto                       pDesc  = m_pActionMap->GetDescriptor(pChild);
    QSharedPointer<xiiQtProxy> pProxy = xiiQtProxy::GetProxy(m_Context, pDesc->m_hAction);
    m_Proxies[pChild->GetGuid()]      = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
      case xiiActionType::Action:
      {
        QAction* pQtAction = static_cast<xiiQtActionProxy*>(pProxy.data())->GetQAction();
        addAction(pQtAction);
      }
      break;

      case xiiActionType::Category:
      {
        if (!actions().isEmpty() && !actions().back()->isSeparator())
          addSeparator()->setParent(pProxy.data());

        CreateView(pChild);

        if (!actions().isEmpty() && !actions().back()->isSeparator())
          addSeparator()->setParent(pProxy.data());
      }
      break;

      case xiiActionType::Menu:
      {
        xiiNamedAction* pNamed = static_cast<xiiNamedAction*>(pProxy->GetAction());

        QMenu* pQtMenu = static_cast<xiiQtMenuProxy*>(pProxy.data())->GetQMenu();
        // TODO pButton leaks!
        QToolButton* pButton = new QToolButton(this);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        pButton->setText(pQtMenu->title());
        pButton->setIcon(xiiQtUiServices::GetCachedIconResource(pNamed->GetIconPath()));
        pButton->setToolTip(pQtMenu->toolTip());

        pNamed->m_StatusUpdateEvent.AddEventHandler([=](xiiAction* pAction) {
          pButton->setIcon(xiiQtUiServices::GetCachedIconResource(pNamed->GetIconPath()));
        });

        // TODO addWidget return value of QAction leaks!
        QAction* pToolButtonAction = addWidget(pButton);
        pToolButtonAction->setParent(pQtMenu);

        xiiQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;

      case xiiActionType::ActionAndMenu:
      {
        QMenu*   pQtMenu   = static_cast<xiiQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQMenu();
        QAction* pQtAction = static_cast<xiiQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQAction();

        // TODO pButton leaks!
        QToolButton* pButton = new QToolButton(this);
        pButton->setDefaultAction(pQtAction);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);

        // TODO addWidget return value of QAction leaks!
        QAction* pToolButtonAction = addWidget(pButton);
        pToolButtonAction->setParent(pQtMenu);

        xiiQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}
