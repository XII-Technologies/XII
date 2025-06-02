#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

#include <EditorFramework/Assets/AssetDocument.h>

xiiQtEngineDocumentWindow::xiiQtEngineDocumentWindow(xiiAssetDocument* pDocument) :
  xiiQtDocumentWindow(pDocument)
{
  pDocument->m_ProcessMessageEvent.AddEventHandler(xiiMakeDelegate(&xiiQtEngineDocumentWindow::ProcessMessageEventHandler, this));
  pDocument->m_CommonAssetUiChangeEvent.AddEventHandler(xiiMakeDelegate(&xiiQtEngineDocumentWindow::CommonAssetUiEventHandler, this));
}

xiiQtEngineDocumentWindow::~xiiQtEngineDocumentWindow()
{
  // make sure the selection gets cleared before the views are destroyed, so that dependent code can clean up first
  GetDocument()->GetSelectionManager()->Clear();

  GetDocument()->m_ProcessMessageEvent.RemoveEventHandler(xiiMakeDelegate(&xiiQtEngineDocumentWindow::ProcessMessageEventHandler, this));
  GetDocument()->m_CommonAssetUiChangeEvent.RemoveEventHandler(xiiMakeDelegate(&xiiQtEngineDocumentWindow::CommonAssetUiEventHandler, this));

  // delete all view widgets, so that they can send their messages before we clean up the engine connection
  DestroyAllViews();
}

xiiEditorEngineConnection* xiiQtEngineDocumentWindow::GetEditorEngineConnection() const
{
  return GetDocument()->GetEditorEngineConnection();
}

static xiiObjectPickingResult s_DummyResult;

const xiiObjectPickingResult& xiiQtEngineDocumentWindow::PickObject(xiiUInt16 uiScreenPosX, xiiUInt16 uiScreenPosY, xiiQtEngineViewWidget* pView) const
{
  if (pView == nullptr)
  {
    pView = GetHoveredViewWidget();
  }

  if (pView != nullptr)
    return pView->PickObject(uiScreenPosX, uiScreenPosY);

  return s_DummyResult;
}

xiiAssetDocument* xiiQtEngineDocumentWindow::GetDocument() const
{
  return static_cast<xiiAssetDocument*>(xiiQtDocumentWindow::GetDocument());
}

void xiiQtEngineDocumentWindow::InternalRedraw()
{
  // TODO: Move this to a better place (some kind of regular update function, not redraw)
  GetDocument()->SyncObjectsToEngine();
}

xiiQtEngineViewWidget* xiiQtEngineDocumentWindow::GetHoveredViewWidget() const
{
  QWidget* pWidget = QApplication::widgetAt(QCursor::pos());

  while (pWidget != nullptr)
  {
    xiiQtEngineViewWidget* pCandidate = qobject_cast<xiiQtEngineViewWidget*>(pWidget);
    if (pCandidate != nullptr)
    {
      if (m_ViewWidgets.Contains(pCandidate))
        return pCandidate;

      return nullptr;
    }
    pWidget = pWidget->parentWidget();
  }
  return nullptr;
}

xiiQtEngineViewWidget* xiiQtEngineDocumentWindow::GetFocusedViewWidget() const
{
  QWidget* pWidget = QApplication::focusWidget();

  while (pWidget != nullptr)
  {
    xiiQtEngineViewWidget* pCandidate = qobject_cast<xiiQtEngineViewWidget*>(pWidget);
    if (pCandidate != nullptr)
    {
      if (m_ViewWidgets.Contains(pCandidate))
        return pCandidate;

      return nullptr;
    }
    pWidget = pWidget->parentWidget();
  }
  return nullptr;
}

xiiQtEngineViewWidget* xiiQtEngineDocumentWindow::GetViewWidgetByID(xiiUInt32 uiViewID) const
{
  for (auto pView : m_ViewWidgets)
  {
    if (pView && pView->GetViewID() == uiViewID)
      return pView;
  }
  return nullptr;
}

xiiArrayPtr<xiiQtEngineViewWidget* const> xiiQtEngineDocumentWindow::GetViewWidgets() const
{
  return m_ViewWidgets;
}

void xiiQtEngineDocumentWindow::AddViewWidget(xiiQtEngineViewWidget* pView)
{
  m_ViewWidgets.PushBack(pView);

  xiiEngineWindowEvent e;
  e.m_Type  = xiiEngineWindowEvent::Type::ViewCreated;
  e.m_pView = pView;

  m_EngineWindowEvent.Broadcast(e);
}

void xiiQtEngineDocumentWindow::RemoveViewWidget(xiiQtEngineViewWidget* pView)
{
  m_ViewWidgets.RemoveAndSwap(pView);

  xiiEngineWindowEvent e;
  e.m_Type  = xiiEngineWindowEvent::Type::ViewDestroyed;
  e.m_pView = pView;

  m_EngineWindowEvent.Broadcast(e);
}

void xiiQtEngineDocumentWindow::CommonAssetUiEventHandler(const xiiCommonAssetUiState& e)
{
  xiiSimpleDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo    = "CommonAssetUiState";
  msg.m_PayloadValue = e.m_fValue;

  switch (e.m_State)
  {
    case xiiCommonAssetUiState::Restart:
      msg.m_sPayload = "Restart";
      break;

    case xiiCommonAssetUiState::Loop:
      msg.m_sPayload = "Loop";
      break;

    case xiiCommonAssetUiState::Pause:
      msg.m_sPayload = "Pause";
      break;

    case xiiCommonAssetUiState::Grid:
      msg.m_sPayload = "Grid";
      break;

    case xiiCommonAssetUiState::SimulationSpeed:
      msg.m_sPayload = "SimulationSpeed";
      break;

    case xiiCommonAssetUiState::Visualizers:
      msg.m_sPayload = "Visualizers";
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (!msg.m_sPayload.IsEmpty())
  {
    GetEditorEngineConnection()->SendMessage(&msg);
  }
}

void xiiQtEngineDocumentWindow::ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiEditorEngineViewMsg>())
  {
    const xiiEditorEngineViewMsg* pViewMsg = static_cast<const xiiEditorEngineViewMsg*>(pMsg);

    if (xiiQtEngineViewWidget* pView = GetViewWidgetByID(pViewMsg->m_uiViewID))
    {
      pView->HandleViewMessage(pViewMsg);
    }
  }
}

void xiiQtEngineDocumentWindow::DestroyAllViews()
{
  while (!m_ViewWidgets.IsEmpty())
  {
    delete m_ViewWidgets[0];
  }
}

void xiiQtEngineDocumentWindow::CreateImageCapture(xiiStringView sOutputPath)
{
  if (!m_ViewWidgets.IsEmpty())
  {
    m_ViewWidgets[0]->TakeScreenshot(sOutputPath);
  }
}
