/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

// Saved test assets exercise the production document, sub-asset and curator lifecycle.
class ResaveTestProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(ResaveTestProperties, xiiReflectedClass);

public:
  xiiString m_sTransform;
  xiiString m_sThumbnail;
  xiiString m_sPackage;
  xiiString m_sValue;
};

class ResaveTestDocument : public xiiSimpleAssetDocument<ResaveTestProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(ResaveTestDocument, xiiAssetDocument);

public:
  ResaveTestDocument(xiiStringView path) : xiiSimpleAssetDocument(path, xiiAssetDocEngineConnection::None) {}
  void MakeReadOnly(bool value) { SetReadOnly(value); }
  void MakePartiallyUnderstood(bool value)
  {
    xiiSet<xiiString> types;
    if (value)
      types.Insert("MissingResaveTestType");
    SetUnknownObjectTypes(types, value ? 1 : 0);
  }

protected:
  xiiTransformStatus InternalTransformAsset(xiiStreamWriter&, xiiStringView, const xiiPlatformProfile*, const xiiAssetFileHeader&, xiiBitflags<xiiTransformFlags>) override
  {
    return xiiTransformStatus("This fixture tests document resaving, with transformation disabled.");
  }
};

class ResaveTestManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(ResaveTestManager, xiiAssetDocumentManager);

public:
  ResaveTestManager()
  {
    m_Type.m_sDocumentTypeName  = "ResaveTest";
    m_Type.m_sFileExtension     = "xiiResaveTest";
    m_Type.m_sAssetCategory     = "Testing";
    m_Type.m_pDocumentType      = xiiGetStaticRTTI<ResaveTestDocument>();
    m_Type.m_pManager           = this;
    m_Type.m_AssetDocumentFlags = xiiAssetDocumentFlags::DisableTransform;
  }
  ~ResaveTestManager() override { CloseAllDocumentsOfManager(); }
  bool           GeneratesProfileSpecificAssets() const override { return false; }
  static xiiUuid GetSubAssetId(xiiUuid owner)
  {
    owner.CombineWithSeed(xiiUuid::MakeStableUuidFromString("ResaveTestChild"));
    return owner;
  }
  void FillOutSubAssetList(const xiiAssetDocumentInfo& info, xiiDynamicArray<xiiSubAssetData>& out_subAssets) const override
  {
    auto& sub   = out_subAssets.ExpandAndGetRef();
    sub.m_Guid  = GetSubAssetId(info.m_DocumentID);
    sub.m_sName = "Child";
    sub.m_sSubAssetsDocumentTypeName.Assign("ResaveTest");
  }

private:
  xiiStatus InternalCreateDocument(xiiStringView, xiiStringView path, bool, xiiDocument*& out_document, const xiiDocumentObject*) override
  {
    out_document = new ResaveTestDocument(path);
    return XII_SUCCESS;
  }
  void                           InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& types) const override { types.PushBack(&m_Type); }
  xiiAssetDocumentTypeDescriptor m_Type;
};
