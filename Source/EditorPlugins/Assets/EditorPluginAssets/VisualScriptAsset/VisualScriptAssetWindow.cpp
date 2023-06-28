#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetWindow.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Dialogs/PickDocumentObjectDlg.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>


//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptPreferences, 1, xiiRTTIDefaultAllocator<xiiVisualScriptPreferences>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DebugObject", m_DebugObject)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptPreferences::xiiVisualScriptPreferences() :
  xiiPreferences(xiiPreferences::Domain::Document, "Visual Script")
{
}

//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptAssetDocumentWindow::xiiQtVisualScriptAssetDocumentWindow(xiiDocument* pDocument, const xiiDocumentObject* pOpenContext) :
  xiiQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "VisualScriptAssetMenuBar_Legacy";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "VisualScriptAssetToolBar_Legacy";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("VisualScriptAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pScene = new xiiQtVisualScriptAssetScene(this);
  m_pScene->SetDocumentNodeManager(static_cast<const xiiDocumentNodeManager*>(pDocument->GetObjectManager()));
  m_pView = new xiiQtNodeView(this);
  m_pView->SetScene(m_pScene);
  setCentralWidget(m_pView);

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("VisualScriptAssetDockWidget");
    pPropertyPanel->setWindowTitle("Node Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  static_cast<xiiVisualScriptAssetDocument*>(pDocument)->m_ActivityEvents.AddEventHandler(
    xiiMakeDelegate(&xiiQtVisualScriptAssetScene::VisualScriptActivityEventHandler, m_pScene));
  static_cast<xiiVisualScriptAssetDocument*>(pDocument)->m_InterDocumentMessages.AddEventHandler(
    xiiMakeDelegate(&xiiQtVisualScriptAssetScene::VisualScriptInterDocumentMessageHandler, m_pScene));

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtVisualScriptAssetDocumentWindow::SelectionEventHandler, this));

  if (pOpenContext != nullptr)
  {
    m_pScene->SetDebugObject(pOpenContext->GetGuid());
  }
  else
  {
    xiiVisualScriptPreferences* pPreferences = xiiPreferences::QueryPreferences<xiiVisualScriptPreferences>(GetDocument());

    m_pScene->SetDebugObject(pPreferences->m_DebugObject);
  }

  FinishWindowCreation();

  SelectionEventHandler(xiiSelectionManagerEvent());
}

xiiQtVisualScriptAssetDocumentWindow::~xiiQtVisualScriptAssetDocumentWindow()
{
  if (GetDocument() != nullptr)
  {
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(
      xiiMakeDelegate(&xiiQtVisualScriptAssetDocumentWindow::SelectionEventHandler, this));

    GetVisualScriptDocument()->m_ActivityEvents.RemoveEventHandler(
      xiiMakeDelegate(&xiiQtVisualScriptAssetScene::VisualScriptActivityEventHandler, m_pScene));
    GetVisualScriptDocument()->m_InterDocumentMessages.RemoveEventHandler(
      xiiMakeDelegate(&xiiQtVisualScriptAssetScene::VisualScriptInterDocumentMessageHandler, m_pScene));
  }
}

xiiVisualScriptAssetDocument* xiiQtVisualScriptAssetDocumentWindow::GetVisualScriptDocument()
{
  return static_cast<xiiVisualScriptAssetDocument*>(GetDocument());
}

void xiiQtVisualScriptAssetDocumentWindow::PickDebugTarget()
{
  xiiGatherObjectsOfTypeMsgInterDoc msg;
  msg.m_pType = xiiGetStaticRTTI<xiiVisualScriptComponent>();

  GetDocument()->BroadcastInterDocumentMessage(&msg, GetDocument());

  xiiHybridArray<xiiQtPickDocumentObjectDlg::Element, 16> objects;

  const xiiUuid    scriptGuid = GetDocument()->GetGuid();
  xiiStringBuilder sScriptGuid;
  xiiConversionUtils::ToString(scriptGuid, sScriptGuid);

  for (auto& res : msg.m_Results)
  {
    const xiiDocumentObject* pObject = res.m_pDocument->GetObjectManager()->GetObject(res.m_ObjectGuid);

    if (pObject == nullptr)
      continue;

    const xiiVariant varScript = pObject->GetTypeAccessor().GetValue("Script");
    if (!varScript.IsValid() || !varScript.IsA<xiiString>())
      continue;

    if (varScript.Get<xiiString>() != sScriptGuid)
      continue;

    auto& obj          = objects.ExpandAndGetRef();
    obj.m_pObject      = pObject;
    obj.m_sDisplayName = res.m_sDisplayName;
  }


  xiiQtPickDocumentObjectDlg dlg(this, objects, m_pScene->GetDebugObject());
  dlg.exec();

  if (dlg.m_pPickedObject != nullptr)
  {
    xiiVisualScriptPreferences* pPreferences = xiiPreferences::QueryPreferences<xiiVisualScriptPreferences>(GetDocument());
    pPreferences->m_DebugObject              = dlg.m_pPickedObject->GetGuid();

    m_pScene->SetDebugObject(pPreferences->m_DebugObject);
  }
}

void xiiQtVisualScriptAssetDocumentWindow::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // Delayed execution
    QTimer::singleShot(1, [this]() {
      // Check again if the selection is empty. This could have changed due to the delayed execution.
      if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
      {
        GetDocument()->GetSelectionManager()->SetSelection(GetVisualScriptDocument()->GetPropertyObject());
      }
    });
  }
}
