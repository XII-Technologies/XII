#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginParticle/EnginePluginParticleDLL.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class xiiParticleComponent;

using xiiParticleEffectResourceHandle = xiiTypedResourceHandle<class xiiParticleEffectResource>;

class XII_ENGINEPLUGINPARTICLE_DLL xiiParticleContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleContext, xiiEngineProcessDocumentContext);

public:
  xiiParticleContext();
  ~xiiParticleContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual void                         OnThumbnailViewContextRequested() override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

  void RestartEffect();
  void SetAutoRestartEffect(bool loop);

private:
  xiiBoundingBoxSphere            m_ThumbnailBoundingVolume;
  xiiParticleEffectResourceHandle m_hParticle;
  xiiMeshResourceHandle           m_hPreviewMeshResource;
  xiiParticleComponent*           m_pComponent = nullptr;
};
