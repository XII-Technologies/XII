#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 8 â€” Occluder Depth Prepass.
///
/// Graphics queue. Renders a very cheap, occluder-only draw list into a small depth buffer
/// (typically half or quarter resolution) used to build the Hi-Z pyramid. Geometry is
/// simplified (no alpha test, no tessellation). Uses indirect draws from a pre-built
/// occluder draw list written by CPU.
struct XII_GRAPHICSCORE_DLL xiiOccluderDepthPrepass
{
  /// \brief Resolution divisor applied to the render target. Default 2 = half resolution.
  xiiUInt32 m_uiResolutionDivisor = 2u;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "OccluderDepthPrepass";
  bool      m_bActive = true;
};

void xiiPopulateOccluderDepthPrepass(xiiOccluderDepthPrepass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

