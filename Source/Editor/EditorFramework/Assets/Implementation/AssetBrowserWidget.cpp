/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/Assets/AssetBrowserInspector.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/EditorPreferences.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QActionGroup>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPromise>
#include <QResizeEvent>
#include <QShortcut>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QThreadPool>
#include <QToolButton>
#include <QTreeWidgetItemIterator>
#include <QUrl>

namespace
{
  using BrowserScope  = xiiQtAssetBrowserFilter::Scope;
  using BrowserStatus = xiiQtAssetBrowserFilter::Status;

  struct ImportCopyResult
  {
    xiiHybridArray<xiiString, 16> m_CopiedFiles;
    xiiHybridArray<xiiString, 16> m_ImportPaths;
    QString                       m_sError;
  };

  class AssetDetailsDelegate final : public QStyledItemDelegate
  {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem&, const QModelIndex& index) const override
    {
      const xiiStringBuilder sPath   = qtToXIIString(index.data(xiiQtAssetBrowserModel::AbsolutePath).toString());
      auto*                  pEditor = new QLineEdit(pParent);
      pEditor->setValidator(new xiiFileNameValidator(pEditor, sPath.GetFileDirectory(), sPath.GetFileNameAndExtension()));
      return pEditor;
    }
  };
} // namespace

xiiEvent<xiiAssetBrowserPreviewRequest&>    xiiQtAssetBrowserWidget::s_PreviewRequests;
xiiEvent<const xiiAssetBrowserContextMenu&> xiiQtAssetBrowserWidget::s_ContextMenuActions;

