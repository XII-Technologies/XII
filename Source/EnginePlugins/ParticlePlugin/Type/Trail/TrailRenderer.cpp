#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

//clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTrailRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTrailRenderer, 1, xiiRTTIDefaultAllocator<xiiParticleTrailRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleTrailRenderer::xiiParticleTrailRenderer()
{
  CreateParticleDataBuffer(m_hBaseDataBuffer, sizeof(xiiBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailDataBuffer, sizeof(xiiTrailParticleShaderData), s_uiParticlesPerBatch);

  // this is kinda stupid, apparently due to stride enforcement I cannot reuse the same buffer for different sizes
  // and instead have to create one buffer with every size ...

  CreateParticleDataBuffer(m_hTrailPointsDataBuffer8, sizeof(xiiTrailParticlePointsData8), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailPointsDataBuffer16, sizeof(xiiTrailParticlePointsData16), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailPointsDataBuffer32, sizeof(xiiTrailParticlePointsData32), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailPointsDataBuffer64, sizeof(xiiTrailParticlePointsData64), s_uiParticlesPerBatch);

  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Particles/Trail.xiiShader");
}

xiiParticleTrailRenderer::~xiiParticleTrailRenderer()
{
  DestroyParticleDataBuffer(m_hBaseDataBuffer);
  DestroyParticleDataBuffer(m_hTrailDataBuffer);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer8);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer16);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer32);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer64);
}

void xiiParticleTrailRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const
{
  types.PushBack(xiiGetStaticRTTI<xiiParticleTrailRenderData>());
}

void xiiParticleTrailRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiRenderContext*     pRenderContext     = renderViewContext.m_pRenderContext;
  xiiGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  TempSystemCB systemConstants(pRenderContext);

  pRenderContext->BindShader(m_hShader);

  // make sure our structured buffer is allocated and bound
  {
    pRenderContext->BindBuffer("particleBaseData", xiiGALDevice::GetDefaultDevice()->GetDefaultResourceView(m_hBaseDataBuffer));
    pRenderContext->BindBuffer("particleTrailData", xiiGALDevice::GetDefaultDevice()->GetDefaultResourceView(m_hTrailDataBuffer));
  }

  // now render all particle effects of type Trail
  for (auto it = batch.GetIterator<xiiParticleTrailRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const xiiParticleTrailRenderData* pRenderData = it;

    if (!ConfigureShader(pRenderData, renderViewContext))
      continue;

    const xiiUInt32 uiBucketSize            = xiiParticleTypeTrail::ComputeTrailPointBucketSize(pRenderData->m_uiMaxTrailPoints);
    const xiiUInt32 uiMaxTrailSegments      = uiBucketSize - 1;
    const xiiUInt32 uiPrimFactor            = 2;
    const xiiUInt32 uiMaxPrimitivesToRender = s_uiParticlesPerBatch * uiMaxTrailSegments * uiPrimFactor;


    pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, uiMaxPrimitivesToRender);

    const xiiBaseParticleShaderData*  pParticleBaseData  = pRenderData->m_BaseParticleData.GetPtr();
    const xiiTrailParticleShaderData* pParticleTrailData = pRenderData->m_TrailParticleData.GetPtr();


    const xiiVec4* pParticlePointsData = pRenderData->m_TrailPointsShared.GetPtr();

    pRenderContext->BindTexture2D("ParticleTexture", pRenderData->m_hTexture);

    systemConstants.SetGenericData(
      pRenderData->m_bApplyObjectTransform, pRenderData->m_GlobalTransform, pRenderData->m_TotalEffectLifeTime, pRenderData->m_uiNumVariationsX, pRenderData->m_uiNumVariationsY, pRenderData->m_uiNumFlipbookAnimationsX, pRenderData->m_uiNumFlipbookAnimationsY, pRenderData->m_fDistortionStrength);
    systemConstants.SetTrailData(pRenderData->m_fSnapshotFraction, pRenderData->m_uiMaxTrailPoints);

    xiiUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();
    while (uiNumParticles > 0)
    {
      // upload this batch of particle data
      const xiiUInt32 uiNumParticlesInBatch = xiiMath::Min<xiiUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hBaseDataBuffer, 0, xiiMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray());
      pParticleBaseData += uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hTrailDataBuffer, 0, xiiMakeArrayPtr(pParticleTrailData, uiNumParticlesInBatch).ToByteArray());
      pParticleTrailData += uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hActiveTrailPointsDataBuffer, 0, xiiMakeArrayPtr(pParticlePointsData, uiNumParticlesInBatch * uiBucketSize).ToByteArray());
      pParticlePointsData += uiNumParticlesInBatch * uiBucketSize;

      // do one drawcall
      pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch * uiMaxTrailSegments * uiPrimFactor).IgnoreResult();
    }
  }
}

bool xiiParticleTrailRenderer::ConfigureShader(const xiiParticleTrailRenderData* pRenderData, const xiiRenderViewContext& renderViewContext) const
{
  auto pRenderContext = renderViewContext.m_pRenderContext;

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

  switch (xiiParticleTypeTrail::ComputeTrailPointBucketSize(pRenderData->m_uiMaxTrailPoints))
  {
    case 8:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT8");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer8;
      break;
    case 16:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT16");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer16;
      break;
    case 32:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT32");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer32;
      break;
    case 64:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT64");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer64;
      break;

    default:
      return false;
  }

  renderViewContext.m_pRenderContext->BindBuffer("particlePointsData", xiiGALDevice::GetDefaultDevice()->GetDefaultResourceView(m_hActiveTrailPointsDataBuffer));
  return true;
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_TrailRenderer);
