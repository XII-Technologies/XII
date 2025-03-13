#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QAction>
#include <QBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSlider>
#include <QWidgetAction>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

xiiRttiMappedObjectFactory<xiiQtProxy>                                                  xiiQtProxy::s_Factory;
xiiMap<xiiActionDescriptorHandle, QWeakPointer<xiiQtProxy>>                             xiiQtProxy::s_GlobalActions;
xiiMap<const xiiDocument*, xiiMap<xiiActionDescriptorHandle, QWeakPointer<xiiQtProxy>>> xiiQtProxy::s_DocumentActions;
xiiMap<QWidget*, xiiMap<xiiActionDescriptorHandle, QWeakPointer<xiiQtProxy>>>           xiiQtProxy::s_WindowActions;
QObject*                                                                                xiiQtProxy::s_pSignalProxy = nullptr;

static xiiQtProxy* QtMenuProxyCreator(const xiiRTTI* pRtti)
{
  return new (xiiQtMenuProxy);
}

static xiiQtProxy* QtCategoryProxyCreator(const xiiRTTI* pRtti)
{
  return new (xiiQtCategoryProxy);
}

static xiiQtProxy* QtButtonProxyCreator(const xiiRTTI* pRtti)
{
  return new (xiiQtButtonProxy);
}

static xiiQtProxy* QtDynamicMenuProxyCreator(const xiiRTTI* pRtti)
{
  return new (xiiQtDynamicMenuProxy);
}

static xiiQtProxy* QtDynamicActionAndMenuProxyCreator(const xiiRTTI* pRtti)
{
  return new (xiiQtDynamicActionAndMenuProxy);
}

static xiiQtProxy* QtSliderProxyCreator(const xiiRTTI* pRtti)
{
  return new (xiiQtSliderProxy);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, QtProxies)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation",
  "ActionManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiQtProxy::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiMenuAction>(), QtMenuProxyCreator);
    xiiQtProxy::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiCategoryAction>(), QtCategoryProxyCreator);
    xiiQtProxy::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiDynamicMenuAction>(), QtDynamicMenuProxyCreator);
    xiiQtProxy::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiDynamicActionAndMenuAction>(), QtDynamicActionAndMenuProxyCreator);
    xiiQtProxy::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiButtonAction>(), QtButtonProxyCreator);
    xiiQtProxy::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiSliderAction>(), QtSliderProxyCreator);
    xiiQtProxy::s_pSignalProxy = new QObject;
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiQtProxy::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiMenuAction>());
    xiiQtProxy::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiCategoryAction>());
    xiiQtProxy::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiDynamicMenuAction>());
    xiiQtProxy::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiDynamicActionAndMenuAction>());
    xiiQtProxy::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiButtonAction>());
    xiiQtProxy::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiSliderAction>());
    xiiQtProxy::s_GlobalActions.Clear();
    xiiQtProxy::s_DocumentActions.Clear();
    xiiQtProxy::s_WindowActions.Clear();
    delete xiiQtProxy::s_pSignalProxy;
    xiiQtProxy::s_pSignalProxy = nullptr;
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool xiiQtProxy::TriggerDocumentAction(xiiDocument* pDocument, QKeyEvent* pEvent, bool bTestOnly)
{
  auto CheckActions = [&](QKeyEvent* pEvent, xiiMap<xiiActionDescriptorHandle, QWeakPointer<xiiQtProxy>>& ref_actions) -> bool {
    for (auto weakActionProxy : ref_actions)
    {
      if (auto pProxy = weakActionProxy.Value().toStrongRef())
      {
        QAction* pQAction = nullptr;
        if (auto pActionProxy = qobject_cast<xiiQtActionProxy*>(pProxy))
        {
          pQAction = pActionProxy->GetQAction();
        }
        else if (auto pActionProxy2 = qobject_cast<xiiQtDynamicActionAndMenuProxy*>(pProxy))
        {
          pQAction = pActionProxy2->GetQAction();
        }

        if (pQAction)
        {
          QKeySequence ks = pQAction->shortcut();
          if (pQAction->isEnabled() && QKeySequence(pEvent->key() | pEvent->modifiers()) == ks)
          {
            if (!bTestOnly)
            {
              pQAction->trigger();
            }
            pEvent->accept();
            return true;
          }
        }
      }
    }
    return false;
  };

  if (pDocument)
  {
    xiiMap<xiiActionDescriptorHandle, QWeakPointer<xiiQtProxy>>& actions = s_DocumentActions[pDocument];
    if (CheckActions(pEvent, actions))
      return true;
  }
  return CheckActions(pEvent, s_GlobalActions);
}

