#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderers/MeshRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshRenderer, 1, xiiRTTIDefaultAllocator<xiiMeshRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiMeshRenderer::xiiMeshRenderer() = default;

xiiMeshRenderer::~xiiMeshRenderer() = default;

void xiiMeshRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiMeshRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
