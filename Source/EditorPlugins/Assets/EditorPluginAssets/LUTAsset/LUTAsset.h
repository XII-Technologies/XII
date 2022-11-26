#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

class xiiTextureAssetProfileConfig;

class xiiLUTAssetDocument : public xiiSimpleAssetDocument<xiiLUTAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLUTAssetDocument, xiiSimpleAssetDocument<xiiLUTAssetProperties>);

public:
  xiiLUTAssetDocument(const char* szDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override
  {
    return xiiStatus(XII_SUCCESS);
  }
  virtual xiiTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
};

//////////////////////////////////////////////////////////////////////////

class xiiLUTAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLUTAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiLUTAssetDocumentGenerator();
  ~xiiLUTAssetDocumentGenerator();

  virtual void      GetImportModes(const char* szParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus Generate(
    const char*                            szDataDirRelativePath,
    const xiiAssetDocumentGenerator::Info& info,
    xiiDocument*&                          out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "xiiLUTAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "LUTs"; }
};
