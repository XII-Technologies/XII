#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginTypeScript/DragDropHandlers/TypeScriptDragDropHandler.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiTypeScriptComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;


float xiiTypeScriptComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "TypeScript") ? 1.0f : 0.0f;
}

void xiiTypeScriptComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "xiiTypeScriptComponent", "Script", GetAssetGuidString(pInfo), xiiUuid(), -1);
  }
  else
  {
    if (pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject("xiiTypeScriptComponent", "Script", GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
    {
      CreateDropObject(pInfo->m_vDropPosition, "xiiTypeScriptComponent", "Script", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
                       pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
