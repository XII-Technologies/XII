#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiGridRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiGridRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGridRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiGridRenderer);

public:
  xiiGridRenderer();
  ~xiiGridRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
