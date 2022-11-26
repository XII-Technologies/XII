#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QInputDialog>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiQtSceneDocumentWindow::xiiQtSceneDocumentWindow(xiiSceneDocument* pDocument) :
  xiiQtSceneDocumentWindowBase(pDocument)
{
  auto ViewFactory = [](xiiQtEngineDocumentWindow* pWindow, xiiEngineViewConfig* pConfig) -> xiiQtEngineViewWidget* {
    xiiQtSceneViewWidget* pWidget = new xiiQtSceneViewWidget(nullptr, static_cast<xiiQtSceneDocumentWindowBase*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new xiiQtQuadViewWidget(pDocument, this, ViewFactory, "EditorPluginScene_ViewToolBar");

  pDocument->SetEditToolConfigDelegate(
    [this](xiiGameObjectEditTool* pTool) { pTool->ConfigureTool(static_cast<xiiGameObjectDocument*>(GetDocument()), this, this); });

  setCentralWidget(m_pQuadViewWidget);

  SetTargetFramerate(60);

  {
    // Menu Bar
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "EditorPluginScene_DocumentMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "EditorPluginScene_DocumentToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  const xiiSceneDocument* pSceneDoc = static_cast<const xiiSceneDocument*>(GetDocument());

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    xiiQtDocumentPanel* pPanelTree = new xiiQtScenegraphPanel(this, static_cast<xiiSceneDocument*>(pDocument));
    pPanelTree->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);
    XII_VERIFY(connect(pPropertyGrid, &xiiQtPropertyGridWidget::ExtendContextMenu, this, &xiiQtSceneDocumentWindow::ExtendPropertyGridContextMenu), "");

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
  }

  // Exposed Parameters
  if (GetSceneDocument()->IsPrefab())
  {
    xiiQtDocumentPanel* pPanel = new xiiQtDocumentPanel(this, pDocument);
    pPanel->setObjectName("SceneSettingsDockWidget");
    pPanel->setWindowTitle(GetSceneDocument()->IsPrefab() ? "Prefab Settings" : "Scene Settings");
    pPanel->show();

    xiiQtPropertyGridWidget*           pPropertyGrid = new xiiQtPropertyGridWidget(pPanel, pDocument, false);
    xiiDeque<const xiiDocumentObject*> selection;
    selection.PushBack(pDocument->GetSettingsObject());
    pPropertyGrid->SetSelection(selection);
    pPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);
  }

  FinishWindowCreation();
}

xiiQtSceneDocumentWindow::~xiiQtSceneDocumentWindow()
{
}

xiiQtSceneDocumentWindowBase::xiiQtSceneDocumentWindowBase(xiiSceneDocument* pDocument) :
  xiiQtGameObjectDocumentWindow(pDocument)
{
  const xiiSceneDocument* pSceneDoc = static_cast<const xiiSceneDocument*>(GetDocument());
  pSceneDoc->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiQtSceneDocumentWindowBase::GameObjectEventHandler, this));
}

xiiQtSceneDocumentWindowBase::~xiiQtSceneDocumentWindowBase()
{
  GetSceneDocument()->m_GameObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtSceneDocumentWindowBase::GameObjectEventHandler, this));
}

xiiSceneDocument* xiiQtSceneDocumentWindowBase::GetSceneDocument() const
{
  return static_cast<xiiSceneDocument*>(GetDocument());
}

void xiiQtSceneDocumentWindowBase::CreateImageCapture(const char* szOutputPath)
{
  m_pQuadViewWidget->GetActiveMainViews()[0]->GetViewWidget()->TakeScreenshot(szOutputPath);
}

void xiiQtSceneDocumentWindowBase::ToggleViews(QWidget* pView)
{
  m_pQuadViewWidget->ToggleViews(pView);
}


xiiObjectAccessorBase* xiiQtSceneDocumentWindowBase::GetObjectAccessor()
{
  return GetDocument()->GetObjectAccessor();
}

bool xiiQtSceneDocumentWindowBase::CanDuplicateSelection() const
{
  return true;
}

