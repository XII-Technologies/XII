#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiLightRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiLightRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLightRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiLightRenderer);

public:
  xiiLightRenderer();
  ~xiiLightRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
