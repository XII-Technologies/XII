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
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  xiiStatus CreateMeshFromFile(xiiAnimatedMeshAssetProperties* pProp, xiiMeshResourceDescriptor& desc);


  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class xiiAnimatedMeshAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimatedMeshAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiAnimatedMeshAssetDocumentGenerator();
  ~xiiAnimatedMeshAssetDocumentGenerator();

  virtual void          GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiAnimatedMeshAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual xiiStatus     Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDocument*& out_pGeneratedDocument) override;
};
