/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class xiiMaterialAssetDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiMaterialAssetDocumentManager();
  ~xiiMaterialAssetDocumentManager();

  virtual xiiString GetRelativeOutputFileName(const xiiAssetDocumentTypeDescriptor* pTypeDescriptor, xiiStringView sDataDirectory, xiiStringView sDocumentPath, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile) const override;
  virtual bool      IsOutputUpToDate(xiiStringView sDocumentPath, xiiStringView sOutputTag, xiiUInt64 uiHash, const xiiAssetDocumentTypeDescriptor* pTypeDescriptor) override;

  static const char* const s_szShaderOutputTag;

private:
  void OnDocumentManagerEvent(const xiiDocumentManager::Event& e);

  virtual void InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;
};
