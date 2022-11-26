#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiParticleContext;

class xiiParticleViewContext : public xiiEngineProcessViewContext
{
public:
  xiiParticleViewContext(xiiParticleContext* pParticleContext);
  ~xiiParticleViewContext();

  void PositionThumbnailCamera(const xiiBoundingBoxSphere& bounds);

protected:
  virtual xiiViewHandle CreateView() override;

  xiiParticleContext* m_pParticleContext;
};