xiiQtAssetBrowserWidget::xiiQtAssetBrowserWidget(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);
  RootLayout->setSizeConstraint(QLayout::SetNoConstraint);
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);

  xiiEditorPreferencesUser* pPreferences = xiiEditorSettings::QuerySettings<xiiEditorPreferencesUser>();

  ButtonListMode->setAccessibleName(tr("List view"));
  ButtonIconMode->setAccessibleName(tr("Grid view"));
  ButtonShowItemsSubFolders->setAccessibleName(tr("Include subfolders"));
  IconSizeSlider->setAccessibleName(tr("Thumbnail size"));
  IconSizeSlider->setToolTip(tr("Thumbnail size (Ctrl+mouse wheel)"));
  IconSizeSlider->setMaximumWidth(82);
  TypeFilter->setAccessibleName(tr("Asset type"));
  TypeFilter->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  TypeFilter->setMinimumContentsLength(12);
  SearchWidget->setMinimumWidth(75);
  ToolBarLayout->setStretchFactor(SearchWidget, 1);
  setMinimumWidth(0);
  for (auto* pButton : {ButtonIconMode, ButtonListMode, ButtonDetailsMode, ButtonShowItemsSubFolders, ButtonFilters, ButtonOptions, ButtonImport, ButtonNew})
  {
    pButton->setAutoRaise(true);
    pButton->setIconSize(QSize(16, 16));
  }

  m_pFilter = new xiiQtAssetBrowserFilter(this);
  m_pFilter->SetShowItemsInSubFolders(pPreferences->m_bAssetBrowserShowItemsInSubFolders);

  auto* pOptions = new QMenu(ButtonOptions);
  ButtonOptions->setText(QStringLiteral("…"));
  ButtonOptions->setToolTip(tr("Browser options"));
  ButtonOptions->setAccessibleName(tr("Browser options"));
  ButtonOptions->setPopupMode(QToolButton::InstantPopup);
  ButtonOptions->setMenu(pOptions);

  auto addNavigation = [this, pOptions](QStyle::StandardPixmap icon, const QString& sText, int iPosition) {
    auto* pAction = pOptions->addAction(style()->standardIcon(icon), sText);
    auto* pButton = new QToolButton(Toolbar);
    pButton->setAutoRaise(true);
    pButton->setIconSize(QSize(16, 16));
    pButton->setDefaultAction(pAction);
    ToolBarLayout->insertWidget(iPosition, pButton);
    return pAction;
  };

  m_pBack        = addNavigation(QStyle::SP_ArrowBack, tr("Back"), 0);
  m_pForward     = addNavigation(QStyle::SP_ArrowForward, tr("Forward"), 1);
  m_pUp          = addNavigation(QStyle::SP_ArrowUp, tr("Parent folder"), 2);
  auto* pRefresh = addNavigation(QStyle::SP_BrowserReload, tr("Refresh asset index"), 3);
  connect(m_pBack, &QAction::triggered, this, [this]() { NavigateHistory(-1); });
  connect(m_pForward, &QAction::triggered, this, [this]() { NavigateHistory(1); });
  connect(m_pUp, &QAction::triggered, this, [this]() {
    const QString sPath  = xiiMakeQString(m_pFilter->GetPathFilter());
    const auto    iSlash = sPath.lastIndexOf(QLatin1Char('/'));
    m_pFilter->SetPathFilter((iSlash >= 0 ? sPath.left(iSlash) : QString()).toUtf8().constData());
  });
  connect(pRefresh, &QAction::triggered, this, []() { xiiAssetCurator::GetSingleton()->QueueCheckFileSystem(); });

  m_pBreadcrumb = new QLabel(Toolbar);
  m_pBreadcrumb->setTextFormat(Qt::RichText);
  m_pBreadcrumb->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
  m_pBreadcrumb->setAccessibleName(tr("Current content folder"));
  m_pBreadcrumbMenu = new QMenu(m_pBreadcrumb);
  ToolBarLayout->insertWidget(4, m_pBreadcrumb);
  connect(m_pBreadcrumb, &QLabel::linkActivated, this, [this](const QString& sLink) {
    const int iParts = sLink.toInt();
    if (iParts < 0)
    {
      m_pBreadcrumbMenu->popup(m_pBreadcrumb->mapToGlobal(QPoint(0, m_pBreadcrumb->height())));
      return;
    }
    const auto parts = xiiMakeQString(m_pFilter->GetPathFilter()).split(QLatin1Char('/'), Qt::SkipEmptyParts);
    m_pFilter->SetPathFilter(parts.mid(0, iParts).join(QLatin1Char('/')).toUtf8().constData());
  });
  ToolBarLayout->removeWidget(TypeFilter);
  ToolBarLayout->insertWidget(5, TypeFilter);
  pOptions->addMenu(m_pBreadcrumbMenu);
  auto* pTypes = pOptions->addMenu(tr("Asset type"));
  connect(pTypes, &QMenu::aboutToShow, this, [this, pTypes]() {
    pTypes->clear();
    for (int i = 0; i < TypeFilter->count(); ++i)
    {
      auto* pAction = pTypes->addAction(TypeFilter->itemIcon(i), TypeFilter->itemText(i), this, [this, i]() { TypeFilter->setCurrentIndex(i); });
      pAction->setCheckable(true);
      pAction->setChecked(i == TypeFilter->currentIndex());
    }
  });
  connect(pOptions, &QMenu::aboutToShow, this, [this, pTypes]() { pTypes->menuAction()->setVisible(m_Mode != Mode::FilePicker); });
  pOptions->addSeparator();
  auto* pSubfolders = pOptions->addAction(tr("Include subfolders"));
  pSubfolders->setCheckable(true);
  connect(pSubfolders, &QAction::triggered, ButtonShowItemsSubFolders, &QToolButton::click);
  connect(pOptions, &QMenu::aboutToShow, this, [this, pSubfolders]() {
    pSubfolders->setChecked(ButtonShowItemsSubFolders->isChecked());
    pSubfolders->setEnabled(ButtonShowItemsSubFolders->isEnabled());
  });
  auto*             pViews     = pOptions->addMenu(tr("View"));
  auto*             pViewGroup = new QActionGroup(pViews);
  const QStringList viewNames  = {tr("Grid"), tr("List"), tr("Details")};
  for (int i = 0; i < viewNames.size(); ++i)
  {
    auto* pView = pViews->addAction(viewNames[i], this, [this, i]() { SetDisplayMode(i); });
    pView->setCheckable(true);
    pViewGroup->addAction(pView);
  }
  connect(pViews, &QMenu::aboutToShow, this, [this, pViewGroup]() { pViewGroup->actions()[m_iDisplayMode]->setChecked(true); });
  auto* pSidebar = pOptions->addAction(tr("Show sidebar"));
  pSidebar->setCheckable(true);
  pSidebar->setChecked(true);
  connect(pSidebar, &QAction::toggled, this, [this](bool bVisible) {
    m_bShowSidebar = bVisible;
    UpdatePanelVisibility();
  });
  connect(pOptions, &QMenu::aboutToShow, this, [this, pSidebar]() { pSidebar->setChecked(m_bShowSidebar); });

  Sidebar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
  Sidebar->setMinimumWidth(153);
  Workspace->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
  m_pInspector = new xiiQtAssetBrowserInspector(this, splitter);
  m_pInspector->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
  m_pInspector->setMinimumWidth(205);
  splitter->addWidget(m_pInspector);
  m_pInspectorAction = pOptions->addAction(style()->standardIcon(QStyle::SP_FileDialogInfoView), tr("Show inspector"));
  m_pInspectorAction->setCheckable(true);
  m_pInspectorAction->setChecked(true);
  connect(m_pInspectorAction, &QAction::toggled, this, [this](bool bVisible) {
    m_bShowInspector  = bVisible;
    m_bForceInspector = bVisible && width() < 465;
    UpdatePanelVisibility();
  });
  auto* pInspectorButton = new QToolButton(Toolbar);
  pInspectorButton->setDefaultAction(m_pInspectorAction);
  pInspectorButton->setAutoRaise(true);
  pInspectorButton->setIconSize(QSize(16, 16));
  ToolBarLayout->insertWidget(ToolBarLayout->indexOf(ButtonOptions), pInspectorButton);

  auto* pFilters = new QMenu(ButtonFilters);
  ButtonFilters->setText(tr("Filters"));
  ButtonFilters->setPopupMode(QToolButton::InstantPopup);
  ButtonFilters->setMenu(pFilters);
  pOptions->addMenu(pFilters)->setText(tr("Filters"));
  auto* pHidden = pFilters->addAction(tr("Show hidden folders"), this, &xiiQtAssetBrowserWidget::OnShowHiddenFolderItemsToggled);
  pHidden->setCheckable(true);
  connect(pFilters, &QMenu::aboutToShow, this, [this, pHidden]() { pHidden->setChecked(m_pFilter->GetShowItemsInHiddenFolders()); });
  auto*             pStates      = pFilters->addMenu(tr("Processing status"));
  auto*             pStatusGroup = new QActionGroup(pStates);
  const QStringList statusNames  = {tr("Any status"), tr("Ready"), tr("Needs attention"), tr("Pending work"), tr("Errors")};
  for (int i = 0; i < statusNames.size(); ++i)
  {
    const auto status  = static_cast<BrowserStatus>(i);
    auto*      pAction = pStates->addAction(statusNames[i], this, [this, status]() { ApplyStatusFilter(status, false); });
    pAction->setCheckable(true);
    pStatusGroup->addAction(pAction);
  }
  connect(pStates, &QMenu::aboutToShow, this, [this, pStatusGroup]() {
    pStatusGroup->actions()[static_cast<int>(m_pFilter->GetStatusFilter())]->setChecked(true);
  });

  auto* pSort      = pFilters->addMenu(tr("Sort by"));
  auto  sortAssets = [this](int iColumn, Qt::SortOrder order) {
    DetailsAssets->header()->setSortIndicator(iColumn, order);
    m_pModel->sort(iColumn, order);
  };
  pSort->addAction(tr("Name"), this, [sortAssets]() { sortAssets(xiiQtAssetBrowserModel::NameColumn, Qt::AscendingOrder); });
  pSort->addAction(tr("Type"), this, [sortAssets]() { sortAssets(xiiQtAssetBrowserModel::TypeColumn, Qt::AscendingOrder); });
  pSort->addAction(tr("Last modified"), this, [sortAssets]() { sortAssets(xiiQtAssetBrowserModel::ModifiedColumn, Qt::DescendingOrder); });

  ButtonDetailsMode->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
  ButtonDetailsMode->setToolTip(tr("Details view"));
  ButtonDetailsMode->setAccessibleName(tr("Details view"));
  ButtonImport->setText(tr("Import"));
  ButtonImport->setIcon(QIcon(":/GuiFoundation/Icons/Import.svg"));
  ButtonImport->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  connect(ButtonImport, &QToolButton::clicked, this, &xiiQtAssetBrowserWidget::ImportFiles);
  ButtonNew->setText(tr("New Asset"));
  ButtonNew->setIcon(QIcon(":/GuiFoundation/Icons/DocumentAdd.svg"));
  ButtonNew->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  ButtonNew->setPopupMode(QToolButton::InstantPopup);
  auto* pNew = new QMenu(ButtonNew);
  ButtonNew->setMenu(pNew);
  connect(pNew, &QMenu::aboutToShow, this, [this, pNew]() {
    pNew->clear();
    AddAssetCreatorMenu(pNew, false, false);
  });
  auto* pCreate = pOptions->addMenu(pNew);
  pCreate->setText(tr("New Asset"));
  auto* pProcessing = new xiiQtMenuActionMapView(pOptions);
  pProcessing->setParent(pOptions, Qt::Popup);
  pProcessing->setTitle(tr("Asset processing"));
  xiiActionContext context;
  context.m_sMapping = "AssetBrowserToolBar";
  pProcessing->SetActionContext(context);
  auto* pProcessingAction = pOptions->addMenu(pProcessing);
  auto* pImport           = pOptions->addAction(tr("Import files…"), this, &xiiQtAssetBrowserWidget::ImportFiles);
  connect(pOptions, &QMenu::aboutToShow, this, [this, pProcessingAction, pImport, pCreate]() {
    pCreate->setVisible(m_Mode == Mode::Browser);
    pProcessingAction->setVisible(m_Mode == Mode::Browser);
    pImport->setVisible(m_Mode == Mode::Browser);
  });

  m_pVisibleCount = new QLabel(StatusBar);
  StatusLayout->addWidget(m_pVisibleCount);
  m_pPendingCount = new QToolButton(StatusBar);
  m_pErrorCount   = new QToolButton(StatusBar);
  m_pIndexStatus  = new QLabel(StatusBar);
  m_pIndexStatus->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  for (auto* pButton : {m_pPendingCount, m_pErrorCount})
  {
    pButton->setAutoRaise(true);
    pButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pButton->setIconSize(QSize(12, 12));
    StatusLayout->addWidget(pButton);
  }
  m_pPendingCount->setIcon(xiiQtAssetBrowserModel::TransformStateIcon(xiiAssetInfo::NeedsTransform));
  m_pErrorCount->setIcon(xiiQtAssetBrowserModel::TransformStateIcon(xiiAssetInfo::TransformError));
  m_pPendingCount->setToolTip(tr("Show assets awaiting processing across all content roots"));
  m_pErrorCount->setToolTip(tr("Show assets with processing errors across all content roots"));
  connect(m_pPendingCount, &QToolButton::clicked, this, [this]() { ApplyStatusFilter(BrowserStatus::Pending, true); });
  connect(m_pErrorCount, &QToolButton::clicked, this, [this]() { ApplyStatusFilter(BrowserStatus::Errors, true); });
  StatusLayout->addWidget(m_pIndexStatus, 1);

  m_pScopes = new QListWidget(Sidebar);
  m_pScopes->setFrameShape(QFrame::NoFrame);
  m_pScopes->setUniformItemSizes(true);
  m_pScopes->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_pScopes->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_pScopes->setAccessibleName(tr("Asset scopes"));
  m_pScopes->setStyleSheet(QStringLiteral("QListView::item { min-height: 22px; padding-left: 4px; }"));
  auto addScope = [this](const QIcon& icon, const QString& sName) {
    auto* pItem = new QListWidgetItem(icon, sName, m_pScopes);
    pItem->setData(Qt::UserRole, sName);
  };
  addScope(QIcon(":/AssetIcons/Icons/AllAssets.svg"), tr("All Assets"));
  addScope(style()->standardIcon(QStyle::SP_BrowserReload), tr("Recently Used"));
  addScope(QIcon::fromTheme(QStringLiteral("starred"), style()->standardIcon(QStyle::SP_DialogYesButton)), tr("Favorites"));
  addScope(xiiQtAssetBrowserModel::TransformStateIcon(xiiAssetInfo::NeedsTransform), tr("Needs Attention"));
  m_pScopes->setFixedHeight(m_pScopes->count() * m_pScopes->sizeHintForRow(0));
  SidebarLayout->insertWidget(0, m_pScopes);
  connect(m_pScopes, &QListWidget::currentRowChanged, this, [this](int iRow) {
    if (iRow < 0)
      return;

    m_pFilter->SetScope(static_cast<BrowserScope>(iRow));
  });

  m_pSearchTimer = new QTimer(this);
  m_pSearchTimer->setSingleShot(true);
  m_pSearchTimer->setInterval(125);
  connect(m_pSearchTimer, &QTimer::timeout, this, [this]() {
    m_pFilter->SetTextFilter(SearchWidget->text().toUtf8().constData());
  });
  connect(SearchWidget, &xiiQtSearchWidget::enterPressed, this, [this]() {
    m_pSearchTimer->stop();
    m_pFilter->SetTextFilter(SearchWidget->text().toUtf8().constData());
    if (m_pModel->rowCount() > 0)
      ActiveView()->setFocus();
  });
  m_pReferenceTimer = new QTimer(this);
  m_pReferenceTimer->setSingleShot(true);
  m_pReferenceTimer->setInterval(200);
  connect(m_pReferenceTimer, &QTimer::timeout, m_pFilter, &xiiQtAssetBrowserFilter::RefreshReferences);
  m_pFolderCountTimer = new QTimer(this);
  m_pFolderCountTimer->setSingleShot(true);
  m_pFolderCountTimer->setInterval(300);
  connect(m_pFolderCountTimer, &QTimer::timeout, this, &xiiQtAssetBrowserWidget::UpdateFolderCounts);
  connect(TreeFolderFilter->model(), &QAbstractItemModel::rowsInserted, this, [this]() { m_pFolderCountTimer->start(); });

  auto* pStatusTimer = new QTimer(this);
  pStatusTimer->setInterval(500);
  connect(pStatusTimer, &QTimer::timeout, this, &xiiQtAssetBrowserWidget::UpdateIndexStatus);
  pStatusTimer->start();
  EmptyView->setText(tr("No matching items"));
  EmptyView->setWordWrap(true);
  EmptyView->setMinimumWidth(0);
  Toolbar->setStyleSheet(QStringLiteral("QWidget#Toolbar { background: palette(alternate-base); border-bottom: 1px solid palette(mid); }"));
  WorkspaceHeader->setStyleSheet(QStringLiteral("QWidget#WorkspaceHeader { border-bottom: 1px solid palette(mid); }"));
  StatusBar->setStyleSheet(QStringLiteral("QWidget#StatusBar { background: palette(alternate-base); border-top: 1px solid palette(mid); }"));
  WorkspaceTitle->setTextFormat(Qt::PlainText);
  WorkspaceTitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  QFont titleFont = WorkspaceTitle->font();
  titleFont.setBold(true);
  WorkspaceTitle->setFont(titleFont);

  m_pStatusChips    = new QWidget(WorkspaceHeader);
  auto* pChipLayout = new QHBoxLayout(m_pStatusChips);
  pChipLayout->setContentsMargins(0, 0, 0, 0);
  pChipLayout->setSpacing(2);
  WorkspaceHeaderLayout->insertWidget(1, m_pStatusChips);
  for (const auto& entry : {qMakePair(tr("All"), BrowserStatus::Any), qMakePair(tr("Ready"), BrowserStatus::Ready), qMakePair(tr("Needs attention"), BrowserStatus::Attention)})
  {
    auto* pChip = new QToolButton(m_pStatusChips);
    pChip->setAutoRaise(true);
    pChip->setCheckable(true);
    pChip->setText(entry.first);
    pChip->setChecked(m_pFilter->GetStatusFilter() == entry.second);
    pChipLayout->addWidget(pChip);
    connect(pChip, &QToolButton::clicked, this, [this, pChip, status = entry.second]() {
      ApplyStatusFilter(status, false);
      pChip->setChecked(true);
    });
    connect(m_pFilter, &xiiQtAssetFilter::FilterChanged, pChip, [this, pChip, status = entry.second]() {
      pChip->setChecked(m_pFilter->GetStatusFilter() == status);
    });
  }
  TreeFolderFilter->setIndentation(14);
  TreeFolderFilter->setColumnCount(2);
  TreeFolderFilter->header()->setStretchLastSection(false);
  TreeFolderFilter->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  TreeFolderFilter->header()->setSectionResizeMode(1, QHeaderView::Fixed);
  TreeFolderFilter->setColumnWidth(1, 42);
  TreeFolderFilter->setFrameShape(QFrame::NoFrame);
  TreeFolderFilter->setStyleSheet(QStringLiteral("QTreeView::item { min-height: 22px; }"));
  ListAssets->setFrameShape(QFrame::NoFrame);

  for (const auto& entry : {qMakePair(QKeySequence(Qt::ALT | Qt::Key_Left), -1), qMakePair(QKeySequence(Qt::ALT | Qt::Key_Right), 1)})
  {
    auto* pShortcut = new QShortcut(entry.first, this);
    pShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(pShortcut, &QShortcut::activated, this, [this, iDirection = entry.second]() { NavigateHistory(iDirection); });
  }
  auto* pFind = new QShortcut(QKeySequence::Find, this);
  pFind->setContext(Qt::WidgetWithChildrenShortcut);
  connect(pFind, &QShortcut::activated, SearchWidget, [this]() { SearchWidget->setFocus(); SearchWidget->selectAll(); });

  TreeFolderFilter->SetFilter(m_pFilter);

  m_pModel = QSharedPointer<xiiQtAssetBrowserModel>(new xiiQtAssetBrowserModel(this, m_pFilter));
  m_pModel->Initialize();
  SearchWidget->setPlaceholderText(tr("Search assets"));
  SearchWidget->SetCaptureTabKey(false);

  IconSizeSlider->setValue(72);

  ListAssets->setModel(m_pModel.data());
  ListAssets->SetIconScale(IconSizeSlider->value());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  ListAssets->setDragEnabled(true);
  ListAssets->setAcceptDrops(true);
  ListAssets->setDropIndicatorShown(true);
  on_ButtonIconMode_clicked();

  splitter->setSizes({174, 450, 240});
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  DetailsAssets->setModel(m_pModel.data());
  DetailsAssets->setSelectionModel(ListAssets->selectionModel());
  DetailsAssets->setRootIsDecorated(false);
  DetailsAssets->setItemsExpandable(false);
  DetailsAssets->setUniformRowHeights(true);
  DetailsAssets->setAlternatingRowColors(true);
  DetailsAssets->setFrameShape(QFrame::NoFrame);
  DetailsAssets->setIconSize(QSize(16, 16));
  DetailsAssets->setSelectionMode(QAbstractItemView::ExtendedSelection);
  DetailsAssets->setSelectionBehavior(QAbstractItemView::SelectRows);
  DetailsAssets->setDragDropMode(QAbstractItemView::DragOnly);
  DetailsAssets->setDefaultDropAction(Qt::MoveAction);
  DetailsAssets->setEditTriggers(QAbstractItemView::NoEditTriggers);
  DetailsAssets->setItemDelegateForColumn(xiiQtAssetBrowserModel::NameColumn, new AssetDetailsDelegate(DetailsAssets));
  DetailsAssets->setContextMenuPolicy(Qt::CustomContextMenu);
  DetailsAssets->header()->setSectionsClickable(true);
  DetailsAssets->header()->setSortIndicator(xiiQtAssetBrowserModel::NameColumn, Qt::AscendingOrder);
  DetailsAssets->header()->setSortIndicatorShown(true);
  DetailsAssets->setColumnWidth(xiiQtAssetBrowserModel::NameColumn, 175);
  DetailsAssets->setColumnWidth(xiiQtAssetBrowserModel::TypeColumn, 110);
  DetailsAssets->setColumnWidth(xiiQtAssetBrowserModel::StatusColumn, 145);
  DetailsAssets->setColumnWidth(xiiQtAssetBrowserModel::SizeColumn, 75);
  DetailsAssets->setColumnWidth(xiiQtAssetBrowserModel::ModifiedColumn, 145);
  connect(DetailsAssets->header(), &QHeaderView::sectionClicked, this, [this](int iColumn) {
    auto* pHeader = DetailsAssets->header();
    if (iColumn == xiiQtAssetBrowserModel::StatusColumn || iColumn == xiiQtAssetBrowserModel::SizeColumn)
      return;
    const auto order = pHeader->sortIndicatorSection() == iColumn && pHeader->sortIndicatorOrder() == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder;
    pHeader->setSortIndicator(iColumn, order);
    m_pModel->sort(iColumn, order);
  });
  connect(DetailsAssets, &QTreeView::doubleClicked, this, &xiiQtAssetBrowserWidget::on_ListAssets_doubleClicked);
  connect(DetailsAssets, &QTreeView::customContextMenuRequested, this, &xiiQtAssetBrowserWidget::on_ListAssets_customContextMenuRequested);
  auto* pRename = new QShortcut(QKeySequence(Qt::Key_F2), DetailsAssets);
  connect(pRename, &QShortcut::activated, this, [this]() {
    if (m_Mode == Mode::Browser)
      RenameCurrent();
  });

  connect(m_pModel.data(), &QAbstractItemModel::rowsInserted, this, &xiiQtAssetBrowserWidget::UpdateWorkspace);
  connect(m_pModel.data(), &QAbstractItemModel::rowsRemoved, this, &xiiQtAssetBrowserWidget::UpdateWorkspace);
  connect(m_pModel.data(), &QAbstractItemModel::modelReset, this, &xiiQtAssetBrowserWidget::UpdateWorkspace);
  SetDisplayMode(0);
  connect(m_pModel.data(), &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& first, const QModelIndex& last, const QList<int>& roles) {
    const auto selected = SelectedAssets();
    if (selected.size() != 1 || selected.constFirst().row() < first.row() || selected.constFirst().row() > last.row())
      return;

    if (roles.contains(xiiQtAssetBrowserModel::FileSize))
      m_pInspector->RefreshFileSize();
    else if (roles.contains(xiiQtAssetBrowserModel::Thumbnail))
      m_pInspector->RefreshPreview();
    else if (!m_pInspector->isHidden())
      m_pInspector->RefreshAsset(); }, Qt::QueuedConnection);

  ButtonShowItemsSubFolders->setEnabled(true);
  ButtonShowItemsSubFolders->setChecked(m_pFilter->GetShowItemsInSubFolders());
  XII_VERIFY(connect(ButtonShowItemsSubFolders, SIGNAL(toggled(bool)), this, SLOT(OnShowSubFolderItemsToggled())) != nullptr, "signal/slot connection failed");

  XII_VERIFY(connect(m_pFilter, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pFilter, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pFilter, SIGNAL(FilterChanged()), this, SLOT(OnFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pModel.data(), SIGNAL(modelReset()), this, SLOT(OnModelReset())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pModel.data(), &xiiQtAssetBrowserModel::editingFinished, this, &xiiQtAssetBrowserWidget::OnFileEditingFinished, Qt::QueuedConnection), "signal/slot connection failed");

  XII_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(OnAssetSelectionChanged(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  connect(m_pFilter, &xiiQtAssetBrowserFilter::ScopeChanged, this, &xiiQtAssetBrowserWidget::OnPathFilterChanged);
  connect(SearchWidget, &xiiQtSearchWidget::textChanged, this, &xiiQtAssetBrowserWidget::OnSearchWidgetTextChanged);

  m_pFilter->SetShowFiles(false);
  UpdateAssetTypes();

  xiiAssetCurator::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::AssetCuratorEventHandler, this));
  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::ProjectEventHandler, this));

  setAcceptDrops(true);
  OnPathFilterChanged();
  UpdatePanelVisibility();
  UpdateIndexStatus();
}

