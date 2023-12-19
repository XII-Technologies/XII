#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>

class xiiTextureAssetProfileConfig;

struct xiiTextureChannelMode
{
  using StorageType = xiiUInt8;

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
  xiiTextureAssetDocument(xiiStringView sDocumentPath);

  // for previewing purposes
  xiiEnum<xiiTextureChannelMode> m_ChannelMode;
  xiiInt32                       m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level
  bool                           m_bIsRenderTarget = false;

protected:
  virtual void               InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override { return xiiStatus(XII_SUCCESS); }
  virtual xiiTransformStatus InternalTransformAsset(const char* szTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

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

  enum class TextureType
  {
    Diffuse,
    Normal,
    Occlusion,
    Roughness,
    Metalness,
    ORM,
    Height,
    HDR,
    Linear,
  };

  virtual void          GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiTextureAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "Images"; }
  virtual xiiStatus     Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDocument*& out_pGeneratedDocument) override;

  static TextureType DetermineTextureType(xiiStringView sFile);
};
