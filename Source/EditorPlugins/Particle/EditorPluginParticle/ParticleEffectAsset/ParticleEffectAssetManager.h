#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class xiiParticleEffectAssetDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEffectAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiParticleEffectAssetDocumentManager();
  ~xiiParticleEffectAssetDocumentManager();


private:
  void OnDocumentManagerEvent(const xiiDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    const char*              szDocumentTypeName,
    const char*              szPath,
    bool                     bCreateNewDocument,
    xiiDocument*&            out_pDocument,
    const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;
};
