#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderers/ClothRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClothRenderer, 1, xiiRTTIDefaultAllocator<xiiClothRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiClothRenderer::xiiClothRenderer() = default;

xiiClothRenderer::~xiiClothRenderer() = default;

void xiiClothRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiClothRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
