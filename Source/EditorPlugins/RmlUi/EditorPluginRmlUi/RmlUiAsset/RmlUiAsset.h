#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

struct xiiRmlUiResourceDescriptor;

class xiiRmlUiAssetDocument : public xiiSimpleAssetDocument<xiiRmlUiAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRmlUiAssetDocument, xiiSimpleAssetDocument<xiiRmlUiAssetProperties>);

public:
  xiiRmlUiAssetDocument(const char* szDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class xiiRmlUiAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRmlUiAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiRmlUiAssetDocumentGenerator();
  ~xiiRmlUiAssetDocumentGenerator();

  virtual void          GetImportModes(xiiStringView sParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus     Generate(xiiStringView sDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument) override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiRmlUiAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "RmlUis"; }
};
