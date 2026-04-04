#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 27 â€” GBuffer Base Pass (Deferred Opaque).
///
/// Graphics queue. Main deferred shading payload generation via indirect draws.
/// Writes albedo, oct-encoded normals, material parameters, and depth using
/// the packed draw commands from DrawCommandBuildPass.
struct XII_GRAPHICSCORE_DLL xiiGBufferBasePass
{
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "GBufferBasePass";
  bool      m_bActive = true;
};

void xiiPopulateGBufferBasePass(xiiGBufferBasePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

