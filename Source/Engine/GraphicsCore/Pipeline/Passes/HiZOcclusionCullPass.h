#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 10 â€” Hi-Z Occlusion Culling.
///
/// Async Compute. Tests candidate instances (from coarse frustum cull) against the Hi-Z pyramid.
/// Culls instances whose projected bounds are fully occluded. Large performance win at scene scale.
struct XII_GRAPHICSCORE_DLL xiiHiZOcclusionCullPass
{
  xiiSharedPtr<xiiGALBuffer> m_pSurvivingInstanceBuffer; ///< RW: instances that passed Hi-Z test.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "HiZOcclusionCullPass";
  bool      m_bActive = true;
};

void xiiPopulateHiZOcclusionCullPass(xiiHiZOcclusionCullPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

