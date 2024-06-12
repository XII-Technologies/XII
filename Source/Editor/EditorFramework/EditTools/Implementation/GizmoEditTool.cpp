#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/GizmoEditTool.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectGizmoEditTool, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGameObjectGizmoEditTool::xiiGameObjectGizmoEditTool()
{
  xiiQtDocumentWindow::s_Events.AddEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::DocumentWindowEventHandler, this));
}

xiiGameObjectGizmoEditTool::~xiiGameObjectGizmoEditTool()
{
  xiiQtDocumentWindow::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::DocumentWindowEventHandler, this));
}

void xiiGameObjectGizmoEditTool::OnConfigured()
{
  GetDocument()->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::GameObjectEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::CommandHistoryEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::SelectionManagerEventHandler, this));
  xiiManipulatorManager::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::ManipulatorManagerEventHandler, this));
  GetWindow()->m_EngineWindowEvent.AddEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::EngineWindowEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::ObjectStructureEventHandler, this));

  // subscribe to all views that already exist
  for (xiiQtEngineViewWidget* pView : GetWindow()->GetViewWidgets())
  {
    if (xiiQtGameObjectViewWidget* pViewWidget = qobject_cast<xiiQtGameObjectViewWidget*>(pView))
    {
      pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::TransformationGizmoEventHandler, this));
    }
  }
}

void xiiGameObjectGizmoEditTool::UpdateGizmoSelectionList()
{
  GetDocument()->ComputeTopLevelSelectedGameObjects(m_GizmoSelection);
}

void xiiGameObjectGizmoEditTool::UpdateGizmoVisibleState()
{
  bool isVisible = false;

  if (IsActive())
  {
    xiiGameObjectDocument* pDocument = GetDocument();

    const auto& selection = pDocument->GetSelectionManager()->GetSelection();

    if (selection.IsEmpty() || !selection.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
      goto done;

    isVisible = true;
    UpdateGizmoTransformation();
  }

done:
  ApplyGizmoVisibleState(isVisible);
}

void xiiGameObjectGizmoEditTool::UpdateGizmoTransformation()
{
  const auto& LatestSelection = GetDocument()->GetSelectionManager()->GetSelection().PeekBack();

  if (LatestSelection->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiGameObject>())
  {
    const xiiTransform tGlobal = GetDocument()->GetGlobalTransform(LatestSelection);

    /// \todo Pivot point
    const xiiVec3 vPivotPoint = tGlobal.m_qRotation * xiiVec3::MakeZero(); // LatestSelection->GetEditorTypeAccessor().GetValue("Pivot").ConvertTo<xiiVec3>();

    xiiTransform mt;
    mt.SetIdentity();

    if (GetDocument()->GetGizmoWorldSpace() && GetSupportedSpaces() != xiiEditToolSupportedSpaces::LocalSpaceOnly)
    {
      mt.m_vPosition = tGlobal.m_vPosition + vPivotPoint;
    }
    else
    {
      mt.m_qRotation = tGlobal.m_qRotation;
      mt.m_vPosition = tGlobal.m_vPosition + vPivotPoint;
    }

    ApplyGizmoTransformation(mt);
  }
}

void xiiGameObjectGizmoEditTool::DocumentWindowEventHandler(const xiiQtDocumentWindowEvent& e)
{
  if (e.m_Type == xiiQtDocumentWindowEvent::WindowClosing && e.m_pWindow == GetWindow())
  {
    GetDocument()->m_GameObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::GameObjectEventHandler, this));
    GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::CommandHistoryEventHandler, this));
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::SelectionManagerEventHandler, this));
    xiiManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::ManipulatorManagerEventHandler, this));
    GetWindow()->m_EngineWindowEvent.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::EngineWindowEventHandler, this));
    GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::ObjectStructureEventHandler, this));
  }
}

