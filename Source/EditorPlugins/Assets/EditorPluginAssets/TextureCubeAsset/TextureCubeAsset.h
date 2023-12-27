#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>

struct xiiTextureCubeChannelMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    RGB,
    Red,
    Green,
    Blue,
    Alpha,

    Default = RGB
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiTextureCubeChannelMode);

class xiiTextureCubeAssetDocument : public xiiSimpleAssetDocument<xiiTextureCubeAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureCubeAssetDocument, xiiSimpleAssetDocument<xiiTextureCubeAssetProperties>);

public:
  xiiTextureCubeAssetDocument(xiiStringView sDocumentPath);

  // for previewing purposes
  xiiEnum<xiiTextureCubeChannelMode> m_ChannelMode;
  xiiInt32                           m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override
  {
    return xiiStatus(XII_SUCCESS);
  }
  virtual xiiTransformStatus InternalTransformAsset(const char* szTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  xiiStatus RunTexConv(const char* szTargetFile, const xiiAssetFileHeader& AssetHeader, bool bUpdateThumbnail);

  virtual void UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class xiiTextureCubeAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureCubeAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiTextureCubeAssetDocumentGenerator();
  ~xiiTextureCubeAssetDocumentGenerator();

  virtual void          GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiTextureCubeAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "Images"; }
  virtual xiiStatus     Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDocument*& out_pGeneratedDocument) override;
};
