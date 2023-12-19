#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetWindow.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <GuiFoundation/Widgets/TimeScrubberWidget.moc.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

xiiQtPropertyAnimAssetDocumentWindow::xiiQtPropertyAnimAssetDocumentWindow(xiiPropertyAnimAssetDocument* pDocument) :
  xiiQtGameObjectDocumentWindow(pDocument)
{
  auto ViewFactory = [](xiiQtEngineDocumentWindow* pWindow, xiiEngineViewConfig* pConfig) -> xiiQtEngineViewWidget* {
    xiiQtGameObjectViewWidget* pWidget = new xiiQtGameObjectViewWidget(nullptr, static_cast<xiiQtPropertyAnimAssetDocumentWindow*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new xiiQtQuadViewWidget(pDocument, this, ViewFactory, "PropertyAnimAssetViewToolBar");

  pDocument->SetEditToolConfigDelegate(
    [this](xiiGameObjectEditTool* pTool) { pTool->ConfigureTool(static_cast<xiiGameObjectDocument*>(GetDocument()), this, this); });

  pDocument->m_PropertyAnimEvents.AddEventHandler(xiiMakeDelegate(&xiiQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler, this));

  setCentralWidget(m_pQuadViewWidget);
  SetTargetFramerate(25);

  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "PropertyAnimAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "PropertyAnimAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("PropertyAnimAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Game Object Graph
  {
    std::unique_ptr<xiiQtDocumentTreeModel> pModel(new xiiQtGameObjectModel(pDocument->GetObjectManager()));
    pModel->AddAdapter(new xiiQtDummyAdapter(pDocument->GetObjectManager(), xiiGetStaticRTTI<xiiDocumentRoot>(), "TempObjects"));
    pModel->AddAdapter(new xiiQtGameObjectAdapter(pDocument->GetObjectManager()));

    xiiQtDocumentPanel* pGameObjectPanel = new xiiQtGameObjectPanel(this, pDocument, "PropertyAnimAsset_ScenegraphContextMenu", std::move(pModel));
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pGameObjectPanel);
  }

  // Property Grid
  {
    xiiQtDocumentPanel* pPanel = new xiiQtDocumentPanel(this, pDocument);
    pPanel->setObjectName("PropertyAnimAssetDockWidget");
    pPanel->setWindowTitle("Object Properties");
    pPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPanel, pDocument);
    pPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);
  }

  // Property Tree View
  {
    xiiQtDocumentPanel* pPanel = new xiiQtDocumentPanel(this, pDocument);
    pPanel->setObjectName("PropertyAnimPropertiesDockWidget");
    pPanel->setWindowTitle("Animated Properties");
    pPanel->show();

    m_pPropertyTreeView = new xiiQtPropertyAnimAssetTreeView(pPanel);
    m_pPropertyTreeView->setHeaderHidden(true);
    m_pPropertyTreeView->setRootIsDecorated(true);
    m_pPropertyTreeView->setUniformRowHeights(true);
    m_pPropertyTreeView->setExpandsOnDoubleClick(false);
    pPanel->setWidget(m_pPropertyTreeView);

    connect(m_pPropertyTreeView, &xiiQtPropertyAnimAssetTreeView::DeleteSelectedItemsEvent, this,
            &xiiQtPropertyAnimAssetDocumentWindow::onDeleteSelectedItems);
    connect(m_pPropertyTreeView, &xiiQtPropertyAnimAssetTreeView::RebindSelectedItemsEvent, this,
            &xiiQtPropertyAnimAssetDocumentWindow::onRebindSelectedItems);

    connect(m_pPropertyTreeView, &QTreeView::doubleClicked, this, &xiiQtPropertyAnimAssetDocumentWindow::onTreeItemDoubleClicked);
    connect(m_pPropertyTreeView, &xiiQtPropertyAnimAssetTreeView::FrameSelectedItemsEvent, this,
            &xiiQtPropertyAnimAssetDocumentWindow::onFrameSelectedTracks);

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanel);
  }

  // Property Model
  {
    m_pPropertiesModel = new xiiQtPropertyAnimModel(GetPropertyAnimDocument(), this);
    m_pPropertyTreeView->setModel(m_pPropertiesModel);
    m_pPropertyTreeView->expandToDepth(2);
    m_pPropertyTreeView->initialize();
  }

  // Selection Model
  {
    m_pPropertyTreeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    m_pPropertyTreeView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    m_pSelectionModel = new QItemSelectionModel(m_pPropertiesModel, this);
    m_pPropertyTreeView->setSelectionModel(m_pSelectionModel);

    connect(m_pSelectionModel, &QItemSelectionModel::selectionChanged, this, &xiiQtPropertyAnimAssetDocumentWindow::onSelectionChanged);
  }

  // Float Curve Panel
  {
    m_pCurvePanel = new xiiQtDocumentPanel(this, pDocument);
    m_pCurvePanel->setObjectName("PropertyAnimFloatCurveDockWidget");
    m_pCurvePanel->setWindowTitle("Curves");
    m_pCurvePanel->show();

    m_pCurveEditor = new xiiQtCurve1DEditorWidget(m_pCurvePanel);
    m_pCurvePanel->setWidget(m_pCurveEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pCurvePanel);
  }

  // Color Gradient Panel
  {
    m_pColorGradientPanel = new xiiQtDocumentPanel(this, pDocument);
    m_pColorGradientPanel->setObjectName("PropertyAnimColorGradientDockWidget");
    m_pColorGradientPanel->setWindowTitle("Color Gradient");
    m_pColorGradientPanel->show();

    m_pGradientEditor = new xiiQtColorGradientEditorWidget(m_pColorGradientPanel);
    m_pColorGradientPanel->setWidget(m_pGradientEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pColorGradientPanel);
  }

  // Event Track Panel
  {
    m_pEventTrackPanel = new xiiQtDocumentPanel(this, pDocument);
    m_pEventTrackPanel->setObjectName("PropertyAnimEventTrackDockWidget");
    m_pEventTrackPanel->setWindowTitle("Event Track");
    m_pEventTrackPanel->show();

    m_pEventTrackEditor = new xiiQtEventTrackEditorWidget(m_pEventTrackPanel);
    m_pEventTrackPanel->setWidget(m_pEventTrackEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pEventTrackPanel);
  }

  // Time Scrubber
  {
    m_pScrubberToolbar = new xiiQtTimeScrubberToolbar(this);
    connect(m_pScrubberToolbar, &xiiQtTimeScrubberToolbar::ScrubberPosChangedEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onScrubberPosChanged);
    connect(m_pScrubberToolbar, &xiiQtTimeScrubberToolbar::PlayPauseEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onPlayPauseClicked);
    connect(m_pScrubberToolbar, &xiiQtTimeScrubberToolbar::RepeatEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onRepeatClicked);
    connect(m_pScrubberToolbar, &xiiQtTimeScrubberToolbar::DurationChangedEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onDurationChangedEvent);
    connect(m_pScrubberToolbar, &xiiQtTimeScrubberToolbar::AdjustDurationEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onAdjustDurationClicked);

    addToolBar(Qt::ToolBarArea::BottomToolBarArea, m_pScrubberToolbar);
  }

  // this would show the document properties
  // pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  // Curve editor events
  {
    connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::InsertCpEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onCurveInsertCpAt);
    connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::CpMovedEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onCurveCpMoved);
    connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::CpDeletedEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onCurveCpDeleted);
    connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::TangentMovedEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onCurveTangentMoved);
    connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::TangentLinkEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onLinkCurveTangents);
    connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::CpTangentModeEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onCurveTangentModeChanged);

    connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::BeginOperationEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onCurveBeginOperation);
    connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::EndOperationEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onCurveEndOperation);
    connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::BeginCpChangesEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onCurveBeginCpChanges);
    connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::EndCpChangesEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onCurveEndCpChanges);
  }

  // Gradient editor events
  {
    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::ColorCpAdded, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientColorCpAdded);
    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::ColorCpMoved, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientColorCpMoved);
    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::ColorCpDeleted, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientColorCpDeleted);
    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::ColorCpChanged, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientColorCpChanged);

    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::AlphaCpAdded, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpAdded);
    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::AlphaCpMoved, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpMoved);
    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::AlphaCpDeleted, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpDeleted);
    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::AlphaCpChanged, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpChanged);

    connect(
      m_pGradientEditor, &xiiQtColorGradientEditorWidget::IntensityCpAdded, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpAdded);
    connect(
      m_pGradientEditor, &xiiQtColorGradientEditorWidget::IntensityCpMoved, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpMoved);
    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::IntensityCpDeleted, this,
            &xiiQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpDeleted);
    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::IntensityCpChanged, this,
            &xiiQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpChanged);

    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::BeginOperation, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientBeginOperation);
    connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::EndOperation, this, &xiiQtPropertyAnimAssetDocumentWindow::onGradientEndOperation);

    // connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::NormalizeRange, this,
    // &xiiQtPropertyAnimAssetDocumentWindow::onGradientNormalizeRange);
  }

  // Event track editor events
  {
    connect(m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::InsertCpEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onEventTrackInsertCpAt);
    connect(m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::CpMovedEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onEventTrackCpMoved);
    connect(m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::CpDeletedEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onEventTrackCpDeleted);

    connect(
      m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::BeginOperationEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onEventTrackBeginOperation);
    connect(
      m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::EndOperationEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onEventTrackEndOperation);
    connect(
      m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::BeginCpChangesEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onEventTrackBeginCpChanges);
    connect(
      m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::EndCpChangesEvent, this, &xiiQtPropertyAnimAssetDocumentWindow::onEventTrackEndCpChanges);
  }

  // GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtPropertyAnimAssetDocumentWindow::PropertyEventHandler,
  // this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    xiiMakeDelegate(&xiiQtPropertyAnimAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtPropertyAnimAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(
    xiiMakeDelegate(&xiiQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler, this));

  FinishWindowCreation();

  {
    const xiiUInt64 uiDuration = GetPropertyAnimDocument()->GetAnimationDurationTicks();
    m_pScrubberToolbar->SetDuration(uiDuration);
  }

  UpdateCurveEditor();
  UpdateGradientEditor();
  UpdateEventTrackEditor();
}

