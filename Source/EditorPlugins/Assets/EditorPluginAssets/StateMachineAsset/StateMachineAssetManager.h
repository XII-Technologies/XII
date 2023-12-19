#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>

class xiiStateMachineAssetManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineAssetManager, xiiAssetDocumentManager);

public:
  xiiStateMachineAssetManager();
  ~xiiStateMachineAssetManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return xiiAssetDocumentManager::OutputReliability::Perfect; }

private:
  void OnDocumentManagerEvent(const xiiDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    xiiStringView            sDocumentTypeName,
    xiiStringView            sPath,
    bool                     bCreateNewDocument,
    xiiDocument*&            out_pDocument,
    const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;
};
