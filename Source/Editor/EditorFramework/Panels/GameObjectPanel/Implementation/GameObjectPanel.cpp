/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QLabel>
#include <QMenu>
#include <QShortcut>
#include <QToolBar>
#include <QToolButton>

xiiQtGameObjectWidget::xiiQtGameObjectWidget(QWidget* pParent, xiiGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<xiiQtDocumentTreeModel> pCustomModel, xiiSelectionManager* pSelection) :
  QWidget(pParent)
{
  setObjectName("xiiQtGameObjectWidget");

  m_pDocument           = pDocument;
  m_sContextMenuMapping = szContextMenuMapping;
  m_pDelegate           = new xiiQtGameObjectDelegate(this, pDocument);

  setLayout(new QVBoxLayout());
  setContentsMargins(0, 0, 0, 0);
  layout()->setObjectName("QVBoxLayout1");
  layout()->setContentsMargins(0, 0, 0, 0);
  layout()->setSpacing(2);

  m_pFilterWidget = new xiiQtSearchWidget(this);
  m_pFilterWidget->SetCaptureTabKey(false);
  m_pFilterWidget->setObjectName("xiiQtSearchWidget");
  m_pFilterWidget->setPlaceholderText("Search by name or component type");
  connect(m_pFilterWidget, &xiiQtSearchWidget::textChanged, this, &xiiQtGameObjectWidget::OnFilterTextChanged);

  auto* pHeader = new QHBoxLayout;
  pHeader->setContentsMargins(0, 0, 0, 0);
  pHeader->setSpacing(2);
  pHeader->addWidget(m_pFilterWidget, 1);
  static_cast<QVBoxLayout*>(layout())->addLayout(pHeader);

  m_pTreeWidget = new xiiQtDocumentTreeView(this, pDocument, std::move(pCustomModel), pSelection);
  m_pTreeWidget->setObjectName("xiiQtDocumentTreeView");
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(true);
  layout()->addWidget(m_pTreeWidget);
  m_pTreeWidget->setItemDelegate(m_pDelegate);
  m_pTreeWidget->setAlternatingRowColors(true);
  m_pTreeWidget->setIndentation(16);
  m_pTreeWidget->setAccessibleName(tr("Scene Outliner"));

  auto* pTools = new QToolBar(tr("Outliner navigation"), this);
  pTools->setToolButtonStyle(Qt::ToolButtonTextOnly);
  pTools->setMovable(false);
  pTools->setIconSize(QSize(16, 16));
  auto* pReveal = pTools->addAction(tr("Reveal"));
  pReveal->setToolTip(tr("Clear the filter and reveal the last selected object"));
  connect(pReveal, &QAction::triggered, this, [this]() {
    m_pFilterWidget->setText(QString());
    m_pTreeWidget->EnsureLastSelectedItemVisible();
  });
  auto* pFocus = pTools->addAction(tr("Frame"));
  pFocus->setToolTip(tr("Frame the selection in the viewport"));
  connect(pFocus, &QAction::triggered, this, [this]() { m_pDocument->TriggerFocusOnSelection(true); });
  connect(pTools->addAction(tr("Collapse")), &QAction::triggered, m_pTreeWidget, &QTreeView::collapseAll);
  auto* pOptions = new QToolButton(this);
  pOptions->setText(QStringLiteral("..."));
  pOptions->setAccessibleName(tr("Outliner navigation"));
  pOptions->setToolTip(tr("Reveal, frame and collapse hierarchy"));
  pOptions->setPopupMode(QToolButton::InstantPopup);
  auto* pMenu = new QMenu(pOptions);
  pMenu->addActions(pTools->actions());
  pOptions->setMenu(pMenu);
  pTools->hide();
  auto* pSummary = new QLabel(this);
  pSummary->setContentsMargins(4, 0, 4, 0);
  pHeader->addWidget(pSummary);
  pHeader->addWidget(pOptions);
  auto* pSelectionManager = pSelection ? pSelection : pDocument->GetSelectionManager();
  auto  UpdateSelection   = [pSelectionManager, pSummary, pReveal, pFocus]() {
    const auto uiCount = pSelectionManager->GetSelection().GetCount();
    pSummary->setText(QString::number(uiCount));
    pSummary->setToolTip(tr("%1 selected").arg(uiCount));
    pSummary->setAccessibleName(pSummary->toolTip());
    pReveal->setEnabled(uiCount != 0);
    pFocus->setEnabled(uiCount != 0);
  };
  pSelectionManager->m_Events.AddEventHandler([UpdateSelection](const xiiSelectionManagerEvent&) { UpdateSelection(); }, m_SelectionSubscription);
  UpdateSelection();
  auto* pFind = new QShortcut(QKeySequence::Find, this);
  pFind->setContext(Qt::WidgetWithChildrenShortcut);
  connect(pFind, &QShortcut::activated, m_pFilterWidget, [this]() { m_pFilterWidget->setFocus(); m_pFilterWidget->selectAll(); });
  connect(m_pFilterWidget, &xiiQtSearchWidget::specialKeyPressed, this, [this](Qt::Key key) {
    if (key == Qt::Key_Down)
    {
      m_pTreeWidget->setFocus();
      if (!m_pTreeWidget->currentIndex().isValid())
        m_pTreeWidget->setCurrentIndex(m_pTreeWidget->model()->index(0, 0));
    }
  });

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
      m_pFilterWidget->setText(QString());
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
  auto* pProxy = m_pTreeWidget->GetProxyFilterModel();
  if (!m_bSearching && !text.isEmpty())
  {
    m_ExpandedBeforeSearch.clear();
    auto SaveExpanded = [&](auto&& self, const QModelIndex& parent) -> void {
      for (int i = 0; i < pProxy->rowCount(parent); ++i)
      {
        const auto index = pProxy->index(i, 0, parent);
        if (m_pTreeWidget->isExpanded(index))
        {
          m_ExpandedBeforeSearch.push_back(pProxy->mapToSource(index));
          self(self, index);
        }
      }
    };
    SaveExpanded(SaveExpanded, QModelIndex());
  }
  m_pTreeWidget->SetFilterText(text);
  if (!text.isEmpty())
    m_pTreeWidget->expandAll();
  else if (m_bSearching)
  {
    m_pTreeWidget->collapseAll();
    for (const auto& index : m_ExpandedBeforeSearch)
      if (index.isValid())
        m_pTreeWidget->setExpanded(pProxy->mapFromSource(index), true);
    m_ExpandedBeforeSearch.clear();
  }
  m_bSearching = !text.isEmpty();
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