xiiQtPropertyAnimAssetDocumentWindow::~xiiQtPropertyAnimAssetDocumentWindow()
{
  GetPropertyAnimDocument()->m_PropertyAnimEvents.RemoveEventHandler(
    xiiMakeDelegate(&xiiQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler, this));
  // GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtPropertyAnimAssetDocumentWindow::PropertyEventHandler,
  // this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    xiiMakeDelegate(&xiiQtPropertyAnimAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(
    xiiMakeDelegate(&xiiQtPropertyAnimAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(
    xiiMakeDelegate(&xiiQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler, this));
}

void xiiQtPropertyAnimAssetDocumentWindow::ToggleViews(QWidget* pView)
{
  m_pQuadViewWidget->ToggleViews(pView);
}

xiiObjectAccessorBase* xiiQtPropertyAnimAssetDocumentWindow::GetObjectAccessor()
{
  return GetPropertyAnimDocument()->GetObjectAccessor();
}

bool xiiQtPropertyAnimAssetDocumentWindow::CanDuplicateSelection() const
{
  return false;
}

void xiiQtPropertyAnimAssetDocumentWindow::DuplicateSelection()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}


void xiiQtPropertyAnimAssetDocumentWindow::InternalRedraw()
{
  xiiEditorInputContext::UpdateActiveInputContext();
  {
    // do not try to redraw while the process is crashed, it is obviously futile
    if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
      return;

    {
      xiiSimulationSettingsMsgToEngine msg;
      msg.m_bSimulateWorld = false;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    {
      xiiGridSettingsMsgToEngine msg = GetGridSettings();
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    {
      xiiWorldSettingsMsgToEngine msg = GetWorldSettings();
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    GetGameObjectDocument()->SendObjectSelection();

    auto pHoveredView = GetHoveredViewWidget();

    for (auto pView : m_ViewWidgets)
    {
      pView->SetEnablePicking(pView == pHoveredView);
      pView->UpdateCameraInterpolation();
      pView->SyncToEngine();
    }
  }
  xiiQtEngineDocumentWindow::InternalRedraw();
}

void xiiQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler(const xiiPropertyAnimAssetDocumentEvent& e)
{
  if (e.m_Type == xiiPropertyAnimAssetDocumentEvent::Type::AnimationLengthChanged)
  {
    const xiiUInt64 uiDuration = e.m_pDocument->GetAnimationDurationTicks();

    m_pScrubberToolbar->SetDuration(uiDuration);
    UpdateCurveEditor();
    UpdateGradientEditor();
    UpdateEventTrackEditor();
  }
  else if (e.m_Type == xiiPropertyAnimAssetDocumentEvent::Type::ScrubberPositionChanged)
  {
    m_pScrubberToolbar->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pCurveEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pGradientEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pEventTrackEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
  }
  else if (e.m_Type == xiiPropertyAnimAssetDocumentEvent::Type::PlaybackChanged)
  {
    if (!m_bAnimTimerInFlight && GetPropertyAnimDocument()->GetPlayAnimation())
    {
      m_bAnimTimerInFlight = true;
      QTimer::singleShot(0, this, SLOT(onPlaybackTick()));
    }

    m_pScrubberToolbar->SetButtonState(GetPropertyAnimDocument()->GetPlayAnimation(), GetPropertyAnimDocument()->GetRepeatAnimation());
  }
}

void xiiQtPropertyAnimAssetDocumentWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  UpdateSelectionData();
}

void xiiQtPropertyAnimAssetDocumentWindow::UpdateSelectionData()
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  m_MapSelectionToTrack.Clear();
  m_pGradientToDisplay  = nullptr;
  m_iMapGradientToTrack = -1;
  m_CurvesToDisplay.Clear();
  m_CurvesToDisplay.m_bOwnsData         = false;
  m_CurvesToDisplay.m_uiFramesPerSecond = pDoc->GetProperties()->m_uiFramesPerSecond;

  xiiSet<xiiInt32> tracks;

  for (const QModelIndex& selIdx : m_pSelectionModel->selection().indexes())
  {
    xiiQtPropertyAnimModelTreeEntry* pTreeItem =
      reinterpret_cast<xiiQtPropertyAnimModelTreeEntry*>(m_pPropertiesModel->data(selIdx, xiiQtPropertyAnimModel::UserRoles::TreeItem).value<void*>());

    xiiQtPropertyAnimModel* pModel = m_pPropertiesModel;

    auto addRecursive = [&tracks, pModel](auto& ref_self, const xiiQtPropertyAnimModelTreeEntry* pTreeItem) -> void {
      if (pTreeItem->m_pTrack != nullptr)
        tracks.Insert(pTreeItem->m_iTrackIdx);

      for (xiiInt32 iChild : pTreeItem->m_Children)
      {
        // cannot use 'addRecursive' here, because the name is not yet fully defined
        ref_self(ref_self, &pModel->GetAllEntries()[iChild]);
      }
    };

    addRecursive(addRecursive, pTreeItem);
  }

  auto& trackArray = pDoc->GetProperties()->m_Tracks;
  for (auto it = tracks.GetIterator(); it.IsValid(); ++it)
  {
    const xiiInt32 iTrackIdx = it.Key();

    // this can happen during undo/redo when the selection still names data that has just been removed
    if (iTrackIdx >= (xiiInt32)trackArray.GetCount())
      continue;

    if (trackArray[iTrackIdx]->m_Target != xiiPropertyAnimTarget::Color)
    {
      m_MapSelectionToTrack.PushBack(iTrackIdx);

      m_CurvesToDisplay.m_Curves.PushBack(&trackArray[iTrackIdx]->m_FloatCurve);
    }
    else
    {
      m_pGradientToDisplay  = &trackArray[iTrackIdx]->m_ColorGradient;
      m_iMapGradientToTrack = iTrackIdx;
    }
  }

  m_pCurveEditor->ClearSelection();

  UpdateCurveEditor();
  UpdateGradientEditor();

  if (m_pEventTrackPanel->hasFocus() || m_pEventTrackEditor->hasFocus() || m_pEventTrackEditor->EventTrackEdit->hasFocus())
  {
  }
  else if (!m_CurvesToDisplay.m_Curves.IsEmpty())
  {
    m_pCurvePanel->raise();
  }
  else if (m_pGradientToDisplay != nullptr)
  {
    m_pColorGradientPanel->raise();
  }
}

void xiiQtPropertyAnimAssetDocumentWindow::onScrubberPosChanged(xiiUInt64 uiTick)
{
  GetPropertyAnimDocument()->SetScrubberPosition(uiTick);
}

void xiiQtPropertyAnimAssetDocumentWindow::onDeleteSelectedItems()
{
  auto pDoc     = GetPropertyAnimDocument();
  auto pHistory = pDoc->GetCommandHistory();

  pHistory->StartTransaction("Delete Tracks");

  m_pGradientToDisplay = nullptr;
  m_CurvesToDisplay.Clear();

  // delete the tracks with the highest index first, otherwise the lower indices become invalid
  // do this before modifying anything, as m_MapSelectionToTrack will change once the remove commands are executed
  xiiHybridArray<xiiInt32, 16> sortedTrackIDs;
  {
    for (xiiInt32 iTrack : m_MapSelectionToTrack)
    {
      sortedTrackIDs.PushBack(iTrack);
    }

    if (m_iMapGradientToTrack >= 0)
    {
      sortedTrackIDs.PushBack(m_iMapGradientToTrack);
    }

    sortedTrackIDs.Sort();
  }

  for (xiiUInt32 i = sortedTrackIDs.GetCount(); i > 0; --i)
  {
    const xiiInt32 iTrack = sortedTrackIDs[i - 1];

    const xiiVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrack);

    if (trackGuid.IsValid())
    {
      xiiRemoveObjectCommand cmd;
      cmd.m_Object = trackGuid.Get<xiiUuid>();

      pHistory->AddCommand(cmd).AssertSuccess();
    }
  }

  m_MapSelectionToTrack.Clear();
  m_iMapGradientToTrack = -1;

  pHistory->FinishTransaction();
}

