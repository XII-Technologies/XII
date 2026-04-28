/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/PrefabDragDropHandler.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/PrefabCache.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPrefabComponentDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiPrefabComponentDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;


float xiiPrefabComponentDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (xiiComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Prefab") ? 1.0f : 0.0f;
}

void xiiPrefabComponentDragDropHandler::OnDragBegin(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_bShiftKeyDown)
  {
    if (pInfo->m_sTargetContext == "viewport")
      CreatePrefab(pInfo->m_vDropPosition, GetAssetGuid(pInfo), pInfo->m_ActiveParentObject, -1);
    else
      CreatePrefab(pInfo->m_vDropPosition, GetAssetGuid(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
  }
  else
  {
    if (pInfo->m_sTargetContext == "viewport")
      CreateDropObject(pInfo->m_vDropPosition, "xiiPrefabReferenceComponent", "Prefab", GetAssetGuidString(pInfo), pInfo->m_ActiveParentObject, -1);
    else
      CreateDropObject(pInfo->m_vDropPosition, "xiiPrefabReferenceComponent", "Prefab", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

void xiiPrefabComponentDragDropHandler::CreatePrefab(const xiiVec3& vPosition, const xiiUuid& AssetGuid, xiiUuid parent, xiiInt32 iInsertChildIndex)
{
  xiiVec3 vPos = vPosition;

  if (vPos.IsNaN())
    vPos.SetZero();

  auto pCmdHistory = m_pDocument->GetCommandHistory();

  xiiInstantiatePrefabCommand PasteCmd;
  PasteCmd.m_Parent           = parent;
  PasteCmd.m_CreateFromPrefab = AssetGuid;
  PasteCmd.m_Index            = iInsertChildIndex;
  PasteCmd.m_sBasePrefabGraph = xiiPrefabCache::GetSingleton()->GetCachedPrefabDocument(AssetGuid);
  PasteCmd.m_RemapGuid        = xiiUuid::MakeUuid();

  if (PasteCmd.m_sBasePrefabGraph.IsEmpty())
    return; // error

  pCmdHistory->AddCommand(PasteCmd).AssertSuccess();

  if (PasteCmd.m_CreatedRootObject.IsValid())
  {
    MoveObjectToPosition(PasteCmd.m_CreatedRootObject, vPos, xiiQuat::MakeIdentity());

    m_DraggedObjects.PushBack(PasteCmd.m_CreatedRootObject);
  }
}

void xiiPrefabComponentDragDropHandler::OnDragUpdate(const xiiDragDropInfo* pInfo)
{
  xiiComponentDragDropHandler::OnDragUpdate(pInfo);

  // the way prefabs are instantiated on the runtime side means the selection is not always immediately 'correct'
  // by resetting the selection, we can fix this
  SelectCreatedObjects();
}
