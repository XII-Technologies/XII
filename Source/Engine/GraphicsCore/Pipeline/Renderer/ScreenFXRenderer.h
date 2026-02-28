#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiScreenFXRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiScreenFXRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenFXRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiScreenFXRenderer);

public:
  xiiScreenFXRenderer();
  ~xiiScreenFXRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
