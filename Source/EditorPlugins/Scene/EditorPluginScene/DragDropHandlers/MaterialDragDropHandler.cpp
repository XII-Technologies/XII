/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/MaterialDragDropHandler.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiMaterialDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiMaterialDragDropHandler::RequestConfiguration(xiiDragDropConfig* pConfigToFillOut)
{
  pConfigToFillOut->m_bPickSelectedObjects = true;
}

float xiiMaterialDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext != "viewport")
    return 0.0f;

  const xiiDocument* pDocument = xiiDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);

  if (!pDocument || !pDocument->GetDynamicRTTI()->IsDerivedFrom<xiiSceneDocument>())
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Material") ? 1.0f : 0.0f;
}

void xiiMaterialDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  m_pDocument          = xiiDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);
  m_AppliedToComponent = xiiUuid::MakeInvalid();
  m_iAppliedToSlot     = -1;
  if (!m_pDocument)
    return;

  m_pDocument->GetCommandHistory()->BeginTemporaryCommands("Drag Material", true);
}

void xiiMaterialDragDropHandler::OnDragUpdate(const xiiDragDropInfo* pInfo)
{
  if (!m_pDocument || !pInfo->m_TargetComponent.IsValid() || pInfo->m_iTargetObjectSubID < 0)
    return;

  const xiiDocumentObject* pComponent = m_pDocument->GetObjectManager()->GetObject(pInfo->m_TargetComponent);

  if (!pComponent)
    return;

  if (m_AppliedToComponent == pInfo->m_TargetComponent && m_iAppliedToSlot == pInfo->m_iTargetObjectSubID)
    return;

  if (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<xiiMeshComponentBase>())
  {
    const auto material = GetAssetGuidString(pInfo);
    if (material.IsEmpty())
      return;
    xiiResizeAndSetObjectPropertyCommand cmd;
    cmd.m_Object    = pInfo->m_TargetComponent;
    cmd.m_Index     = pInfo->m_iTargetObjectSubID;
    cmd.m_sProperty = "MaterialOverrides";
    cmd.m_NewValue  = material;

    m_pDocument->GetCommandHistory()->StartTransaction("Assign Material");
    const auto status = m_pDocument->GetCommandHistory()->AddCommand(cmd);
    if (status.Failed())
    {
      m_pDocument->GetCommandHistory()->CancelTransaction();
      m_pDocument->ShowDocumentStatus(xiiFmt("Could not assign material: {}", status.GetMessageString()));
      return;
    }
    m_pDocument->GetCommandHistory()->FinishTransaction();
    m_AppliedToComponent = pInfo->m_TargetComponent;
    m_iAppliedToSlot     = pInfo->m_iTargetObjectSubID;
  }
}

void xiiMaterialDragDropHandler::OnDragCancel()
{
  if (m_pDocument)
    m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}

void xiiMaterialDragDropHandler::OnDrop(const xiiDragDropInfo* pInfo)
{
  if (!m_pDocument)
    return;
  if (pInfo->m_TargetComponent.IsValid() && m_AppliedToComponent == pInfo->m_TargetComponent && m_iAppliedToSlot == pInfo->m_iTargetObjectSubID)
  {
    const xiiDocumentObject* pComponent = m_pDocument->GetObjectManager()->GetObject(pInfo->m_TargetComponent);

    if (pComponent && pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<xiiMeshComponentBase>())
    {
      m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
      return;
    }
  }

  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}
