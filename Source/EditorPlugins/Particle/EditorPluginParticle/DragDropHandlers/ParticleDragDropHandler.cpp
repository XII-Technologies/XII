#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginParticle/DragDropHandlers/ParticleDragDropHandler.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiParticleComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;


float xiiParticleComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Particle Effect") ? 1.0f : 0.0f;
}

void xiiParticleComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "xiiParticleComponent", "Effect", GetAssetGuidString(pInfo), xiiUuid(), -1);

    m_vAlignAxisWithNormal = xiiVec3::UnitZAxis();
  }
  else
    CreateDropObject(pInfo->m_vDropPosition, "xiiParticleComponent", "Effect", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
