#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiSkyRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiSkyRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkyRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiSkyRenderer);

public:
  xiiSkyRenderer();
  ~xiiSkyRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
