/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetScene.moc.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QTimer>

xiiQtRenderPipelineAssetDocumentWindow::xiiQtRenderPipelineAssetDocumentWindow(xiiDocument* pDocument) :
  xiiQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "RenderPipelineAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "RenderPipelineAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("RenderPipelineAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    m_pScene = new xiiQtRenderPipelineAssetScene(this);
    m_pScene->InitScene(static_cast<const xiiDocumentNodeManager*>(pDocument->GetObjectManager()));

    m_pView = new xiiQtNodeView(this);
    m_pView->SetScene(m_pScene);

    xiiQtDocumentPanel* pCentral = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("PipelineView");
    pCentral->setWindowTitle("Pipeline");
    pCentral->setWidget(m_pView);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("RenderPipelineAssetDockWidget");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new xiiQtAssetStatusIndicator((xiiAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);
  }

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtRenderPipelineAssetDocumentWindow::SelectionEventHandler, this));
  FinishWindowCreation();
  SelectionEventHandler(xiiSelectionManagerEvent());
}

xiiQtRenderPipelineAssetDocumentWindow::~xiiQtRenderPipelineAssetDocumentWindow()
{
  if (GetDocument())
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtRenderPipelineAssetDocumentWindow::SelectionEventHandler, this));
}

void xiiQtRenderPipelineAssetDocumentWindow::SelectionEventHandler(const xiiSelectionManagerEvent& event)
{
  if (!GetDocument() || !GetDocument()->GetSelectionManager()->IsSelectionEmpty())
    return;
  // Match other graph assets: select pipeline properties after the graph has
  // finished clearing its visual selection, without a recursive selection event.
  QTimer::singleShot(0, this, [this]() {
    if (GetDocument() && GetDocument()->GetSelectionManager()->IsSelectionEmpty())
      if (const auto* settings = static_cast<xiiRenderPipelineAssetDocument*>(GetDocument())->GetSettingsObject())
        GetDocument()->GetSelectionManager()->SetSelection(settings);
  });
}
