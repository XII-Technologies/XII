#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticlePointRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticlePointRenderer, 1, xiiRTTIDefaultAllocator<xiiParticlePointRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticlePointRenderer::xiiParticlePointRenderer()
{
  CreateParticleDataBuffer(m_hBaseDataBuffer, sizeof(xiiBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hBillboardDataBuffer, sizeof(xiiBillboardQuadParticleShaderData), s_uiParticlesPerBatch);

  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Particles/Point.xiiShader");
}


xiiParticlePointRenderer::~xiiParticlePointRenderer()
{
  DestroyParticleDataBuffer(m_hBaseDataBuffer);
  DestroyParticleDataBuffer(m_hBillboardDataBuffer);
}

void xiiParticlePointRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const
{
  types.PushBack(xiiGetStaticRTTI<xiiParticlePointRenderData>());
}

void xiiParticlePointRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiRenderContext*     pRenderContext     = renderViewContext.m_pRenderContext;
  xiiGALDevice*         pDevice            = xiiGALDevice::GetDefaultDevice();
  xiiGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  TempSystemCB systemConstants(pRenderContext);

  pRenderContext->BindShader(m_hShader);

  // make sure our structured buffer is allocated and bound
  {
    pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Points, s_uiParticlesPerBatch);
    pRenderContext->BindBuffer("particleBaseData", pDevice->GetDefaultResourceView(m_hBaseDataBuffer));
    pRenderContext->BindBuffer("particleBillboardQuadData", pDevice->GetDefaultResourceView(m_hBillboardDataBuffer));
  }

  // now render all particle effects of type Point
  for (auto it = batch.GetIterator<xiiParticlePointRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const xiiParticlePointRenderData* pRenderData = it;

    const xiiBaseParticleShaderData*          pParticleBaseData      = pRenderData->m_BaseParticleData.GetPtr();
    const xiiBillboardQuadParticleShaderData* pParticleBillboardData = pRenderData->m_BillboardParticleData.GetPtr();

    xiiUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();

    systemConstants.SetGenericData(pRenderData->m_bApplyObjectTransform, pRenderData->m_GlobalTransform, pRenderData->m_TotalEffectLifeTime, 1, 1, 1, 1);

    while (uiNumParticles > 0)
    {
      // upload this batch of particle data
      const xiiUInt32 uiNumParticlesInBatch = xiiMath::Min<xiiUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hBaseDataBuffer, 0, xiiMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray());
      pParticleBaseData += uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hBillboardDataBuffer, 0, xiiMakeArrayPtr(pParticleBillboardData, uiNumParticlesInBatch).ToByteArray());
      pParticleBillboardData += uiNumParticlesInBatch;

      // do one drawcall
      pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch).IgnoreResult();
    }
  }
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_PointRenderer);
