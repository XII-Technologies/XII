#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginProcGen/DragDropHandlers/ProcGenDragDropHandler.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcPlacementComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiProcPlacementComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;


float xiiProcPlacementComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "ProcGen Graph") ? 1.0f : 0.0f;
}

void xiiProcPlacementComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  xiiUuid  targetObject;
  xiiInt32 iTargetInsertChildIndex = -1;

  if (pInfo->m_sTargetContext != "viewport")
  {
    targetObject            = pInfo->m_TargetObject;
    iTargetInsertChildIndex = pInfo->m_iTargetObjectInsertChildIndex;
  }

  CreateDropObject(pInfo->m_vDropPosition, "xiiProcPlacementComponent", "Resource", GetAssetGuidString(pInfo), targetObject, iTargetInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
