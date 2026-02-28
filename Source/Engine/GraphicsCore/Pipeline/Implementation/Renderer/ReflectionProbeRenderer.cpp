#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderer/ReflectionProbeRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectionProbeRenderer, 1, xiiRTTIDefaultAllocator<xiiReflectionProbeRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiReflectionProbeRenderer::xiiReflectionProbeRenderer() = default;

xiiReflectionProbeRenderer::~xiiReflectionProbeRenderer() = default;

void xiiReflectionProbeRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiReflectionProbeRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
