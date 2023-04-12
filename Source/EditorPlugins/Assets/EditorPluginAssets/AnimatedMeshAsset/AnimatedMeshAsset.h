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
  xiiAnimatedMeshAssetDocument(const char* szDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

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

  virtual void          GetImportModes(xiiStringView sParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus     Generate(xiiStringView sDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument) override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiAnimatedMeshAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "Meshes"; }
};
