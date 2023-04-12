#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>

struct xiiTextureCubeChannelMode
{
  typedef xiiUInt8 StorageType;

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
  xiiTextureCubeAssetDocument(const char* szDocumentPath);

  // for previewing purposes
  xiiEnum<xiiTextureCubeChannelMode> m_ChannelMode;
  xiiInt32                           m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override
  {
    return xiiStatus(XII_SUCCESS);
  }
  virtual xiiTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

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

  virtual void          GetImportModes(xiiStringView sParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus     Generate(xiiStringView szDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument) override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiTextureCubeAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "Images"; }
};