void xiiQtSceneDocumentWindowBase::DuplicateSelection()
{
  GetSceneDocument()->DuplicateSelection();
}

void xiiQtSceneDocumentWindowBase::SnapSelectionToPosition(bool bSnapEachObject)
{
  const float fSnap = xiiSnapProvider::GetTranslationSnapValue();

  if (fSnap == 0.0f)
    return;

  const xiiDeque<const xiiDocumentObject*>& selection = GetSceneDocument()->GetSelectionManager()->GetSelection();
  if (selection.IsEmpty())
    return;

  const auto& pivotObj = selection.PeekBack();

  xiiVec3 vPivotSnapOffset;

  if (!bSnapEachObject)
  {
    // if we snap by the pivot object only, the last selected object must be a valid game object
    if (!pivotObj->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
      return;

    const xiiVec3 vPivotPos   = GetSceneDocument()->GetGlobalTransform(pivotObj).m_vPosition;
    xiiVec3       vSnappedPos = vPivotPos;
    xiiSnapProvider::SnapTranslation(vSnappedPos);

    vPivotSnapOffset = vSnappedPos - vPivotPos;

    if (vPivotSnapOffset.IsZero())
      return;
  }

  xiiDeque<xiiSelectedGameObject> gizmoSelection;
  GetGameObjectDocument()->ComputeTopLevelSelectedGameObjects(gizmoSelection);

  if (gizmoSelection.IsEmpty())
    return;

  auto CmdHistory = GetDocument()->GetCommandHistory();

  CmdHistory->StartTransaction("Snap to Position");

  bool bDidAny = false;

  for (xiiUInt32 sel = 0; sel < gizmoSelection.GetCount(); ++sel)
  {
    const auto& obj = gizmoSelection[sel];

    xiiTransform vSnappedPos = obj.m_GlobalTransform;

    // if we snap each object individually, compute the snap position for each one here
    if (bSnapEachObject)
    {
      vSnappedPos.m_vPosition = obj.m_GlobalTransform.m_vPosition;
      xiiSnapProvider::SnapTranslation(vSnappedPos.m_vPosition);

      if (obj.m_GlobalTransform.m_vPosition == vSnappedPos.m_vPosition)
        continue;
    }
    else
    {
      // otherwise use the offset from the pivot point for repositioning
      vSnappedPos.m_vPosition += vPivotSnapOffset;
    }

    bDidAny = true;
    GetSceneDocument()->SetGlobalTransform(obj.m_pObject, vSnappedPos, TransformationChanges::Translation);
  }

  if (bDidAny)
    CmdHistory->FinishTransaction();
  else
    CmdHistory->CancelTransaction();

  gizmoSelection.Clear();

  ShowTemporaryStatusBarMsg(xiiFmt("Snap to Grid ({})", bSnapEachObject ? "Each Object" : "Pivot"));
}

void xiiQtSceneDocumentWindowBase::GameObjectEventHandler(const xiiGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGameObjectEvent::Type::TriggerFocusOnSelection_Hovered:
      // Focus is done by xiiQtGameObjectDocumentWindow
      GetSceneDocument()->ShowOrHideSelectedObjects(xiiSceneDocument::ShowOrHide::Show);
      break;

    case xiiGameObjectEvent::Type::TriggerFocusOnSelection_All:
      // Focus is done by xiiQtGameObjectDocumentWindow
      GetSceneDocument()->ShowOrHideSelectedObjects(xiiSceneDocument::ShowOrHide::Show);
      break;

    case xiiGameObjectEvent::Type::TriggerSnapSelectionPivotToGrid:
      SnapSelectionToPosition(false);
      break;

    case xiiGameObjectEvent::Type::TriggerSnapEachSelectedObjectToGrid:
      SnapSelectionToPosition(true);
      break;

    default:
      break;
  }
}

void xiiQtSceneDocumentWindowBase::InternalRedraw()
{
  // If play the game is on, only render (in editor) if the window is active
  xiiSceneDocument* doc = GetSceneDocument();
  if (doc->GetGameMode() == GameMode::Play && !window()->isActiveWindow())
    return;

  xiiEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  xiiQtEngineDocumentWindow::InternalRedraw();
}

