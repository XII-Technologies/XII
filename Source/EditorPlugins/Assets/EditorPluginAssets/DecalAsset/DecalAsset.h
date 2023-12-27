#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class xiiAssetFileHeader;
struct xiiPropertyMetaStateEvent;

struct xiiDecalMode
{
  using StorageType = xiiInt8;

  enum Enum
  {
    BaseColor,
    BaseColorNormal,
    BaseColorORM,
    BaseColorNormalORM,
    BaseColorEmissive,

    Default = BaseColor
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiDecalMode);

class xiiDecalAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalAssetProperties, xiiReflectedClass);

public:
  xiiDecalAssetProperties();

  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

  xiiEnum<xiiDecalMode> m_Mode;
  bool                  m_bBlendModeColorize = false;

  xiiString m_sAlphaMask;
  xiiString m_sBaseColor;
  xiiString m_sNormal;
  xiiString m_sORM;
  xiiString m_sEmissive;

  bool NeedsBaseColor() const { return true; }
  bool NeedsNormal() const { return m_Mode == xiiDecalMode::BaseColorNormal || m_Mode == xiiDecalMode::BaseColorNormalORM; }
  bool NeedsORM() const { return m_Mode == xiiDecalMode::BaseColorORM || m_Mode == xiiDecalMode::BaseColorNormalORM; }
  bool NeedsEmissive() const { return m_Mode == xiiDecalMode::BaseColorEmissive; }
};


class xiiDecalAssetDocument : public xiiSimpleAssetDocument<xiiDecalAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalAssetDocument, xiiSimpleAssetDocument<xiiDecalAssetProperties>);

public:
  xiiDecalAssetDocument(xiiStringView sDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class xiiDecalAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiDecalAssetDocumentGenerator();
  ~xiiDecalAssetDocumentGenerator();

  virtual void          GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiDecalAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "Images"; }
  virtual xiiStatus     Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDocument*& out_pGeneratedDocument) override;
};
