#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiSpriteRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiSpriteRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpriteRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiSpriteRenderer);

public:
  xiiSpriteRenderer();
  ~xiiSpriteRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
