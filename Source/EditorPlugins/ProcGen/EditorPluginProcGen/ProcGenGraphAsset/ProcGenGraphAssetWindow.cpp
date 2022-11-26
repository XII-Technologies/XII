#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetWindow.moc.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

xiiProcGenGraphAssetDocumentWindow::xiiProcGenGraphAssetDocumentWindow(xiiProcGenGraphAssetDocument* pDocument) :
  xiiQtDocumentWindow(pDocument)
{
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiProcGenGraphAssetDocumentWindow::TransationEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiProcGenGraphAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "ProcGenAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "ProcGenAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ProcGenAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("ProcGenAssetDockWidget");
    pPropertyPanel->setWindowTitle("Node Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  m_pScene = new xiiQtProcGenScene(this);
  m_pScene->SetDocumentNodeManager(static_cast<const xiiDocumentNodeManager*>(pDocument->GetObjectManager()));
  m_pView = new xiiQtNodeView(this);
  m_pView->SetScene(m_pScene);
  setCentralWidget(m_pView);

  UpdatePreview();

  FinishWindowCreation();
}

xiiProcGenGraphAssetDocumentWindow::~xiiProcGenGraphAssetDocumentWindow()
{
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiProcGenGraphAssetDocumentWindow::TransationEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiProcGenGraphAssetDocumentWindow::PropertyEventHandler, this));

  RestoreResource();
}

xiiProcGenGraphAssetDocument* xiiProcGenGraphAssetDocumentWindow::GetProcGenGraphDocument()
{
  return static_cast<xiiProcGenGraphAssetDocument*>(GetDocument());
}

void xiiProcGenGraphAssetDocumentWindow::UpdatePreview()
{
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  xiiResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "ProcGen Graph";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiContiguousMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter            memoryWriter(&streamStorage);

  // Write Path
  xiiStringBuilder sAbsFilePath = GetDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("xiiProcGenGraph");
  // Write Header
  memoryWriter << sAbsFilePath;
  const xiiUInt64    uiHash = xiiAssetCurator::GetSingleton()->GetAssetDependencyHash(GetDocument()->GetGuid());
  xiiAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetProcGenGraphDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();
  // Write Asset Data
  if (GetProcGenGraphDocument()->WriteAsset(memoryWriter, xiiAssetCurator::GetSingleton()->GetActiveAssetProfile(), true).Succeeded())
  {
    msg.m_Data = xiiArrayPtr<const xiiUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

    xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void xiiProcGenGraphAssetDocumentWindow::RestoreResource()
{
  xiiRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "ProcGen Graph";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiProcGenGraphAssetDocumentWindow::PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  // this event is only needed for changes to the DebugPin
  if (e.m_sProperty == "DebugPin")
  {
    UpdatePreview();
  }
}

void xiiProcGenGraphAssetDocumentWindow::TransationEventHandler(const xiiCommandHistoryEvent& e)
{
  if (e.m_Type == xiiCommandHistoryEvent::Type::TransactionEnded || e.m_Type == xiiCommandHistoryEvent::Type::UndoEnded || e.m_Type == xiiCommandHistoryEvent::Type::RedoEnded)
  {
    UpdatePreview();
  }
}
