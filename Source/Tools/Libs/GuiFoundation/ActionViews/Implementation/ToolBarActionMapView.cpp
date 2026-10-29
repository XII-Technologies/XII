/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QVBoxLayout>

xiiQtToolBarActionMapView::xiiQtToolBarActionMapView(QString sTitle, QWidget* pParent) :
  QToolBar(sTitle, pParent)
{
  setIconSize(QSize(16, 16));
  setFloatable(false);
  setMovable(false);

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
  QToolBar::setVisible(bVisible);
}

void xiiQtToolBarActionMapView::ClearActionContext()
{
  ClearView();
  m_pActionMap = nullptr;
  m_Context    = {};
}

void xiiQtToolBarActionMapView::SetShowMenuLabels(bool bShow)
{
  m_bShowMenuLabels = bShow;
  for (auto* pAction : actions())
    if (auto* pButton = qobject_cast<QToolButton*>(widgetForAction(pAction)); pButton && pButton->menu())
      pButton->setToolButtonStyle(bShow ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
}

void xiiQtToolBarActionMapView::ClearView()
{
  clear();
  qDeleteAll(m_OwnedActions);
  m_OwnedActions.clear();
  m_Proxies.Clear();
}

void xiiQtToolBarActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->BuildActionTree();

  if (m_bGrouped)
  {
    for (auto* pChild : pObject->GetChildren())
    {
      auto* pDescriptor = m_pActionMap->GetDescriptor(pChild);
      if (pDescriptor->m_hAction.GetDescriptor()->m_Type != xiiActionType::Category)
      {
        // Root-level actions still use their original mapping, below the groups.
        continue;
      }
      auto pProxy                  = xiiQtProxy::GetProxy(m_Context, pDescriptor->m_hAction);
      m_Proxies[pChild->GetGuid()] = pProxy;
      auto* pGroup                 = new QWidget(this);
      pGroup->setObjectName(QStringLiteral("CommandDeckGroup"));
      auto* pLayout = new QVBoxLayout(pGroup);
      pLayout->setContentsMargins(8, 4, 8, 3);
      pLayout->setSpacing(2);
      auto* pTools = new QToolBar(pGroup);
      pTools->setIconSize(QSize(20, 20));
      pTools->setMovable(false);
      pTools->setFloatable(false);
      pTools->setToolButtonStyle(Qt::ToolButtonIconOnly);
      CreateView(pChild, pTools);
      pLayout->addWidget(pTools);
      const auto& name   = pDescriptor->m_hAction.GetDescriptor()->m_sActionName;
      QString     sTitle = xiiMakeQString(xiiTranslate(name));
      sTitle.replace(QStringLiteral("Category"), QString());
      sTitle = sTitle.trimmed();
      if (name == "GizmoCategory")
        sTitle = tr("Transform");
      if (name == "SceneCategory")
        sTitle = tr("Run");
      if (name == "CmdHistoryCategory")
        sTitle = tr("History");
      if (name == "AssetCategory")
        sTitle = tr("Build");
      auto* pLabel = new QLabel(sTitle, pGroup);
      pLabel->setAlignment(Qt::AlignCenter);
      pLabel->setForegroundRole(QPalette::PlaceholderText);
      pLayout->addWidget(pLabel);
      m_OwnedActions.push_back(addWidget(pGroup));
    }
    CreateView(pObject, this);
  }
  else
    CreateView(pObject, this);

  if (!actions().isEmpty() && actions().back()->isSeparator())
  {
    QAction* pAction = actions().back();
    removeAction(pAction);
    m_OwnedActions.removeOne(pAction);
    pAction->deleteLater();
  }
}

void xiiQtToolBarActionMapView::CreateView(const xiiActionMap::TreeNode* pObject, QToolBar* pTarget)
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
        pTarget->addAction(pQtAction);
        if (m_bGrouped)
        {
          const auto name = xiiMakeQString(pDesc->m_hAction.GetDescriptor()->m_sActionName);
          if (name.startsWith(QStringLiteral("Scene.GameMode.")) || name == QStringLiteral("Document.Save") ||
              name == QStringLiteral("Gizmo.Translate") || name == QStringLiteral("Gizmo.Rotate") || name == QStringLiteral("Gizmo.Scale"))
            if (auto* pButton = qobject_cast<QToolButton*>(pTarget->widgetForAction(pQtAction)))
              pButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        }
      }
      break;

      case xiiActionType::Category:
      {
        if (m_bGrouped && pTarget == this)
          break;
        if (!pTarget->actions().isEmpty() && !pTarget->actions().back()->isSeparator())
          m_OwnedActions.push_back(pTarget->addSeparator());
        CreateView(pChild, pTarget);
      }
      break;

      case xiiActionType::Menu:
      {
        xiiNamedAction* pNamed = static_cast<xiiNamedAction*>(pProxy->GetAction());

        QMenu*       pQtMenu = static_cast<xiiQtMenuProxy*>(pProxy.data())->GetQMenu();
        QToolButton* pButton = new QToolButton(pTarget);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        pButton->setText(pQtMenu->title());
        pButton->setIcon(xiiQtUiServices::GetCachedIconResource(pNamed->GetIconPath()));
        pButton->setToolTip(pQtMenu->toolTip());
        pButton->setToolButtonStyle(m_bShowMenuLabels ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
        connect(pQtMenu->menuAction(), &QAction::changed, pButton, [pButton, pQtMenu]() {
          pButton->setText(pQtMenu->title());
          pButton->setIcon(pQtMenu->icon());
          pButton->setToolTip(pQtMenu->toolTip());
        });

        QAction* pToolButtonAction = pTarget->addWidget(pButton);
        m_OwnedActions.push_back(pToolButtonAction);

        // A second presentation can borrow the same scoped menu. Do not append
        // its static children again when mirroring a document toolbar.
        if (pQtMenu->actions().isEmpty())
          xiiQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;

      case xiiActionType::ActionAndMenu:
      {
        QMenu*   pQtMenu   = static_cast<xiiQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQMenu();
        QAction* pQtAction = static_cast<xiiQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQAction();

        QToolButton* pButton = new QToolButton(pTarget);
        pButton->setDefaultAction(pQtAction);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);
        pButton->setToolButtonStyle(m_bShowMenuLabels ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);

        QAction* pToolButtonAction = pTarget->addWidget(pButton);
        m_OwnedActions.push_back(pToolButtonAction);

        if (pQtMenu->actions().isEmpty())
          xiiQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}
