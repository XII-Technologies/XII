#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderers/DynamicMeshRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicMeshRenderer, 1, xiiRTTIDefaultAllocator<xiiDynamicMeshRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDynamicMeshRenderer::xiiDynamicMeshRenderer() = default;

xiiDynamicMeshRenderer::~xiiDynamicMeshRenderer() = default;

void xiiDynamicMeshRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiDynamicMeshRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
