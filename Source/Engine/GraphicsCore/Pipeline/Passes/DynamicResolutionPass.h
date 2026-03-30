#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiDynamicResolutionPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicResolutionPass, xiiRenderPipelinePass);

public:
  xiiDynamicResolutionPass();
  ~xiiDynamicResolutionPass();

  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

private:
  float m_fCurrentScale = 1.0f;
};
