/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <JoltPlugin/Resources/JoltCollisionMeshResource.h>

class XII_EDITORPLUGINJOLT_DLL xiiJoltCollisionMeshAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltCollisionMeshAssetProperties, xiiReflectedClass);

public:
  xiiString                             m_sMesh;
  xiiEnum<xiiJoltCollisionGeometryType> m_Type;
  /// Indexed by mesh material slot. Empty entries inherit the actor surface.
  xiiDynamicArray<xiiString> m_Surfaces;
};

class XII_EDITORPLUGINJOLT_DLL xiiJoltCollisionMeshAssetDocument final : public xiiSimpleAssetDocument<xiiJoltCollisionMeshAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltCollisionMeshAssetDocument, xiiSimpleAssetDocument<xiiJoltCollisionMeshAssetProperties>);

public:
  explicit xiiJoltCollisionMeshAssetDocument(xiiStringView sDocumentPath);

protected:
  xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile,
                                            const xiiAssetFileHeader& assetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
};

class XII_EDITORPLUGINJOLT_DLL xiiJoltCollisionMeshAssetDocumentManager final : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltCollisionMeshAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiJoltCollisionMeshAssetDocumentManager();
  bool GeneratesProfileSpecificAssets() const override { return true; }

private:
  xiiStatus InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument,
                                   xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext) override;
  void      InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_types) const override;
  xiiUInt64 ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const override;

  xiiAssetDocumentTypeDescriptor m_TypeDescriptor;
};
