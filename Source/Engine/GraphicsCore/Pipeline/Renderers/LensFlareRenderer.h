#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiLensFlareRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiLensFlareRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLensFlareRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiLensFlareRenderer);

public:
  xiiLensFlareRenderer();
  ~xiiLensFlareRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
