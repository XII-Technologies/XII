#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 13 â€” Motion Vector Pass.
///
/// Graphics queue. Renders per-pixel screen-space velocity from current and
/// previous-frame transforms. Mandatory for TAA, temporal reprojection, and
/// most denoising passes.
struct XII_GRAPHICSCORE_DLL xiiMotionVectorPass
{
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "MotionVectorPass";
  bool      m_bActive = true;
};

void xiiPopulateMotionVectorPass(xiiMotionVectorPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

