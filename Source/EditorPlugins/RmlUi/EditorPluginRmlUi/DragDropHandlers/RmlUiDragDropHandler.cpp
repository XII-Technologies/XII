#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginRmlUi/DragDropHandlers/RmlUiDragDropHandler.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRmlUiComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiRmlUiComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;


float xiiRmlUiComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "RmlUi") ? 1.0f : 0.0f;
}

void xiiRmlUiComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "xiiRmlUiCanvas2DComponent", "RmlFile", GetAssetGuidString(pInfo), xiiUuid(), -1);
  }
  else
  {
    CreateDropObject(pInfo->m_vDropPosition, "xiiRmlUiCanvas2DComponent", "RmlFile", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
                     pInfo->m_iTargetObjectInsertChildIndex);
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
