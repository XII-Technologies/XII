#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 51 â€” Transparent and Refractive Pass.
///
/// Graphics queue. Sorts transparent objects and composites them over the opaque HDR
/// using depth testing and additive / alpha blending. Refractive objects sample the
/// opaque HDR color buffer. OIT policy (weighted blended / linked-list) is configurable.
struct XII_GRAPHICSCORE_DLL xiiTransparentPass
{
  bool m_bUseWeightedBlendedOIT = true; ///< false = depth-sorted back-to-front blending.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "TransparentPass";
  bool      m_bActive = true;
};

void xiiPopulateTransparentPass(xiiTransparentPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

