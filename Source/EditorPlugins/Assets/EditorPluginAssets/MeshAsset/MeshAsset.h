#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>

class xiiMeshResourceDescriptor;
class xiiGeometry;
class xiiMaterialAssetDocument;

class xiiMeshAssetDocument : public xiiSimpleAssetDocument<xiiMeshAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshAssetDocument, xiiSimpleAssetDocument<xiiMeshAssetProperties>);

public:
  xiiMeshAssetDocument(xiiStringView sDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  void               CreateMeshFromGeom(xiiMeshAssetProperties* pProp, xiiMeshResourceDescriptor& desc);
  xiiTransformStatus CreateMeshFromFile(xiiMeshAssetProperties* pProp, xiiMeshResourceDescriptor& desc, bool bAllowMaterialImport);

  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  virtual void UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class xiiMeshAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiMeshAssetDocumentGenerator();
  ~xiiMeshAssetDocumentGenerator();

  virtual void          GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiMeshAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual xiiStatus     Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDocument*& out_pGeneratedDocument) override;
};
