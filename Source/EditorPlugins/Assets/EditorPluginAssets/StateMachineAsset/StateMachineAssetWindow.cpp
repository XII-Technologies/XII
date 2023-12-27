#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAssetWindow.moc.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>



xiiQtStateMachineAssetDocumentWindow::xiiQtStateMachineAssetDocumentWindow(xiiDocument* pDocument) :
  xiiQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "StateMachineAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "StateMachineAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("StateMachineAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pScene = new xiiQtStateMachineAssetScene(this);
  m_pScene->InitScene(static_cast<const xiiDocumentNodeManager*>(pDocument->GetObjectManager()));

  m_pView = new xiiQtNodeView(this);
  m_pView->SetScene(m_pScene);
  setCentralWidget(m_pView);

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("StateMachineAssetDockWidget");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  FinishWindowCreation();
}

xiiQtStateMachineAssetDocumentWindow::~xiiQtStateMachineAssetDocumentWindow() = default;
