#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiDynamicMeshRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiDynamicMeshRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicMeshRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiDynamicMeshRenderer);

public:
  xiiDynamicMeshRenderer();
  ~xiiDynamicMeshRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
