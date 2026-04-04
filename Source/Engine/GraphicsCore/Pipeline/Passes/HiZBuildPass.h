#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief Pass 9 — Hi-Z Pyramid Build.
///
/// Async Compute. Reads the occluder depth buffer and builds the full mip-chain depth pyramid
/// via repeated 2x2 max-reduce dispatches. Each mip level is dispatched separately to avoid
/// UAV/SRV hazards. The pyramid is the central resource for Hi-Z occlusion culling and SSR.
class XII_GRAPHICSCORE_DLL xiiHiZBuildPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHiZBuildPass, xiiRenderPipelinePass);

public:
  xiiHiZBuildPass();
  virtual ~xiiHiZBuildPass();

  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
};