xiiQtAssetBrowserWidget::~xiiQtAssetBrowserWidget()
{
  if (m_pImportPool)
    m_pImportPool->waitForDone();

  SaveProjectNavigation();
  m_pInspector->Clear();
  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::ProjectEventHandler, this));
  xiiAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::AssetCuratorEventHandler, this));

  DetailsAssets->setModel(nullptr);
  ListAssets->setModel(nullptr);
}

void xiiQtAssetBrowserWidget::dragEnterEvent(QDragEnterEvent* pEvent)
{
  if (m_Mode == Mode::Browser && !pEvent->source() && pEvent->mimeData()->hasUrls())
    pEvent->acceptProposedAction();
}

void xiiQtAssetBrowserWidget::dragMoveEvent(QDragMoveEvent* pEvent)
{
  pEvent->acceptProposedAction();
}

void xiiQtAssetBrowserWidget::dragLeaveEvent(QDragLeaveEvent* pEvent)
{
  pEvent->accept();
}

void xiiQtAssetBrowserWidget::dropEvent(QDropEvent* pEvent)
{
  if (m_Mode != Mode::Browser || !pEvent->mimeData()->hasUrls())
  {
    pEvent->ignore();
    return;
  }

  pEvent->acceptProposedAction();
  ImportUrls(pEvent->mimeData()->urls());
}

void xiiQtAssetBrowserWidget::ImportFiles()
{
  const QStringList paths = QFileDialog::getOpenFileNames(this, tr("Import source files"));
  QList<QUrl>       urls;
  for (const QString& sPath : paths)
    urls.push_back(QUrl::fromLocalFile(sPath));

  if (!urls.isEmpty())
    ImportUrls(urls);
}

void xiiQtAssetBrowserWidget::ImportUrls(const QList<QUrl>& urls)
{
  if (m_bCopyingImport || !xiiToolsProject::IsProjectOpen())
    return;

  xiiString sTargetDir;
  if (TreeFolderFilter->currentItem())
    sTargetDir = qtToXIIString(TreeFolderFilter->currentItem()->data(0, xiiQtAssetBrowserModel::AbsolutePath).toString());
  if (sTargetDir.IsEmpty())
    sTargetDir = xiiToolsProject::GetSingleton()->GetProjectDirectory();

  QStringList sources;
  for (const auto& url : urls)
  {
    if (url.isLocalFile())
      sources.push_back(url.toLocalFile());
  }
  if (sources.isEmpty())
    return;

  if (!m_pImportPool)
    m_pImportPool = new QThreadPool(this);
  m_bCopyingImport = true;
  UpdateIndexStatus();

  const xiiString sProject = xiiToolsProject::GetSingleton()->GetProjectFile();
  auto*           pWatcher = new QFutureWatcher<ImportCopyResult>(this);
  connect(pWatcher, &QFutureWatcher<ImportCopyResult>::finished, this, [this, pWatcher, sProject]() {
    m_bCopyingImport  = false;
    const auto result = pWatcher->result();
    pWatcher->deleteLater();
    UpdateIndexStatus();
    if (!xiiToolsProject::IsProjectOpen() || xiiToolsProject::GetSingleton()->GetProjectFile() != sProject)
      return;
    if (!result.m_sError.isEmpty())
    {
      xiiQtUiServices::MessageBoxWarning(xiiFmt("{}", qtToXIIString(result.m_sError)));
      return;
    }
    for (const auto& sPath : result.m_CopiedFiles)
      xiiFileSystemModel::GetSingleton()->NotifyOfChange(sPath);
    xiiAssetDocumentGenerator::ImportAssets(result.m_ImportPaths);
  });

  QPromise<ImportCopyResult> promise;
  pWatcher->setFuture(promise.future());
  m_pImportPool->start([sources = std::move(sources), sTargetDir, promise = std::move(promise)]() mutable {
    promise.start();
    ImportCopyResult result;
    for (const QString& sPath : sources)
    {
      const QFileInfo  source(sPath);
      xiiStringBuilder sSource = qtToXIIString(source.absoluteFilePath());
      xiiStringBuilder sTarget = sTargetDir;
      sTarget.AppendPath(qtToXIIString(source.fileName()));
      sSource.MakeCleanPath();

      if (sSource == sTarget && source.isFile())
      {
        result.m_ImportPaths.PushBack(sSource);
        continue;
      }

      if (QFileInfo::exists(xiiMakeQString(sTarget)) || (source.isDir() && sTarget.IsPathBelowFolder(sSource)))
      {
        result.m_sError = xiiQtAssetBrowserWidget::tr("Cannot import “%1” into “%2”. Choose a destination outside the source folder with an available name.").arg(sPath, xiiMakeQString(sTargetDir));
        break;
      }

      if (source.isDir() && xiiOSFile::CopyFolder(sSource, sTarget, &result.m_CopiedFiles).Succeeded())
        continue;
      if (source.isFile() && QFile::copy(xiiMakeQString(sSource), xiiMakeQString(sTarget)))
      {
        result.m_CopiedFiles.PushBack(sTarget);
        continue;
      }
      result.m_sError = xiiQtAssetBrowserWidget::tr("Could not copy “%1” to “%2”. Check that the source exists and the destination is writable.").arg(sPath, xiiMakeQString(sTarget));
      break;
    }

    if (result.m_sError.isEmpty())
    {
      for (const auto& file : result.m_CopiedFiles)
        result.m_ImportPaths.PushBack(file);
    }
    else
    {
      for (const auto& file : result.m_CopiedFiles)
        xiiOSFile::DeleteFile(file).IgnoreResult();
    }
    promise.addResult(std::move(result));
    promise.finish();
  });
}

void xiiQtAssetBrowserWidget::UpdateAssetTypes()
{
  const auto& assetTypes0 = xiiAssetDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  xiiMap<xiiString, const xiiDocumentTypeDescriptor*> assetTypes;
  for (auto it : assetTypes0)
  {
    assetTypes[xiiTranslate(it.Key())] = it.Value();
  }

  {
    xiiQtScopedBlockSignals block(TypeFilter);

    TypeFilter->clear();

    if (m_Mode == Mode::Browser)
    {
      // '<All Files>' Filter
      TypeFilter->addItem(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("<All Files>"));

      // '<Importable Files>' Filter
      TypeFilter->addItem(QIcon(QLatin1String(":/EditorFramework/Icons/ImportableFileType.svg")), QLatin1String("<Importable Files>"));
    }

    // '<All Assets>' Filter
    TypeFilter->addItem(QIcon(QLatin1String(":/AssetIcons/Icons/AllAssets.svg")), QLatin1String("<All Assets>"));

    for (const auto& it : assetTypes)
    {
      const xiiStringBuilder sType(";", it.Value()->m_sDocumentTypeName, ";");
      if (!m_sAllTypesFilter.IsEmpty() && !m_sAllTypesFilter.FindSubString(sType))
        continue;

      TypeFilter->addItem(xiiQtUiServices::GetCachedIconResource(it.Value()->m_sIcon, xiiColorScheme::GetCategoryColor(it.Value()->m_sAssetCategory, xiiColorScheme::CategoryColorUsage::AssetMenuIcon)), QString::fromUtf8(it.Key(), it.Key().GetElementCount()));
      TypeFilter->setItemData(TypeFilter->count() - 1, QString::fromUtf8(it.Value()->m_sDocumentTypeName, it.Value()->m_sDocumentTypeName.GetElementCount()), Qt::UserRole);
    }
  }

  // make sure to apply the previously active type filter settings to the UI
  if (m_Mode == Mode::Browser)
  {
    xiiSet<xiiString> importExtensions;
    xiiAssetDocumentGenerator::GetSupportsFileTypes(importExtensions);
    m_pFilter->UpdateImportExtensions(importExtensions);
  }

  OnTypeFilterChanged();
}

