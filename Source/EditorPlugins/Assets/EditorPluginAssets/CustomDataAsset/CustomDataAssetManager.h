/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class xiiCustomDataAssetDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCustomDataAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiCustomDataAssetDocumentManager();
  ~xiiCustomDataAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override
  {
    // CustomData structs are typically defined in plugins, which may have changed, so they are a candidate for clearing them from the asset cache
    return xiiAssetDocumentManager::OutputReliability::Unknown;
  }

private:
  void OnDocumentManagerEvent(const xiiDocumentManager::Event& e);

  virtual void InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;
};
