/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GraphicsCore/AnimationSystem/EditableSkeleton.h>

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
  xiiSkeletonAssetDocument(xiiStringView sDocumentPath);
  ~xiiSkeletonAssetDocument();

  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

  xiiStatus WriteResource(xiiStreamWriter& inout_stream, const xiiEditableSkeleton& skeleton) const;

  bool m_bIsTransforming = false;

  virtual xiiManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return xiiManipulatorSearchStrategy::SelectedObject;
  }

  const xiiEvent<const xiiSkeletonAssetEvent&>& Events() const { return m_Events; }

  void SetRenderBones(bool bEnable);
  bool GetRenderBones() const { return m_bRenderBones; }

  void SetRenderColliders(bool bEnable);
  bool GetRenderColliders() const { return m_bRenderColliders; }

  void SetRenderJoints(bool bEnable);
  bool GetRenderJoints() const { return m_bRenderJoints; }

  void SetRenderSwingLimits(bool bEnable);
  bool GetRenderSwingLimits() const { return m_bRenderSwingLimits; }

  void SetRenderTwistLimits(bool bEnable);
  bool GetRenderTwistLimits() const { return m_bRenderTwistLimits; }

  void SetRenderPreviewMesh(bool bEnable);
  bool GetRenderPreviewMesh() const { return m_bRenderPreviewMesh; }

protected:
  virtual void               UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  const xiiEditableSkeleton* MergeWithNewSkeleton(xiiEditableSkeleton& newSkeleton);

  xiiEvent<const xiiSkeletonAssetEvent&> m_Events;
  bool                                   m_bRenderBones       = true;
  bool                                   m_bRenderColliders   = true;
  bool                                   m_bRenderJoints      = false; // currently not exposed
  bool                                   m_bRenderSwingLimits = true;
  bool                                   m_bRenderTwistLimits = true;
  bool                                   m_bRenderPreviewMesh = true;
};

//////////////////////////////////////////////////////////////////////////

class xiiSkeletonAssetDocumentGenerator : public xiiAssetDocumentGenerator
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkeletonAssetDocumentGenerator, xiiAssetDocumentGenerator);

public:
  xiiSkeletonAssetDocumentGenerator();
  ~xiiSkeletonAssetDocumentGenerator();

  virtual void          GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual xiiStringView GetDocumentExtension() const override { return "xiiSkeletonAsset"; }
  virtual xiiStringView GetGeneratorGroup() const override { return "AnimationSkeletonGroup"; }
  virtual xiiStatus     Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments) override;
};