void xiiQtAssetBrowserWidget::SetMode(Mode mode)
{
  if (m_Mode == mode)
    return;

  m_Mode = mode;

  const bool bBrowser = mode == Mode::Browser;
  if (!bBrowser)
    m_sProjectSettings.clear();
  TreeFolderFilter->SetDialogMode(!bBrowser);
  ListAssets->SetDialogMode(!bBrowser);
  DetailsAssets->setSelectionMode(bBrowser ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
  DetailsAssets->setDragDropMode(bBrowser ? QAbstractItemView::DragOnly : QAbstractItemView::NoDragDrop);
  m_pInspectorAction->setVisible(bBrowser);
  UpdatePanelVisibility();
  ButtonNew->setVisible(bBrowser);
  ButtonImport->setVisible(bBrowser);
  m_pScopes->setVisible(bBrowser);
  m_pPendingCount->setVisible(bBrowser);
  m_pErrorCount->setVisible(bBrowser);
  m_pIndexStatus->setVisible(bBrowser);
  TreeFolderFilter->setColumnHidden(1, !bBrowser);
  TypeFilter->setVisible(mode != Mode::FilePicker && width() >= 640);

  if (mode == Mode::AssetPicker)
    m_pFilter->SetShowFiles(false);
  else
  {
    m_pFilter->SetShowFiles(true);
    m_pFilter->SetShowNonImportableFiles(true);
  }
  UpdateAssetTypes();
}

void xiiQtAssetBrowserWidget::SaveState(const char* szSettingsName)
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String(szSettingsName));
  {
    Settings.setValue("SplitterGeometry", splitter->saveGeometry());
    Settings.setValue("SplitterState", splitter->saveState());
    Settings.setValue("IconSize", IconSizeSlider->value());
    Settings.setValue("DisplayMode", m_iDisplayMode);
    Settings.setValue("SidebarVisible", m_bShowSidebar);
    Settings.setValue("InspectorVisible", m_bShowInspector);
    Settings.setValue("DetailsHeader", DetailsAssets->header()->saveState());
  }
  Settings.endGroup();
}

void xiiQtAssetBrowserWidget::RestoreState(const char* szSettingsName)
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String(szSettingsName));
  {
    splitter->restoreGeometry(Settings.value("SplitterGeometry", splitter->saveGeometry()).toByteArray());
    splitter->restoreState(Settings.value("SplitterState", splitter->saveState()).toByteArray());
    IconSizeSlider->setValue(Settings.value("IconSize", IconSizeSlider->value()).toInt());

    SetDisplayMode(qBound(0, Settings.value("DisplayMode", Settings.value("IconMode", true).toBool() ? 0 : 1).toInt(), 2));
    m_bShowSidebar   = Settings.value("SidebarVisible", true).toBool();
    m_bShowInspector = Settings.value("InspectorVisible", true).toBool();
    m_pInspectorAction->setChecked(m_bShowInspector);
    UpdatePanelVisibility();
    if (DetailsAssets->header()->restoreState(Settings.value("DetailsHeader").toByteArray()))
      m_pModel->sort(DetailsAssets->header()->sortIndicatorSection(), DetailsAssets->header()->sortIndicatorOrder());
  }
  Settings.endGroup();
}

void xiiQtAssetBrowserWidget::ProjectEventHandler(const xiiToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiToolsProjectEvent::Type::ProjectOpened:
    {
      // this is necessary to detect new asset types when a plugin has been loaded (on project load)
      UpdateAssetTypes();
      RestoreProjectNavigation();
      m_pFolderCountTimer->start();
      UpdateIndexStatus();
    }
    break;
    case xiiToolsProjectEvent::Type::ProjectClosing:
      SaveProjectNavigation();
      m_pInspector->Clear();
      m_sProjectSettings.clear();
      m_pSearchTimer->stop();
      m_pReferenceTimer->stop();
      break;
    case xiiToolsProjectEvent::Type::ProjectClosed:
    {
      m_pFilter->Reset();
      m_pFilter->SetFavorites({});
      m_PathHistory  = {NavigationLocation{}};
      m_iPathHistory = 0;
      OnPathFilterChanged();
    }
    break;
    default:
      break;
  }
}

void xiiQtAssetBrowserWidget::AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset, bool bCreateSubmenu)
{
  if (m_Mode != Mode::Browser)
    return;

  const xiiHybridArray<xiiDocumentManager*, 16>& managers = xiiDocumentManager::GetAllDocumentManagers();

  xiiDynamicArray<const xiiDocumentTypeDescriptor*> documentTypes;

  QMenu* pSubMenu = bCreateSubmenu ? pMenu->addMenu(QIcon(":/GuiFoundation/Icons/DocumentAdd.svg"), tr("New")) : pMenu;

  for (xiiDocumentManager* pMan : managers)
  {
    if (!pMan->GetDynamicRTTI()->IsDerivedFrom<xiiAssetDocumentManager>())
      continue;

    pMan->GetSupportedDocumentTypes(documentTypes);
  }

  documentTypes.Sort([](const xiiDocumentTypeDescriptor* a, const xiiDocumentTypeDescriptor* b) -> bool { return xiiTranslate(a->m_sDocumentTypeName).Compare(xiiTranslate(b->m_sDocumentTypeName)) < 0; });

  QAction* pAction = pSubMenu->addAction(xiiMakeQString(xiiTranslate("Folder")));
  pAction->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Folder.svg"));
  connect(pAction, &QAction::triggered, static_cast<eqQtAssetBrowserFolderView*>(TreeFolderFilter), &eqQtAssetBrowserFolderView::NewFolder);

  pSubMenu->addSeparator();

  for (const xiiDocumentTypeDescriptor* desc : documentTypes)
  {
    if (!desc->m_bCanCreate || desc->m_sFileExtension.IsEmpty())
      continue;

    QAction* pAction = pSubMenu->addAction(xiiMakeQString(xiiTranslate(desc->m_sDocumentTypeName)));
    pAction->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(desc->m_sIcon, xiiColorScheme::GetCategoryColor(desc->m_sAssetCategory, xiiColorScheme::CategoryColorUsage::MenuEntryIcon)));
    pAction->setProperty("AssetType", desc->m_sDocumentTypeName.GetData());
    pAction->setProperty("AssetManager", QVariant::fromValue<void*>(desc->m_pManager));
    pAction->setProperty("Extension", desc->m_sFileExtension.GetData());
    pAction->setProperty("UseSelection", useSelectedAsset);

    connect(pAction, &QAction::triggered, this, &xiiQtAssetBrowserWidget::NewAsset);
  }
}

void xiiQtAssetBrowserWidget::AddImportedViaMenu(QMenu* pMenu)
{
  QModelIndexList selection = SelectedAssets();

  // Find all uses
  xiiSet<xiiUuid> importedVia;
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(xiiQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (!bImportable)
      continue;

    xiiString sAbsPath = qtToXIIString(m_pModel->data(id, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
    xiiAssetCurator::GetSingleton()->FindAllUses(sAbsPath, importedVia);
  }

  // Sort by path
  xiiHybridArray<xiiUuid, 8> importedViaSorted;
  {
    importedViaSorted.Reserve(importedVia.GetCount());
    xiiAssetCurator::xiiLockedSubAssetTable allAssets = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
    for (const xiiUuid& guid : importedVia)
    {
      if (allAssets->Contains(guid))
        importedViaSorted.PushBack(guid);
    }

    importedViaSorted.Sort([&](const xiiUuid& a, const xiiUuid& b) -> bool { return allAssets->Find(a).Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath().Compare(allAssets->Find(b).Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath()) < 0; });
  }

  if (importedViaSorted.IsEmpty())
    return;

  // Create actions to open
  QMenu* pSubMenu = pMenu->addMenu("Imported via");
  pSubMenu->setIcon(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")));

  for (const xiiUuid& guid : importedViaSorted)
  {
    const xiiAssetCurator::xiiLockedSubAsset pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);
    QIcon                                    icon      = xiiQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, xiiColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, xiiColorScheme::CategoryColorUsage::OverlayIcon));
    QString                                  sRelPath  = xiiMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());

    QAction* pAction = pSubMenu->addAction(sRelPath);
    pAction->setIcon(icon);
    pAction->setProperty("AbsPath", xiiMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath()));
    connect(pAction, &QAction::triggered, this, &xiiQtAssetBrowserWidget::OnOpenImportReferenceAsset);
  }
}

void xiiQtAssetBrowserWidget::GetSelectedImportableFiles(xiiDynamicArray<xiiString>& out_Files) const
{
  out_Files.Clear();

  QModelIndexList selection = SelectedAssets();
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(xiiQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (bImportable)
    {
      out_Files.PushBack(qtToXIIString(id.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString()));
    }
  }
}

