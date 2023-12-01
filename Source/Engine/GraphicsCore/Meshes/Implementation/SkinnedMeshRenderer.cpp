#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Meshes/SkinnedMeshComponent.h>
#include <GraphicsCore/Meshes/SkinnedMeshRenderer.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkinnedMeshRenderer, 1, xiiRTTIDefaultAllocator<xiiSkinnedMeshRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiUInt32 xiiSkinnedMeshRenderer::s_uiSkinningBufferUpdates = 0;

xiiSkinnedMeshRenderer::xiiSkinnedMeshRenderer()  = default;
xiiSkinnedMeshRenderer::~xiiSkinnedMeshRenderer() = default;

void xiiSkinnedMeshRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiSkinnedMeshRenderData>());
}

void xiiSkinnedMeshRenderer::SetAdditionalData(const xiiRenderViewContext& renderViewContext, const xiiMeshRenderData* pRenderData) const
{
  // Don't call base class implementation here since the state will be overwritten in this method anyways.

  xiiGALDevice*     pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiRenderContext* pContext = renderViewContext.m_pRenderContext;

  auto pSkinnedRenderData = static_cast<const xiiSkinnedMeshRenderData*>(pRenderData);

  if (pSkinnedRenderData->m_hSkinningTransforms.IsInvalidated())
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "TRUE");

    if (pSkinnedRenderData->m_bTransformsUpdated != nullptr && *pSkinnedRenderData->m_bTransformsUpdated == false)
    {
      // if this is the first renderer that is supposed to actually render the skinned mesh, upload the skinning matrices
      *pSkinnedRenderData->m_bTransformsUpdated = true;
      pContext->GetCommandEncoder()->UpdateBuffer(pSkinnedRenderData->m_hSkinningTransforms, 0, pSkinnedRenderData->m_pNewSkinningTransformData);

      // TODO: could expose this somewhere (xiiStats?)
      s_uiSkinningBufferUpdates++;
    }

    pContext->BindBuffer("skinningTransforms", pDevice->GetDefaultResourceView(pSkinnedRenderData->m_hSkinningTransforms));
  }
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_SkinnedMeshRenderer);
