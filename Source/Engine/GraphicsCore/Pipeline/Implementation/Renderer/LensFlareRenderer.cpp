#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderer/LensFlareRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLensFlareRenderer, 1, xiiRTTIDefaultAllocator<xiiLensFlareRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiLensFlareRenderer::xiiLensFlareRenderer() = default;

xiiLensFlareRenderer::~xiiLensFlareRenderer() = default;

void xiiLensFlareRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiLensFlareRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
