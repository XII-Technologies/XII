#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/InputContexts/SceneSelectionContext.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <QKeyEvent>

bool xiiQtSceneViewWidget::s_bContextMenuInitialized = false;

xiiQtSceneViewWidget::xiiQtSceneViewWidget(QWidget* pParent, xiiQtGameObjectDocumentWindow* pOwnerWindow, xiiEngineViewConfig* pViewConfig) :
  xiiQtGameObjectViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_bAllowPickSelectedWhileDragging = false;

  if (xiiDynamicCast<xiiScene2Document*>(pOwnerWindow->GetDocument()))
  {
    //#TODO Not the cleanest solution but this replaces the default selection context of the base class.
    const xiiUInt32 uiSelectionIndex = m_InputContexts.IndexOf(m_pSelectionContext);
    XII_DEFAULT_DELETE(m_pSelectionContext);
    m_pSelectionContext               = XII_DEFAULT_NEW(xiiSceneSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
    m_InputContexts[uiSelectionIndex] = m_pSelectionContext;
  }
}

xiiQtSceneViewWidget::~xiiQtSceneViewWidget() = default;

bool xiiQtSceneViewWidget::IsPickingAgainstSelectionAllowed() const
{
  if (m_bInDragAndDropOperation && m_bAllowPickSelectedWhileDragging)
  {
    return true;
  }

  return xiiQtEngineViewWidget::IsPickingAgainstSelectionAllowed();
}

void xiiQtSceneViewWidget::OnOpenContextMenu(QPoint globalPos)
{
  if (!s_bContextMenuInitialized)
  {
    s_bContextMenuInitialized = true;

    xiiActionMapManager::RegisterActionMap("SceneViewContextMenu").IgnoreResult();

    xiiGameObjectSelectionActions::MapViewContextMenuActions("SceneViewContextMenu");
    xiiSelectionActions::MapViewContextMenuActions("SceneViewContextMenu");
    xiiEditActions::MapViewContextMenuActions("SceneViewContextMenu");
    xiiSceneActions::MapViewContextMenuActions("SceneViewContextMenu");
  }

  {
    xiiQtMenuActionMapView menu(nullptr);

    xiiActionContext context;
    context.m_sMapping  = "SceneViewContextMenu";
    context.m_pDocument = GetDocumentWindow()->GetDocument();
    context.m_pWindow   = this;
    menu.SetActionContext(context);

    menu.exec(globalPos);
  }
}

void xiiQtSceneViewWidget::dragEnterEvent(QDragEnterEvent* e)
{
  xiiQtEngineViewWidget::dragEnterEvent(e);

  // can only drag & drop objects around in perspective mode
  // when dragging between two windows, the editor crashes
  // can be reproduced with two perspective windows as well
  // if (m_pViewConfig->m_Perspective != xiiSceneViewPerspective::Perspective)
  // return;

  m_LastDragMoveEvent               = xiiTime::Now();
  m_bAllowPickSelectedWhileDragging = false;

  {
    const QPoint           screenPos = e->position().toPoint();
    xiiObjectPickingResult res       = PickObject(screenPos.x(), screenPos.y());

    xiiDragDropInfo info;
    info.m_pMimeData                     = e->mimeData();
    info.m_TargetDocument                = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext                = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition                 = res.m_vPickedPosition;
    info.m_vDropNormal                   = res.m_vPickedNormal;
    info.m_iTargetObjectSubID            = res.m_uiPartIndex;
    info.m_TargetObject                  = res.m_PickedObject;
    info.m_TargetComponent               = res.m_PickedComponent;
    info.m_bShiftKeyDown                 = e->modifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown                  = e->modifiers() & Qt::ControlModifier;

    xiiDragDropConfig cfg;
    if (xiiDragDropHandler::BeginDragDropOperation(&info, &cfg))
    {
      m_bAllowPickSelectedWhileDragging = cfg.m_bPickSelectedObjects;

      e->acceptProposedAction();
      return;
    }
  }

  m_bInDragAndDropOperation = false;
}

void xiiQtSceneViewWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
  xiiDragDropHandler::CancelDragDrop();

  xiiQtEngineViewWidget::dragLeaveEvent(e);
}

void xiiQtSceneViewWidget::dragMoveEvent(QDragMoveEvent* e)
{
  const xiiTime tNow = xiiTime::Now();

  if (tNow - m_LastDragMoveEvent < xiiTime::MakeFromSeconds(1.0 / 25.0))
    return;

  m_LastDragMoveEvent = tNow;

  if (xiiDragDropHandler::IsHandlerActive())
  {
    const QPoint           screenPos = e->position().toPoint();
    xiiObjectPickingResult res       = PickObject(screenPos.x(), screenPos.y());

    xiiDragDropInfo info;
    info.m_pMimeData                     = e->mimeData();
    info.m_TargetDocument                = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext                = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition                 = res.m_vPickedPosition;
    info.m_vDropNormal                   = res.m_vPickedNormal;
    info.m_iTargetObjectSubID            = res.m_uiPartIndex;
    info.m_TargetObject                  = res.m_PickedObject;
    info.m_TargetComponent               = res.m_PickedComponent;
    info.m_bShiftKeyDown                 = e->modifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown                  = e->modifiers() & Qt::ControlModifier;

    xiiDragDropHandler::UpdateDragDropOperation(&info);
  }
}

void xiiQtSceneViewWidget::dropEvent(QDropEvent* e)
{
  if (xiiDragDropHandler::IsHandlerActive())
  {
    const QPoint           screenPos = e->position().toPoint();
    xiiObjectPickingResult res       = PickObject(screenPos.x(), screenPos.y());

    xiiDragDropInfo info;
    info.m_pMimeData                     = e->mimeData();
    info.m_TargetDocument                = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext                = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition                 = res.m_vPickedPosition;
    info.m_vDropNormal                   = res.m_vPickedNormal;
    info.m_iTargetObjectSubID            = res.m_uiPartIndex;
    info.m_TargetObject                  = res.m_PickedObject;
    info.m_TargetComponent               = res.m_PickedComponent;
    info.m_bShiftKeyDown                 = e->modifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown                  = e->modifiers() & Qt::ControlModifier;

    xiiDragDropHandler::FinishDragDrop(&info);
  }

  xiiQtEngineViewWidget::dropEvent(e);
}
