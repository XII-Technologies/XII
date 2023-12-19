#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <GraphicsCore/Declarations.h>

class XII_ENGINEPLUGINASSETS_DLL xiiAnimationClipContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationClipContext, xiiEngineProcessDocumentContext);

public:
  xiiAnimationClipContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg);
  void SetPlaybackPosition(double pos);

  xiiGameObject*     m_pGameObject = nullptr;
  xiiString          m_sAnimatedMeshToUse;
  xiiComponentHandle m_hAnimMeshComponent;
  xiiComponentHandle m_hAnimControllerComponent;
};
