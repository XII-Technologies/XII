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
XII_CHECK_AT_COMPILETIME(sizeof(xiiPerLensFlareData) == 48);

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

void xiiLensFlareRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitTransparent);
}

void xiiLensFlareRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiGALDevice*     pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiRenderContext* pContext = renderViewContext.m_pRenderContext;

  const xiiLensFlareRenderData* pRenderData = batch.GetFirstData<xiiLensFlareRenderData>();

  const xiiUInt32    uiBufferSize   = xiiMath::RoundUp(batch.GetCount(), 128u);
  xiiGALBufferHandle hLensFlareData = CreateLensFlareDataBuffer(uiBufferSize);
  XII_SCOPE_EXIT(DeleteLensFlareDataBuffer(hLensFlareData));

  pContext->BindShader(m_hShader);
  pContext->BindBuffer("lensFlareData", pDevice->GetBuffer(hLensFlareData)->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  pContext->BindTexture2D("LensFlareTexture", pRenderData->m_hTexture);

  FillLensFlareData(batch);

  if (m_LensFlareData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    auto pCommandList = pContext->GetCommandList();

    void* pMappedData = nullptr;
    if (pCommandList->MapBuffer(hLensFlareData, xiiGALMapType::Write, xiiGALMapFlags::Discard, pMappedData).Succeeded())
    {
      auto pDataToUpdate = m_LensFlareData.GetByteArrayPtr();
      memcpy(pMappedData, pDataToUpdate.GetPtr(), pDataToUpdate.GetCount());

      pCommandList->UnmapBuffer(hLensFlareData, xiiGALMapType::Write).AssertSuccess();
    }
    else
    {
      xiiLog::Error("Failed to map buffer to update content.");
    }

    pContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::TriangleList, m_LensFlareData.GetCount() * 2);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

xiiGALBufferHandle xiiLensFlareRenderer::CreateLensFlareDataBuffer(xiiUInt32 uiBufferSize) const
{
  xiiGALBufferCreationDescription desc;
  desc.m_uiElementByteStride = sizeof(xiiPerLensFlareData);
  desc.m_uiSize              = desc.m_uiElementByteStride * uiBufferSize;
  desc.m_Mode                = xiiGALBufferMode::Structured;
  desc.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  desc.m_ResourceUsage       = xiiGALResourceUsage::Dynamic;
  desc.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  return xiiGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void xiiLensFlareRenderer::DeleteLensFlareDataBuffer(xiiGALBufferHandle hBuffer) const
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
    LensFlareData.AspectRatioAndShift = xiiShaderUtilities::Float2ToRG16F(xiiVec2(pRenderData->m_fAspectRatio, pRenderData->m_fShiftToCenter));
    LensFlareData.ColorRG             = xiiShaderUtilities::PackFloat16intoUint(pRenderData->m_Color.x, pRenderData->m_Color.y);
    LensFlareData.ColorBA             = xiiShaderUtilities::PackFloat16intoUint(pRenderData->m_Color.z, pRenderData->m_Color.w);
    LensFlareData.Flags               = (pRenderData->m_bInverseTonemap ? LENS_FLARE_INVERSE_TONEMAP : 0) | (pRenderData->m_bGreyscaleTexture ? LENS_FLARE_GREYSCALE_TEXTURE : 0) | (pRenderData->m_bApplyFog ? LENS_FLARE_APPLY_FOG : 0);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_LensFlareRenderer);
