#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Passes 42-44 — RT Global Illumination (Final Gather / Probe Trace, Temporal, Denoise).
///
/// Async Compute. Typically the highest-cost RT stage. Traces one or more indirect
/// diffuse rays per pixel using the TLAS and evaluates irradiance at hit points.
/// Temporal reservoir (RESTIR-style) accumulation provides stability.
/// Denoise splits diffuse and specular indirect branches.
/// Requires device RT support — no-ops if unavailable.
class XII_GRAPHICSCORE_DLL xiiRTGlobalIlluminationPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRTGlobalIlluminationPass, xiiRenderPipelinePass);

public:
  xiiRTGlobalIlluminationPass();
  virtual ~xiiRTGlobalIlluminationPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  xiiUInt32 m_uiRaysPerPixel           = 1u;
  xiiUInt32 m_uiReservoirCandidateCount = 8u;  ///< Reservoir initial candidate set size.
  xiiUInt32 m_uiMaxBounces             = 1u;   ///< Indirect bounce depth (1 = single-bounce GI).
  bool      m_bEnableSpecularBranch    = true;  ///< Include specular indirect in denoise split.
};