void xiiQtPropertyAnimAssetDocumentWindow::onRebindSelectedItems()
{
  auto pDoc     = GetPropertyAnimDocument();
  auto pHistory = pDoc->GetCommandHistory();

  xiiHybridArray<xiiUuid, 16> rebindTracks;

  for (xiiInt32 iTrack : m_MapSelectionToTrack)
  {
    const xiiVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrack);

    if (trackGuid.IsValid())
      rebindTracks.PushBack(trackGuid.Get<xiiUuid>());
  }

  if (m_iMapGradientToTrack >= 0)
  {
    const xiiVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);

    if (trackGuid.IsValid())
      rebindTracks.PushBack(trackGuid.Get<xiiUuid>());
  }

  bool    ok     = false;
  QString result = QInputDialog::getText(this, "Change Animation Binding", "New Binding Path:", QLineEdit::Normal, "", &ok);

  if (!ok)
    return;

  m_pSelectionModel->clear();

  xiiStringBuilder path = result.toUtf8().data();
  ;
  path.MakeCleanPath();
  const xiiVariant varRes = path.GetData();

  pHistory->StartTransaction("Rebind Tracks");

  for (const xiiUuid guid : rebindTracks)
  {
    xiiSetObjectPropertyCommand cmdSet;
    cmdSet.m_Object = guid;

    cmdSet.m_sProperty = "ObjectPath";
    cmdSet.m_NewValue  = varRes;
    pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
  }

  pHistory->FinishTransaction();
}

