#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Passes 18-19 â€” Local Light Shadow Atlas.
///
/// CPU/Graphics (pass 18): deterministic atlas allocation for spot/point shadows.
/// Graphics (pass 19): renders local shadow casters into allocated atlas pages,
/// batched by material and shadow mode. Both passes are combined here since the
/// allocation is a prerequisite of the rendering.
struct XII_GRAPHICSCORE_DLL xiiLocalLightShadowPass
{
  xiiUInt32 m_uiAtlasSize     = 4096u; ///< Local shadow atlas resolution.
  xiiUInt32 m_uiMaxShadowedLights = 64u;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "LocalLightShadowPass";
  bool      m_bActive = true;
};

void xiiPopulateLocalLightShadowPass(xiiLocalLightShadowPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

