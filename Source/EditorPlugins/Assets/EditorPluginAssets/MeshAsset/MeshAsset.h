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
  xiiMeshAssetDocument(const char* szDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

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

  virtual void      GetImportModes(const char* szParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus Generate(
    const char*                            szDataDirRelativePath,
    const xiiAssetDocumentGenerator::Info& info,
    xiiDocument*&                          out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "xiiMeshAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "Meshes"; }
};
