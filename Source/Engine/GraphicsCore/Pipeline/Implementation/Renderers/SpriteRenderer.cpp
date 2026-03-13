#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderers/SpriteRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpriteRenderer, 1, xiiRTTIDefaultAllocator<xiiSpriteRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSpriteRenderer::xiiSpriteRenderer() = default;

xiiSpriteRenderer::~xiiSpriteRenderer() = default;

void xiiSpriteRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiSpriteRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