void xiiQtAssetBrowserWidget::on_ListAssets_clicked(const QModelIndex& index)
{
  const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  Q_EMIT ItemSelected(m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
}

void xiiQtAssetBrowserWidget::on_ListAssets_activated(const QModelIndex& index)
{
  const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  Q_EMIT ItemSelected(m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
}

void xiiQtAssetBrowserWidget::on_ListAssets_doubleClicked(const QModelIndex& clicked)
{
  const QModelIndex                           index    = clicked.siblingAtColumn(xiiQtAssetBrowserModel::NameColumn);
  const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  const xiiUuid                               guid     = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();

  if (itemType.IsAnySet(xiiAssetBrowserItemFlags::Asset | xiiAssetBrowserItemFlags::SubAsset))
  {
    if (guid.IsValid())
    {
      xiiAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
    }
    Q_EMIT ItemChosen(guid, m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
  else if (itemType.IsSet(xiiAssetBrowserItemFlags::File))
  {
    Q_EMIT ItemChosen(xiiUuid::MakeInvalid(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
  else if (itemType.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory))
  {
    m_pFilter->SetPathFilter(m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data());
  }
}

QAbstractItemView* xiiQtAssetBrowserWidget::ActiveView() const
{
  return m_iDisplayMode == 2 ? static_cast<QAbstractItemView*>(DetailsAssets) : ListAssets;
}

QModelIndex xiiQtAssetBrowserWidget::CurrentAssetIndex() const
{
  return ActiveView()->currentIndex().siblingAtColumn(xiiQtAssetBrowserModel::NameColumn);
}

QModelIndexList xiiQtAssetBrowserWidget::SelectedAssets() const
{
  auto items = ListAssets->selectionModel()->selectedIndexes();
  items.removeIf([](const QModelIndex& index) { return index.column() != xiiQtAssetBrowserModel::NameColumn; });
  return items;
}

void xiiQtAssetBrowserWidget::SetDisplayMode(int iMode)
{
  m_iDisplayMode = iMode;
  m_pModel->SetIconMode(iMode == 0);
  ListAssets->SetIconMode(iMode == 0);
  ButtonIconMode->setChecked(iMode == 0);
  ButtonListMode->setChecked(iMode == 1);
  ButtonDetailsMode->setChecked(iMode == 2);
  IconSizeSlider->setVisible(iMode == 0);
  UpdateWorkspace();
  if (CurrentAssetIndex().isValid())
    ActiveView()->scrollTo(CurrentAssetIndex());
}

void xiiQtAssetBrowserWidget::UpdateWorkspace()
{
  const int iVisible  = m_pModel->rowCount();
  const int iSelected = SelectedAssets().size();
  m_pVisibleCount->setText(tr("%1 visible · %2 selected").arg(iVisible).arg(iSelected));
  AssetViews->setCurrentWidget(iVisible ? static_cast<QWidget*>(ActiveView()) : EmptyView);
}

void xiiQtAssetBrowserWidget::UpdateIndexStatus()
{
  if (m_Mode != Mode::Browser)
    return;

  auto*                                          pCurator  = xiiAssetCurator::GetSingleton();
  const bool                                     bReady    = pCurator->IsReady();
  const bool                                     bScanning = pCurator->IsCheckingFileSystem();
  xiiUInt32                                      uiTotal   = 0;
  xiiHybridArray<xiiUInt32, xiiAssetInfo::COUNT> counts;
  pCurator->GetAssetTransformStats(uiTotal, counts);
  xiiUInt32 uiPending = 0;
  xiiUInt32 uiErrors  = 0;
  for (int i = 0; i < xiiAssetInfo::COUNT; ++i)
  {
    if (i == xiiAssetInfo::UpToDate)
      continue;
    if (i < xiiAssetInfo::TransformError)
      uiPending += counts[i];
    else
      uiErrors += counts[i];
  }
  m_pPendingCount->setText(tr("%n pending", nullptr, uiPending));
  m_pPendingCount->setEnabled(uiPending > 0);
  m_pErrorCount->setText(tr("%n errors", nullptr, uiErrors));
  m_pErrorCount->setEnabled(uiErrors > 0);

  QString sIndex = tr("No asset index");
  if (bScanning)
    sIndex = tr("Indexing…");
  else if (bReady)
    sIndex = tr("%n indexed", nullptr, uiTotal);

  if (bReady)
  {
    if (const auto* pProfile = pCurator->GetActiveAssetProfile())
      sIndex += tr(" · %1").arg(xiiMakeQString(pProfile->GetConfigName()));
    const auto progress = xiiAssetProcessor::GetSingleton()->GetTransformProgress();
    if (m_bCopyingImport)
      sIndex += tr(" · Copying import sources…");
    else if (progress.m_bActive)
      sIndex += tr(" · Processing %1/%2").arg(progress.m_uiCompleted).arg(progress.m_uiTotal);
    else if (xiiAssetProcessor::GetSingleton()->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Running)
      sIndex += tr(" · Background processing enabled");
  }
  m_pIndexStatus->setText(sIndex);
  m_pIndexStatus->setToolTip(sIndex);
  if (!m_pInspector->isHidden())
    m_pInspector->RefreshProcessingStatus();

  for (const auto& entry : {qMakePair(BrowserScope::Content, uiTotal), qMakePair(BrowserScope::Attention, uiPending + uiErrors)})
  {
    auto* pItem = m_pScopes->item(static_cast<int>(entry.first));
    pItem->setText(tr("%1  (%2)").arg(pItem->data(Qt::UserRole).toString()).arg(entry.second));
  }
}

void xiiQtAssetBrowserWidget::ApplyStatusFilter(BrowserStatus status, bool bGlobal)
{
  if (bGlobal)
  {
    SearchWidget->setText({});
    m_pFilter->SetScope(BrowserScope::Content);
    if (m_Mode == Mode::Browser)
      TypeFilter->setCurrentIndex(2);
  }

  m_pFilter->SetStatusFilter(status);
}

void xiiQtAssetBrowserWidget::UpdateFolderCounts()
{
  if (m_Mode != Mode::Browser)
    return;

  QHash<QString, xiiUInt32> counts;
  {
    const auto assets = xiiAssetCurator::GetSingleton()->GetKnownAssets();
    for (const auto& asset : *assets)
    {
      QString   sPath  = xiiMakeQString(asset.Value()->m_Path.GetDataDirParentRelativePath());
      qsizetype iSlash = sPath.lastIndexOf(QLatin1Char('/'));
      while (iSlash >= 0)
      {
        sPath.truncate(iSlash);
        ++counts[sPath];
        iSlash = sPath.lastIndexOf(QLatin1Char('/'));
      }
    }
    counts[QString()] = assets->GetCount();
  }
  for (QTreeWidgetItemIterator it(TreeFolderFilter); *it; ++it)
  {
    const auto sPath = (*it)->data(0, xiiQtAssetBrowserModel::RelativePath).toString();
    const auto count = counts.value(sPath);
    (*it)->setText(1, count > 0 ? QString::number(count) : QString());
    (*it)->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
  }
}

void xiiQtAssetBrowserWidget::SaveProjectNavigation() const
{
  if (m_Mode != Mode::Browser || m_sProjectSettings.isEmpty())
    return;

  QSettings settings;
  settings.beginGroup(m_sProjectSettings);
  settings.setValue("Folder", xiiMakeQString(m_pFilter->GetPathFilter()));
  settings.setValue("Scope", static_cast<int>(m_pFilter->GetScope()));
  QStringList expanded;
  for (QTreeWidgetItemIterator it(TreeFolderFilter); *it; ++it)
  {
    if ((*it)->isExpanded())
      expanded.push_back((*it)->data(0, xiiQtAssetBrowserModel::RelativePath).toString());
  }
  settings.setValue("Expanded", expanded);
}

void xiiQtAssetBrowserWidget::RestoreProjectNavigation()
{
  if (m_Mode != Mode::Browser || !xiiToolsProject::IsProjectOpen())
    return;

  const auto& sProject = xiiToolsProject::GetSingleton()->GetProjectFile();
  m_sProjectSettings   = QStringLiteral("AssetBrowser/Projects/%1").arg(xiiHashingUtils::StringHash(sProject.GetView()), 0, 16);
  QSettings settings;
  settings.beginGroup(m_sProjectSettings);
  xiiSet<xiiUuid> favorites;
  for (const auto& sEntry : settings.value("Favorites").toStringList())
  {
    const xiiString sGuid = qtToXIIString(sEntry);
    if (xiiConversionUtils::IsStringUuid(sGuid))
      favorites.Insert(xiiConversionUtils::ConvertStringToUuid(sGuid));
  }
  m_pFilter->SetFavorites(favorites);

  const auto          paths = settings.value("Expanded").toStringList();
  const QSet<QString> expanded(paths.begin(), paths.end());
  const auto          sFolder       = settings.value("Folder").toString();
  bool                bFolderExists = sFolder.isEmpty();
  for (QTreeWidgetItemIterator it(TreeFolderFilter); *it; ++it)
  {
    const auto sPath = (*it)->data(0, xiiQtAssetBrowserModel::RelativePath).toString();
    if (sPath == sFolder)
      bFolderExists = true;
    if (settings.contains("Expanded"))
      (*it)->setExpanded(sPath.isEmpty() || expanded.contains(sPath));
  }
  const auto scope = static_cast<BrowserScope>(qBound(0, settings.value("Scope").toInt(), static_cast<int>(BrowserScope::Attention)));
  m_pFilter->SetLocation(bFolderExists ? sFolder.toUtf8().constData() : "", scope);
}

void xiiQtAssetBrowserWidget::ToggleFavorite(xiiUuid guid)
{
  if (m_sProjectSettings.isEmpty() || !guid.IsValid())
    return;

  m_pFilter->SetFavorite(guid, !m_pFilter->GetFavorites().Contains(guid));
  QStringList      favorites;
  xiiStringBuilder sGuid;
  for (const auto& id : m_pFilter->GetFavorites())
    favorites.push_back(xiiMakeQString(xiiConversionUtils::ToString(id, sGuid)));

  QSettings settings;
  settings.beginGroup(m_sProjectSettings);
  settings.setValue("Favorites", favorites);
}

QSize xiiQtAssetBrowserWidget::minimumSizeHint() const
{
  return QSize(210, 100);
}

void xiiQtAssetBrowserWidget::resizeEvent(QResizeEvent* pEvent)
{
  QWidget::resizeEvent(pEvent);
  if (!m_pBreadcrumb || !m_pStatusChips)
    return;

  const bool bWide       = pEvent->size().width() >= 640;
  const bool bNavigation = pEvent->size().width() >= 245;
  for (int i = 0; i < 4; ++i)
    ToolBarLayout->itemAt(i)->widget()->setVisible(bNavigation);
  TypeFilter->setVisible(bWide && m_Mode != Mode::FilePicker);
  m_pBreadcrumb->setVisible(bWide);
  m_pPendingCount->setVisible(bNavigation && m_Mode == Mode::Browser);
  m_pErrorCount->setVisible(bNavigation && m_Mode == Mode::Browser);
  ButtonIconMode->setVisible(bWide);
  ButtonListMode->setVisible(bWide);
  ButtonDetailsMode->setVisible(bWide);
  ButtonShowItemsSubFolders->setVisible(bWide);
  ButtonFilters->setVisible(bWide);
  UpdatePanelVisibility();
}

void xiiQtAssetBrowserWidget::UpdatePanelVisibility()
{
  if (!m_pInspectorAction || !m_pModel)
    return;

  if (width() >= 465)
    m_bForceInspector = false;

  const bool bShowInspector = m_Mode == Mode::Browser && m_bShowInspector && (width() >= 465 || m_bForceInspector);
  const bool bShowSidebar   = m_bShowSidebar && width() >= (bShowInspector ? 690 : 270);
  Sidebar->setVisible(bShowSidebar);
  if (m_pStatusChips)
    m_pStatusChips->setVisible(m_Mode == Mode::Browser && width() >= 1000);
  if (bShowInspector && m_pInspector->isHidden())
    m_pInspector->SetSelection(SelectedAssets());
  m_pInspector->setVisible(bShowInspector);
  const QSignalBlocker blocker(m_pInspectorAction);
  m_pInspectorAction->setChecked(bShowInspector);
}

void xiiQtAssetBrowserWidget::on_ButtonListMode_clicked()
{
  SetDisplayMode(1);
}

void xiiQtAssetBrowserWidget::on_ButtonIconMode_clicked()
{
  SetDisplayMode(0);
}

void xiiQtAssetBrowserWidget::on_ButtonDetailsMode_clicked()
{
  SetDisplayMode(2);
}

void xiiQtAssetBrowserWidget::on_IconSizeSlider_valueChanged(int iValue)
{
  ListAssets->SetIconScale(iValue);
}

void xiiQtAssetBrowserWidget::on_ListAssets_ViewZoomed(xiiInt32 iIconSizePercentage)
{
  xiiQtScopedBlockSignals block(IconSizeSlider);
  IconSizeSlider->setValue(iIconSizePercentage);
}

void xiiQtAssetBrowserWidget::OnTextFilterChanged()
{
  OnFilterChanged();

  m_pSearchTimer->stop();
  const QString sError = m_pFilter->GetQueryError();
  SearchWidget->setToolTip(sError.isEmpty() ? tr("Use type:, name:, path:, status:, tag:, meta.<property>:, ref: or ref-all:.") : sError);
  EmptyView->setText(sError.isEmpty() ? tr("No matching items") : sError);
  const QString sText = xiiMakeQString(m_pFilter->GetTextFilter());
  if (SearchWidget->text() != sText)
  {
    SearchWidget->setText(sText);
    QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
  }
}

void xiiQtAssetBrowserWidget::OnFilterChanged()
{
  const QString sText      = xiiMakeQString(m_pFilter->GetTextFilter());
  const bool    bAvailable = sText.isEmpty() && m_pFilter->GetScope() == BrowserScope::Content && m_pFilter->GetStatusFilter() == BrowserStatus::Any;
  ButtonShowItemsSubFolders->setEnabled(bAvailable);
  ButtonShowItemsSubFolders->blockSignals(true);
  ButtonShowItemsSubFolders->setChecked(!bAvailable || m_pFilter->GetShowItemsInSubFolders());
  ButtonShowItemsSubFolders->blockSignals(false);
}

void xiiQtAssetBrowserWidget::OnTypeFilterChanged()
{
  const int           iAllAssets = m_Mode == Mode::Browser ? 2 : 0;
  const xiiStringView sFilter    = m_pFilter->GetTypeFilter();
  int                 iType      = iAllAssets;
  if (sFilter == m_sAllTypesFilter && m_Mode == Mode::Browser && m_pFilter->GetShowFiles())
    iType = m_pFilter->GetShowNonImportableFiles() ? 0 : 1;
  else if (sFilter != m_sAllTypesFilter)
  {
    for (int i = iAllAssets + 1; i < TypeFilter->count(); ++i)
    {
      const xiiStringBuilder sType(";", qtToXIIString(TypeFilter->itemData(i).toString()), ";");
      if (sType == sFilter)
      {
        iType = i;
        break;
      }
    }
  }

  const QSignalBlocker block(TypeFilter);
  TypeFilter->setCurrentIndex(iType);

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void xiiQtAssetBrowserWidget::OnPathFilterChanged()
{
  const QString            sPath = xiiMakeQString(m_pFilter->GetPathFilter());
  const auto               scope = m_pFilter->GetScope();
  const NavigationLocation current{sPath, scope};
  if (!m_bHistoryNavigation && m_PathHistory[m_iPathHistory] != current)
  {
    m_PathHistory = m_PathHistory.mid(0, m_iPathHistory + 1);
    m_PathHistory.push_back(current);
    if (m_PathHistory.size() > 64)
      m_PathHistory.removeFirst();
    m_iPathHistory = m_PathHistory.size() - 1;
  }
  m_pBack->setEnabled(m_iPathHistory > 0);
  m_pForward->setEnabled(m_iPathHistory + 1 < m_PathHistory.size());
  m_pUp->setEnabled(!sPath.isEmpty());
  const auto parts   = sPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
  const int  iScope  = static_cast<int>(scope);
  const auto sFolder = parts.isEmpty() ? m_pScopes->item(iScope)->data(Qt::UserRole).toString() : parts.constLast();
  {
    const QSignalBlocker blockScopes(m_pScopes);
    const QSignalBlocker blockTree(TreeFolderFilter);
    m_pScopes->setCurrentRow(sPath.isEmpty() ? iScope : -1);
    if (sPath.isEmpty())
      TreeFolderFilter->clearSelection();
  }
  WorkspaceTitle->setText(sFolder);
  WorkspaceTitle->setToolTip(sPath);
  const auto link = [this](int iParts, const QString& sName, int iWidth) {
    return QStringLiteral("<a href=\"%1\">%2</a>").arg(iParts).arg(fontMetrics().elidedText(sName, Qt::ElideRight, iWidth).toHtmlEscaped());
  };
  QString sBreadcrumb = parts.isEmpty() ? link(0, sFolder, 118) :
    parts.size() == 1                   ? link(0, tr("Content"), 52) :
                                          link(-1, QStringLiteral("…"), 18);
  for (qsizetype i = qMax<qsizetype>(0, parts.size() - 2); i < parts.size(); ++i)
    sBreadcrumb += QStringLiteral(" › ") + link(i + 1, parts[i], i == parts.size() - 1 ? 82 : 46);
  m_pBreadcrumb->setText(sBreadcrumb);
  m_pBreadcrumb->setToolTip(sPath);
  m_pBreadcrumbMenu->setTitle(tr("Content path"));
  m_pBreadcrumbMenu->clear();
  m_pBreadcrumbMenu->addAction(tr("Content"), this, [this]() { m_pFilter->SetPathFilter(""); });
  for (qsizetype i = 0; i < parts.size(); ++i)
  {
    const auto sAncestor = parts.mid(0, i + 1).join(QLatin1Char('/'));
    m_pBreadcrumbMenu->addAction(sAncestor, this, [this, sAncestor]() { m_pFilter->SetPathFilter(sAncestor.toUtf8().constData()); });
  }
  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void xiiQtAssetBrowserWidget::NavigateHistory(int iDirection)
{
  const int iNext = m_iPathHistory + iDirection;
  if (iNext < 0 || iNext >= m_PathHistory.size())
    return;
  m_iPathHistory       = iNext;
  m_bHistoryNavigation = true;
  const auto location  = m_PathHistory[m_iPathHistory];
  m_pFilter->SetLocation(location.m_sPath.toUtf8().constData(), location.m_Scope);
  m_bHistoryNavigation = false;
}

void xiiQtAssetBrowserWidget::OnSearchWidgetTextChanged(const QString& text)
{
  if (text.isEmpty())
  {
    m_pSearchTimer->stop();
    m_pFilter->SetTextFilter("");
  }
  else
    m_pSearchTimer->start();
}

void xiiQtAssetBrowserWidget::keyPressEvent(QKeyEvent* e)
{
  QWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Delete && m_Mode == Mode::Browser)
  {
    e->accept();
    DeleteSelection();
    return;
  }

  if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
  {
    e->accept();
    OnListOpenAssetDocument();
    return;
  }
}

void xiiQtAssetBrowserWidget::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::BackButton || e->button() == Qt::MouseButton::ForwardButton)
  {
    NavigateHistory(e->button() == Qt::MouseButton::BackButton ? -1 : 1);
    e->accept();
    return;
  }

  QWidget::mousePressEvent(e);
}

void xiiQtAssetBrowserWidget::RenameCurrent()
{
  m_bOpenAfterRename = false;

  if (CurrentAssetIndex().isValid())
  {
    ActiveView()->edit(CurrentAssetIndex());
  }
}

void xiiQtAssetBrowserWidget::DeleteSelection()
{
  QStringList paths;
  for (const QModelIndex& index : SelectedAssets())
  {
    const xiiBitflags<xiiAssetBrowserItemFlags> flags = static_cast<xiiAssetBrowserItemFlags::Enum>(index.data(xiiQtAssetBrowserModel::ItemFlags).toInt());
    if (flags.IsAnySet(xiiAssetBrowserItemFlags::SubAsset | xiiAssetBrowserItemFlags::DataDirectory))
    {
      xiiQtUiServices::MessageBoxWarning("Sub-assets and data directories cannot be deleted.");
      return;
    }
    const auto sPath = index.data(xiiQtAssetBrowserModel::AbsolutePath).toString();
    if (!sPath.isEmpty())
      paths.push_back(sPath);
  }
  eqQtAssetBrowserFolderView::DeleteContentPaths(paths);
}

void xiiQtAssetBrowserWidget::OnImportAsAboutToShow()
{
  QMenu* pMenu = qobject_cast<QMenu*>(sender());

  if (!pMenu->actions().isEmpty())
    return;

  xiiHybridArray<xiiString, 8> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  if (filesToImport.IsEmpty())
    return;

  xiiSet<xiiString> extensions;
  xiiStringBuilder  sExt;
  for (const auto& file : filesToImport)
  {
    sExt = file.GetFileExtension();
    sExt.ToLower();
    extensions.Insert(sExt);
  }

  xiiHybridArray<xiiAssetDocumentGenerator*, 16> generators;
  xiiAssetDocumentGenerator::CreateGenerators(generators);
  xiiHybridArray<xiiAssetDocumentGenerator::ImportMode, 16> importModes;

  for (xiiAssetDocumentGenerator* pGen : generators)
  {
    for (xiiStringView ext : extensions)
    {
      if (pGen->SupportsFileType(ext))
      {
        pGen->GetImportModes({}, importModes);
        break;
      }
    }
  }

  for (const auto& mode : importModes)
  {
    QAction* act = pMenu->addAction(QIcon(xiiMakeQString(mode.m_sIcon)), xiiMakeQString(xiiTranslate(mode.m_sName)));
    act->setData(xiiMakeQString(mode.m_sName));
    connect(act, &QAction::triggered, this, &xiiQtAssetBrowserWidget::OnImportAsClicked);
  }

  xiiAssetDocumentGenerator::DestroyGenerators(generators);
}

void xiiQtAssetBrowserWidget::OnImportAsClicked()
{
  xiiHybridArray<xiiString, 8> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  QAction*  act   = qobject_cast<QAction*>(sender());
  xiiString sMode = qtToXIIString(act->data().toString());

  xiiHybridArray<xiiAssetDocumentGenerator*, 16> generators;
  xiiAssetDocumentGenerator::CreateGenerators(generators);

  xiiHybridArray<xiiAssetDocumentGenerator::ImportMode, 16> importModes;
  for (xiiAssetDocumentGenerator* pGen : generators)
  {
    importModes.Clear();
    pGen->GetImportModes({}, importModes);

    for (const auto& mode : importModes)
    {
      if (mode.m_sName == sMode)
      {
        for (const xiiString& file : filesToImport)
        {
          if (pGen->SupportsFileType(file))
          {
            pGen->Import(file, sMode, false).LogFailure();
          }
        }

        goto done;
      }
    }
  }

done:
  xiiAssetDocumentGenerator::DestroyGenerators(generators);
}

void xiiQtAssetBrowserWidget::AssetCuratorEventHandler(const xiiAssetCuratorEvent& e)
{
  if (e.m_Type == xiiAssetCuratorEvent::Type::AssetListReset)
    UpdateAssetTypes();

  if (e.m_Type == xiiAssetCuratorEvent::Type::AssetListReset || e.m_Type == xiiAssetCuratorEvent::Type::AssetAdded ||
      e.m_Type == xiiAssetCuratorEvent::Type::AssetRemoved || e.m_Type == xiiAssetCuratorEvent::Type::AssetMoved)
    m_pFolderCountTimer->start();

  if (m_pFilter->HasReferenceQuery() && !m_pReferenceTimer->isActive())
    m_pReferenceTimer->start();
}

void xiiQtAssetBrowserWidget::on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  const bool bClickedValid = TreeFolderFilter->indexAt(pt).isValid();

  if (bClickedValid)
  {
    const bool bIsRoot = TreeFolderFilter->currentItem() && TreeFolderFilter->currentItem() == TreeFolderFilter->topLevelItem(0);
    if (TreeFolderFilter->currentItem() && !bIsRoot)
    {
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), TreeFolderFilter, SLOT(TreeOpenExplorer()));
    }

    if (TreeFolderFilter->currentItem() && !bIsRoot)
    {
      // Delete
      const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)TreeFolderFilter->currentItem()->data(0, xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      QAction*                                    pDelete  = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), TreeFolderFilter, &eqQtAssetBrowserFolderView::DeleteFolder);
      if (itemType.IsSet(xiiAssetBrowserItemFlags::DataDirectory))
      {
        pDelete->setEnabled(false);
        pDelete->setToolTip("Data directories can't be deleted.");
      }

      // Create
      AddAssetCreatorMenu(&m, false);
    }

    m.addSeparator();
  }

  {
    QAction* pAction = m.addAction(QLatin1String("Show Items in hidden folders"), this, SLOT(OnShowHiddenFolderItemsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowItemsInHiddenFolders());
    pAction->setToolTip("Whether to ignore '_data' folders when showing items in sub-folders is enabled.");
  }

  {
    QAction* pAction = m.addAction(QIcon(":/GuiFoundation/Icons/SaveAll.svg"), QLatin1String("Re-save Assets in Folder"), this, SLOT(OnResaveAssets()));
    pAction->setToolTip("Opens every document and saves it. Used to get all documents to the latest version.");
  }

  m.exec(TreeFolderFilter->viewport()->mapToGlobal(pt));
}

void xiiQtAssetBrowserWidget::on_TypeFilter_currentIndexChanged(int index)
{
  xiiQtScopedBlockSignals block(TypeFilter);

  xiiStringBuilder sFilter;

  switch (m_Mode)
  {
    case Mode::Browser:
      m_pFilter->SetShowNonImportableFiles(index == 0);
      m_pFilter->SetShowFiles(index == 0 || index == 1);
      break;
    case Mode::AssetPicker:
      m_pFilter->SetShowNonImportableFiles(false);
      m_pFilter->SetShowFiles(false);
      break;
    case Mode::FilePicker:
      m_pFilter->SetShowNonImportableFiles(true);
      m_pFilter->SetShowFiles(true);
      break;
  }


  if (index <= (m_Mode == Mode::Browser ? 2 : 0))
    sFilter = m_sAllTypesFilter;
  else
    sFilter.Set(";", qtToXIIString(TypeFilter->itemData(index).toString()), ";");

  m_pFilter->SetTypeFilter(sFilter);
}

void xiiQtAssetBrowserWidget::OnShowSubFolderItemsToggled()
{
  m_pFilter->SetShowItemsInSubFolders(!m_pFilter->GetShowItemsInSubFolders());

  xiiEditorPreferencesUser* pPreferences             = xiiEditorSettings::QuerySettings<xiiEditorPreferencesUser>();
  pPreferences->m_bAssetBrowserShowItemsInSubFolders = m_pFilter->GetShowItemsInSubFolders();
}

void xiiQtAssetBrowserWidget::OnShowHiddenFolderItemsToggled()
{
  m_pFilter->SetShowItemsInHiddenFolders(!m_pFilter->GetShowItemsInHiddenFolders());
}

void xiiQtAssetBrowserWidget::OnResaveAssets()
{
  if (QTreeWidgetItem* pCurrentItem = TreeFolderFilter->currentItem())
  {
    QModelIndex      id       = TreeFolderFilter->indexFromItem(pCurrentItem);
    xiiStringBuilder sAbsPath = qtToXIIString(id.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString());

    xiiQtUiServices::MessageBoxStatus(xiiAssetCurator::GetSingleton()->ResaveAllAssets(sAbsPath), "Resaving assets failed.");
  }
}

void xiiQtAssetBrowserWidget::on_ListAssets_customContextMenuRequested(const QPoint& pt)
{
  auto*             pView   = ActiveView();
  const QModelIndex clicked = pView->indexAt(pt).siblingAtColumn(xiiQtAssetBrowserModel::NameColumn);
  if (clicked.isValid() && !pView->selectionModel()->isSelected(clicked))
    pView->selectionModel()->setCurrentIndex(clicked, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

  QMenu m;
  m.setToolTipsVisible(true);

  if (ListAssets->selectionModel()->hasSelection())
  {
    bool bShowDocumentActions = false;

    if (m_Mode == Mode::Browser)
    {
      QString sTitle = "Open Selection";
      QIcon   icon   = QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg"));

      bool bShowOpenWith = false;

      if (SelectedAssets().count() == 1)
      {
        const QModelIndex                           firstItem = SelectedAssets()[0];
        const xiiBitflags<xiiAssetBrowserItemFlags> itemType  = (xiiAssetBrowserItemFlags::Enum)firstItem.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
        if (itemType.IsAnySet(xiiAssetBrowserItemFlags::Asset | xiiAssetBrowserItemFlags::SubAsset))
        {
          sTitle               = "Open Document";
          bShowDocumentActions = true;
        }
        else if (itemType.IsSet(xiiAssetBrowserItemFlags::File))
        {
          sTitle        = "Open File";
          bShowOpenWith = true;
        }
        else if (itemType.IsAnySet(xiiAssetBrowserItemFlags::DataDirectory | xiiAssetBrowserItemFlags::Folder))
        {
          sTitle = "Enter Folder";
          icon   = QIcon(QLatin1String(":/EditorFramework/Icons/Folder.svg"));
        }
      }
      m.setDefaultAction(m.addAction(icon, sTitle, this, SLOT(OnListOpenAssetDocument())));

      if (bShowOpenWith)
      {
        m.addAction(icon, "Open With...", this, SLOT(OnListOpenFileWith()));
      }
    }
    else
      m.setDefaultAction(m.addAction(QLatin1String("Select"), this, SLOT(OnListOpenAssetDocument())));

    m.addAction(tr("Copy content paths"), this, [this]() {
      QStringList paths;
      for (const auto& index : SelectedAssets())
        paths.push_back(index.data(xiiQtAssetBrowserModel::RelativePath).toString());
      QApplication::clipboard()->setText(paths.join(QLatin1Char('\n')));
    });
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/ZoomOut.svg")), QLatin1String("Filter to this Path"), this, SLOT(OnFilterToThisPath()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnListOpenExplorer()));

    if (bShowDocumentActions)
    {
      m.addAction(QIcon(QLatin1String(":/EditorFramework/Icons/TransformAsset.svg")), QLatin1String("Transform"), this, SLOT(OnTransform()));
      const auto item = CurrentAssetIndex();
      const auto guid = item.data(xiiQtAssetBrowserModel::SubAssetGuid).value<xiiUuid>();
      m.addAction(tr("Quick preview"), this, [this]() { m_pInspector->SetSelection(SelectedAssets()); m_pInspector->ShowQuickPreview(); });
      m.addAction(tr("Inspect dependencies"), this, [this]() { ShowInspectorTab(xiiQtAssetBrowserInspector::Dependencies); });
      m.addAction(tr("Inspect processing diagnostics"), this, [this]() { ShowInspectorTab(xiiQtAssetBrowserInspector::Diagnostics); });
      m.addAction(tr("Force transform / reimport"), this, [this]() { TransformSelection(true); });
      bool bCanRegenerate = false;
      {
        const auto asset = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);
        if (asset)
        {
          const auto flags = asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_AssetDocumentFlags;
          bCanRegenerate   = !flags.IsSet(xiiAssetDocumentFlags::DisableTransform) &&
            flags.IsSet(asset->m_bMainAsset ? xiiAssetDocumentFlags::AutoThumbnailOnTransform : xiiAssetDocumentFlags::SubAssetsAutoThumbnailOnTransform);
        }
      }
      if (bCanRegenerate)
        m.addAction(tr("Regenerate thumbnail"), this, [this]() { TransformSelection(true); });
      else if (item.data(xiiQtAssetBrowserModel::TransformState).toInt() == xiiAssetInfo::NeedsThumbnail)
        m.addAction(tr("Generate thumbnail"), this, &xiiQtAssetBrowserWidget::OnTransform);
      const xiiBitflags<xiiAssetBrowserItemFlags> itemFlags = static_cast<xiiAssetBrowserItemFlags::Enum>(item.data(xiiQtAssetBrowserModel::ItemFlags).toInt());
      if (itemFlags.IsSet(xiiAssetBrowserItemFlags::Asset))
        m.addAction(tr("Duplicate asset"), this, [this, item = QPersistentModelIndex(item)]() { DuplicateAsset(item); });
      m.addAction(m_pFilter->GetFavorites().Contains(guid) ? tr("Remove from Favorites") : tr("Add to Favorites"), this, [this, guid]() { ToggleFavorite(guid); });
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnListCopyAssetGuid()));

      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct references to this asset"), this, [&]() { OnListFindAllReferences(false); });
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct and indirect references to this asset"), this, [&]() { OnListFindAllReferences(true); });
    }
  }


  if (m_Mode == Mode::Browser && ListAssets->selectionModel()->hasSelection())
  {
    QModelIndexList selection   = SelectedAssets();
    bool            bImportable = false;
    bool            bAllFiles   = true;
    for (const QModelIndex& id : selection)
    {
      bImportable |= id.data(xiiQtAssetBrowserModel::UserRoles::Importable).toBool();

      const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)id.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      if (itemType.IsAnySet(xiiAssetBrowserItemFlags::SubAsset | xiiAssetBrowserItemFlags::DataDirectory))
      {
        bAllFiles = false;
      }
    }

    // Rename
    {
      bool bCanRename = true;

      QModelIndex id = CurrentAssetIndex();

      const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)id.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      if (itemType.IsAnySet(xiiAssetBrowserItemFlags::SubAsset | xiiAssetBrowserItemFlags::DataDirectory))
      {
        bCanRename = false;
      }

      QAction* pRename = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Rename.svg")), QLatin1String("Rename"), this, SLOT(RenameCurrent()));
      pRename->setShortcut(QKeySequence("F2"));
      if (!bCanRename)
      {
        pRename->setEnabled(false);
        pRename->setToolTip("Sub-assets and data directories can't be renamed.");
      }
    }

    // Delete
    {
      QAction* pDelete = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), this, SLOT(DeleteSelection()));
      pDelete->setShortcut(QKeySequence("Del"));
      if (!bAllFiles)
      {
        pDelete->setEnabled(false);
        pDelete->setToolTip("Sub-assets and data directories can't be deleted.");
      }
    }

    // Import assets
    if (bImportable)
    {
      m.addSeparator();
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")), QLatin1String("Import..."), this, SLOT(ImportSelection()));
      QMenu* imp = m.addMenu(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")), "Import As");
      connect(imp, &QMenu::aboutToShow, this, &xiiQtAssetBrowserWidget::OnImportAsAboutToShow);
      AddImportedViaMenu(&m);
    }
  }

  m.addSeparator();

  auto pSortAction = m.addAction(QLatin1String("Sort by Recently Used"), this, SLOT(OnListToggleSortByRecentlyUsed()));
  pSortAction->setCheckable(true);
  pSortAction->setChecked(m_pFilter->GetSortByRecentUse());
  pSortAction->setEnabled(m_pFilter->GetScope() != BrowserScope::Recent);

  m.addSeparator();
  AddAssetCreatorMenu(&m, true);

  if (m_Mode == Mode::Browser)
  {
    xiiHybridArray<xiiUuid, 8> selection;
    for (const auto& item : SelectedAssets())
    {
      const auto guid = item.data(xiiQtAssetBrowserModel::SubAssetGuid).value<xiiUuid>();
      if (guid.IsValid())
        selection.PushBack(guid);
    }
    if (!selection.IsEmpty())
      s_ContextMenuActions.Broadcast({&m, selection});
  }

  m.exec(ActiveView()->viewport()->mapToGlobal(pt));
}