void xiiQtPropertyAnimAssetDocumentWindow::onPlaybackTick()
{
  m_bAnimTimerInFlight = false;

  if (!GetPropertyAnimDocument()->GetPlayAnimation())
    return;

  GetPropertyAnimDocument()->ExecuteAnimationPlaybackStep();

  m_bAnimTimerInFlight = true;
  QTimer::singleShot(0, this, SLOT(onPlaybackTick()));
}

void xiiQtPropertyAnimAssetDocumentWindow::onPlayPauseClicked()
{
  GetPropertyAnimDocument()->SetPlayAnimation(!GetPropertyAnimDocument()->GetPlayAnimation());
}

void xiiQtPropertyAnimAssetDocumentWindow::onRepeatClicked()
{
  GetPropertyAnimDocument()->SetRepeatAnimation(!GetPropertyAnimDocument()->GetRepeatAnimation());
}

void xiiQtPropertyAnimAssetDocumentWindow::onAdjustDurationClicked()
{
  GetPropertyAnimDocument()->AdjustDuration();
}

void xiiQtPropertyAnimAssetDocumentWindow::onDurationChangedEvent(double duration)
{
  GetPropertyAnimDocument()->SetAnimationDurationTicks((xiiUInt64)(duration * 4800.0));
}

void xiiQtPropertyAnimAssetDocumentWindow::onTreeItemDoubleClicked(const QModelIndex& index)
{
  xiiQtPropertyAnimModelTreeEntry* pTreeItem =
    reinterpret_cast<xiiQtPropertyAnimModelTreeEntry*>(m_pPropertiesModel->data(index, xiiQtPropertyAnimModel::UserRoles::TreeItem).value<void*>());

  if (pTreeItem != nullptr && pTreeItem->m_pTrack != nullptr)
  {
    if (pTreeItem->m_pTrack->m_Target == xiiPropertyAnimTarget::Color)
    {
      m_pGradientEditor->FrameGradient();
      m_pColorGradientPanel->raise();
    }
    else
    {
      m_pCurveEditor->FrameCurve();
      m_pCurvePanel->raise();
    }
  }
  else
  {
    if (!m_CurvesToDisplay.m_Curves.IsEmpty())
    {
      m_pCurveEditor->FrameCurve();
      m_pCurvePanel->raise();
    }
    else if (m_pGradientToDisplay != nullptr)
    {
      m_pGradientEditor->FrameGradient();
      m_pColorGradientPanel->raise();
    }
  }
}