xiiRttiMappedObjectFactory<xiiQtProxy>& xiiQtProxy::GetFactory()
{
  return s_Factory;
}

QSharedPointer<xiiQtProxy> xiiQtProxy::GetProxy(xiiActionContext& ref_context, xiiActionDescriptorHandle hDesc)
{
  QSharedPointer<xiiQtProxy> pProxy;
  const xiiActionDescriptor* pDesc = hDesc.GetDescriptor();
  if (pDesc->m_Type != xiiActionType::Action && pDesc->m_Type != xiiActionType::ActionAndMenu)
  {
    auto pAction = pDesc->CreateAction(ref_context);
    pProxy       = QSharedPointer<xiiQtProxy>(xiiQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
    XII_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
    pProxy->SetAction(pAction);
    XII_ASSERT_DEV(pProxy->GetAction()->GetContext().m_pDocument == ref_context.m_pDocument, "invalid document pointer");
    return pProxy;
  }

  // xiiActionType::Action will be cached to ensure only one QAction exist in its scope to prevent shortcut collisions.
  switch (pDesc->m_Scope)
  {
    case xiiActionScope::Global:
    {
      QWeakPointer<xiiQtProxy> pTemp = s_GlobalActions[hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(ref_context);
        pProxy       = QSharedPointer<xiiQtProxy>(xiiQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        XII_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
        pProxy->SetAction(pAction);
        s_GlobalActions[hDesc] = pProxy.toWeakRef();
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }

      break;
    }

    case xiiActionScope::Document:
    {
      const xiiDocument* pDocument = ref_context.m_pDocument; // may be null

      QWeakPointer<xiiQtProxy> pTemp = s_DocumentActions[pDocument][hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(ref_context);
        pProxy       = QSharedPointer<xiiQtProxy>(xiiQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        XII_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
        pProxy->SetAction(pAction);
        s_DocumentActions[pDocument][hDesc] = pProxy;
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }

      break;
    }

    case xiiActionScope::Window:
    {
      bool bExisted = true;
      auto it       = s_WindowActions.FindOrAdd(ref_context.m_pWindow, &bExisted);
      if (!bExisted)
      {
        s_pSignalProxy->connect(ref_context.m_pWindow, &QObject::destroyed, s_pSignalProxy, [ref_context]() { s_WindowActions.Remove(ref_context.m_pWindow); });
      }
      QWeakPointer<xiiQtProxy> pTemp = it.Value()[hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(ref_context);
        pProxy       = QSharedPointer<xiiQtProxy>(xiiQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        XII_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
        pProxy->SetAction(pAction);
        it.Value()[hDesc] = pProxy;
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }

      break;
    }
  }

  // make sure we don't use actions that are meant for a different document
  if (pProxy != nullptr && pProxy->GetAction()->GetContext().m_pDocument != nullptr)
  {
    // if this assert fires, you might have tried to map an action into multiple documents, which uses xiiActionScope::Global
    xiiAction*              pAction = pProxy->GetAction();
    const xiiActionContext& ctxt    = pAction->GetContext();
    xiiDocument*            pDoc    = ctxt.m_pDocument;
    XII_ASSERT_DEV(pDoc == ref_context.m_pDocument, "invalid document pointer");
  }
  return pProxy;
}

xiiQtProxy::xiiQtProxy()
{
  m_pAction = nullptr;
}

xiiQtProxy::~xiiQtProxy()
{
  if (m_pAction != nullptr)
    xiiActionManager::GetActionDescriptor(m_pAction->GetDescriptorHandle())->DeleteAction(m_pAction);
}

void xiiQtProxy::SetAction(xiiAction* pAction)
{
  m_pAction = pAction;
}

//////////////////// xiiQtMenuProxy /////////////////////

xiiQtMenuProxy::xiiQtMenuProxy()
{
  m_pMenu = nullptr;
}

xiiQtMenuProxy::~xiiQtMenuProxy()
{
  m_pMenu->deleteLater();
  delete m_pMenu;
}

void xiiQtMenuProxy::Update()
{
  auto pMenu = static_cast<xiiMenuAction*>(m_pAction);

  m_pMenu->setIcon(xiiQtUiServices::GetCachedIconResource(pMenu->GetIconPath()));
  m_pMenu->setTitle(xiiMakeQString(xiiTranslate(pMenu->GetName())));
}

void xiiQtMenuProxy::SetAction(xiiAction* pAction)
{
  xiiQtProxy::SetAction(pAction);

  m_pMenu = new QMenu();
  m_pMenu->setToolTipsVisible(true);
  Update();
}

QMenu* xiiQtMenuProxy::GetQMenu()
{
  return m_pMenu;
}

//////////////////////////////////////////////////////////////////////////
//////////////////// xiiQtButtonProxy /////////////////////
//////////////////////////////////////////////////////////////////////////

xiiQtButtonProxy::xiiQtButtonProxy()
{
  m_pQtAction = nullptr;
}

xiiQtButtonProxy::~xiiQtButtonProxy()
{
  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(xiiMakeDelegate(&xiiQtButtonProxy::StatusUpdateEventHandler, this));

  if (m_pQtAction != nullptr)
  {
    m_pQtAction->deleteLater();
  }
  m_pQtAction = nullptr;
}

void xiiQtButtonProxy::Update()
{
  if (m_pQtAction == nullptr)
    return;

  auto pButton = static_cast<xiiButtonAction*>(m_pAction);


  const xiiActionDescriptor* pDesc = m_pAction->GetDescriptorHandle().GetDescriptor();
  m_pQtAction->setShortcut(QKeySequence(xiiMakeQString(pDesc->m_sShortcut)));

  const QString sDisplayShortcut = m_pQtAction->shortcut().toString(QKeySequence::NativeText);
  QString       sTooltip         = xiiMakeQString(xiiTranslateTooltip(pButton->GetName()));

  xiiStringBuilder sDisplay = xiiTranslate(pButton->GetName());

  if (sTooltip.isEmpty())
  {
    sTooltip = sDisplay;
    sTooltip.replace("&", "");
  }

  if (!sDisplayShortcut.isEmpty())
  {
    sTooltip.append(" (");
    sTooltip.append(sDisplayShortcut);
    sTooltip.append(")");
  }

  if (!pButton->GetAdditionalDisplayString().IsEmpty())
    sDisplay.Append(" '", pButton->GetAdditionalDisplayString(), "'"); // TODO: translate this as well?

  m_pQtAction->setIcon(xiiQtUiServices::GetCachedIconResource(pButton->GetIconPath()));
  m_pQtAction->setText(xiiMakeQString(sDisplay));
  m_pQtAction->setToolTip(sTooltip);
  m_pQtAction->setCheckable(pButton->IsCheckable());
  m_pQtAction->setChecked(pButton->IsChecked());
  m_pQtAction->setEnabled(pButton->IsEnabled());
  m_pQtAction->setVisible(pButton->IsVisible());
}


void SetupQAction(xiiAction* pAction, QPointer<QAction>& ref_pQtAction, QObject* pTarget)
{
  xiiActionDescriptorHandle  hDesc = pAction->GetDescriptorHandle();
  const xiiActionDescriptor* pDesc = hDesc.GetDescriptor();

  if (ref_pQtAction == nullptr)
  {
    ref_pQtAction = new QAction(nullptr);
    XII_VERIFY(QObject::connect(ref_pQtAction, SIGNAL(triggered(bool)), pTarget, SLOT(OnTriggered())) != nullptr, "connection failed");

    switch (pDesc->m_Scope)
    {
      case xiiActionScope::Global:
      {
        // Parent is null so the global actions don't get deleted.
        ref_pQtAction->setShortcutContext(Qt::ShortcutContext::ApplicationShortcut);
      }
      break;
      case xiiActionScope::Document:
      {
        // Parent is set to the window belonging to the document.
        xiiQtDocumentWindow* pWindow = xiiQtDocumentWindow::FindWindowByDocument(pAction->GetContext().m_pDocument);
        XII_ASSERT_DEBUG(pWindow != nullptr, "You can't map a xiiActionScope::Document action without that document existing!");

        ref_pQtAction->setParent(pWindow);
        ref_pQtAction->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
      }
      break;
      case xiiActionScope::Window:
      {
        ref_pQtAction->setParent(pAction->GetContext().m_pWindow);
        ref_pQtAction->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
      }
      break;
    }
  }
}

void xiiQtButtonProxy::SetAction(xiiAction* pAction)
{
  XII_ASSERT_DEV(m_pAction == nullptr, "Implementation error! Expected a valid action.");

  xiiQtProxy::SetAction(pAction);
  m_pAction->m_StatusUpdateEvent.AddEventHandler(xiiMakeDelegate(&xiiQtButtonProxy::StatusUpdateEventHandler, this));

  SetupQAction(m_pAction, m_pQtAction, this);

  Update();
}

QAction* xiiQtButtonProxy::GetQAction()
{
  return m_pQtAction;
}

void xiiQtButtonProxy::StatusUpdateEventHandler(xiiAction* pAction)
{
  Update();
}

void xiiQtButtonProxy::OnTriggered()
{
  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  m_pAction->Execute(m_pQtAction->isChecked());

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

void xiiQtDynamicMenuProxy::SetAction(xiiAction* pAction)
{
  xiiQtMenuProxy::SetAction(pAction);

  XII_VERIFY(connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(SlotMenuAboutToShow())) != nullptr, "signal/slot connection failed");
}

void xiiQtDynamicMenuProxy::SlotMenuAboutToShow()
{
  m_pMenu->clear();

  static_cast<xiiDynamicMenuAction*>(m_pAction)->GetEntries(m_Entries);

  if (m_Entries.IsEmpty())
  {
    m_pMenu->addAction("<empty>")->setEnabled(false);
  }
  else
  {
    for (xiiUInt32 i = 0; i < m_Entries.GetCount(); ++i)
    {
      const auto& p = m_Entries[i];

      if (p.m_ItemFlags.IsSet(xiiDynamicMenuAction::Item::ItemFlags::Separator))
      {
        m_pMenu->addSeparator();
      }
      else
      {
        auto pAction = m_pMenu->addAction(xiiMakeQString(p.m_sDisplay));
        pAction->setData(i);
        pAction->setIcon(p.m_Icon);
        pAction->setCheckable(p.m_CheckState != xiiDynamicMenuAction::Item::CheckMark::NotCheckable);
        pAction->setChecked(p.m_CheckState == xiiDynamicMenuAction::Item::CheckMark::Checked);

        XII_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(SlotMenuEntryTriggered())) != nullptr, "signal/slot connection failed");
      }
    }
  }
}

void xiiQtDynamicMenuProxy::SlotMenuEntryTriggered()
{
  QAction* pAction = qobject_cast<QAction*>(sender());
  if (!pAction)
    return;

  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  xiiUInt32 index = pAction->data().toUInt();
  m_pAction->Execute(m_Entries[index].m_UserValue);

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

//////////////////////////////////////////////////////////////////////////
//////////////////// xiiQtDynamicActionAndMenuProxy /////////////////////
//////////////////////////////////////////////////////////////////////////

xiiQtDynamicActionAndMenuProxy::xiiQtDynamicActionAndMenuProxy()
{
  m_pQtAction = nullptr;
}

xiiQtDynamicActionAndMenuProxy::~xiiQtDynamicActionAndMenuProxy()
{
  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(xiiMakeDelegate(&xiiQtDynamicActionAndMenuProxy::StatusUpdateEventHandler, this));

  if (m_pQtAction != nullptr)
  {
    m_pQtAction->deleteLater();
  }
  m_pQtAction = nullptr;
}


void xiiQtDynamicActionAndMenuProxy::Update()
{
  xiiQtDynamicMenuProxy::Update();

  if (m_pQtAction == nullptr)
    return;

  auto pButton = static_cast<xiiDynamicActionAndMenuAction*>(m_pAction);

  const xiiActionDescriptor* pDesc = m_pAction->GetDescriptorHandle().GetDescriptor();
  m_pQtAction->setShortcut(QKeySequence(xiiMakeQString(pDesc->m_sShortcut)));

  xiiStringBuilder sDisplay = xiiTranslate(pButton->GetName());

  if (!pButton->GetAdditionalDisplayString().IsEmpty())
    sDisplay.Append(" '", pButton->GetAdditionalDisplayString(), "'"); // TODO: translate this as well?

  const QString sDisplayShortcut = m_pQtAction->shortcut().toString(QKeySequence::NativeText);
  QString       sTooltip         = xiiMakeQString(xiiTranslateTooltip(pButton->GetName()));

  if (sTooltip.isEmpty())
  {
    sTooltip = sDisplay;
    sTooltip.replace("&", "");
  }

  if (!sDisplayShortcut.isEmpty())
  {
    sTooltip.append(" (");
    sTooltip.append(sDisplayShortcut);
    sTooltip.append(")");
  }

  m_pQtAction->setIcon(xiiQtUiServices::GetCachedIconResource(pButton->GetIconPath()));
  m_pQtAction->setText(xiiMakeQString(sDisplay.GetView()));
  m_pQtAction->setToolTip(sTooltip);
  m_pQtAction->setEnabled(pButton->IsEnabled());
  m_pQtAction->setVisible(pButton->IsVisible());
}


void xiiQtDynamicActionAndMenuProxy::SetAction(xiiAction* pAction)
{
  xiiQtDynamicMenuProxy::SetAction(pAction);

  m_pAction->m_StatusUpdateEvent.AddEventHandler(xiiMakeDelegate(&xiiQtDynamicActionAndMenuProxy::StatusUpdateEventHandler, this));

  SetupQAction(m_pAction, m_pQtAction, this);

  Update();
}

QAction* xiiQtDynamicActionAndMenuProxy::GetQAction()
{
  return m_pQtAction;
}

void xiiQtDynamicActionAndMenuProxy::OnTriggered()
{
  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  m_pAction->Execute(xiiVariant());

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

void xiiQtDynamicActionAndMenuProxy::StatusUpdateEventHandler(xiiAction* pAction)
{
  Update();
}


//////////////////////////////////////////////////////////////////////////
//////////////////// xiiQtSliderProxy /////////////////////
//////////////////////////////////////////////////////////////////////////

xiiQtSliderWidgetAction::xiiQtSliderWidgetAction(QWidget* pParent) :
  QWidgetAction(pParent)
{
}

xiiQtLabeledSlider::xiiQtLabeledSlider(QWidget* pParent) :
  QWidget(pParent)
{
  m_pLabel  = new QLabel(this);
  m_pSlider = new QSlider(this);
  setLayout(new QHBoxLayout(this));

  layout()->addWidget(m_pLabel);
  layout()->addWidget(m_pSlider);

  setMaximumWidth(300);
}

void xiiQtSliderWidgetAction::setMinimum(int value)
{
  m_iMinimum = value;

  const QList<QWidget*> widgets = createdWidgets();

  for (QWidget* pWidget : widgets)
  {
    xiiQtLabeledSlider* pGroup = qobject_cast<xiiQtLabeledSlider*>(pWidget);
    pGroup->m_pSlider->setMinimum(m_iMinimum);
  }
}

void xiiQtSliderWidgetAction::setMaximum(int value)
{
  m_iMaximum = value;

  const QList<QWidget*> widgets = createdWidgets();

  for (QWidget* pWidget : widgets)
  {
    xiiQtLabeledSlider* pGroup = qobject_cast<xiiQtLabeledSlider*>(pWidget);
    pGroup->m_pSlider->setMaximum(m_iMaximum);
  }
}

void xiiQtSliderWidgetAction::setValue(int value)
{
  m_iValue = value;

  const QList<QWidget*> widgets = createdWidgets();

  for (QWidget* pWidget : widgets)
  {
    xiiQtLabeledSlider* pGroup = qobject_cast<xiiQtLabeledSlider*>(pWidget);
    pGroup->m_pSlider->setValue(m_iValue);
  }
}

void xiiQtSliderWidgetAction::OnValueChanged(int value)
{
  Q_EMIT valueChanged(value);
}

QWidget* xiiQtSliderWidgetAction::createWidget(QWidget* parent)
{
  xiiQtLabeledSlider* pGroup = new xiiQtLabeledSlider(parent);
  pGroup->m_pSlider->setOrientation(Qt::Orientation::Horizontal);

  XII_VERIFY(connect(pGroup->m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(OnValueChanged(int))) != nullptr, "connection failed");

  pGroup->m_pLabel->setText(text());
  pGroup->m_pLabel->installEventFilter(this);
  pGroup->m_pLabel->setToolTip(toolTip());
  pGroup->installEventFilter(this);
  pGroup->m_pSlider->setMinimum(m_iMinimum);
  pGroup->m_pSlider->setMaximum(m_iMaximum);
  pGroup->m_pSlider->setValue(m_iValue);
  pGroup->m_pSlider->setToolTip(toolTip());

  return pGroup;
}

bool xiiQtSliderWidgetAction::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::Type::MouseButtonPress || e->type() == QEvent::Type::MouseButtonRelease || e->type() == QEvent::Type::MouseButtonDblClick)
  {
    e->accept();
    return true;
  }

  return false;
}

xiiQtSliderProxy::xiiQtSliderProxy()
{
  m_pQtAction = nullptr;
}

xiiQtSliderProxy::~xiiQtSliderProxy()
{
  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(xiiMakeDelegate(&xiiQtSliderProxy::StatusUpdateEventHandler, this));

  if (m_pQtAction != nullptr)
  {
    m_pQtAction->deleteLater();
  }
  m_pQtAction = nullptr;
}

void xiiQtSliderProxy::Update()
{
  if (m_pQtAction == nullptr)
    return;

  auto pAction = static_cast<xiiSliderAction*>(m_pAction);

  const xiiActionDescriptor* pDesc = m_pAction->GetDescriptorHandle().GetDescriptor();

  xiiQtSliderWidgetAction* pSliderAction = qobject_cast<xiiQtSliderWidgetAction*>(m_pQtAction);
  xiiQtScopedBlockSignals  bs(pSliderAction);

  xiiInt32 minVal, maxVal;
  pAction->GetRange(minVal, maxVal);
  pSliderAction->setMinimum(minVal);
  pSliderAction->setMaximum(maxVal);
  pSliderAction->setValue(pAction->GetValue());
  pSliderAction->setText(xiiMakeQString(xiiTranslate(pAction->GetName())));
  pSliderAction->setToolTip(xiiMakeQString(xiiTranslateTooltip(pAction->GetName())));
  pSliderAction->setEnabled(pAction->IsEnabled());
  pSliderAction->setVisible(pAction->IsVisible());
}

void xiiQtSliderProxy::SetAction(xiiAction* pAction)
{
  XII_ASSERT_DEV(m_pAction == nullptr, "Es darf nicht sein, es kann nicht sein!");

  xiiQtProxy::SetAction(pAction);
  m_pAction->m_StatusUpdateEvent.AddEventHandler(xiiMakeDelegate(&xiiQtSliderProxy::StatusUpdateEventHandler, this));

  xiiActionDescriptorHandle  hDesc = m_pAction->GetDescriptorHandle();
  const xiiActionDescriptor* pDesc = hDesc.GetDescriptor();

  if (m_pQtAction == nullptr)
  {
    m_pQtAction = new xiiQtSliderWidgetAction(nullptr);

    XII_VERIFY(connect(m_pQtAction, SIGNAL(valueChanged(int)), this, SLOT(OnValueChanged(int))) != nullptr, "connection failed");
  }

  Update();
}

QAction* xiiQtSliderProxy::GetQAction()
{
  return m_pQtAction;
}


void xiiQtSliderProxy::OnValueChanged(int value)
{
  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  // make sure all instances of the slider get updated, by setting the new value
  m_pQtAction->setValue(value);
  m_pAction->Execute(value);

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

void xiiQtSliderProxy::StatusUpdateEventHandler(xiiAction* pAction)
{
  Update();
}
