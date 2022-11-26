#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/LUTAsset/LUTAsset.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

//////////////////////////////////////////////////////////////////////////
// xiiLUTAssetActions
//////////////////////////////////////////////////////////////////////////


void xiiLUTAssetActions::RegisterActions() {}

void xiiLUTAssetActions::UnregisterActions() {}

void xiiLUTAssetActions::MapActions(const char* szMapping, const char* szPath) {}


//////////////////////////////////////////////////////////////////////////
// xiiQtTextureAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

xiiQtLUTAssetDocumentWindow::xiiQtLUTAssetDocumentWindow(xiiLUTAssetDocument* pDocument) :
  xiiQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "LUTAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "LUTAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("LUTAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    /*
        TODO: Add live 3D preview of the LUT with a slider for the strength etc.

        SetTargetFramerate(10);

        m_ViewConfig.m_Camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
        m_ViewConfig.ApplyPerspectiveSetting(90);

        m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig);
        m_pViewWidget->ConfigureOrbitCameraVolume(xiiVec3(0), xiiVec3(1.0f), xiiVec3(-1, 0, 0));
        AddViewWidget(m_pViewWidget);
        xiiQtViewWidgetContainer* pContainer = new xiiQtViewWidgetContainer(this, m_pViewWidget, nullptr);

        setCentralWidget(pContainer);*/
  }

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("LUTAssetDockWidget");
    pPropertyPanel->setWindowTitle("LUT Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}
