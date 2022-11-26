#pragma once

#include <EditorFramework/DragDrop/DragDropHandler.h>

class xiiDocument;

class XII_EDITORFRAMEWORK_DLL xiiAssetDragDropHandler : public xiiDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetDragDropHandler, xiiDragDropHandler);

public:
protected:
  bool IsAssetType(const xiiDragDropInfo* pInfo) const;

  xiiString GetAssetGuidString(const xiiDragDropInfo* pInfo) const;

  xiiUuid GetAssetGuid(const xiiDragDropInfo* pInfo) const { return xiiConversionUtils::ConvertStringToUuid(GetAssetGuidString(pInfo)); }

  xiiString GetAssetsDocumentTypeName(const xiiUuid& assetTypeGuid) const;

  bool IsSpecificAssetType(const xiiDragDropInfo* pInfo, const char* szType) const;

  xiiDocument* m_pDocument;
};
