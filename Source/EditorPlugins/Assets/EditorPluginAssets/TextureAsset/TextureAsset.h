#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>

class xiiTextureAssetProfileConfig;

struct xiiTextureChannelMode
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    RGBA,
    RGB,
    Red,
    Green,
    Blue,
    Alpha,

    Default = RGBA
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiTextureChannelMode);

class xiiTextureAssetDocument : public xiiSimpleAssetDocument<xiiTextureAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureAssetDocument, xiiSimpleAssetDocument<xiiTextureAssetProperties>);

public:
  xiiTextureAssetDocument(const char* szDocumentPath);

  // for previewing purposes
  xiiEnum<xiiTextureChannelMode> m_ChannelMode;
  xiiInt32                       m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level
  bool                           m_bIsRenderTarget = false;

protected:
  virtual void               InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override { return xiiStatus(XII_SUCCESS); }
  virtual xiiTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  xiiStatus RunTexConv(const char* szTargetFile, const xiiAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const xiiTextureAssetProfileConfig* pAssetConfig);

  virtual void UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class xiiTextureAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiTextureAssetDocumentGenerator();
  ~xiiTextureAssetDocumentGenerator();

  virtual void          GetImportModes(xiiStringView sParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus     Generate(xiiStringView sDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument) override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiTextureAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "Images"; }
};
