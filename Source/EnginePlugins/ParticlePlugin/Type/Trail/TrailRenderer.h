#pragma once

#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/TrailShaderData.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleTrailRenderData final : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTrailRenderData, xiiRenderData);

public:
  xiiTexture2DResourceHandle              m_hTexture;
  xiiUInt16                               m_uiMaxTrailPoints;
  float                                   m_fSnapshotFraction;
  xiiArrayPtr<xiiBaseParticleShaderData>  m_BaseParticleData;
  xiiArrayPtr<xiiTrailParticleShaderData> m_TrailParticleData;
  xiiArrayPtr<xiiVec4>                    m_TrailPointsShared;
  xiiEnum<xiiParticleTypeRenderMode>      m_RenderMode;
  bool                                    m_bApplyObjectTransform = true;
  xiiTime                                 m_TotalEffectLifeTime;
  xiiUInt8                                m_uiNumVariationsX         = 1;
  xiiUInt8                                m_uiNumVariationsY         = 1;
  xiiUInt8                                m_uiNumFlipbookAnimationsX = 1;
  xiiUInt8                                m_uiNumFlipbookAnimationsY = 1;
  xiiTexture2DResourceHandle              m_hDistortionTexture;
  float                                   m_fDistortionStrength = 0;
};

/// \brief Implements rendering of a trail particle systems
class XII_PARTICLEPLUGIN_DLL xiiParticleTrailRenderer final : public xiiParticleRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTrailRenderer, xiiParticleRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiParticleTrailRenderer);

public:
  xiiParticleTrailRenderer();
  ~xiiParticleTrailRenderer();

  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const override;
  virtual void RenderBatch(
    const xiiRenderViewContext&  renderContext,
    const xiiRenderPipelinePass* pPass,
    const xiiRenderDataBatch&    batch) const override;

protected:
  bool ConfigureShader(const xiiParticleTrailRenderData* pRenderData, const xiiRenderViewContext& renderViewContext) const;

  static const xiiUInt32 s_uiParticlesPerBatch = 512;
  xiiGALBufferHandle     m_hBaseDataBuffer;
  xiiGALBufferHandle     m_hTrailDataBuffer;
  xiiGALBufferHandle     m_hTrailPointsDataBuffer8;
  xiiGALBufferHandle     m_hTrailPointsDataBuffer16;
  xiiGALBufferHandle     m_hTrailPointsDataBuffer32;
  xiiGALBufferHandle     m_hTrailPointsDataBuffer64;

  mutable xiiGALBufferHandle m_hActiveTrailPointsDataBuffer;
};
