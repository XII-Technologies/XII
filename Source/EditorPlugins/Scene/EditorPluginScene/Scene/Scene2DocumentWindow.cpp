#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerPanel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/Scene2DocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>

#include <QInputDialog>
#include <QLayout>

xiiQtScene2DocumentWindow::xiiQtScene2DocumentWindow(xiiScene2Document* pDocument) :
  xiiQtSceneDocumentWindowBase(pDocument)
{
  auto ViewFactory = [](xiiQtEngineDocumentWindow* pWindow, xiiEngineViewConfig* pConfig) -> xiiQtEngineViewWidget* {
    xiiQtSceneViewWidget* pWidget = new xiiQtSceneViewWidget(nullptr, static_cast<xiiQtSceneDocumentWindowBase*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new xiiQtQuadViewWidget(pDocument, this, ViewFactory, "EditorPluginScene_ViewToolBar");

  pDocument->SetEditToolConfigDelegate([this](xiiGameObjectEditTool* pTool) { pTool->ConfigureTool(static_cast<xiiGameObjectDocument*>(GetDocument()), this, this); });

  {
    xiiQtDocumentPanel* pViewPanel = new xiiQtDocumentPanel(this, pDocument);
    pViewPanel->setObjectName("xiiQtDocumentPanel");
    pViewPanel->setWindowTitle("3D View");
    pViewPanel->setWidget(m_pQuadViewWidget);

    m_pDockManager->setCentralWidget(pViewPanel);
  }

  xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
  SetTargetFrameRate(pPreferences->GetMaxFramerate());

  {
    // Menu Bar
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "EditorPluginScene_Scene2MenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "EditorPluginScene_Scene2ToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  {
    // Panels
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();
    pPropertyPanel->layout()->setObjectName("PropertyPanelLayout");

    xiiQtDocumentPanel* pPanelTree = new xiiQtScenegraphPanel(this, pDocument);
    pPanelTree->show();

    xiiQtLayerPanel* pLayers = new xiiQtLayerPanel(this, pDocument);
    pLayers->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);
    XII_VERIFY(connect(pPropertyGrid, &xiiQtPropertyGridWidget::ExtendContextMenu, this, &xiiQtScene2DocumentWindow::ExtendPropertyGridContextMenu), "");

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);
    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pLayers);
    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pPanelTree);
  }
  FinishWindowCreation();
}

xiiQtScene2DocumentWindow::~xiiQtScene2DocumentWindow() = default;

bool xiiQtScene2DocumentWindow::InternalCanCloseWindow()
{
  // I guess this is to remove the focus from other widgets like input boxes, such that they may modify the document.
  setFocus();
  clearFocus();

  xiiScene2Document* pDoc = static_cast<xiiScene2Document*>(GetDocument());
  if (pDoc && pDoc->IsAnyLayerModified())
  {
    QMessageBox::StandardButton res = xiiQtUiServices::MessageBoxQuestion("Save scene and all layers before closing?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

    if (res == QMessageBox::StandardButton::Cancel)
      return false;

    if (res == QMessageBox::StandardButton::Yes)
    {
      xiiStatus err = SaveAllLayers();

      if (err.Failed())
      {
        xiiQtUiServices::GetSingleton()->MessageBoxStatus(err, "Saving the scene failed.");
        return false;
      }
    }
  }

  return true;
}

xiiStatus xiiQtScene2DocumentWindow::SaveAllLayers()
{
  xiiScene2Document* pDoc = static_cast<xiiScene2Document*>(GetDocument());

  xiiHybridArray<xiiSceneDocument*, 16> layers;
  pDoc->GetLoadedLayers(layers);

  for (auto pLayer : layers)
  {
    xiiStatus res = pLayer->SaveDocument();

    if (res.Failed())
    {
      return res;
    }
  }

  return xiiStatus(XII_SUCCESS);
}
