#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiAtmosphereRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiAtmosphereRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiAtmosphereRenderer);

public:
  xiiAtmosphereRenderer();
  ~xiiAtmosphereRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
