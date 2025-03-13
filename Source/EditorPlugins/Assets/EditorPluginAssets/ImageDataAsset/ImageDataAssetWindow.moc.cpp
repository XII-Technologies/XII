#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

//////////////////////////////////////////////////////////////////////////
// xiiQtImageDataAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

xiiQtImageDataAssetDocumentWindow::xiiQtImageDataAssetDocumentWindow(xiiImageDataAssetDocument* pDocument) :
  xiiQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "ImageDataAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "ImageDataAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ImageDataAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    m_pImageWidget = new xiiQtImageWidget(this);

    xiiQtDocumentPanel* pCentral = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("ImageDataView");
    pCentral->setWindowTitle("Image");
    pCentral->setWidget(m_pImageWidget);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("ImageDataProperties");
    pPropertyPanel->setWindowTitle("Image Properties");

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

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();

  pDocument->Events().AddEventHandler(xiiMakeDelegate(&xiiQtImageDataAssetDocumentWindow::ImageDataAssetEventHandler, this), m_EventUnsubscriper);
}

void xiiQtImageDataAssetDocumentWindow::ImageDataAssetEventHandler(const xiiImageDataAssetEvent& e)
{
  if (e.m_Type != xiiImageDataAssetEvent::Type::Transformed)
    return;

  UpdatePreview();
}

void xiiQtImageDataAssetDocumentWindow::UpdatePreview()
{
  auto pImageDoc = static_cast<xiiImageDataAssetDocument*>(GetDocument());

  xiiStringBuilder path = pImageDoc->GetProperties()->m_sInputFile;
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(path))
    return;

  QPixmap pixmap;
  if (!pixmap.load(path.GetData(), nullptr, Qt::AutoColor))
    return;

  m_pImageWidget->SetImage(pixmap);
}
