#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetWindow.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonPanel.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

xiiQtSkeletonAssetDocumentWindow::xiiQtSkeletonAssetDocumentWindow(xiiSkeletonAssetDocument* pDocument) :
  xiiQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "SkeletonAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "SkeletonAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SkeletonAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  xiiQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFrameRate(25);

    m_ViewConfig.m_Camera.LookAt(xiiVec3(-1.6f, 0, 0), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig, true);
    m_pViewWidget->ConfigureRelative(xiiVec3(0, 0, 1), xiiVec3(5.0f), xiiVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new xiiQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, "SkeletonAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("SkeletonAssetDockWidget");
    pPropertyPanel->setWindowTitle("Skeleton Properties");
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

  // Tree View
  {
    xiiQtDocumentPanel* pPanelTree = new xiiQtSkeletonPanel(GetContainerWindow()->GetDockManager(), this, static_cast<xiiSkeletonAssetDocument*>(pDocument));
    pPanelTree->show();

    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pPanelTree);
  }

  pDocument->Events().AddEventHandler(xiiMakeDelegate(&xiiQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler, this));

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtSkeletonAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtSkeletonAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtSkeletonAssetDocumentWindow::CommandEventHandler, this));

  FinishWindowCreation();
}

xiiQtSkeletonAssetDocumentWindow::~xiiQtSkeletonAssetDocumentWindow()
{
  static_cast<xiiSkeletonAssetDocument*>(GetDocument())->Events().RemoveEventHandler(xiiMakeDelegate(&xiiQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler, this));

  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtSkeletonAssetDocumentWindow::CommandEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtSkeletonAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtSkeletonAssetDocumentWindow::PropertyEventHandler, this));

  RestoreResource();
}

xiiSkeletonAssetDocument* xiiQtSkeletonAssetDocumentWindow::GetSkeletonDocument()
{
  return static_cast<xiiSkeletonAssetDocument*>(GetDocument());
}

void xiiQtSkeletonAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  auto* pDoc = GetSkeletonDocument();

  {
    xiiSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo    = "RenderBones";
    msg.m_PayloadValue = pDoc->GetRenderBones();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    xiiSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo    = "RenderColliders";
    msg.m_PayloadValue = pDoc->GetRenderColliders();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    xiiSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo    = "RenderJoints";
    msg.m_PayloadValue = pDoc->GetRenderJoints();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    xiiSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo    = "RenderSwingLimits";
    msg.m_PayloadValue = pDoc->GetRenderSwingLimits();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    xiiSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo    = "RenderTwistLimits";
    msg.m_PayloadValue = pDoc->GetRenderTwistLimits();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    xiiSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";

    if (pDoc->GetRenderPreviewMesh())
      msg.m_sPayload = pDoc->GetProperties()->m_sPreviewMesh;
    else
      msg.m_sPayload = "";

    GetDocument()->SendMessageToEngine(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(true);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void xiiQtSkeletonAssetDocumentWindow::QueryObjectBBox(xiiInt32 iPurpose /*= 0*/)
{
  xiiQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}


void xiiQtSkeletonAssetDocumentWindow::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  xiiStringBuilder filter;

  switch (e.m_Type)
  {
    case xiiSelectionManagerEvent::Type::SelectionCleared:
    case xiiSelectionManagerEvent::Type::SelectionSet:
    case xiiSelectionManagerEvent::Type::ObjectAdded:
    case xiiSelectionManagerEvent::Type::ObjectRemoved:
    {
      const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

      for (auto pObj : sel)
      {
        xiiVariant name = pObj->GetTypeAccessor().GetValue("Name");
        if (name.IsValid() && name.CanConvertTo<xiiString>())
        {
          filter.Append(name.ConvertTo<xiiString>().GetData(), ";");
        }
      }

      xiiSimpleDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "HighlightBones";
      msg.m_sPayload  = filter;

      GetDocument()->SendMessageToEngine(&msg);
    }
    break;

    case xiiSelectionManagerEvent::Type::ChangedRuntimeOverrideSelection:
      // ignore
      break;
  }
}

void xiiQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler(const xiiSkeletonAssetEvent& e)
{
  if (e.m_Type == xiiSkeletonAssetEvent::Transformed)
  {
    SendLiveResourcePreview();
  }
}

void xiiQtSkeletonAssetDocumentWindow::PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  // additionally do live updates for these specific properties
  if (e.m_sProperty == "LocalRotation" ||                                                 // joint offset rotation
      e.m_sProperty == "Offset" || e.m_sProperty == "Rotation" ||                         // all shapes
      e.m_sProperty == "Radius" || e.m_sProperty == "Length" ||                           // sphere and capsule
      e.m_sProperty == "Width" || e.m_sProperty == "Thickness" ||                         // box
      e.m_sProperty == "SwingLimitY" || e.m_sProperty == "SwingLimitZ" ||                 // joint swing limit
      e.m_sProperty == "TwistLimitHalfAngle" || e.m_sProperty == "TwistLimitCenterAngle") // joint twist limit
  {
    SendLiveResourcePreview();
  }
}

void xiiQtSkeletonAssetDocumentWindow::CommandEventHandler(const xiiCommandHistoryEvent& e)
{
  if (e.m_Type == xiiCommandHistoryEvent::Type::TransactionEnded || e.m_Type == xiiCommandHistoryEvent::Type::UndoEnded || e.m_Type == xiiCommandHistoryEvent::Type::RedoEnded)
  {
    SendLiveResourcePreview();
  }
}

void xiiQtSkeletonAssetDocumentWindow::SendLiveResourcePreview()
{
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  xiiSkeletonAssetDocument* pDoc = xiiDynamicCast<xiiSkeletonAssetDocument*>(GetDocument());

  if (pDoc->m_bIsTransforming)
    return;

  xiiResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Skeleton";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiContiguousMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter            memoryWriter(&streamStorage);


  // Write Path
  xiiStringBuilder sAbsFilePath = pDoc->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("xiiSkeleton");

  // Write Header
  memoryWriter << sAbsFilePath;
  const xiiUInt64    uiHash = xiiAssetCurator::GetSingleton()->GetAssetDependencyHash(pDoc->GetGuid());
  xiiAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, pDoc->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  pDoc->WriteResource(memoryWriter, *pDoc->GetProperties()).AssertSuccess();
  msg.m_Data = xiiArrayPtr<const xiiUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtSkeletonAssetDocumentWindow::RestoreResource()
{
  xiiRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Skeleton";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtSkeletonAssetDocumentWindow::InternalRedraw()
{
  xiiEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  xiiQtEngineDocumentWindow::InternalRedraw();
}

void xiiQtSkeletonAssetDocumentWindow::ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiQuerySelectionBBoxResultMsgToEditor>())
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

  xiiQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
}