void xiiQtAssetBrowserWidget::OnListOpenAssetDocument()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  QModelIndexList selection = SelectedAssets();

  for (auto& index : selection)
  {
    // Only enter folders on a single selection. Otherwise the results are undefined.
    const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    if (selection.count() > 1 && itemType.IsAnySet(xiiAssetBrowserItemFlags::DataDirectory | xiiAssetBrowserItemFlags::Folder))
      continue;
    on_ListAssets_doubleClicked(index);
  }
}

void xiiQtAssetBrowserWidget::OnListOpenFileWith()
{
  if (!CurrentAssetIndex().isValid())
    return;

  xiiString sPath = m_pModel->data(CurrentAssetIndex(), xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  xiiQtUiServices::OpenWith(sPath);
}

void xiiQtAssetBrowserWidget::OnTransform()
{
  TransformSelection(false);
}

void xiiQtAssetBrowserWidget::TransformSelection(bool bForce)
{
  xiiSet<xiiUuid>          unique;
  xiiDynamicArray<xiiUuid> assets;
  for (const auto& index : SelectedAssets())
  {
    const auto guid = index.data(xiiQtAssetBrowserModel::AssetGuid).value<xiiUuid>();
    if (guid.IsValid() && !unique.Contains(guid))
    {
      unique.Insert(guid);
      assets.PushBack(guid);
    }
  }
  if (assets.IsEmpty())
    return;

  xiiBitflags<xiiTransformFlags> flags = xiiTransformFlags::TriggeredManually;
  flags.AddOrRemove(xiiTransformFlags::ForceTransform, bForce);
  const auto status = xiiAssetProcessor::GetSingleton()->QueueTransformAssets(assets, flags);
  if (status.Failed())
    xiiQtUiServices::MessageBoxStatus(status, "Could not queue the selected assets for processing.");
}

void xiiQtAssetBrowserWidget::NavigateToAsset(xiiUuid guid)
{
  if (m_Mode != Mode::Browser || !guid.IsValid())
    return;

  SearchWidget->setText({});
  m_pFilter->SetStatusFilter(BrowserStatus::Any);
  TypeFilter->setCurrentIndex(2);
  SetSelectedAsset(guid);
}

void xiiQtAssetBrowserWidget::NavigateToFile(const QString& sPath)
{
  if (m_Mode != Mode::Browser)
    return;

  SearchWidget->setText({});
  m_pFilter->SetStatusFilter(BrowserStatus::Any);
  TypeFilter->setCurrentIndex(0);
  SetSelectedFile(qtToXIIString(sPath));
}

void xiiQtAssetBrowserWidget::DuplicateAsset(const QModelIndex& index)
{
  if (!index.isValid())
    return;

  const auto guid = index.data(xiiQtAssetBrowserModel::AssetGuid).value<xiiUuid>();
  if (const auto* pDocument = xiiDocumentManager::GetDocumentByGuid(guid); pDocument && pDocument->IsModified())
  {
    xiiQtUiServices::MessageBoxInformation("Save the document before duplicating it.");
    return;
  }

  const xiiString          sSource  = qtToXIIString(index.data(xiiQtAssetBrowserModel::AbsolutePath).toString());
  xiiAssetDocumentManager* pManager = nullptr;
  {
    const auto asset = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);
    if (!asset)
      return;
    pManager = asset->m_pAssetInfo->GetManager();
  }

  xiiStringBuilder sTarget;
  for (xiiUInt32 uiCopy = 1;; ++uiCopy)
  {
    sTarget.Set(sSource);
    xiiStringBuilder sName(sSource.GetFileName());
    if (uiCopy == 1)
      sName.Append(" Copy");
    else
      sName.AppendFormat(" Copy {}", uiCopy);
    sTarget.ChangeFileName(sName);
    if (!QFileInfo::exists(xiiMakeQString(sTarget)))
      break;
  }

  xiiUuid    clone;
  const auto status = pManager->CloneDocument(sSource, sTarget, clone);
  if (status.Failed())
  {
    xiiQtUiServices::MessageBoxStatus(status, "Could not duplicate the selected asset.");
    return;
  }
  xiiAssetCurator::GetSingleton()->MainThreadTick(true);
  m_pModel->OnFileSystemUpdate();
  NavigateToAsset(clone);
}

