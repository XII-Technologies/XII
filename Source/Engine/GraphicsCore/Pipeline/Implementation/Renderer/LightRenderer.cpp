#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderer/LightRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLightRenderer, 1, xiiRTTIDefaultAllocator<xiiLightRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiLightRenderer::xiiLightRenderer() = default;

xiiLightRenderer::~xiiLightRenderer() = default;

void xiiLightRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiLightRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
