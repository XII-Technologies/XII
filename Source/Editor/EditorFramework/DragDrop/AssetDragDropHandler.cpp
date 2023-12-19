#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DragDrop/AssetDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetDragDropHandler, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

bool xiiAssetDragDropHandler::IsAssetType(const xiiDragDropInfo* pInfo) const
{
  return pInfo->m_pMimeData->hasFormat("application/xiiEditor.AssetGuid");
}

xiiString xiiAssetDragDropHandler::GetAssetGuidString(const xiiDragDropInfo* pInfo) const
{
  QByteArray  ba = pInfo->m_pMimeData->data("application/xiiEditor.AssetGuid");
  QDataStream stream(&ba, QIODevice::ReadOnly);

  xiiHybridArray<QString, 1> guids;
  stream >> guids;

  if (guids.GetCount() > 1)
  {
    xiiLog::Warning("Dragging more than one asset type is currently not supported");
  }

  return guids[0].toUtf8().data();
}

xiiString xiiAssetDragDropHandler::GetAssetsDocumentTypeName(const xiiUuid& assetTypeGuid) const
{
  return xiiAssetCurator::GetSingleton()->GetSubAsset(assetTypeGuid)->m_Data.m_sSubAssetsDocumentTypeName.GetData();
}

bool xiiAssetDragDropHandler::IsSpecificAssetType(const xiiDragDropInfo* pInfo, const char* szType) const
{
  if (!IsAssetType(pInfo))
    return false;

  const xiiUuid guid = GetAssetGuid(pInfo);

  return GetAssetsDocumentTypeName(guid) == szType;
}
