#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginKraut/DragDropHandlers/KrautTreeDragDropHandler.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiKrautTreeComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiKrautTreeComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;


float xiiKrautTreeComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Kraut Tree") ? 1.0f : 0.0f;
}

void xiiKrautTreeComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "xiiKrautTreeComponent", "KrautTree", GetAssetGuidString(pInfo), xiiUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "xiiKrautTreeComponent", "KrautTree", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
                     pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