void xiiQtAssetBrowserWidget::ShowInspectorTab(int iTab)
{
  m_pInspectorAction->setChecked(true);
  m_pInspector->SetSelection(SelectedAssets());
  m_pInspector->ShowTab(iTab);
}

void xiiQtAssetBrowserWidget::OnListOpenExplorer()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  xiiString sPath = m_pModel->data(CurrentAssetIndex(), xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  xiiQtUiServices::OpenInExplorer(sPath, true);
}

void xiiQtAssetBrowserWidget::OnListCopyAssetGuid()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  xiiStringBuilder tmp;
  xiiUuid          guid = m_pModel->data(CurrentAssetIndex(), xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();

  QClipboard* clipboard = QApplication::clipboard();
  QMimeData*  mimeData  = new QMimeData();
  mimeData->setText(xiiConversionUtils::ToString(guid, tmp).GetData());
  clipboard->setMimeData(mimeData);

  xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt("Copied asset GUID: {}", tmp), xiiTime::MakeFromSeconds(5));
}

void xiiQtAssetBrowserWidget::OnFilterToThisPath()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  xiiStringBuilder sPath = m_pModel->data(CurrentAssetIndex(), xiiQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data();
  sPath.PathParentDirectory();
  sPath.Trim("/");

  m_pFilter->SetPathFilter(sPath);
}

