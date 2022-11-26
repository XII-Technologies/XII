#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Declarations.h>

class XII_ENGINEPLUGINASSETS_DLL xiiSkeletonContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkeletonContext, xiiEngineProcessDocumentContext);

public:
  xiiSkeletonContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  xiiSkeletonResourceHandle GetSkeleton() const { return m_hSkeleton; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg);

  xiiGameObject*            m_pGameObject = nullptr;
  xiiSkeletonResourceHandle m_hSkeleton;
  xiiComponentHandle        m_hSkeletonComponent;
  xiiComponentHandle        m_hPoseComponent;
};
