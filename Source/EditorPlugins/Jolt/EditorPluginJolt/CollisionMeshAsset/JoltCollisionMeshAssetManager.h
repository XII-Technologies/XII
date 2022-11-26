#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class xiiJoltCollisionMeshAssetDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltCollisionMeshAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiJoltCollisionMeshAssetDocumentManager();
  ~xiiJoltCollisionMeshAssetDocumentManager();

private:
  void OnDocumentManagerEvent(const xiiDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    const char*              szDocumentTypeName,
    const char*              szPath,
    bool                     bCreateNewDocument,
    xiiDocument*&            out_pDocument,
    const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual xiiUInt64 ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const override;

  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;
  xiiAssetDocumentTypeDescriptor m_DocTypeDesc2;
};
