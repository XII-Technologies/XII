/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

class xiiAnimationClipAssetDocument;
struct xiiPropertyMetaStateEvent;

struct xiiRootMotionSource
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    None,
    Constant,
    // FromFeet,
    // AvgFromFeet,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiRootMotionSource);

class xiiAnimationClipAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationClipAssetProperties, xiiReflectedClass);

public:
  xiiAnimationClipAssetProperties();
  ~xiiAnimationClipAssetProperties();

  xiiString                    m_sSourceFile;
  xiiString                    m_sAnimationClipToExtract;
  xiiDynamicArray<xiiString>   m_AvailableClips;
  float                        m_fSampleRate  = 30.0f;
  bool                         m_bAdditive    = false;
  xiiUInt32                    m_uiFirstFrame = 0;
  xiiUInt32                    m_uiNumFrames  = 0;
  xiiString                    m_sPreviewMesh;
  xiiEnum<xiiRootMotionSource> m_RootMotionMode;
  xiiVec3                      m_vConstantRootMotion = xiiVec3::MakeZero();

  xiiEventTrackData m_EventTrack;

  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);
};

//////////////////////////////////////////////////////////////////////////

class xiiAnimationClipAssetDocument : public xiiSimpleAssetDocument<xiiAnimationClipAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationClipAssetDocument, xiiSimpleAssetDocument<xiiAnimationClipAssetProperties>);

public:
  xiiAnimationClipAssetDocument(xiiStringView sDocumentPath);

  virtual void   SetCommonAssetUiState(xiiCommonAssetUiState::Enum state, double value) override;
  virtual double GetCommonAssetUiState(xiiCommonAssetUiState::Enum state) const override;

  xiiUuid InsertEventTrackCpAt(xiiInt64 iTickX, const char* szValue);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;


private:
  float m_fSimulationSpeed = 1.0f;
};

//////////////////////////////////////////////////////////////////////////

class xiiAnimationClipAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationClipAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiAnimationClipAssetDocumentGenerator();
  ~xiiAnimationClipAssetDocumentGenerator();

  virtual void          GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiAnimationClipAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "AnimationClipGroup"; }
  virtual xiiStatus     Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments) override;
};
