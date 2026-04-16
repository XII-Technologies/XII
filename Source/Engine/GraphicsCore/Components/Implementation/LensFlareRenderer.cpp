#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/LensFlareComponent.h>
#include <GraphicsCore/Components/LensFlareRenderer.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Shader/ShaderUtils.h>

#include <Shaders/Materials/LensFlareData.h>
static_assert(sizeof(xiiPerLensFlareData) == 48);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLensFlareRenderer, 1, xiiRTTIDefaultAllocator<xiiLensFlareRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiLensFlareRenderer::xiiLensFlareRenderer()
{
  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Materials/LensFlareMaterial.xiiShader");
}

xiiLensFlareRenderer::~xiiLensFlareRenderer() = default;

void xiiLensFlareRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiLensFlareRenderData>());
}

void xiiLensFlareRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  const xiiLensFlareRenderData* pRenderData = batch.GetFirstData<xiiLensFlareRenderData>();

  const xiiUInt32            uiBufferSize   = xiiMath::RoundUp(batch.GetCount(), 128U);
  xiiSharedPtr<xiiGALBuffer> pLensFlareData = CreateLensFlareDataBuffer(uiBufferSize);
  XII_SCOPE_EXIT(DeleteLensFlareDataBuffer(pLensFlareData));

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindBuffer("lensFlareData", pLensFlareData);
  renderViewContext.m_pRenderContext->BindTexture2D("LensFlareTexture", pRenderData->m_hTexture);

  FillLensFlareData(batch);

  if (m_LensFlareData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(renderViewContext.m_pRenderContext->GetCommandList(), pLensFlareData, 0, m_LensFlareData.GetByteArrayPtr()).AssertSuccess();

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, m_LensFlareData.GetCount() * 2);
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
}

xiiSharedPtr<xiiGALBuffer> xiiLensFlareRenderer::CreateLensFlareDataBuffer(xiiUInt32 uiBufferSize) const
{
  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_uiElementByteStride = sizeof(xiiPerLensFlareData);
  bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride * uiBufferSize;
  bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;

  return xiiGPUResourcePool::GetDefaultInstance()->GetBuffer(bufferDescription);
}

void xiiLensFlareRenderer::DeleteLensFlareDataBuffer(xiiSharedPtr<xiiGALBuffer> hBuffer) const
{
  xiiGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void xiiLensFlareRenderer::FillLensFlareData(const xiiRenderDataBatch& batch) const
{
  m_LensFlareData.Clear();
  m_LensFlareData.Reserve(batch.GetCount());

  for (auto it = batch.GetIterator<xiiLensFlareRenderData>(); it.IsValid(); ++it)
  {
    const xiiLensFlareRenderData* pRenderData = it;

    auto& LensFlareData               = m_LensFlareData.ExpandAndGetRef();
    LensFlareData.WorldSpacePosition  = pRenderData->m_GlobalTransform.m_vPosition;
    LensFlareData.Size                = pRenderData->m_fSize;
    LensFlareData.MaxScreenSize       = pRenderData->m_fMaxScreenSize;
    LensFlareData.OcclusionRadius     = pRenderData->m_fOcclusionSampleRadius;
    LensFlareData.OcclusionSpread     = pRenderData->m_fOcclusionSampleSpread;
    LensFlareData.DepthOffset         = pRenderData->m_fOcclusionDepthOffset;
    LensFlareData.AspectRatioAndShift = xiiGALShaderUtilities::Float2ToRG16F(xiiVec2(pRenderData->m_fAspectRatio, pRenderData->m_fShiftToCenter));
    LensFlareData.ColorRG             = xiiGALShaderUtilities::PackFloat16intoUint(pRenderData->m_Color.x, pRenderData->m_Color.y);
    LensFlareData.ColorBA             = xiiGALShaderUtilities::PackFloat16intoUint(pRenderData->m_Color.z, pRenderData->m_Color.w);
    LensFlareData.Flags               = (pRenderData->m_bInverseTonemap ? LENS_FLARE_INVERSE_TONEMAP : 0) | (pRenderData->m_bGreyscaleTexture ? LENS_FLARE_GREYSCALE_TEXTURE : 0) | (pRenderData->m_bApplyFog ? LENS_FLARE_APPLY_FOG : 0);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_LensFlareRenderer);
