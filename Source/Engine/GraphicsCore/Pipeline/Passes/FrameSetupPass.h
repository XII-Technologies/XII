#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiFrameSetupPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFrameSetupPass, xiiRenderPipelinePass);

public:
  xiiFrameSetupPass();
  ~xiiFrameKickoffPass();

  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph) override;
};
