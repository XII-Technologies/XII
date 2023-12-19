#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>

class xiiVisualScriptClassAssetManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptClassAssetManager, xiiAssetDocumentManager);

public:
  xiiVisualScriptClassAssetManager();
  ~xiiVisualScriptClassAssetManager();

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
