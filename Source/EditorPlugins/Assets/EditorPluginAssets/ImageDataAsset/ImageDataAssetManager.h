#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class xiiImageDataAssetDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImageDataAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiImageDataAssetDocumentManager();
  ~xiiImageDataAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return xiiAssetDocumentManager::OutputReliability::Perfect; }

private:
  void OnDocumentManagerEvent(const xiiDocumentManager::Event& e);

  virtual void InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;
};
