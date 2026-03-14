#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderers/DebugPrimitiveRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDebugPrimitiveRenderer, 1, xiiRTTIDefaultAllocator<xiiDebugPrimitiveRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDebugPrimitiveRenderer::xiiDebugPrimitiveRenderer() = default;

xiiDebugPrimitiveRenderer::~xiiDebugPrimitiveRenderer() = default;

void xiiDebugPrimitiveRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiDebugPrimitiveRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
