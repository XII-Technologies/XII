/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginMujoco/EditorPluginMujocoDLL.h>

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class XII_EDITORPLUGINMUJOCO_DLL xiiMuJoCoModelAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMuJoCoModelAssetProperties, xiiReflectedClass);

public:
  xiiString m_sSourceFile;
  bool      m_bWarningsAsErrors = false;
  /// Compiler inputs discovered through MuJoCo. Managed by the standard source-import pass.
  xiiDynamicArray<xiiString> m_Dependencies;
};

class XII_EDITORPLUGINMUJOCO_DLL xiiMuJoCoModelAssetDocument final : public xiiSimpleAssetDocument<xiiMuJoCoModelAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMuJoCoModelAssetDocument, xiiSimpleAssetDocument<xiiMuJoCoModelAssetProperties>);

public:
  explicit xiiMuJoCoModelAssetDocument(xiiStringView sDocumentPath);

protected:
  xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile,
                                            const xiiAssetFileHeader& assetHeader, xiiBitflags<xiiTransformFlags> flags) override;
  void               UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;
};

class XII_EDITORPLUGINMUJOCO_DLL xiiMuJoCoModelAssetDocumentManager final : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMuJoCoModelAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiMuJoCoModelAssetDocumentManager();
  bool GeneratesProfileSpecificAssets() const override { return true; }

private:
  xiiStatus                      InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument,
                                                        xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext) override;
  void                           InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_types) const override;
  xiiUInt64                      ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const override;
  xiiAssetDocumentTypeDescriptor m_TypeDescriptor;
};

class XII_EDITORPLUGINMUJOCO_DLL xiiMuJoCoModelAssetDocumentGenerator final : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMuJoCoModelAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiMuJoCoModelAssetDocumentGenerator();
  void          GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<ImportMode>& out_modes) const override;
  xiiStringView GetDocumentExtension() const override { return "xiiMuJoCoModelAsset"; }
  xiiStringView GetGeneratorGroup() const override { return "MuJoCoModel"; }
  xiiStatus     Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments) override;
};
