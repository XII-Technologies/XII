/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/Animation/AnimationPreview.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GraphicsCore/Declarations.h>

class XII_ENGINEPLUGINASSETS_DLL xiiAnimationClipContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationClipContext, xiiEngineProcessDocumentContext);

public:
  xiiAnimationClipContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  bool m_bDisplayGrid = true;

protected:
  virtual xiiStatus OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void      QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg);
  void      SetPlaybackPosition(double pos);
  xiiResult UpdatePreviewPose();

  xiiGameObject*                 m_pGameObject = nullptr;
  xiiString                      m_sAnimatedMeshToUse;
  xiiComponentHandle             m_hAnimMeshComponent;
  xiiAnimationClipResourceHandle m_hAnimationClip;
  xiiAnimationPreview            m_Preview;
  double                         m_fPlaybackPosition = 0.0;
};
