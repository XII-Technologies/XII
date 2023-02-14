#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleRenderer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleRenderer::TempSystemCB::TempSystemCB(xiiRenderContext* pRenderContext)
{
  // TODO This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage(m_pConstants, XII_STRINGIZE(xiiParticleSystemConstants));

  pRenderContext->BindConstantBuffer("xiiParticleSystemConstants", m_hConstantBuffer);
}

xiiParticleRenderer::TempSystemCB::~TempSystemCB()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void xiiParticleRenderer::TempSystemCB::SetGenericData(bool bApplyObjectTransform, const xiiTransform& ObjectTransform, xiiTime effectLifeTime, xiiUInt8 uiNumVariationsX, xiiUInt8 uiNumVariationsY, xiiUInt8 uiNumFlipbookAnimsX, xiiUInt8 uiNumFlipbookAnimsY, float fDistortionStrength /*= 0*/)
{
  xiiParticleSystemConstants& cb  = m_pConstants->GetDataForWriting();
  cb.TextureAtlasVariationFramesX = uiNumVariationsX;
  cb.TextureAtlasVariationFramesY = uiNumVariationsY;
  cb.TextureAtlasFlipbookFramesX  = uiNumFlipbookAnimsX;
  cb.TextureAtlasFlipbookFramesY  = uiNumFlipbookAnimsY;
  cb.DistortionStrength           = fDistortionStrength;
  cb.TotalEffectLifeTime          = effectLifeTime.AsFloatInSeconds();

  if (bApplyObjectTransform)
    cb.ObjectToWorldMatrix = ObjectTransform.GetAsMat4();
  else
    cb.ObjectToWorldMatrix.SetIdentity();
}


void xiiParticleRenderer::TempSystemCB::SetTrailData(float fSnapshotFraction, xiiInt32 iNumUsedTrailPoints)
{
  xiiParticleSystemConstants& cb = m_pConstants->GetDataForWriting();
  cb.SnapshotFraction            = fSnapshotFraction;
  cb.NumUsedTrailPoints          = iNumUsedTrailPoints;
}

xiiParticleRenderer::xiiParticleRenderer()  = default;
xiiParticleRenderer::~xiiParticleRenderer() = default;

void xiiParticleRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& categories) const
{
  categories.PushBack(xiiDefaultRenderDataCategories::LitTransparent);
}

void xiiParticleRenderer::CreateParticleDataBuffer(xiiGALBufferHandle& inout_hBuffer, xiiUInt32 uiDataTypeSize, xiiUInt32 uiNumParticlesPerBatch)
{
  if (inout_hBuffer.IsInvalidated())
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiStructSize                = uiDataTypeSize;
    desc.m_uiTotalSize                 = uiNumParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferType                  = xiiGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer      = true;
    desc.m_bAllowShaderResourceView    = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    inout_hBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }
}


void xiiParticleRenderer::DestroyParticleDataBuffer(xiiGALBufferHandle& inout_hBuffer)
{
  if (!inout_hBuffer.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroyBuffer(inout_hBuffer);
    inout_hBuffer.Invalidate();
  }
}

void xiiParticleRenderer::BindParticleShader(xiiRenderContext* pRenderContext, const char* szShader) const
{
  if (!m_hShader.IsValid())
  {
    // m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>(szShader);
  }

  pRenderContext->BindShader(m_hShader);
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Renderer_ParticleRenderer);
