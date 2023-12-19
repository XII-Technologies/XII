#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/SkeletonDragDropHandler.h>

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkeletonComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiSkeletonComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;

float xiiSkeletonComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Skeleton") ? 1.0f : 0.0f;
}

void xiiSkeletonComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "xiiSkeletonComponent", "Skeleton", GetAssetGuidString(pInfo), xiiUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "xiiSkeletonComponent", "Skeleton", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