void xiiGameObjectGizmoEditTool::UpdateManipulatorVisibility()
{
  xiiManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetDocument()->GetActiveEditTool() != nullptr);
}

void xiiGameObjectGizmoEditTool::GameObjectEventHandler(const xiiGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGameObjectEvent::Type::ActiveEditToolChanged:
    case xiiGameObjectEvent::Type::GizmoTransformMayBeInvalid:
      UpdateGizmoVisibleState();
      UpdateManipulatorVisibility();
      break;

    default:
      break;
  }
}

void xiiGameObjectGizmoEditTool::CommandHistoryEventHandler(const xiiCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
    case xiiCommandHistoryEvent::Type::UndoEnded:
    case xiiCommandHistoryEvent::Type::RedoEnded:
    case xiiCommandHistoryEvent::Type::TransactionEnded:
    case xiiCommandHistoryEvent::Type::TransactionCanceled:
      UpdateGizmoVisibleState();
      break;

    default:
      break;
  }
}

void xiiGameObjectGizmoEditTool::SelectionManagerEventHandler(const xiiSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
    case xiiSelectionManagerEvent::Type::SelectionCleared:
      m_GizmoSelection.Clear();
      UpdateGizmoVisibleState();
      break;

    case xiiSelectionManagerEvent::Type::SelectionSet:
    case xiiSelectionManagerEvent::Type::ObjectAdded:
      XII_ASSERT_DEBUG(m_GizmoSelection.IsEmpty(), "This array should have been cleared when the gizmo lost focus");
      UpdateGizmoVisibleState();
      break;

    default:
      break;
  }
}

void xiiGameObjectGizmoEditTool::ManipulatorManagerEventHandler(const xiiManipulatorManagerEvent& e)
{
  if (!IsActive())
    return;

  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() && !e.m_bHideManipulators)
  {
    GetDocument()->SetActiveEditTool(nullptr);
  }
}

void xiiGameObjectGizmoEditTool::EngineWindowEventHandler(const xiiEngineWindowEvent& e)
{
  if (xiiQtGameObjectViewWidget* pViewWidget = qobject_cast<xiiQtGameObjectViewWidget*>(e.m_pView))
  {
    switch (e.m_Type)
    {
      case xiiEngineWindowEvent::Type::ViewCreated:
        pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiGameObjectGizmoEditTool::TransformationGizmoEventHandler, this));
        break;

      default:
        break;
    }
  }
}

void xiiGameObjectGizmoEditTool::ObjectStructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  if (!IsActive() || m_bInGizmoInteraction)
    return;

  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      UpdateGizmoVisibleState();
      break;

    default:
      break;
  }
}

void xiiGameObjectGizmoEditTool::TransformationGizmoEventHandler(const xiiGizmoEvent& e)
{
  if (!IsActive())
    return;

  xiiObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();

  switch (e.m_Type)
  {
    case xiiGizmoEvent::Type::BeginInteractions:
    {
      m_bMergeTransactions = false;

      TransformationGizmoEventHandlerImpl(e);

      UpdateGizmoSelectionList();

      pAccessor->BeginTemporaryCommands("Transform Object");
    }
    break;

    case xiiGizmoEvent::Type::Interaction:
    {
      m_bInGizmoInteraction = true;
      pAccessor->StartTransaction("Transform Object");

      TransformationGizmoEventHandlerImpl(e);

      m_bInGizmoInteraction = false;
    }
    break;

    case xiiGizmoEvent::Type::EndInteractions:
    {
      pAccessor->FinishTemporaryCommands();
      m_GizmoSelection.Clear();

      if (m_bMergeTransactions)
        GetDocument()->GetCommandHistory()->MergeLastTwoTransactions(); //#TODO: this should be interleaved transactions
    }
    break;

    case xiiGizmoEvent::Type::CancelInteractions:
    {
      pAccessor->CancelTemporaryCommands();
      m_GizmoSelection.Clear();
    }
    break;

    default:
      break;
  }
}
