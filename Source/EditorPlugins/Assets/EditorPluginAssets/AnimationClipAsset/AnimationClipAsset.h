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
  bool                         m_bAdditive    = false;
  xiiUInt32                    m_uiFirstFrame = 0;
  xiiUInt32                    m_uiNumFrames  = 0;
  xiiString                    m_sPreviewMesh;
  xiiEnum<xiiRootMotionSource> m_RootMotionMode;
  xiiVec3                      m_vConstantRootMotion;
  // xiiString m_sJoint1;
  // xiiString m_sJoint2;

  xiiEventTrackData m_EventTrack;

  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);
};

//////////////////////////////////////////////////////////////////////////

class xiiAnimationClipAssetDocument : public xiiSimpleAssetDocument<xiiAnimationClipAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationClipAssetDocument, xiiSimpleAssetDocument<xiiAnimationClipAssetProperties>);

public:
  xiiAnimationClipAssetDocument(const char* szDocumentPath);

  virtual void   SetCommonAssetUiState(xiiCommonAssetUiState::Enum state, double value) override;
  virtual double GetCommonAssetUiState(xiiCommonAssetUiState::Enum state) const override;

  xiiUuid InsertEventTrackCpAt(xiiInt64 tickX, const char* szValue);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  // void ApplyCustomRootMotion(xiiAnimationClipResourceDescriptor& anim) const;
  // void ExtractRootMotionFromFeet(xiiAnimationClipResourceDescriptor& anim, const xiiSkeleton& skeleton) const;
  // void MakeRootMotionConstantAverage(xiiAnimationClipResourceDescriptor& anim) const;

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

  virtual void        GetImportModes(const char* szParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus   Generate(const char* szDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "xiiAnimationClipAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "AnimationClipGroup"; }
};
