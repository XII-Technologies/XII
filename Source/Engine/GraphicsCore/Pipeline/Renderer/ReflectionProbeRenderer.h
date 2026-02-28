#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiReflectionProbeRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiReflectionProbeRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReflectionProbeRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiReflectionProbeRenderer);

public:
  xiiReflectionProbeRenderer();
  ~xiiReflectionProbeRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
