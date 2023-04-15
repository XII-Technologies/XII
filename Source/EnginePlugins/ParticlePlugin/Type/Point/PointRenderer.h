#pragma once

#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/BillboardQuadParticleShaderData.h>

class XII_PARTICLEPLUGIN_DLL xiiParticlePointRenderData final : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticlePointRenderData, xiiRenderData);

public:
  xiiArrayPtr<xiiBaseParticleShaderData>          m_BaseParticleData;
  xiiArrayPtr<xiiBillboardQuadParticleShaderData> m_BillboardParticleData;
  bool                                            m_bApplyObjectTransform = true;
  xiiTime                                         m_TotalEffectLifeTime;
};

/// \brief Implements rendering of particle systems
class XII_PARTICLEPLUGIN_DLL xiiParticlePointRenderer final : public xiiParticleRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticlePointRenderer, xiiParticleRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiParticlePointRenderer);

public:
  xiiParticlePointRenderer();
  ~xiiParticlePointRenderer();

  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(
    const xiiRenderViewContext&  renderContext,
    const xiiRenderPipelinePass* pPass,
    const xiiRenderDataBatch&    batch) const override;

protected:
  static const xiiUInt32 s_uiParticlesPerBatch = 1024;
  xiiGALBufferHandle     m_hBaseDataBuffer;
  xiiGALBufferHandle     m_hBillboardDataBuffer;
};
