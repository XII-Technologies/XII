#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/MaterialDragDropHandler.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
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

  if (!pDocument->GetDynamicRTTI()->IsDerivedFrom<xiiSceneDocument>())
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Material") ? 1.0f : 0.0f;
}

void xiiMaterialDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  m_pDocument = xiiDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);
  XII_ASSERT_DEV(m_pDocument != nullptr, "Invalid document GUID in drag & drop operation");

  m_pDocument->GetCommandHistory()->BeginTemporaryCommands("Drag Material", true);
}

void xiiMaterialDragDropHandler::OnDragUpdate(const xiiDragDropInfo* pInfo)
{
  if (!pInfo->m_TargetComponent.IsValid())
    return;

  const xiiDocumentObject* pComponent = m_pDocument->GetObjectManager()->GetObject(pInfo->m_TargetComponent);

  if (!pComponent)
    return;

  if (m_AppliedToComponent == pInfo->m_TargetComponent && m_iAppliedToSlot == pInfo->m_iTargetObjectSubID)
    return;

  m_AppliedToComponent = pInfo->m_TargetComponent;
  m_iAppliedToSlot     = pInfo->m_iTargetObjectSubID;

  if (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<xiiMeshComponent>())
  {
    xiiResizeAndSetObjectPropertyCommand cmd;
    cmd.m_Object    = pInfo->m_TargetComponent;
    cmd.m_Index     = pInfo->m_iTargetObjectSubID;
    cmd.m_sProperty = "Materials";
    cmd.m_NewValue  = GetAssetGuidString(pInfo);

    m_pDocument->GetCommandHistory()->StartTransaction("Assign Material");
    m_pDocument->GetCommandHistory()->AddCommand(cmd);
    m_pDocument->GetCommandHistory()->FinishTransaction();
  }

  if (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGreyBoxComponent>())
  {
    xiiSetObjectPropertyCommand cmd;
    cmd.m_Object    = pInfo->m_TargetComponent;
    cmd.m_sProperty = "Material";
    cmd.m_NewValue  = GetAssetGuidString(pInfo);

    m_pDocument->GetCommandHistory()->StartTransaction("Assign Material");
    m_pDocument->GetCommandHistory()->AddCommand(cmd);
    m_pDocument->GetCommandHistory()->FinishTransaction();
  }
}

void xiiMaterialDragDropHandler::OnDragCancel()
{
  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}

void xiiMaterialDragDropHandler::OnDrop(const xiiDragDropInfo* pInfo)
{
  if (pInfo->m_TargetComponent.IsValid())
  {
    const xiiDocumentObject* pComponent = m_pDocument->GetObjectManager()->GetObject(pInfo->m_TargetComponent);

    if (pComponent && (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<xiiMeshComponent>() || pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGreyBoxComponent>()))
    {
      m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
      return;
    }
  }

  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}
