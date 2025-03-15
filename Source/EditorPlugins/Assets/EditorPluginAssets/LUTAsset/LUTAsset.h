#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

class xiiTextureAssetProfileConfig;

class xiiLUTAssetDocument : public xiiSimpleAssetDocument<xiiLUTAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLUTAssetDocument, xiiSimpleAssetDocument<xiiLUTAssetProperties>);

public:
  xiiLUTAssetDocument(xiiStringView sDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override
  {
    return xiiStatus(XII_SUCCESS);
  }
  virtual xiiTransformStatus InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
};

//////////////////////////////////////////////////////////////////////////

class xiiLUTAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLUTAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiLUTAssetDocumentGenerator();
  ~xiiLUTAssetDocumentGenerator();

  virtual void          GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiLUTAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "LUTs"; }
  virtual xiiStatus     Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments) override;
};
