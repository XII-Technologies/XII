#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Passes 42-44 â€” RT Global Illumination (Final Gather / Probe Trace, Temporal, Denoise).
///
/// Async Compute. Typically the highest-cost RT stage. Traces one or more indirect
/// diffuse rays per pixel using the TLAS and evaluates irradiance at hit points.
/// Temporal reservoir (RESTIR-style) accumulation provides stability.
/// Denoise splits diffuse and specular indirect branches.
/// Requires device RT support â€” no-ops if unavailable.
struct XII_GRAPHICSCORE_DLL xiiRTGlobalIlluminationPass
{
  xiiUInt32 m_uiRaysPerPixel           = 1u;
  xiiUInt32 m_uiReservoirCandidateCount = 8u;  ///< Reservoir initial candidate set size.
  xiiUInt32 m_uiMaxBounces             = 1u;   ///< Indirect bounce depth (1 = single-bounce GI).
  bool      m_bEnableSpecularBranch    = true;  ///< Include specular indirect in denoise split.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "RTGlobalIlluminationPass";
  bool      m_bActive = true;
};

void xiiPopulateRTGlobalIlluminationPass(xiiRTGlobalIlluminationPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

