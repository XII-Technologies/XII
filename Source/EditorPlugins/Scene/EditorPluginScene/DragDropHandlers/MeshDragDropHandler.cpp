#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/MeshDragDropHandler.h>

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiMeshComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;

float xiiMeshComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Mesh") ? 1.0f : 0.0f;
}

void xiiMeshComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "xiiMeshComponent", "Mesh", GetAssetGuidString(pInfo), xiiUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "xiiMeshComponent", "Mesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimatedMeshComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiAnimatedMeshComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;

float xiiAnimatedMeshComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Animated Mesh") ? 1.0f : 0.0f;
}

void xiiAnimatedMeshComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "xiiAnimatedMeshComponent", "Mesh", GetAssetGuidString(pInfo), xiiUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "xiiAnimatedMeshComponent", "Mesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
