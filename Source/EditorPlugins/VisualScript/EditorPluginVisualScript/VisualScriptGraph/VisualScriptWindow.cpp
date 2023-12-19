#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraphQt.moc.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>


xiiQtVisualScriptWindow::xiiQtVisualScriptWindow(xiiDocument* pDocument) :
  xiiQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "VisualScriptAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "VisualScriptAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("VisualScriptAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pScene = new xiiQtVisualScriptNodeScene(this);
  m_pScene->InitScene(static_cast<const xiiDocumentNodeManager*>(pDocument->GetObjectManager()));

  m_pView = new xiiQtNodeView(this);
  m_pView->SetScene(m_pScene);
  setCentralWidget(m_pView);

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("VisualScriptAssetDockWidget");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtVisualScriptWindow::SelectionEventHandler, this));

  FinishWindowCreation();

  SelectionEventHandler(xiiSelectionManagerEvent());
}

xiiQtVisualScriptWindow::~xiiQtVisualScriptWindow()
{
  if (GetDocument() != nullptr)
  {
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(
      xiiMakeDelegate(&xiiQtVisualScriptWindow::SelectionEventHandler, this));
  }
}

void xiiQtVisualScriptWindow::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1,
                       [this]() {
                         auto pDocument         = GetDocument();
                         auto pSelectionManager = pDocument->GetSelectionManager();

                         // Check again if the selection is empty. This could have changed due to the delayed execution.
                         if (pSelectionManager->IsSelectionEmpty())
                         {
                           pSelectionManager->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
                         }
                       });
  }
}
