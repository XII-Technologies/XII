/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DragDrop/AssetDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetDragDropHandler, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

bool xiiAssetDragDropHandler::IsAssetType(const xiiDragDropInfo* pInfo) const
{
  return pInfo && pInfo->m_pMimeData && pInfo->m_pMimeData->hasFormat("application/xiiEditor.AssetGuid");
}

xiiString xiiAssetDragDropHandler::GetAssetGuidString(const xiiDragDropInfo* pInfo) const
{
  if (!IsAssetType(pInfo))
    return {};
  QByteArray  ba = pInfo->m_pMimeData->data("application/xiiEditor.AssetGuid");
  QDataStream stream(&ba, QIODevice::ReadOnly);

  xiiUInt32 count = 0;
  stream >> count;
  if (stream.status() != QDataStream::Ok || count == 0)
    return {};
  if (count > 1)
  {
    xiiLog::Warning("Dragging more than one asset type is currently not supported");
  }

  // Only the first asset is supported. Reading it directly also avoids reserving an
  // array using an unchecked count from a malformed or external drag payload.
  QString first;
  stream >> first;
  const xiiString guid = first.toUtf8().data();
  return stream.status() == QDataStream::Ok && xiiConversionUtils::IsStringUuid(guid) ? guid : xiiString();
}

xiiString xiiAssetDragDropHandler::GetAssetsDocumentTypeName(const xiiUuid& assetTypeGuid) const
{
  if (!assetTypeGuid.IsValid())
    return {};
  const auto asset = xiiAssetCurator::GetSingleton()->GetSubAsset(assetTypeGuid);
  return asset ? asset->m_Data.m_sSubAssetsDocumentTypeName.GetString() : xiiString();
}

bool xiiAssetDragDropHandler::IsSpecificAssetType(const xiiDragDropInfo* pInfo, const char* szType) const
{
  if (!IsAssetType(pInfo))
    return false;

  const xiiUuid guid = GetAssetGuid(pInfo);

  return GetAssetsDocumentTypeName(guid) == szType;
}
