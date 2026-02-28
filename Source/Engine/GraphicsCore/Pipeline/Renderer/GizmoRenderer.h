#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiGizmoRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiGizmoRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGizmoRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiGizmoRenderer);

public:
  xiiGizmoRenderer();
  ~xiiGizmoRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
