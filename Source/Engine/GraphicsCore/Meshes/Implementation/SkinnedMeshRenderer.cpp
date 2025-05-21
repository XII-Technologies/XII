#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Meshes/SkinnedMeshComponent.h>
#include <GraphicsCore/Meshes/SkinnedMeshRenderer.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Resources/Buffer.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkinnedMeshRenderer, 1, xiiRTTIDefaultAllocator<xiiSkinnedMeshRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSkinnedMeshRenderer::xiiSkinnedMeshRenderer()  = default;
xiiSkinnedMeshRenderer::~xiiSkinnedMeshRenderer() = default;

void xiiSkinnedMeshRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiSkinnedMeshRenderData>());
}

void xiiSkinnedMeshRenderer::SetAdditionalData(const xiiRenderViewContext& renderViewContext, const xiiMeshRenderData* pRenderData) const
{
  // Don't call base class implementation here since the state will be overwritten in this method anyways.

  xiiSharedPtr<xiiGALDevice> pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiRenderContext*          pContext = renderViewContext.m_pRenderContext;

  auto pSkinnedRenderData = static_cast<const xiiSkinnedMeshRenderData*>(pRenderData);

  if (pSkinnedRenderData->m_hSkinningTransforms.IsInvalidated())
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "TRUE");

    pContext->BindBuffer("skinningTransforms", pDevice->GetBuffer(pSkinnedRenderData->m_hSkinningTransforms)->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  }
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_SkinnedMeshRenderer);
