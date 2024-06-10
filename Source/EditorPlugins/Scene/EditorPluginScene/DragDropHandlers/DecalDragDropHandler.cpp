#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/DecalDragDropHandler.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiDecalComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;


float xiiDecalComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Decal") ? 1.0f : 0.0f;
}

void xiiDecalComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  xiiVariantArray var;
  var.PushBack(GetAssetGuidString(pInfo));

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "xiiDecalComponent", "Decals", var, xiiUuid(), -1);

    m_vAlignAxisWithNormal = -xiiVec3::MakeAxisX();
  }
  else
    CreateDropObject(pInfo->m_vDropPosition, "xiiDecalComponent", "Decals", var, pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