void xiiQtPropertyAnimAssetDocumentWindow::onFrameSelectedTracks()
{
  if (!m_CurvesToDisplay.m_Curves.IsEmpty())
  {
    m_pCurveEditor->FrameCurve();
    m_pCurvePanel->raise();
  }
  else if (m_pGradientToDisplay != nullptr)
  {
    m_pGradientEditor->FrameGradient();
    m_pColorGradientPanel->raise();
  }
}

xiiPropertyAnimAssetDocument* xiiQtPropertyAnimAssetDocumentWindow::GetPropertyAnimDocument()
{
  return static_cast<xiiPropertyAnimAssetDocument*>(GetDocument());
}

// void xiiQtPropertyAnimAssetDocumentWindow::PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
//{
//  if (static_cast<xiiPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pObject, e.m_sProperty))
//    return;
//
//  // TODO: only update what needs to be updated
//
//  //m_bUpdateEventTrackEditor = true;
//  //m_bUpdateCurveEditor = true;
//  //m_bUpdateGradientEditor = true;
//}

void xiiQtPropertyAnimAssetDocumentWindow::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  if (e.m_pNewParent &&
      static_cast<xiiPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pNewParent, e.m_sParentProperty))
    return;
  if (e.m_pPreviousParent &&
      static_cast<xiiPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pPreviousParent, e.m_sParentProperty))
    return;

  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      UpdateSelectionData();
      break;

    default:
      break;
  }
}


void xiiQtPropertyAnimAssetDocumentWindow::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  // this would show the document properties
  // if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  //{
  //  // delayed execution
  //  QTimer::singleShot(1, [this]()
  //  {
  //    GetDocument()->GetSelectionManager()->SetSelection(GetPropertyAnimDocument()->GetPropertyObject());
  //  });
  //}
}


void xiiQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler(const xiiCommandHistoryEvent& e)
{
  if (e.m_Type == xiiCommandHistoryEvent::Type::TransactionEnded || e.m_Type == xiiCommandHistoryEvent::Type::UndoEnded ||
      e.m_Type == xiiCommandHistoryEvent::Type::RedoEnded)
  {
    UpdateCurveEditor();
    UpdateGradientEditor();
    UpdateEventTrackEditor();
  }
}

void xiiQtPropertyAnimAssetDocumentWindow::UpdateCurveEditor()
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  m_pCurveEditor->SetCurveExtents(0, pDoc->GetAnimationDurationTime().GetSeconds(), true, true);
  m_pCurveEditor->SetCurves(m_CurvesToDisplay);
}


void xiiQtPropertyAnimAssetDocumentWindow::UpdateGradientEditor()
{
  if (m_pGradientToDisplay == nullptr || m_iMapGradientToTrack < 0)
  {
    // TODO: clear gradient editor ?
    xiiColorGradient empty;
    m_pGradientEditor->SetColorGradient(empty);
  }
  else
  {
    xiiColorGradient gradient;
    m_pGradientToDisplay->FillGradientData(gradient);
    m_pGradientEditor->SetColorGradient(gradient);
  }
}


void xiiQtPropertyAnimAssetDocumentWindow::UpdateEventTrackEditor()
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  m_pEventTrackEditor->SetData(GetPropertyAnimDocument()->GetProperties()->m_EventTrack, pDoc->GetAnimationDurationTime().GetSeconds());
}

void xiiQtPropertyAnimAssetDocumentWindow::onCurveBeginOperation(QString name)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands(name.toUtf8().data());
}

void xiiQtPropertyAnimAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void xiiQtPropertyAnimAssetDocumentWindow::onCurveBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void xiiQtPropertyAnimAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateCurveEditor();
}

