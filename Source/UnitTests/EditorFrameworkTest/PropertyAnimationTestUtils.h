/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>

namespace
{
  // Create fixtures through the real document implementation without creating Qt windows or an asset-curator project.
  class PropertyAnimationSourceManager : public xiiAssetDocumentManager
  {
  public:
    PropertyAnimationSourceManager()
    {
      m_Type.m_sDocumentTypeName  = "PropertyAnim";
      m_Type.m_sFileExtension     = "xiiPropertyAnimAsset";
      m_Type.m_pDocumentType      = xiiGetStaticRTTI<xiiPropertyAnimAssetDocument>();
      m_Type.m_pManager           = this;
      m_Type.m_AssetDocumentFlags = xiiAssetDocumentFlags::DisableTransform;
    }

    ~PropertyAnimationSourceManager() override { CloseAllDocumentsOfManager(); }

  private:
    xiiStatus InternalCreateDocument(xiiStringView, xiiStringView path, bool, xiiDocument*& out_document, const xiiDocumentObject*) override
    {
      out_document = new xiiPropertyAnimAssetDocument(path);
      return XII_SUCCESS;
    }
    void                           InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& types) const override { types.PushBack(&m_Type); }
    bool                           GeneratesProfileSpecificAssets() const override { return false; }
    xiiAssetDocumentTypeDescriptor m_Type;
  };
} // namespace
