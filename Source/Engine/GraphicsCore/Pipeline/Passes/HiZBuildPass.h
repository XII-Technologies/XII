#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief Pass 9 â€” Hi-Z Pyramid Build.
///
/// Async Compute. Reads the occluder depth buffer and builds the full mip-chain depth pyramid
/// via repeated 2x2 max-reduce dispatches. Each mip level is dispatched separately to avoid
/// UAV/SRV hazards. The pyramid is the central resource for Hi-Z occlusion culling and SSR.
struct XII_GRAPHICSCORE_DLL xiiHiZBuildPass
{
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "HiZBuildPass";
  bool      m_bActive = true;
};

void xiiPopulateHiZBuildPass(xiiHiZBuildPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

