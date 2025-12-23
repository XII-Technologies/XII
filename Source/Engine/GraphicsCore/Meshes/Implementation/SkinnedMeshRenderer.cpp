#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Meshes/SkinnedMeshComponent.h>
#include <GraphicsCore/Meshes/SkinnedMeshRenderer.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkinnedMeshRenderer, 1, xiiRTTIDefaultAllocator<xiiSkinnedMeshRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSkinnedMeshRenderer::xiiSkinnedMeshRenderer()  = default;
xiiSkinnedMeshRenderer::~xiiSkinnedMeshRenderer() = default;

void xiiSkinnedMeshRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiSkinnedMeshRenderData>());
}

void xiiSkinnedMeshRenderer::SetAdditionalData(const xiiRenderViewContext& renderViewContext, const xiiMeshRenderData* pRenderData) const
{
  // Don't call base class implementation here since the state will be overwritten in this method anyways.

  auto pSkinnedRenderData = static_cast<const xiiSkinnedMeshRenderData*>(pRenderData);

  if (!pSkinnedRenderData->m_pSkinningTransforms)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "TRUE");
    renderViewContext.m_pRenderContext->BindBuffer("skinningTransforms", pSkinnedRenderData->m_pSkinningTransforms);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_SkinnedMeshRenderer);
