#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

struct xiiKrautTreeResourceDescriptor;
struct xiiKrautGeneratorResourceDescriptor;

namespace xiiModelImporter2
{
  enum class TextureSemantic : xiiInt8;
}

class xiiKrautTreeAssetDocument : public xiiSimpleAssetDocument<xiiKrautTreeAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiKrautTreeAssetDocument, xiiSimpleAssetDocument<xiiKrautTreeAssetProperties>);

public:
  xiiKrautTreeAssetDocument(const char* szDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  void SyncBackAssetProperties(xiiKrautTreeAssetProperties*& pProp, const xiiKrautGeneratorResourceDescriptor& desc);

  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////


class xiiKrautTreeAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiKrautTreeAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiKrautTreeAssetDocumentGenerator();
  ~xiiKrautTreeAssetDocumentGenerator();

  virtual void          GetImportModes(xiiStringView sParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus     Generate(xiiStringView sDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument) override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiKrautTreeAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "KrautTrees"; }
};
