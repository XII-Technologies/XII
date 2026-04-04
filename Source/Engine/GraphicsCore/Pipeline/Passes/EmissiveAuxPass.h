#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 28 â€” Emissive and Material Aux Pass.
///
/// Graphics queue. Separate emissive and special-material terms into a dedicated
/// R16G16B16A16F target. Keeping this separate improves culling (many passes have
/// no emissive contribution) and allows independent blending strategies.
struct XII_GRAPHICSCORE_DLL xiiEmissiveAuxPass
{
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "EmissiveAuxPass";
  bool      m_bActive = true;
};

void xiiPopulateEmissiveAuxPass(xiiEmissiveAuxPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

