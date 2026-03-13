#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiDecalRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiDecalRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiDecalRenderer);

public:
  xiiDecalRenderer();
  ~xiiDecalRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
