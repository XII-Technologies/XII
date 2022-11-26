#pragma once

#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/BillboardQuadParticleShaderData.h>
#include <RendererCore/../../../Data/Base/Shaders/Particles/TangentQuadParticleShaderData.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleQuadRenderData final : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleQuadRenderData, xiiRenderData);

public:
  xiiEnum<xiiParticleTypeRenderMode>              m_RenderMode;
  xiiTexture2DResourceHandle                      m_hTexture;
  xiiArrayPtr<xiiBaseParticleShaderData>          m_BaseParticleData;
  xiiArrayPtr<xiiBillboardQuadParticleShaderData> m_BillboardParticleData;
  xiiArrayPtr<xiiTangentQuadParticleShaderData>   m_TangentParticleData;
  xiiTime                                         m_TotalEffectLifeTime;
  bool                                            m_bApplyObjectTransform    = true;
  xiiUInt8                                        m_uiNumVariationsX         = 1;
  xiiUInt8                                        m_uiNumVariationsY         = 1;
  xiiUInt8                                        m_uiNumFlipbookAnimationsX = 1;
  xiiUInt8                                        m_uiNumFlipbookAnimationsY = 1;

  xiiTexture2DResourceHandle m_hDistortionTexture;
  float                      m_fDistortionStrength = 0;
  xiiTempHashedString        m_QuadModePermutation;
};

/// \brief Implements rendering of particle systems
class XII_PARTICLEPLUGIN_DLL xiiParticleQuadRenderer final : public xiiParticleRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleQuadRenderer, xiiParticleRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiParticleQuadRenderer);

public:
  xiiParticleQuadRenderer();
  ~xiiParticleQuadRenderer();

  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const override;
  virtual void RenderBatch(
    const xiiRenderViewContext&  renderContext,
    const xiiRenderPipelinePass* pPass,
    const xiiRenderDataBatch&    batch) const override;


protected:
  void ConfigureRenderMode(const xiiParticleQuadRenderData* pRenderData, xiiRenderContext* pRenderContext) const;

  static const xiiUInt32 s_uiParticlesPerBatch = 1024;
  xiiGALBufferHandle     m_hBaseDataBuffer;
  xiiGALBufferHandle     m_hBillboardDataBuffer;
  xiiGALBufferHandle     m_hTangentDataBuffer;
};
