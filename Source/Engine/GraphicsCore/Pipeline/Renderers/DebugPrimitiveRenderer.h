#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiDebugPrimitiveRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiDebugPrimitiveRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDebugPrimitiveRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiDebugPrimitiveRenderer);

public:
  xiiDebugPrimitiveRenderer();
  ~xiiDebugPrimitiveRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
