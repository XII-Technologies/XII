#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

//////////////////////////////////////////////////////////////////////////

struct xiiPropertyMetaStateEvent;
class xiiSkeletonAssetDocument;

struct xiiSkeletonAssetEvent
{
  enum Type
  {
    RenderStateChanged,
    Transformed,
  };

  xiiSkeletonAssetDocument* m_pDocument = nullptr;
  Type                      m_Type;
};

class xiiSkeletonAssetDocument : public xiiSimpleAssetDocument<xiiEditableSkeleton>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkeletonAssetDocument, xiiSimpleAssetDocument<xiiEditableSkeleton>);

public:
  xiiSkeletonAssetDocument(const char* szDocumentPath);
  ~xiiSkeletonAssetDocument();

  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

  xiiStatus WriteResource(xiiStreamWriter& stream) const;

  bool m_bIsTransforming = false;

  virtual xiiManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return xiiManipulatorSearchStrategy::SelectedObject;
  }

  const xiiEvent<const xiiSkeletonAssetEvent&>& Events() const { return m_Events; }

  void SetRenderBones(bool enable);
  bool GetRenderBones() const { return m_bRenderBones; }

  void SetRenderColliders(bool enable);
  bool GetRenderColliders() const { return m_bRenderColliders; }

  void SetRenderJoints(bool enable);
  bool GetRenderJoints() const { return m_bRenderJoints; }

  void SetRenderSwingLimits(bool enable);
  bool GetRenderSwingLimits() const { return m_bRenderSwingLimits; }

  void SetRenderTwistLimits(bool enable);
  bool GetRenderTwistLimits() const { return m_bRenderTwistLimits; }

protected:
  virtual void               UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  void MergeWithNewSkeleton(xiiEditableSkeleton& newSkeleton);

  xiiEvent<const xiiSkeletonAssetEvent&> m_Events;
  bool                                   m_bRenderBones       = true;
  bool                                   m_bRenderColliders   = true;
  bool                                   m_bRenderJoints      = false; // currently not exposed
  bool                                   m_bRenderSwingLimits = true;
  bool                                   m_bRenderTwistLimits = true;
};

//////////////////////////////////////////////////////////////////////////

class xiiSkeletonAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkeletonAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiSkeletonAssetDocumentGenerator();
  ~xiiSkeletonAssetDocumentGenerator();

  virtual void      GetImportModes(const char* szParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual xiiStatus Generate(
    const char*                            szDataDirRelativePath,
    const xiiAssetDocumentGenerator::Info& info,
    xiiDocument*&                          out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "xiiSkeletonAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "AnimationSkeletonGroup"; }
};
