#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderer/GizmoRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGizmoRenderer, 1, xiiRTTIDefaultAllocator<xiiGizmoRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGizmoRenderer::xiiGizmoRenderer() = default;

xiiGizmoRenderer::~xiiGizmoRenderer() = default;

void xiiGizmoRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiGizmoRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
