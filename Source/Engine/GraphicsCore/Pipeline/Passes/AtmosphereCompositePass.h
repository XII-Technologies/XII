#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 49 â€” Sky and Atmosphere Composite.
///
/// Graphics/Compute. Blends sky radiance using pre-built atmosphere LUTs into
/// skybox pixels identified via the depth prepass (depth == far plane).
/// Applied before transparents so transparent objects can receive sky ambient.
struct XII_GRAPHICSCORE_DLL xiiAtmosphereCompositePass
{
  float m_fSunSolidAngle = 6.8e-5f; ///< Sun disc solid angle (radians^2). Controls sun disc size.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "AtmosphereCompositePass";
  bool      m_bActive = true;
};

void xiiPopulateAtmosphereCompositePass(xiiAtmosphereCompositePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

