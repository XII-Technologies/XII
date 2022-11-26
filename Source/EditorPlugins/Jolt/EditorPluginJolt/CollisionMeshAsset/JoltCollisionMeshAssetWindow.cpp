#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

xiiQtJoltCollisionMeshAssetDocumentWindow::xiiQtJoltCollisionMeshAssetDocumentWindow(xiiAssetDocument* pDocument) :
  xiiQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "JoltCollisionMeshAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "JoltCollisionMeshAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("JoltCollisionMeshAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  xiiQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(xiiVec3(-1.6f, 0, 0), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureOrbitCameraVolume(xiiVec3(0, 0, 1), xiiVec3(10.0f), xiiVec3(-5, 1, 2));
    AddViewWidget(m_pViewWidget);
    pContainer = new xiiQtViewWidgetContainer(this, m_pViewWidget, "MeshAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("JoltCollisionMeshAssetDockWidget");
    pPropertyPanel->setWindowTitle("Collision Mesh Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pAssetDoc = pDocument;

  FinishWindowCreation();

  QueryObjectBBox(0);
}


void xiiQtJoltCollisionMeshAssetDocumentWindow::SendRedrawMsg()
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

  QueryObjectBBox(-1);
}

void xiiQtJoltCollisionMeshAssetDocumentWindow::QueryObjectBBox(xiiInt32 iPurpose)
{
  xiiQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void xiiQtJoltCollisionMeshAssetDocumentWindow::InternalRedraw()
{
  xiiEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  xiiQtEngineDocumentWindow::InternalRedraw();
}

void xiiQtJoltCollisionMeshAssetDocumentWindow::ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiQuerySelectionBBoxResultMsgToEditor>())
  {
    const xiiQuerySelectionBBoxResultMsgToEditor* pMessage = static_cast<const xiiQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (pMessage->m_vCenter.IsValid() && pMessage->m_vHalfExtents.IsValid())
    {
      const xiiVec3 vHalfExtents = pMessage->m_vHalfExtents.CompMax(xiiVec3(0.1f));

      m_pViewWidget->GetOrbitCamera()->SetOrbitVolume(pMessage->m_vCenter, vHalfExtents * 2.0f, pMessage->m_vCenter + xiiVec3(5, -2, 3) * vHalfExtents.GetLength() * 0.3f, pMessage->m_iPurpose == 0);
    }
    else if (pMessage->m_iPurpose == 0)
    {
      // try again
      QueryObjectBBox(pMessage->m_iPurpose);
    }

    return;
  }

  xiiQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
}
