/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/Animation/AnimationPreview.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/Declarations.h>
#include <SharedPluginAssets/SkeletonAsset/SkeletonPreview.h>

class XII_ENGINEPLUGINASSETS_DLL xiiSkeletonContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkeletonContext, xiiEngineProcessDocumentContext);

public:
  xiiSkeletonContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  xiiSkeletonResourceHandle GetSkeleton() const { return m_hSkeleton; }

  /// Caller holds world write access. Acquires current resources and refreshes cached tooling geometry.
  xiiResult DrawSkeleton(const xiiView& view);

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void                 QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg);
  xiiResult            UpdatePreview();
  xiiBoundingBoxSphere GetPreviewBounds();

  xiiGameObject*                    m_pGameObject = nullptr;
  xiiSkeletonResourceHandle         m_hSkeleton;
  xiiString                         m_sAnimatedMeshToUse;
  xiiComponentHandle                m_hAnimMeshComponent;
  xiiUniquePtr<xiiEditableSkeleton> m_pAuthoring;
  xiiSkeletonPreviewSettings        m_Settings;
  xiiSkeletonPreview                m_Preview;
  xiiAnimationPreview               m_AnimationPreview;
  xiiUInt32                         m_uiSkeletonChangeCounter = xiiInvalidIndex;
  xiiUInt32                         m_uiMeshChangeCounter     = xiiInvalidIndex;
  bool                              m_bPreviewDirty           = true;
  bool                              m_bMeshPoseDirty          = true;
};
