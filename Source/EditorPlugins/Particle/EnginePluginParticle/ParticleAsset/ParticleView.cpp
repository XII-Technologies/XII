#include <EnginePluginParticle/EnginePluginParticlePCH.h>

#include <EnginePluginParticle/ParticleAsset/ParticleContext.h>
#include <EnginePluginParticle/ParticleAsset/ParticleView.h>
#include <RendererCore/Pipeline/View.h>

xiiParticleViewContext::xiiParticleViewContext(xiiParticleContext* pParticleContext) :
  xiiEngineProcessViewContext(pParticleContext)
{
  m_pParticleContext = pParticleContext;
}

xiiParticleViewContext::~xiiParticleViewContext() {}

void xiiParticleViewContext::PositionThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);

  FocusCameraOnObject(m_Camera, bounds, 45.0f, -xiiVec3(-1.8f, 1.8f, 1.0f));
}

xiiViewHandle xiiParticleViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Particle Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
