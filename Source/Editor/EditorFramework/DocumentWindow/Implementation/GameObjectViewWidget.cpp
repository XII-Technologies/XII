#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>

xiiQtGameObjectViewWidget::xiiQtGameObjectViewWidget(QWidget* pParent, xiiQtGameObjectDocumentWindow* pOwnerWindow, xiiEngineViewConfig* pViewConfig) :
  xiiQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  m_pSelectionContext  = XII_DEFAULT_NEW(xiiSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
  m_pCameraMoveContext = XII_DEFAULT_NEW(xiiCameraMoveContext, pOwnerWindow, this);
  m_pOrthoGizmoContext = XII_DEFAULT_NEW(xiiOrthoGizmoContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);

  m_pCameraMoveContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pCameraMoveContext->LoadState();

  // add the input contexts in the order in which they are supposed to be processed
  m_InputContexts.PushBack(m_pOrthoGizmoContext);
  m_InputContexts.PushBack(m_pSelectionContext);
  m_InputContexts.PushBack(m_pCameraMoveContext);
}

xiiQtGameObjectViewWidget::~xiiQtGameObjectViewWidget()
{
  XII_DEFAULT_DELETE(m_pOrthoGizmoContext);
  XII_DEFAULT_DELETE(m_pSelectionContext);
  XII_DEFAULT_DELETE(m_pCameraMoveContext);
}

void xiiQtGameObjectViewWidget::SyncToEngine()
{
  m_pSelectionContext->SetWindowConfig(xiiVec2I32(width(), height()));

  xiiQtEngineViewWidget::SyncToEngine();
}

void xiiQtGameObjectViewWidget::HandleMarqueePickingResult(const xiiViewMarqueePickingResultMsgToEditor* pMsg)
{
  auto pSelMan = GetDocumentWindow()->GetDocument()->GetSelectionManager();
  auto pObjMan = GetDocumentWindow()->GetDocument()->GetObjectManager();

  if (m_uiLastMarqueeActionID != pMsg->m_uiActionIdentifier)
  {
    m_uiLastMarqueeActionID = pMsg->m_uiActionIdentifier;

    m_MarqueeBaseSelection.Clear();

    if (pMsg->m_uiWhatToDo == 0) // set selection
      pSelMan->Clear();

    const auto& curSel = pSelMan->GetSelection();
    for (auto pObj : curSel)
    {
      m_MarqueeBaseSelection.PushBack(pObj->GetGuid());
    }
  }

  xiiDeque<const xiiDocumentObject*> newSelection;

  for (xiiUuid guid : m_MarqueeBaseSelection)
  {
    auto pObject = pObjMan->GetObject(guid);
    newSelection.PushBack(pObject);
  }

  const xiiDocumentObject* pRoot = pObjMan->GetRootObject();

  for (xiiUuid guid : pMsg->m_ObjectGuids)
  {
    const xiiDocumentObject* pObject = pObjMan->GetObject(guid);

    if (pMsg->m_uiWhatToDo == 2) // remove from selection
    {
      // keep selection order
      newSelection.RemoveAndCopy(pObject);
    }
    else // add/set selection
    {
      if (!newSelection.Contains(pObject))
        newSelection.PushBack(pObject);
    }
  }

  pSelMan->SetSelection(newSelection);
}
