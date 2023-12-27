#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

//////////////////////////////////////////////////////////////////////////
// xiiQtTextureCubeAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

xiiQtTextureCubeAssetDocumentWindow::xiiQtTextureCubeAssetDocumentWindow(xiiTextureCubeAssetDocument* pDocument) :
  xiiQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "TextureCubeAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "TextureCubeAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureCubeAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(xiiVec3(0), xiiVec3(0.0f), xiiVec3(-1, 0, 0));
    AddViewWidget(m_pViewWidget);
    xiiQtViewWidgetContainer* pContainer = new xiiQtViewWidgetContainer(this, m_pViewWidget, nullptr);

    setCentralWidget(pContainer);
  }

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("TextureCubeAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void xiiQtTextureCubeAssetDocumentWindow::InternalRedraw()
{
  xiiEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  xiiQtEngineDocumentWindow::InternalRedraw();
}

void xiiQtTextureCubeAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const xiiTextureCubeAssetDocument* pDoc = static_cast<const xiiTextureCubeAssetDocument*>(GetDocument());

    xiiDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ChannelMode";
    msg.m_iValue    = pDoc->m_ChannelMode.GetValue();
    msg.m_fValue    = pDoc->m_iTextureLod;

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}
