#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 7 â€” Coarse Frustum Culling.
///
/// Async Compute. Tests per-instance AABBs against the six camera frustum planes.
/// This is the first reduction pass before any depth tests. Outputs a compact list
/// of candidate instances that survive frustum rejection.
struct XII_GRAPHICSCORE_DLL xiiCoarseFrustumCullPass
{
  xiiSharedPtr<xiiGALBuffer> m_pVisibleCandidateBuffer; ///< RW: surviving instance indices (uint per instance).
  xiiSharedPtr<xiiGALBuffer> m_pFrustumPlanesBuffer;    ///< CBV: 6 float4 frustum planes, written CPU-side.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "CoarseFrustumCullPass";
  bool      m_bActive = true;
};

void xiiPopulateCoarseFrustumCullPass(xiiCoarseFrustumCullPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