void xiiQtPropertyAnimAssetDocumentWindow::onCurveInsertCpAt(xiiUInt32 uiCurveIdx, xiiInt64 tickX, double clickPosY)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  xiiPropertyAnimAssetDocument* pDoc      = GetPropertyAnimDocument();
  const xiiInt32                iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const xiiVariant              trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  pDoc->InsertCurveCpAt(trackGuid.Get<xiiUuid>(), tickX, clickPosY);
}

void xiiQtPropertyAnimAssetDocumentWindow::onCurveCpMoved(xiiUInt32 uiCurveIdx, xiiUInt32 cpIdx, xiiInt64 iTickX, double newPosY)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  iTickX = xiiMath::Max<xiiInt64>(iTickX, 0);

  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  const xiiInt32           iTrackIdx   = m_MapSelectionToTrack[uiCurveIdx];
  const xiiVariant         trackGuid   = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const xiiDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<xiiUuid>());
  const xiiVariant         curveGuid   = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const xiiDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<xiiUuid>());
  const xiiVariant         cpGuid       = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue  = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue  = newPosY;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtPropertyAnimAssetDocumentWindow::onCurveCpDeleted(xiiUInt32 uiCurveIdx, xiiUInt32 cpIdx)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  const xiiInt32           iTrackIdx   = m_MapSelectionToTrack[uiCurveIdx];
  const xiiVariant         trackGuid   = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const xiiDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<xiiUuid>());
  const xiiVariant         curveGuid   = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const xiiDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<xiiUuid>());
  const xiiVariant         cpGuid       = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  xiiRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<xiiUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtPropertyAnimAssetDocumentWindow::onCurveTangentMoved(xiiUInt32 uiCurveIdx, xiiUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  const xiiInt32           iTrackIdx   = m_MapSelectionToTrack[uiCurveIdx];
  const xiiVariant         trackGuid   = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const xiiDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<xiiUuid>());
  const xiiVariant         curveGuid   = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const xiiDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<xiiUuid>());
  const xiiVariant         cpGuid       = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<xiiUuid>();

  // clamp tangents to one side
  if (rightTangent)
    newPosX = xiiMath::Max(newPosX, 0.0f);
  else
    newPosX = xiiMath::Min(newPosX, 0.0f);

  cmdSet.m_sProperty = rightTangent ? "RightTangent" : "LeftTangent";
  cmdSet.m_NewValue  = xiiVec2(newPosX, newPosY);
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtPropertyAnimAssetDocumentWindow::onLinkCurveTangents(xiiUInt32 uiCurveIdx, xiiUInt32 cpIdx, bool bLink)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  const xiiInt32           iTrackIdx   = m_MapSelectionToTrack[uiCurveIdx];
  const xiiVariant         trackGuid   = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const xiiDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<xiiUuid>());
  const xiiVariant         curveGuid   = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const xiiDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<xiiUuid>());
  const xiiVariant         cpGuid       = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  xiiSetObjectPropertyCommand cmdLink;
  cmdLink.m_Object    = cpGuid.Get<xiiUuid>();
  cmdLink.m_sProperty = "Linked";
  cmdLink.m_NewValue  = bLink;
  GetDocument()->GetCommandHistory()->AddCommand(cmdLink).AssertSuccess();

  if (bLink)
  {
    const xiiVec2 leftTangent = pDoc->GetProperties()->m_Tracks[iTrackIdx]->m_FloatCurve.m_ControlPoints[cpIdx].m_LeftTangent;
    const xiiVec2 rightTangent(-leftTangent.x, -leftTangent.y);

    onCurveTangentMoved(uiCurveIdx, cpIdx, rightTangent.x, rightTangent.y, true);
  }
}

void xiiQtPropertyAnimAssetDocumentWindow::onCurveTangentModeChanged(xiiUInt32 uiCurveIdx, xiiUInt32 cpIdx, bool rightTangent, int mode)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  const xiiInt32           iTrackIdx   = m_MapSelectionToTrack[uiCurveIdx];
  const xiiVariant         trackGuid   = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const xiiDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<xiiUuid>());
  const xiiVariant         curveGuid   = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const xiiDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<xiiUuid>());
  const xiiVariant         cpGuid       = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  xiiSetObjectPropertyCommand cmd;
  cmd.m_Object    = cpGuid.Get<xiiUuid>();
  cmd.m_sProperty = rightTangent ? "RightTangentMode" : "LeftTangentMode";
  cmd.m_NewValue  = mode;
  GetDocument()->GetCommandHistory()->AddCommand(cmd).AssertSuccess();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void xiiQtPropertyAnimAssetDocumentWindow::onGradientColorCpAdded(double posX, const xiiColorGammaUB& color)
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const xiiVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  xiiInt64         tickX     = xiiColorGradientAssetData::TickFromTime(xiiTime::MakeFromSeconds(posX));
  pDoc->InsertGradientColorCpAt(trackGuid.Get<xiiUuid>(), tickX, color);
}


void xiiQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpAdded(double posX, xiiUInt8 alpha)
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const xiiVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  xiiInt64         tickX     = xiiColorGradientAssetData::TickFromTime(xiiTime::MakeFromSeconds(posX));
  pDoc->InsertGradientAlphaCpAt(trackGuid.Get<xiiUuid>(), tickX, alpha);
}


void xiiQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpAdded(double posX, float intensity)
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const xiiVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  xiiInt64         tickX     = xiiColorGradientAssetData::TickFromTime(xiiTime::MakeFromSeconds(posX));
  pDoc->InsertGradientIntensityCpAt(trackGuid.Get<xiiUuid>(), tickX, intensity);
}

void xiiQtPropertyAnimAssetDocumentWindow::MoveGradientCP(xiiInt32 idx, double newPosX, const char* szArrayName)
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const xiiVariant         trackGuid      = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const xiiDocumentObject* trackObject    = pDoc->GetObjectManager()->GetObject(trackGuid.Get<xiiUuid>());
  const xiiUuid            gradientGuid   = trackObject->GetTypeAccessor().GetValue("Gradient").Get<xiiUuid>();
  const xiiDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  xiiVariant objGuid = gradientObject->GetTypeAccessor().GetValue(szArrayName, idx);

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Move Control Point");

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue  = pDoc->GetProperties()->m_Tracks[m_iMapGradientToTrack]->m_ColorGradient.TickFromTime(xiiTime::MakeFromSeconds(newPosX));
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void xiiQtPropertyAnimAssetDocumentWindow::onGradientColorCpMoved(xiiInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "ColorCPs");
}

void xiiQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpMoved(xiiInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "AlphaCPs");
}


void xiiQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpMoved(xiiInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "IntensityCPs");
}

void xiiQtPropertyAnimAssetDocumentWindow::RemoveGradientCP(xiiInt32 idx, const char* szArrayName)
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const xiiVariant         trackGuid      = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const xiiDocumentObject* trackObject    = pDoc->GetObjectManager()->GetObject(trackGuid.Get<xiiUuid>());
  const xiiUuid            gradientGuid   = trackObject->GetTypeAccessor().GetValue("Gradient").Get<xiiUuid>();
  const xiiDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  xiiVariant objGuid = gradientObject->GetTypeAccessor().GetValue(szArrayName, idx);

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  xiiRemoveObjectCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<xiiUuid>();
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void xiiQtPropertyAnimAssetDocumentWindow::onGradientColorCpDeleted(xiiInt32 idx)
{
  RemoveGradientCP(idx, "ColorCPs");
}

void xiiQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpDeleted(xiiInt32 idx)
{
  RemoveGradientCP(idx, "AlphaCPs");
}

void xiiQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpDeleted(xiiInt32 idx)
{
  RemoveGradientCP(idx, "IntensityCPs");
}

void xiiQtPropertyAnimAssetDocumentWindow::onGradientColorCpChanged(xiiInt32 idx, const xiiColorGammaUB& color)
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const xiiVariant         trackGuid      = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const xiiDocumentObject* trackObject    = pDoc->GetObjectManager()->GetObject(trackGuid.Get<xiiUuid>());
  const xiiUuid            gradientGuid   = trackObject->GetTypeAccessor().GetValue("Gradient").Get<xiiUuid>();
  const xiiDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  xiiVariant objGuid = gradientObject->GetTypeAccessor().GetValue("ColorCPs", idx);

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Color");

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue  = color.r;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue  = color.g;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue  = color.b;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}


void xiiQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpChanged(xiiInt32 idx, xiiUInt8 alpha)
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const xiiVariant         trackGuid      = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const xiiDocumentObject* trackObject    = pDoc->GetObjectManager()->GetObject(trackGuid.Get<xiiUuid>());
  const xiiUuid            gradientGuid   = trackObject->GetTypeAccessor().GetValue("Gradient").Get<xiiUuid>();
  const xiiDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  xiiVariant objGuid = gradientObject->GetTypeAccessor().GetValue("AlphaCPs", idx);

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Alpha");

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue  = alpha;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void xiiQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpChanged(xiiInt32 idx, float intensity)
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const xiiVariant         trackGuid      = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const xiiDocumentObject* trackObject    = pDoc->GetObjectManager()->GetObject(trackGuid.Get<xiiUuid>());
  const xiiUuid            gradientGuid   = trackObject->GetTypeAccessor().GetValue("Gradient").Get<xiiUuid>();
  const xiiDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  xiiVariant objGuid = gradientObject->GetTypeAccessor().GetValue("IntensityCPs", idx);

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Intensity");

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue  = intensity;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void xiiQtPropertyAnimAssetDocumentWindow::onGradientBeginOperation()
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Gradient");
}

void xiiQtPropertyAnimAssetDocumentWindow::onGradientEndOperation(bool commit)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void xiiQtPropertyAnimAssetDocumentWindow::onEventTrackInsertCpAt(xiiInt64 tickX, QString value)
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  pDoc->InsertEventTrackCpAt(tickX, value.toUtf8().data());
}

