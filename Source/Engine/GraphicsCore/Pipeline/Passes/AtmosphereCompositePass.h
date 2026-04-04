#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 49 — Sky and Atmosphere Composite.
///
/// Graphics/Compute. Blends sky radiance using pre-built atmosphere LUTs into
/// skybox pixels identified via the depth prepass (depth == far plane).
/// Applied before transparents so transparent objects can receive sky ambient.
class XII_GRAPHICSCORE_DLL xiiAtmosphereCompositePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereCompositePass, xiiRenderPipelinePass);

public:
  xiiAtmosphereCompositePass();
  virtual ~xiiAtmosphereCompositePass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  float m_fSunSolidAngle = 6.8e-5f; ///< Sun disc solid angle (radians^2). Controls sun disc size.
};
