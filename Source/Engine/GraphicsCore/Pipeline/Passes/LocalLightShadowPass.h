#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Passes 18-19 — Local Light Shadow Atlas.
///
/// CPU/Graphics (pass 18): deterministic atlas allocation for spot/point shadows.
/// Graphics (pass 19): renders local shadow casters into allocated atlas pages,
/// batched by material and shadow mode. Both passes are combined here since the
/// allocation is a prerequisite of the rendering.
class XII_GRAPHICSCORE_DLL xiiLocalLightShadowPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLocalLightShadowPass, xiiRenderPipelinePass);

public:
  xiiLocalLightShadowPass();
  virtual ~xiiLocalLightShadowPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  xiiUInt32 m_uiAtlasSize     = 4096u; ///< Local shadow atlas resolution.
  xiiUInt32 m_uiMaxShadowedLights = 64u;
};
