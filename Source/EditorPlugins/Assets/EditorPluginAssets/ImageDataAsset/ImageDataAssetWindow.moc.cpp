#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

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

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("ImageDataAssetDockWidget");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pImageWidget = new xiiQtImageWidget(this);

  setCentralWidget(m_pImageWidget);

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