void xiiQtPropertyAnimAssetDocumentWindow::onEventTrackCpMoved(xiiUInt32 cpIdx, xiiInt64 iTickX)
{
  iTickX = xiiMath::Max<xiiInt64>(iTickX, 0);

  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  xiiObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const xiiAbstractProperty* pTrackProp = xiiGetStaticRTTI<xiiPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  const xiiUuid              trackGuid  = accessor.Get<xiiUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const xiiDocumentObject*   pTrackObj  = accessor.GetObject(trackGuid);

  const xiiVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue  = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtPropertyAnimAssetDocumentWindow::onEventTrackCpDeleted(xiiUInt32 cpIdx)
{
  xiiPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  xiiObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const xiiAbstractProperty* pTrackProp = xiiGetStaticRTTI<xiiPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  const xiiUuid              trackGuid  = accessor.Get<xiiUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const xiiDocumentObject*   pTrackObj  = accessor.GetObject(trackGuid);

  const xiiVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  xiiRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<xiiUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtPropertyAnimAssetDocumentWindow::onEventTrackBeginOperation(QString name)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Events");
}

void xiiQtPropertyAnimAssetDocumentWindow::onEventTrackEndOperation(bool commit)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void xiiQtPropertyAnimAssetDocumentWindow::onEventTrackBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void xiiQtPropertyAnimAssetDocumentWindow::onEventTrackEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateEventTrackEditor();
}

//////////////////////////////////////////////////////////////////////////

xiiQtPropertyAnimAssetTreeView::xiiQtPropertyAnimAssetTreeView(QWidget* pParent) :
  QTreeView(pParent)
{
  setContextMenuPolicy(Qt::ContextMenuPolicy::DefaultContextMenu);
}

void xiiQtPropertyAnimAssetTreeView::initialize()
{
  connect(model(), &QAbstractItemModel::modelAboutToBeReset, this, &xiiQtPropertyAnimAssetTreeView::onBeforeModelReset);
  connect(model(), &QAbstractItemModel::modelReset, this, &xiiQtPropertyAnimAssetTreeView::onAfterModelReset);
}

void xiiQtPropertyAnimAssetTreeView::storeExpandState(const QModelIndex& parent)
{
  const QAbstractItemModel* pModel = model();

  const xiiUInt32 numRows = pModel->rowCount(parent);
  for (xiiUInt32 row = 0; row < numRows; ++row)
  {
    QModelIndex idx = pModel->index(row, 0, parent);

    const bool expanded = isExpanded(idx);

    QString path = pModel->data(idx, xiiQtPropertyAnimModel::UserRoles::Path).toString();

    if (!expanded)
      m_NotExpandedState.insert(path);

    storeExpandState(idx);
  }
}

void xiiQtPropertyAnimAssetTreeView::restoreExpandState(const QModelIndex& parent, QModelIndexList& newSelection)
{
  const QAbstractItemModel* pModel = model();

  const xiiUInt32 numRows = pModel->rowCount(parent);
  for (xiiUInt32 row = 0; row < numRows; ++row)
  {
    QModelIndex idx = pModel->index(row, 0, parent);

    QString path = pModel->data(idx, xiiQtPropertyAnimModel::UserRoles::Path).toString();

    const bool notExpanded = m_NotExpandedState.contains(path);

    if (!notExpanded)
      setExpanded(idx, true);

    if (m_SelectedItems.contains(path))
      newSelection.append(idx);

    restoreExpandState(idx, newSelection);
  }
}

void xiiQtPropertyAnimAssetTreeView::onBeforeModelReset()
{
  m_NotExpandedState.clear();
  m_SelectedItems.clear();

  storeExpandState(QModelIndex());

  const QAbstractItemModel* pModel = model();

  for (QModelIndex idx : selectionModel()->selectedRows())
  {
    QString path = pModel->data(idx, xiiQtPropertyAnimModel::UserRoles::Path).toString();
    m_SelectedItems.insert(path);
  }
}

void xiiQtPropertyAnimAssetTreeView::onAfterModelReset()
{
  QModelIndexList newSelection;
  restoreExpandState(QModelIndex(), newSelection);

  // changing the selection is not possible in onAfterModelReset, probably because the items are not yet fully valid
  // has to be done shortly after
  QTimer::singleShot(0, this, [this, newSelection]() {
    selectionModel()->clearSelection();
    for (const auto& idx : newSelection)
    {
      selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
    } });
}

void xiiQtPropertyAnimAssetTreeView::keyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key::Key_Delete)
  {
    Q_EMIT DeleteSelectedItemsEvent();
  }
  else
  {
    QTreeView::keyPressEvent(e);
  }
}

void xiiQtPropertyAnimAssetTreeView::contextMenuEvent(QContextMenuEvent* event)
{
  QMenu m;
  m.setToolTipsVisible(true);
  QAction* pFrameAction   = m.addAction("Frame Curve");
  QAction* pRemoveAction  = m.addAction("Remove Track");
  QAction* pBindingAction = m.addAction("Change Binding...");
  m.setDefaultAction(pFrameAction);

  pRemoveAction->setShortcut(Qt::Key_Delete);

  connect(pFrameAction, &QAction::triggered, this, [this](bool) { Q_EMIT FrameSelectedItemsEvent(); });
  connect(pRemoveAction, &QAction::triggered, this, [this](bool) { Q_EMIT DeleteSelectedItemsEvent(); });
  connect(pBindingAction, &QAction::triggered, this, [this](bool) { Q_EMIT RebindSelectedItemsEvent(); });

  m.exec(QCursor::pos());
}
