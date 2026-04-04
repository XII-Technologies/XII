#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 8 — Occluder Depth Prepass.
///
/// Graphics queue. Renders a very cheap, occluder-only draw list into a small depth buffer
/// (typically half or quarter resolution) used to build the Hi-Z pyramid. Geometry is
/// simplified (no alpha test, no tessellation). Uses indirect draws from a pre-built
/// occluder draw list written by CPU.
class XII_GRAPHICSCORE_DLL xiiOccluderDepthPrepass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOccluderDepthPrepass, xiiRenderPipelinePass);

public:
  xiiOccluderDepthPrepass();
  virtual ~xiiOccluderDepthPrepass();

  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  /// \brief Resolution divisor applied to the render target. Default 2 = half resolution.
  xiiUInt32 m_uiResolutionDivisor = 2u;
};
