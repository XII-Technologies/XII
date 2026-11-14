/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>

class xiiMeshResourceDescriptor;
class xiiMaterialAssetDocument;

class xiiAnimatedMeshAssetDocument : public xiiSimpleAssetDocument<xiiAnimatedMeshAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimatedMeshAssetDocument, xiiSimpleAssetDocument<xiiAnimatedMeshAssetProperties>);

public:
  xiiAnimatedMeshAssetDocument(xiiStringView sDocumentPath);

protected:
  xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  xiiTransformStatus CreateMeshFromFile(xiiAnimatedMeshAssetProperties* pProp, xiiMeshResourceDescriptor& desc, bool bAllowMaterialImport);

  xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class xiiAnimatedMeshAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimatedMeshAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiAnimatedMeshAssetDocumentGenerator();
  ~xiiAnimatedMeshAssetDocumentGenerator() override;

  void          GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const override;
  xiiStringView GetDocumentExtension() const override { return "xiiAnimatedMeshAsset"; }
  xiiStringView GetGeneratorGroup() const override { return "Meshes"; }
  xiiStatus     Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments) override;

private:
  xiiStatus GenerateComplete(xiiStringView sInputFileAbs, xiiDynamicArray<xiiDocument*>& out_generatedDocuments);
};
