/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <SharedPluginAssets/Common/Messages.h>

xiiQtAnimatedMeshAssetDocumentWindow::xiiQtAnimatedMeshAssetDocumentWindow(xiiAnimatedMeshAssetDocument* pDocument) :
  xiiQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtAnimatedMeshAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "AnimatedMeshAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "AnimatedMeshAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AnimatedMeshAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  xiiQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFrameRate(25);

    m_ViewConfig.m_Camera.LookAt(xiiVec3(-1.6f, 0, 0), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(xiiVec3(0, 0, 1), xiiVec3(10.0f), xiiVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new xiiQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, "AnimatedMeshAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("AnimatedMeshAssetDockWidget");
    pPropertyPanel->setWindowTitle("Mesh Properties");
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

  FinishWindowCreation();

  UpdatePreview();

  m_pHighlightTimer = new QTimer();
  connect(m_pHighlightTimer, &QTimer::timeout, this, &xiiQtAnimatedMeshAssetDocumentWindow::HighlightTimer);
  m_pHighlightTimer->setInterval(500);
  m_pHighlightTimer->start();
}

xiiQtAnimatedMeshAssetDocumentWindow::~xiiQtAnimatedMeshAssetDocumentWindow()
{
  m_pHighlightTimer->stop();

  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtAnimatedMeshAssetDocumentWindow::PropertyEventHandler, this));
}

xiiAnimatedMeshAssetDocument* xiiQtAnimatedMeshAssetDocumentWindow::GetMeshDocument()
{
  return static_cast<xiiAnimatedMeshAssetDocument*>(GetDocument());
}

void xiiQtAnimatedMeshAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void xiiQtAnimatedMeshAssetDocumentWindow::QueryObjectBBox(xiiInt32 iPurpose /* = 0*/)
{
  xiiQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void xiiQtAnimatedMeshAssetDocumentWindow::PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  // if (e.m_sProperty == "Resource") // any material change
  {
    UpdatePreview();
  }
}

bool xiiQtAnimatedMeshAssetDocumentWindow::UpdatePreview()
{
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return false;

  if (GetMeshDocument()->GetProperties() == nullptr)
    return false;

  const auto& materials = GetMeshDocument()->GetProperties()->m_Slots;

  xiiEditorEngineSetMaterialsMsg msg;
  msg.m_Materials.SetCount(materials.GetCount());

  xiiUInt32 uiSlot       = 0;
  bool      bHighlighted = false;

  for (xiiUInt32 i = 0; i < materials.GetCount(); ++i)
  {
    msg.m_Materials[i] = materials[i].m_sResource;

    if (materials[i].m_bHighlight)
    {
      if (uiSlot == m_uiHighlightSlots)
      {
        bHighlighted       = true;
        msg.m_Materials[i] = "Editor/Materials/HighlightMesh.xiiMaterial";
      }

      ++uiSlot;
    }
  }

  GetEditorEngineConnection()->SendMessage(&msg);

  return bHighlighted;
}

void xiiQtAnimatedMeshAssetDocumentWindow::InternalRedraw()
{
  xiiEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  xiiQtEngineDocumentWindow::InternalRedraw();
}

void xiiQtAnimatedMeshAssetDocumentWindow::ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg)
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

void xiiQtAnimatedMeshAssetDocumentWindow::HighlightTimer()
{
  if (m_uiHighlightSlots & XII_BIT(31))
    m_uiHighlightSlots &= ~XII_BIT(31);
  else
    m_uiHighlightSlots |= XII_BIT(31);

  if (m_uiHighlightSlots & XII_BIT(31))
  {
    UpdatePreview();
  }
  else
  {
    if (UpdatePreview())
    {
      ++m_uiHighlightSlots;
    }
    else
    {
      m_uiHighlightSlots = XII_BIT(31);
    }
  }
}
