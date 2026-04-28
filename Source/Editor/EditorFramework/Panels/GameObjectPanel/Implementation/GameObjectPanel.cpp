/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>

xiiQtGameObjectWidget::xiiQtGameObjectWidget(QWidget* pParent, xiiGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<xiiQtDocumentTreeModel> pCustomModel, xiiSelectionManager* pSelection)
{
  setObjectName("xiiQtGameObjectWidget");

  m_pDocument           = pDocument;
  m_sContextMenuMapping = szContextMenuMapping;
  m_pDelegate           = new xiiQtGameObjectDelegate(this, pDocument);

  setLayout(new QVBoxLayout());
  setContentsMargins(0, 0, 0, 0);
  layout()->setObjectName("QVBoxLayout1");
  layout()->setContentsMargins(0, 0, 0, 0);

  m_pFilterWidget = new xiiQtSearchWidget(this);
  m_pFilterWidget->setObjectName("xiiQtSearchWidget");
  m_pFilterWidget->setPlaceholderText("Search by name or component type");
  connect(m_pFilterWidget, &xiiQtSearchWidget::textChanged, this, &xiiQtGameObjectWidget::OnFilterTextChanged);

  layout()->addWidget(m_pFilterWidget);

  m_pTreeWidget = new xiiQtDocumentTreeView(this, pDocument, std::move(pCustomModel), pSelection);
  m_pTreeWidget->setObjectName("xiiQtDocumentTreeView");
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(true);
  layout()->addWidget(m_pTreeWidget);
  m_pTreeWidget->setItemDelegate(m_pDelegate);

  m_pDocument->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiQtGameObjectWidget::DocumentSceneEventHandler, this));

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  XII_VERIFY(connect(m_pTreeWidget, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnItemDoubleClicked(const QModelIndex&))) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr, "signal/slot connection failed");
}

xiiQtGameObjectWidget::~xiiQtGameObjectWidget()
{
  m_pDocument->m_GameObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtGameObjectWidget::DocumentSceneEventHandler, this));
}

void xiiQtGameObjectWidget::DocumentSceneEventHandler(const xiiGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGameObjectEvent::Type::TriggerShowSelectionInScenegraph:
    {
      m_pTreeWidget->EnsureLastSelectedItemVisible();
    }
    break;

    default:
      break;
  }
}

void xiiQtGameObjectWidget::OnItemDoubleClicked(const QModelIndex&)
{
  m_pDocument->TriggerFocusOnSelection(true);
}

void xiiQtGameObjectWidget::OnRequestContextMenu(QPoint pos)
{
  xiiQtMenuActionMapView menu(nullptr);

  xiiActionContext context;
  context.m_sMapping  = m_sContextMenuMapping;
  context.m_pDocument = m_pDocument;
  context.m_pWindow   = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));
}

void xiiQtGameObjectWidget::OnFilterTextChanged(const QString& text)
{
  m_pTreeWidget->GetProxyFilterModel()->SetFilterText(text);
}

//////////////////////////////////////////////////////////////////////////

xiiQtGameObjectPanel::xiiQtGameObjectPanel(ads::CDockManager* pDockManager, QWidget* pParent, xiiGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<xiiQtDocumentTreeModel> pCustomModel) :
  xiiQtDocumentPanel(pDockManager, pParent, pDocument)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("xiiQtGameObjectPanel");

  m_pMainWidget = new xiiQtGameObjectWidget(this, pDocument, szContextMenuMapping, std::move(pCustomModel));
  setWidget(m_pMainWidget);
}

xiiQtGameObjectPanel::~xiiQtGameObjectPanel() = default;
