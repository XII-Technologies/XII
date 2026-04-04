#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 50 â€” Opaque Final Composite.
///
/// Graphics/Compute. Central fusion point before transparents.
/// Combines: direct lighting + indirect lighting + emissive + volumetric scattering + sky.
/// Output is the authoritative HDR scene color buffer.
struct XII_GRAPHICSCORE_DLL xiiOpaqueCompositePass
{
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "OpaqueCompositePass";
  bool      m_bActive = true;
};

void xiiPopulateOpaqueCompositePass(xiiOpaqueCompositePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

