#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>

class xiiGeometry;
class xiiChunkStreamWriter;
struct xiiJoltCookingMesh;

class xiiJoltCollisionMeshAssetDocument : public xiiSimpleAssetDocument<xiiJoltCollisionMeshAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltCollisionMeshAssetDocument, xiiSimpleAssetDocument<xiiJoltCollisionMeshAssetProperties>);

public:
  xiiJoltCollisionMeshAssetDocument(const char* szDocumentPath, bool bConvexMesh);

  static xiiStatus WriteToStream(xiiChunkStreamWriter& stream, const xiiJoltCookingMesh& mesh, const xiiJoltCollisionMeshAssetProperties* pProp);

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  xiiStatus                  CreateMeshFromFile(xiiJoltCookingMesh& outMesh);
  xiiStatus                  CreateMeshFromGeom(xiiGeometry& geom, xiiJoltCookingMesh& outMesh);
  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  bool m_bIsConvexMesh = false;

  virtual void UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////


class xiiJoltCollisionMeshAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltCollisionMeshAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiJoltCollisionMeshAssetDocumentGenerator();
  ~xiiJoltCollisionMeshAssetDocumentGenerator();

  virtual void        GetImportModes(const char* szParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus   Generate(const char* szDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "xiiJoltCollisionMeshAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "JoltCollisionMeshes"; }
};

class xiiJoltConvexCollisionMeshAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltConvexCollisionMeshAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiJoltConvexCollisionMeshAssetDocumentGenerator();
  ~xiiJoltConvexCollisionMeshAssetDocumentGenerator();

  virtual void        GetImportModes(const char* szParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus   Generate(const char* szDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "xiiJoltConvexCollisionMeshAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "JoltCollisionMeshes"; }
};
