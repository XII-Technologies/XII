#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiClothRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiClothRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClothRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiClothRenderer);

public:
  xiiClothRenderer();
  ~xiiClothRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
