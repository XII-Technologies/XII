/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <GuiFoundation/Widgets/TimeScrubberWidget.moc.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

xiiQtAnimationClipAssetDocumentWindow::xiiQtAnimationClipAssetDocumentWindow(xiiAnimationClipAssetDocument* pDocument) :
  xiiQtEngineDocumentWindow(pDocument), m_Clock("AssetClip")
{
  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "AnimationClipAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "AnimationClipAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AnimationClipAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  xiiQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFrameRate(25);

    m_ViewConfig.m_Camera.LookAt(xiiVec3(-1.6f, 0, 0), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(xiiVec3(0, 0, 1), xiiVec3(5.0f), xiiVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new xiiQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, "AnimationClipAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("AnimationClipAssetDockWidget");
    pPropertyPanel->setWindowTitle("Animation Clip Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new xiiQtAssetStatusIndicator(GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  // Time Scrubber
  {
    m_pTimeScrubber = new xiiQtTimeScrubberWidget(pContainer);
    m_pTimeScrubber->SetDuration(xiiTime::MakeFromSeconds(1));

    pContainer->GetLayout()->addWidget(m_pTimeScrubber);

    connect(m_pTimeScrubber, &xiiQtTimeScrubberWidget::ScrubberPosChangedEvent, this, &xiiQtAnimationClipAssetDocumentWindow::OnScrubberPosChangedEvent);
  }

  // Event Track Panel
  {
    m_pEventTrackPanel = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    m_pEventTrackPanel->setObjectName("AnimClipEventTrackDockWidget");
    m_pEventTrackPanel->setWindowTitle("Event Track");
    m_pEventTrackPanel->show();

    m_pEventTrackEditor = new xiiQtEventTrackEditorWidget(m_pEventTrackPanel);
    m_pEventTrackPanel->setWidget(m_pEventTrackEditor);

    m_pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pEventTrackPanel);

    UpdateEventTrackEditor();
  }

  // Event track editor events
  {
    connect(m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::InsertCpEvent, this, &xiiQtAnimationClipAssetDocumentWindow::onEventTrackInsertCpAt);
    connect(m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::CpMovedEvent, this, &xiiQtAnimationClipAssetDocumentWindow::onEventTrackCpMoved);
    connect(m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::CpDeletedEvent, this, &xiiQtAnimationClipAssetDocumentWindow::onEventTrackCpDeleted);

    connect(m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::BeginOperationEvent, this, &xiiQtAnimationClipAssetDocumentWindow::onEventTrackBeginOperation);
    connect(m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::EndOperationEvent, this, &xiiQtAnimationClipAssetDocumentWindow::onEventTrackEndOperation);
    connect(m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::BeginCpChangesEvent, this, &xiiQtAnimationClipAssetDocumentWindow::onEventTrackBeginCpChanges);
    connect(m_pEventTrackEditor, &xiiQtEventTrackEditorWidget::EndCpChangesEvent, this, &xiiQtAnimationClipAssetDocumentWindow::onEventTrackEndCpChanges);
  }

  FinishWindowCreation();

  GetAnimationClipDocument()->m_CommonAssetUiChangeEvent.AddEventHandler(xiiMakeDelegate(&xiiQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler, this));
}

xiiQtAnimationClipAssetDocumentWindow::~xiiQtAnimationClipAssetDocumentWindow()
{
  GetAnimationClipDocument()->m_CommonAssetUiChangeEvent.RemoveEventHandler(xiiMakeDelegate(&xiiQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler, this));
}

xiiAnimationClipAssetDocument* xiiQtAnimationClipAssetDocumentWindow::GetAnimationClipDocument()
{
  return static_cast<xiiAnimationClipAssetDocument*>(GetDocument());
}

void xiiQtAnimationClipAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    xiiSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo    = "PlaybackPos";
    msg.m_PayloadValue = (double)(m_PlaybackPosition.GetSeconds() / m_ClipDuration.GetSeconds());
    GetDocument()->SendMessageToEngine(&msg);
  }

  {
    xiiSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";
    msg.m_sPayload  = GetAnimationClipDocument()->GetProperties()->m_sPreviewMesh;
    GetDocument()->SendMessageToEngine(&msg);
  }

  {
    xiiSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "SimulationSpeed";

    if (GetAnimationClipDocument()->GetCommonAssetUiState(xiiCommonAssetUiState::Pause) != 0.0f)
      msg.m_PayloadValue = 0.0;
    else
      msg.m_PayloadValue = GetAnimationClipDocument()->GetCommonAssetUiState(xiiCommonAssetUiState::SimulationSpeed);

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void xiiQtAnimationClipAssetDocumentWindow::QueryObjectBBox(xiiInt32 iPurpose /*= 0*/)
{
  xiiQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void xiiQtAnimationClipAssetDocumentWindow::UpdateEventTrackEditor()
{
  auto* pDoc = GetAnimationClipDocument();

  m_pEventTrackEditor->SetData(pDoc->GetProperties()->m_EventTrack, m_ClipDuration.GetSeconds());
}

void xiiQtAnimationClipAssetDocumentWindow::InternalRedraw()
{
  if (m_pTimeScrubber == nullptr)
    return;

  if (m_ClipDuration.IsPositive())
  {
    m_Clock.Update();

    const double fSpeed = GetAnimationClipDocument()->GetCommonAssetUiState(xiiCommonAssetUiState::SimulationSpeed);

    if (GetAnimationClipDocument()->GetCommonAssetUiState(xiiCommonAssetUiState::Pause) == 0)
    {
      m_PlaybackPosition += m_Clock.GetTimeDiff() * fSpeed;
    }

    if (m_PlaybackPosition > m_ClipDuration)
    {
      if (GetAnimationClipDocument()->GetCommonAssetUiState(xiiCommonAssetUiState::Loop) != 0)
      {
        m_PlaybackPosition -= m_ClipDuration;
      }
      else
      {
        m_PlaybackPosition = m_ClipDuration;
      }
    }
  }

  m_PlaybackPosition = xiiMath::Clamp(m_PlaybackPosition, xiiTime::MakeZero(), m_ClipDuration);
  m_pTimeScrubber->SetScrubberPosition(m_PlaybackPosition);
  m_pEventTrackEditor->SetScrubberPosition(m_PlaybackPosition);

  xiiEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  xiiQtEngineDocumentWindow::InternalRedraw();
}

void xiiQtAnimationClipAssetDocumentWindow::ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = xiiDynamicCast<const xiiQuerySelectionBBoxResultMsgToEditor*>(pMsg0))
  {
    const xiiQuerySelectionBBoxResultMsgToEditor* pMessage = static_cast<const xiiQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (pMessage->m_vCenter.IsValid() && pMessage->m_vHalfExtents.IsValid())
    {
      m_pViewWidget->SetOrbitVolume(pMessage->m_vCenter, pMessage->m_vHalfExtents.CompMax(xiiVec3(0.1f)));
    }
    else
    {
      // try again
      QueryObjectBBox(pMessage->m_iPurpose);
    }

    return;
  }

  if (auto pMsg = xiiDynamicCast<const xiiSimpleDocumentConfigMsgToEditor*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "ClipDuration")
    {
      const xiiTime newDuration = pMsg->m_PayloadValue.Get<xiiTime>();

      if (m_ClipDuration != newDuration)
      {
        m_ClipDuration = newDuration;

        m_pTimeScrubber->SetDuration(m_ClipDuration);

        UpdateEventTrackEditor();
      }
    }
  }

  xiiQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg0);
}

void xiiQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler(const xiiCommonAssetUiState& e)
{
  xiiQtEngineDocumentWindow::CommonAssetUiEventHandler(e);

  if (e.m_State == xiiCommonAssetUiState::Restart)
  {
    m_PlaybackPosition = xiiTime::MakeFromSeconds(-1);
  }
}

void xiiQtAnimationClipAssetDocumentWindow::OnScrubberPosChangedEvent(xiiUInt64 uiNewScrubberTickPos)
{
  if (m_pTimeScrubber == nullptr || m_ClipDuration.IsZeroOrNegative())
    return;

  m_PlaybackPosition = xiiTime::MakeFromSeconds(uiNewScrubberTickPos / 4800.0);
}

void xiiQtAnimationClipAssetDocumentWindow::onEventTrackInsertCpAt(xiiInt64 tickX, QString value)
{
  auto* pDoc = GetAnimationClipDocument();
  pDoc->InsertEventTrackCpAt(tickX, value.toUtf8().data());
}

void xiiQtAnimationClipAssetDocumentWindow::onEventTrackCpMoved(xiiUInt32 cpIdx, xiiInt64 iTickX)
{
  iTickX = xiiMath::Max<xiiInt64>(iTickX, 0);

  auto* pDoc = GetAnimationClipDocument();

  xiiObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const xiiAbstractProperty* pTrackProp = xiiGetStaticRTTI<xiiAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  const xiiUuid              trackGuid  = accessor.Get<xiiUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const xiiDocumentObject*   pTrackObj  = accessor.GetObject(trackGuid);

  const xiiVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue  = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtAnimationClipAssetDocumentWindow::onEventTrackCpDeleted(xiiUInt32 cpIdx)
{
  auto* pDoc = GetAnimationClipDocument();

  xiiObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const xiiAbstractProperty* pTrackProp = xiiGetStaticRTTI<xiiAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  const xiiUuid              trackGuid  = accessor.Get<xiiUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const xiiDocumentObject*   pTrackObj  = accessor.GetObject(trackGuid);

  const xiiVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  xiiRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<xiiUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtAnimationClipAssetDocumentWindow::onEventTrackBeginOperation(QString name)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Events");
}

void xiiQtAnimationClipAssetDocumentWindow::onEventTrackEndOperation(bool commit)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void xiiQtAnimationClipAssetDocumentWindow::onEventTrackBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void xiiQtAnimationClipAssetDocumentWindow::onEventTrackEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateEventTrackEditor();
}

void xiiQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler(const xiiCommandHistoryEvent& e)
{
  // also listen to TransactionCanceled, which is sent when a no-op happens (e.g. asset transform with no change)
  // because the event track data object may still get replaced, and we have to get the new pointer
  if (e.m_Type == xiiCommandHistoryEvent::Type::TransactionEnded || e.m_Type == xiiCommandHistoryEvent::Type::UndoEnded ||
      e.m_Type == xiiCommandHistoryEvent::Type::RedoEnded ||
      e.m_Type == xiiCommandHistoryEvent::Type::TransactionCanceled)
  {
    UpdateEventTrackEditor();
  }
}