void xiiQtAssetBrowserWidget::OnListFindAllReferences(bool transitive)
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  xiiUuid          guid = m_pModel->data(CurrentAssetIndex(), xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();
  xiiStringBuilder sAssetGuid;
  xiiConversionUtils::ToString(guid, sAssetGuid);

  xiiStringBuilder sFilter;
  sFilter.SetFormat("{}:{}", transitive ? "ref-all" : "ref", sAssetGuid);
  m_pFilter->SetTextFilter(sFilter);
  m_pFilter->SetPathFilter("");
}

void xiiQtAssetBrowserWidget::OnSelectionTimer()
{
  if (m_pModel->rowCount() == 1)
  {
    auto index = m_pModel->index(0, 0);

    ListAssets->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  }
}

void xiiQtAssetBrowserWidget::OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  UpdateWorkspace();
  const auto selection = SelectedAssets();
  if (m_Mode == Mode::Browser && !m_pInspector->isHidden())
    m_pInspector->SetSelection(selection);
  else
    m_pInspector->Clear();

  if (!ListAssets->selectionModel()->hasSelection())
  {
    Q_EMIT ItemCleared();
  }
  else if (selection.size() == 1)
  {
    const auto& index = selection.constFirst();

    const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

    xiiUuid guid = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();
    Q_EMIT ItemSelected(guid, m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
}

void xiiQtAssetBrowserWidget::OnModelReset()
{
  m_pInspector->Clear();
  Q_EMIT ItemCleared();
}

void xiiQtAssetBrowserWidget::NewAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  xiiAssetDocumentManager* pManager       = (xiiAssetDocumentManager*)pSender->property("AssetManager").value<void*>();
  xiiString                sAssetType     = pSender->property("AssetType").toString().toUtf8().data();
  xiiString                sStartFileName = xiiTranslate(sAssetType);
  xiiString                sExtension     = pSender->property("Extension").toString().toUtf8().data();
  bool                     useSelection   = pSender->property("UseSelection").toBool();

  QString sStartDir;

  // find path
  {
    if (TreeFolderFilter->selectionModel()->hasSelection())
    {
      auto idx  = TreeFolderFilter->selectionModel()->selection().indexes()[0];
      sStartDir = TreeFolderFilter->itemFromIndex(idx)->data(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
    }

    // this will take precedence
    if (useSelection && ListAssets->selectionModel()->hasSelection())
    {
      xiiString sPath = m_pModel->data(CurrentAssetIndex(), xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

      if (!sPath.IsEmpty())
      {
        xiiStringBuilder temp = sPath;
        sPath                 = temp.GetFileDirectory();

        sStartDir = sPath.GetData();

        sStartFileName = temp.GetFileName();
      }
    }
  }

  if (sStartDir.isEmpty())
  {
    // this happens when the root node is selected
    sStartDir = xiiToolsProject::GetSingleton()->GetProjectDirectory().GetData();
  }

  xiiStringBuilder sNewAsset = qtToXIIString(sStartDir);
  xiiStringBuilder sBaseFileName;
  xiiPathUtils::MakeValidFilename(sStartFileName, ' ', sBaseFileName);
  sNewAsset.AppendFormat("/{}.{}", sBaseFileName, sExtension);

  for (xiiUInt32 i = 2; xiiOSFile::ExistsFile(sNewAsset); i++)
  {
    sNewAsset = qtToXIIString(sStartDir);
    sNewAsset.AppendFormat("/{}{}.{}", sBaseFileName, i, sExtension);
  }

  sNewAsset.MakeCleanPath();

  xiiDocument* pDoc;
  if (pManager->CreateDocument(sAssetType, sNewAsset, pDoc, xiiDocumentFlags::Default).Failed())
  {
    xiiLog::Error("Failed to create document: {}", sNewAsset);
    return;
  }

  {
    xiiStringBuilder sRelativePath = sNewAsset;
    if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sRelativePath))
    {
      m_pFilter->SetTemporaryPinnedItem(sRelativePath);
    }
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sNewAsset);
    m_pModel->OnFileSystemUpdate();
  }

  xiiInt32 iNewIndex = m_pModel->FindIndex(sNewAsset);
  if (iNewIndex != -1)
  {
    m_bOpenAfterRename = true;
    QModelIndex idx    = m_pModel->index(iNewIndex, 0);
    ListAssets->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    ActiveView()->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
    ActiveView()->edit(idx);
  }
}

void xiiQtAssetBrowserWidget::OnFileEditingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset)
{
  xiiStringBuilder sOldPath = qtToXIIString(sAbsPath);
  xiiStringBuilder sNewPath = sOldPath;
  sNewPath.ChangeFileName(qtToXIIString(sNewName));

  if (sOldPath != sNewPath)
  {
    if (xiiOSFile::MoveFileOrDirectory(sOldPath, sNewPath).Failed())
    {
      xiiQtUiServices::MessageBoxWarning(xiiFmt("Failed to rename '{}' to '{}'. Check that the destination is writable and the name is available.", sOldPath, sNewPath));
      return;
    }

    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sNewPath);
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sOldPath);

    // If we have a temporary item, make sure that any renames ensure that the item is still set as the new temporary
    // A common case is: a type filter is active that excludes a newly created asset. Thus, on creation the new asset is set as the pinned item. Editing of the item is started and the user gives it a new name and we end up here. We want the item to remain pinned.
    if (!m_pFilter->GetTemporaryPinnedItem().IsEmpty())
    {
      xiiStringBuilder sOldRelativePath = sOldPath;
      xiiStringBuilder sNewRelativePath = sNewPath;
      if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sOldRelativePath) && xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewRelativePath))
      {
        if (sOldRelativePath == m_pFilter->GetTemporaryPinnedItem())
        {
          m_pFilter->SetTemporaryPinnedItem(sNewRelativePath);
        }
      }
    }
    m_pModel->OnFileSystemUpdate();

    // it is necessary to flush the events queued on the main thread, otherwise opening the asset may not work as intended
    xiiAssetCurator::GetSingleton()->MainThreadTick(true);

    xiiInt32 iNewIndex = m_pModel->FindIndex(sNewPath);
    if (iNewIndex != -1)
    {
      QModelIndex idx = m_pModel->index(iNewIndex, 0);
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      ActiveView()->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
    }
  }

  if (m_bOpenAfterRename)
  {
    m_bOpenAfterRename = false;

    xiiInt32 iNewIndex = m_pModel->FindIndex(sNewPath);
    if (iNewIndex != -1)
    {
      QModelIndex idx = m_pModel->index(iNewIndex, 0);
      on_ListAssets_doubleClicked(idx);
    }
  }
}

void xiiQtAssetBrowserWidget::ImportSelection()
{
  xiiHybridArray<xiiString, 4> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  if (filesToImport.IsEmpty())
    return;

  xiiAssetDocumentGenerator::ImportAssets(filesToImport);

  QModelIndexList selection = SelectedAssets();
  for (const QModelIndex& id : selection)
  {
    Q_EMIT m_pModel->dataChanged(id, id);
  }
}

void xiiQtAssetBrowserWidget::OnOpenImportReferenceAsset()
{
  QAction*  pSender  = qobject_cast<QAction*>(sender());
  xiiString sAbsPath = qtToXIIString(pSender->property("AbsPath").toString());

  xiiQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, xiiDocumentFlags::RequestWindow | xiiDocumentFlags::AddToRecentFilesList);
}

void xiiQtAssetBrowserWidget::OnListToggleSortByRecentlyUsed()
{
  m_pFilter->SetSortByRecentUse(!m_pFilter->GetSortByRecentUse());
}

void xiiQtAssetBrowserWidget::SetSelectedAsset(xiiUuid preselectedAsset)
{
  if (!preselectedAsset.IsValid())
    return;

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToItem", Qt::ConnectionType::QueuedConnection, Q_ARG(xiiUuid, preselectedAsset));
}

void xiiQtAssetBrowserWidget::SetSelectedFile(xiiStringView sAbsPath)
{
  if (sAbsPath.IsEmpty())
    return;

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToFile", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, xiiMakeQString(sAbsPath)));
}

void xiiQtAssetBrowserWidget::OnScrollToItem(xiiUuid preselectedAsset)
{
  xiiStringBuilder sPath;
  {
    const auto asset = xiiAssetCurator::GetSingleton()->GetSubAsset(preselectedAsset);
    if (!asset)
      return;
    sPath = asset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
  }

  xiiStringBuilder sFolder = sPath;
  sFolder.PathParentDirectory();
  sFolder.Trim("/");
  m_pFilter->SetPathFilter(sFolder);
  m_pFilter->SetTextFilter("");

  int iRow = m_pModel->FindAssetIndex(preselectedAsset);
  if (iRow < 0 && m_Mode == Mode::Browser)
  {
    m_pFilter->SetTemporaryPinnedItem(sPath);
    iRow = m_pModel->FindAssetIndex(preselectedAsset);
  }
  if (iRow >= 0)
  {
    const auto index = m_pModel->index(iRow, 0);
    ListAssets->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    ActiveView()->scrollTo(index, QAbstractItemView::EnsureVisible);
    return;
  }

  raise();
}

void xiiQtAssetBrowserWidget::OnScrollToFile(QString sPreselectedFile)
{
  const xiiString  sAbsolute = qtToXIIString(sPreselectedFile);
  xiiStringBuilder sRelative = sAbsolute;
  if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sRelative))
  {
    xiiStringBuilder sFolder = sRelative;
    sFolder.PathParentDirectory();
    sFolder.Trim("/");
    m_pFilter->SetPathFilter(sFolder);
  }

  int iRow = m_pModel->FindIndex(sAbsolute);
  if (iRow < 0 && sRelative != sAbsolute && m_Mode == Mode::FilePicker)
  {
    m_pFilter->SetTemporaryPinnedItem(sRelative);
    iRow = m_pModel->FindIndex(sAbsolute);
  }
  if (iRow >= 0)
  {
    const auto index = m_pModel->index(iRow, 0);
    ListAssets->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    ActiveView()->scrollTo(index, QAbstractItemView::EnsureVisible);
  }
}

void xiiQtAssetBrowserWidget::ShowOnlyTheseTypeFilters(xiiStringView sFilters)
{
  m_sAllTypesFilter.Clear();
  if (!sFilters.IsEmpty())
    m_sAllTypesFilter = xiiStringBuilder(";", sFilters, ";");

  m_pFilter->SetTypeFilter(m_sAllTypesFilter);
  UpdateAssetTypes();
}

void xiiQtAssetBrowserWidget::UseFileExtensionFilters(xiiStringView sFileExtensions)
{
  m_pFilter->SetFileExtensionFilters(sFileExtensions);
}

void xiiQtAssetBrowserWidget::SetRequiredTag(xiiStringView sRequiredTag)
{
  m_pFilter->SetRequiredTag(sRequiredTag);
}
