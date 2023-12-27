#include <EditorPluginScene/EditorPluginScenePCH.h>

#include "EditorFramework/GUI/RawDocumentTreeModel.moc.h"
#include <Core/World/GameObject.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/LayerDragDropHandler.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <QIODevice>
#include <QMimeData>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLayerDragDropHandler, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

const xiiRTTI* xiiLayerDragDropHandler::GetCommonBaseType(const xiiDragDropInfo* pInfo) const
{
  QByteArray                             encodedData = pInfo->m_pMimeData->data("application/xiiEditor.ObjectSelection");
  QDataStream                            stream(&encodedData, QIODevice::ReadOnly);
  xiiHybridArray<xiiDocumentObject*, 32> Dragged;
  stream >> Dragged;

  const xiiRTTI* pCommonBaseType = nullptr;
  for (const xiiDocumentObject* pItem : Dragged)
  {
    pCommonBaseType = pCommonBaseType == nullptr ? pItem->GetType() : xiiReflectionUtils::GetCommonBaseType(pCommonBaseType, pItem->GetType());
  }
  return pCommonBaseType;
}


//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLayerOnLayerDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiLayerOnLayerDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;

float xiiLayerOnLayerDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext == "layertree" && pInfo->m_pMimeData->hasFormat("application/xiiEditor.ObjectSelection"))
  {
    if (xiiScene2Document* pDoc = xiiDynamicCast<xiiScene2Document*>(xiiDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
    {
      const xiiDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);
      if (pTarget && pInfo->m_pAdapter && GetCommonBaseType(pInfo)->IsDerivedFrom(xiiGetStaticRTTI<xiiSceneLayerBase>()))
      {
        const xiiAbstractProperty* pTargetProp = pInfo->m_pAdapter->GetType()->FindPropertyByName(pInfo->m_pAdapter->GetChildProperty());
        if (pTargetProp && xiiGetStaticRTTI<xiiSceneLayerBase>()->IsDerivedFrom(pTargetProp->GetSpecificType()))
          return 1.0f;
      }
    }
  }
  return 0;
}

void xiiLayerOnLayerDragDropHandler::OnDrop(const xiiDragDropInfo* pInfo)
{
  if (xiiScene2Document* pDoc = xiiDynamicCast<xiiScene2Document*>(xiiDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
  {
    const xiiUuid activeDoc = pDoc->GetActiveLayer();
    XII_VERIFY(pDoc->SetActiveLayer(pDoc->GetGuid()).Succeeded(), "Failed to set active document.");
    {
      // We need to make a copy of the info as the target document is actually the scene here, not the active document.
      xiiDragDropInfo info  = *pInfo;
      info.m_TargetDocument = pDoc->GetGuid();
      xiiQtDocumentTreeModel::MoveObjects(info);
    }
    XII_VERIFY(pDoc->SetActiveLayer(activeDoc).Succeeded(), "Failed to set active document.");
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectOnLayerDragDropHandler, 1, xiiRTTIDefaultAllocator<xiiGameObjectOnLayerDragDropHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;

float xiiGameObjectOnLayerDragDropHandler::CanHandle(const xiiDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext == "layertree" && pInfo->m_pMimeData->hasFormat("application/xiiEditor.ObjectSelection"))
  {
    if (xiiScene2Document* pDoc = xiiDynamicCast<xiiScene2Document*>(xiiDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
    {
      const xiiDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);
      if (pTarget && pTarget->GetType() == xiiGetStaticRTTI<xiiSceneLayer>() && pInfo->m_iTargetObjectInsertChildIndex == -1 && GetCommonBaseType(pInfo) == xiiGetStaticRTTI<xiiGameObject>())
      {
        xiiObjectAccessorBase* pAccessor = pDoc->GetSceneObjectAccessor();
        xiiUuid                layerGuid = pAccessor->Get<xiiUuid>(pTarget, "Layer");
        if (pDoc->IsLayerLoaded(layerGuid))
          return 1.0f;
      }
    }
  }
  return 0;
}

void xiiGameObjectOnLayerDragDropHandler::OnDrop(const xiiDragDropInfo* pInfo)
{
  if (xiiScene2Document* pDoc = xiiDynamicCast<xiiScene2Document*>(xiiDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
  {
    const xiiDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);

    QByteArray                             encodedData = pInfo->m_pMimeData->data("application/xiiEditor.ObjectSelection");
    QDataStream                            stream(&encodedData, QIODevice::ReadOnly);
    xiiHybridArray<xiiDocumentObject*, 32> Dragged;
    stream >> Dragged;

    // We are dragging game objects on another layer => delete objects and recreate in target layer.
    xiiSceneDocument*      pSourceDoc = xiiDynamicCast<xiiSceneDocument*>(Dragged[0]->GetDocumentObjectManager()->GetDocument());
    xiiObjectAccessorBase* pAccessor  = pDoc->GetSceneObjectAccessor();
    xiiUuid                layerGuid  = pAccessor->Get<xiiUuid>(pTarget, "Layer");
    xiiSceneDocument*      pTargetDoc = pDoc->GetLayerDocument(layerGuid);

    if (pSourceDoc != pTargetDoc && pTargetDoc)
    {
      const xiiUuid activeDoc = pDoc->GetActiveLayer();
      {
        // activeDoc should already match pSourceDoc, but just to be sure.
        XII_VERIFY(pDoc->SetActiveLayer(pSourceDoc->GetGuid()).Succeeded(), "Failed to set active document.");

        xiiResult res = xiiActionManager::ExecuteAction(nullptr, "Selection.Copy", pSourceDoc, xiiVariant());
        if (res.Failed())
        {
          xiiLog::Error("Failed to copy selection while moving objects between layers.");
          return;
        }
        res = xiiActionManager::ExecuteAction(nullptr, "Selection.Delete", pSourceDoc, xiiVariant());
        if (res.Failed())
        {
          xiiLog::Error("Failed to copy selection while moving objects between layers.");
          return;
        }
      }
      {
        XII_VERIFY(pDoc->SetActiveLayer(pTargetDoc->GetGuid()).Succeeded(), "Failed to set active document.");
        xiiResult res = xiiActionManager::ExecuteAction(nullptr, "Selection.PasteAtOriginalLocation", pTargetDoc, xiiVariant());
        if (res.Failed())
        {
          xiiLog::Error("Failed to paste selection while moving objects between layers.");
          return;
        }
      }
      XII_VERIFY(pDoc->SetActiveLayer(activeDoc).Succeeded(), "Failed to set active document.");
    }
  }
}
