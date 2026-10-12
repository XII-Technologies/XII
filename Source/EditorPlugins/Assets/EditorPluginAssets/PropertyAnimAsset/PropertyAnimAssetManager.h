/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <Foundation/Types/Status.h>

class XII_EDITORPLUGINASSETS_DLL xiiPropertyAnimAssetDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPropertyAnimAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiPropertyAnimAssetDocumentManager();
  ~xiiPropertyAnimAssetDocumentManager();

  /// Source documents have no runtime resource to redirect to.
  xiiString GetAssetTableEntry(const xiiSubAsset* pSubAsset, xiiStringView sDataDirectory, const xiiPlatformProfile* pAssetProfile) const override { return {}; }

private:
  void OnDocumentManagerEvent(const xiiDocumentManager::Event& e);

  virtual xiiStatus InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext) override;
  virtual void      InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;
};
