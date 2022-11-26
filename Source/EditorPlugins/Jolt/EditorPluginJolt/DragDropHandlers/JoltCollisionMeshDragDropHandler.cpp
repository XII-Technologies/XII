#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginJolt/DragDropHandlers/JoltCollisionMeshDragDropHandler.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltCollisionMeshComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiJoltCollisionMeshComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;


float xiiJoltCollisionMeshComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return (IsSpecificAssetType(pInfo, "Jolt_Colmesh_Triangle") || IsSpecificAssetType(pInfo, "Jolt_Colmesh_Convex")) ? 1.0f : 0.0f;
}

void xiiJoltCollisionMeshComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "xiiJoltStaticActorComponent", "CollisionMesh", GetAssetGuidString(pInfo), xiiUuid(), -1);
  else
  {
    if (pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject("xiiJoltStaticActorComponent", "CollisionMesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
      CreateDropObject(pInfo->m_vDropPosition, "xiiJoltStaticActorComponent", "CollisionMesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
                       pInfo->m_iTargetObjectInsertChildIndex);
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
