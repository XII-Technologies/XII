#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Quad/QuadParticleRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleQuadRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleQuadRenderer, 1, xiiRTTIDefaultAllocator<xiiParticleQuadRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleQuadRenderer::xiiParticleQuadRenderer()
{
  CreateParticleDataBuffer(m_hBaseDataBuffer, sizeof(xiiBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hBillboardDataBuffer, sizeof(xiiBillboardQuadParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTangentDataBuffer, sizeof(xiiTangentQuadParticleShaderData), s_uiParticlesPerBatch);

  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Particles/QuadParticle.xiiShader");
}

xiiParticleQuadRenderer::~xiiParticleQuadRenderer()
{
  DestroyParticleDataBuffer(m_hBaseDataBuffer);
  DestroyParticleDataBuffer(m_hBillboardDataBuffer);
  DestroyParticleDataBuffer(m_hTangentDataBuffer);
}

void xiiParticleQuadRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const
{
  types.PushBack(xiiGetStaticRTTI<xiiParticleQuadRenderData>());
}

void xiiParticleQuadRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiRenderContext*     pRenderContext     = renderViewContext.m_pRenderContext;
  xiiGALDevice*         pDevice            = xiiGALDevice::GetDefaultDevice();
  xiiGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  TempSystemCB systemConstants(pRenderContext);

  pRenderContext->BindShader(m_hShader);

  // make sure our structured buffer is allocated and bound
  {
    pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, s_uiParticlesPerBatch * 2);

    pRenderContext->BindBuffer("particleBaseData", pDevice->GetDefaultResourceView(m_hBaseDataBuffer));
    pRenderContext->BindBuffer("particleBillboardQuadData", pDevice->GetDefaultResourceView(m_hBillboardDataBuffer));
    pRenderContext->BindBuffer("particleTangentQuadData", pDevice->GetDefaultResourceView(m_hTangentDataBuffer));
  }

  // now render all particle effects of type Quad
  for (auto it = batch.GetIterator<xiiParticleQuadRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const xiiParticleQuadRenderData* pRenderData = it;

    const xiiBaseParticleShaderData*          pParticleBaseData      = pRenderData->m_BaseParticleData.GetPtr();
    const xiiBillboardQuadParticleShaderData* pParticleBillboardData = pRenderData->m_BillboardParticleData.GetPtr();
    const xiiTangentQuadParticleShaderData*   pParticleTangentData   = pRenderData->m_TangentParticleData.GetPtr();

    xiiUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();

    pRenderContext->BindTexture2D("ParticleTexture", pRenderData->m_hTexture);

    ConfigureRenderMode(pRenderData, pRenderContext);

    systemConstants.SetGenericData(
      pRenderData->m_bApplyObjectTransform, pRenderData->m_GlobalTransform, pRenderData->m_TotalEffectLifeTime, pRenderData->m_uiNumVariationsX, pRenderData->m_uiNumVariationsY, pRenderData->m_uiNumFlipbookAnimationsX, pRenderData->m_uiNumFlipbookAnimationsY, pRenderData->m_fDistortionStrength);

    pRenderContext->SetShaderPermutationVariable("PARTICLE_QUAD_MODE", pRenderData->m_QuadModePermutation);

    while (uiNumParticles > 0)
    {
      // upload this batch of particle data
      const xiiUInt32 uiNumParticlesInBatch = xiiMath::Min<xiiUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hBaseDataBuffer, 0, xiiMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray());
      pParticleBaseData += uiNumParticlesInBatch;

      if (pParticleBillboardData != nullptr)
      {
        pGALCommandEncoder->UpdateBuffer(m_hBillboardDataBuffer, 0, xiiMakeArrayPtr(pParticleBillboardData, uiNumParticlesInBatch).ToByteArray());
        pParticleBillboardData += uiNumParticlesInBatch;
      }

      if (pParticleTangentData != nullptr)
      {
        pGALCommandEncoder->UpdateBuffer(m_hTangentDataBuffer, 0, xiiMakeArrayPtr(pParticleTangentData, uiNumParticlesInBatch).ToByteArray());
        pParticleTangentData += uiNumParticlesInBatch;
      }

      // do one drawcall
      renderViewContext.m_pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch * 2).IgnoreResult();
    }
  }
}

void xiiParticleQuadRenderer::ConfigureRenderMode(const xiiParticleQuadRenderData* pRenderData, xiiRenderContext* pRenderContext) const
{
  switch (pRenderData->m_RenderMode)
  {
    case xiiParticleTypeRenderMode::Additive:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_ADDITIVE");
      break;
    case xiiParticleTypeRenderMode::Blended:
    case xiiParticleTypeRenderMode::BlendedForeground:
    case xiiParticleTypeRenderMode::BlendedBackground:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_BLENDED");
      break;
    case xiiParticleTypeRenderMode::Opaque:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_OPAQUE");
      break;
    case xiiParticleTypeRenderMode::Distortion:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_DISTORTION");
      pRenderContext->BindTexture2D("ParticleDistortionTexture", pRenderData->m_hDistortionTexture);
      break;
    case xiiParticleTypeRenderMode::BlendAdd:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_BLENDADD");
      break;
  }
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Quad_QuadParticleRenderer);