void xiiQtSceneDocumentWindowBase::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    xiiSimulationSettingsMsgToEngine msg;
    auto                             pSceneDoc = GetSceneDocument();
    msg.m_bSimulateWorld                       = pSceneDoc->GetGameMode() != GameMode::Off;
    msg.m_fSimulationSpeed                     = pSceneDoc->GetSimulationSpeed();
    GetEditorEngineConnection()->SendMessage(&msg);
  }
  {
    xiiGridSettingsMsgToEngine msg = GetGridSettings();
    GetEditorEngineConnection()->SendMessage(&msg);
  }
  {
    xiiWorldSettingsMsgToEngine msg = GetWorldSettings();
    GetEditorEngineConnection()->SendMessage(&msg);
  }

  GetGameObjectDocument()->SendObjectSelection();

  auto pHoveredView = GetHoveredViewWidget();

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(pView == pHoveredView);
    pView->SetPickTransparent(GetGameObjectDocument()->GetPickTransparent());
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void xiiQtSceneDocumentWindowBase::ExtendPropertyGridContextMenu(
  QMenu&                                         menu,
  const xiiHybridArray<xiiPropertySelection, 8>& items,
  const xiiAbstractProperty*                     pProp)
{
  if (!GetSceneDocument()->IsPrefab())
    return;

  xiiUInt32 iExposed = 0;
  for (xiiUInt32 i = 0; i < items.GetCount(); i++)
  {
    xiiInt32 index = GetSceneDocument()->FindExposedParameter(items[i].m_pObject, pProp, items[i].m_Index);
    if (index != -1)
      iExposed++;
  }
  menu.addSeparator();
  {
    QAction* pAction = menu.addAction("Expose as Parameter");
    pAction->setEnabled(iExposed < items.GetCount());
    connect(pAction, &QAction::triggered, pAction, [this, &menu, &items, pProp]() {
      while (true)
      {
        bool bOk = false;
        QString name = QInputDialog::getText(this, "Parameter Name", "Name:", QLineEdit::Normal, pProp->GetPropertyName(), &bOk);

        if (!bOk)
          return;

        if (!xiiStringUtils::IsValidIdentifierName(name.toUtf8().data()))
        {
          xiiQtUiServices::GetSingleton()->MessageBoxInformation("This name is not a valid identifier.\nAllowed characters are a-z, A-Z, "
                                                                "0-9 and _.\nWhitespace and special characters are not allowed.");
          continue; // try again
        }

        auto pAccessor = GetSceneDocument()->GetObjectAccessor();
        pAccessor->StartTransaction("Expose as Parameter");
        for (const xiiPropertySelection& sel : items)
        {
          xiiInt32 index = GetSceneDocument()->FindExposedParameter(sel.m_pObject, pProp, sel.m_Index);
          if (index == -1)
          {
            GetSceneDocument()->AddExposedParameter(name.toUtf8(), sel.m_pObject, pProp, sel.m_Index).LogFailure();
          }
        }
        pAccessor->FinishTransaction();
        return;
      } });
  }
  {
    QAction* pAction = menu.addAction("Remove Exposed Parameter");
    pAction->setEnabled(iExposed > 0);
    connect(pAction, &QAction::triggered, pAction, [this, &menu, &items, pProp]() {
      auto pAccessor = GetSceneDocument()->GetObjectAccessor();
      pAccessor->StartTransaction("Remove Exposed Parameter");
      for (const xiiPropertySelection& sel : items)
      {
        xiiInt32 index = GetSceneDocument()->FindExposedParameter(sel.m_pObject, pProp, sel.m_Index);
        if (index != -1)
        {
          GetSceneDocument()->RemoveExposedParameter(index).LogFailure();
        }
      }
      pAccessor->FinishTransaction(); });
  }
}

void xiiQtSceneDocumentWindowBase::ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg)
{
  xiiQtGameObjectDocumentWindow::ProcessMessageEventHandler(pMsg);
}
