#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderers/AtmosphereRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtmosphereRenderer, 1, xiiRTTIDefaultAllocator<xiiAtmosphereRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAtmosphereRenderer::xiiAtmosphereRenderer() = default;

xiiAtmosphereRenderer::~xiiAtmosphereRenderer() = default;

void xiiAtmosphereRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiAtmosphereRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
