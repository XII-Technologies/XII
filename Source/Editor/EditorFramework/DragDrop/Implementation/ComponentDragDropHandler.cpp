#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <ToolsFoundation/Command/TreeCommands.h>


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiComponentDragDropHandler, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiComponentDragDropHandler::CreateDropObject(const xiiVec3& vPosition, const char* szType, const char* szProperty, const xiiVariant& value, xiiUuid parent, xiiInt32 iInsertChildIndex)
{
  xiiVec3 vPos = vPosition;

  if (vPos.IsNaN())
    vPos.SetZero();

  xiiUuid ObjectGuid = xiiUuid::MakeUuid();

  xiiAddObjectCommand cmd;
  cmd.m_Parent = parent;
  cmd.m_Index  = iInsertChildIndex;
  cmd.SetType("xiiGameObject");
  cmd.m_NewObjectGuid   = ObjectGuid;
  cmd.m_sParentProperty = "Children";

  auto history = m_pDocument->GetCommandHistory();

  XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

  xiiSetObjectPropertyCommand cmd2;
  cmd2.m_Object = ObjectGuid;

  cmd2.m_sProperty = "LocalPosition";
  cmd2.m_NewValue  = vPos;
  XII_VERIFY(history->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");

  AttachComponentToObject(szType, szProperty, value, ObjectGuid);

  m_DraggedObjects.PushBack(ObjectGuid);
}

void xiiComponentDragDropHandler::AttachComponentToObject(const char* szType, const char* szProperty, const xiiVariant& value, xiiUuid ObjectGuid)
{
  auto history = m_pDocument->GetCommandHistory();

  xiiUuid CmpGuid = xiiUuid::MakeUuid();

  xiiAddObjectCommand cmd;

  cmd.SetType(szType);
  cmd.m_sParentProperty = "Components";
  cmd.m_Index           = -1;
  cmd.m_NewObjectGuid   = CmpGuid;
  cmd.m_Parent          = ObjectGuid;
  XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

  if (value.IsA<xiiVariantArray>())
  {
    xiiResizeAndSetObjectPropertyCommand cmd2;
    cmd2.m_Object    = CmpGuid;
    cmd2.m_sProperty = szProperty;
    cmd2.m_NewValue  = value.Get<xiiVariantArray>()[0];
    cmd2.m_Index     = 0;
    XII_VERIFY(history->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");
  }
  else
  {
    xiiSetObjectPropertyCommand cmd2;
    cmd2.m_Object    = CmpGuid;
    cmd2.m_sProperty = szProperty;
    cmd2.m_NewValue  = value;
    XII_VERIFY(history->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");
  }
}

void xiiComponentDragDropHandler::MoveObjectToPosition(const xiiUuid& guid, const xiiVec3& vPosition, const xiiQuat& qRotation)
{
  auto history = m_pDocument->GetCommandHistory();

  xiiSetObjectPropertyCommand cmd2;
  cmd2.m_Object = guid;

  cmd2.m_sProperty = "LocalPosition";
  cmd2.m_NewValue  = vPosition;
  history->AddCommand(cmd2).AssertSuccess();

  if (qRotation.IsValid())
  {
    cmd2.m_sProperty = "LocalRotation";
    cmd2.m_NewValue  = qRotation;
    history->AddCommand(cmd2).AssertSuccess();
  }
}

void xiiComponentDragDropHandler::MoveDraggedObjectsToPosition(xiiVec3 vPosition, bool bAllowSnap, const xiiVec3& normal)
{
  if (m_DraggedObjects.IsEmpty() || !vPosition.IsValid())
    return;

  if (bAllowSnap)
  {
    xiiSnapProvider::SnapTranslation(vPosition);
  }

  auto history = m_pDocument->GetCommandHistory();

  history->StartTransaction("Move to Position");

  xiiQuat rot;
  rot.SetIdentity();

  if (normal.IsValid() && !m_vAlignAxisWithNormal.IsZero(0.01f))
  {
    rot.SetShortestRotation(m_vAlignAxisWithNormal, normal);
  }

  for (const auto& guid : m_DraggedObjects)
  {
    MoveObjectToPosition(guid, vPosition, rot);
  }

  history->FinishTransaction();
}

void xiiComponentDragDropHandler::SelectCreatedObjects()
{
  xiiDeque<const xiiDocumentObject*> NewSel;
  for (const auto& id : m_DraggedObjects)
  {
    NewSel.PushBack(m_pDocument->GetObjectManager()->GetObject(id));
  }

  m_pDocument->GetSelectionManager()->SetSelection(NewSel);
}

void xiiComponentDragDropHandler::BeginTemporaryCommands()
{
  m_pDocument->GetCommandHistory()->BeginTemporaryCommands("Adjust Objects");
}

void xiiComponentDragDropHandler::EndTemporaryCommands()
{
  m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
}

void xiiComponentDragDropHandler::CancelTemporaryCommands()
{
  if (m_DraggedObjects.IsEmpty())
    return;

  m_pDocument->GetSelectionManager()->Clear();

  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}

void xiiComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  m_pDocument = xiiDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);
  XII_ASSERT_DEV(m_pDocument != nullptr, "Invalid document GUID in drag & drop operation");

  m_pDocument->GetCommandHistory()->StartTransaction("Drag Object");
}

void xiiComponentDragDropHandler::OnDragUpdate(const xiiDragDropInfo* pInfo)
{
  xiiVec3 vPos = pInfo->m_vDropPosition;

  if (vPos.IsNaN() || !pInfo->m_TargetObject.IsValid())
    vPos.SetZero();

  xiiVec3 vNormal = pInfo->m_vDropNormal;

  if (!vNormal.IsValid() || vNormal.IsZero())
    vNormal = xiiVec3(1, 0, 0);

  MoveDraggedObjectsToPosition(vPos, !pInfo->m_bCtrlKeyDown, vNormal);
}

void xiiComponentDragDropHandler::OnDragCancel()
{
  CancelTemporaryCommands();
  m_pDocument->GetCommandHistory()->CancelTransaction();

  m_DraggedObjects.Clear();
}

void xiiComponentDragDropHandler::OnDrop(const xiiDragDropInfo* pInfo)
{
  EndTemporaryCommands();
  m_pDocument->GetCommandHistory()->FinishTransaction();

  SelectCreatedObjects();

  m_DraggedObjects.Clear();
}

float xiiComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext != "viewport" && pInfo->m_sTargetContext != "scenetree")
    return 0.0f;

  const xiiDocument* pDocument = xiiDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);

  const xiiRTTI* pRttiScene = xiiRTTI::FindTypeByName("xiiSceneDocument");

  if (pRttiScene == nullptr)
    return 0.0f;

  if (!pDocument->GetDynamicRTTI()->IsDerivedFrom(pRttiScene))
    return 0.0f;

  return 1.0f;
}
