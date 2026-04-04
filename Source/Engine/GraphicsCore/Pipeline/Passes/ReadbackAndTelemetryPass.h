#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 64 â€” Readbacks and Telemetry.
///
/// Copy queue. Issues asynchronous GPU readbacks of selected debug targets and
/// per-pass performance counters â€” NEVER blocks the frame. Results are consumed
/// CPU-side on a ring-buffer two or three frames later. Writes GPU frame timing
/// into the m_pFrameTimingBuffer consumed by DynamicResolutionPass next frame.
struct XII_GRAPHICSCORE_DLL xiiReadbackAndTelemetryPass
{
  static constexpr xiiUInt32 k_uiRingSize = 3u;
  xiiSharedPtr<xiiGALBuffer> m_pReadbackBuffers[k_uiRingSize]; ///< CPU-visible readback ring.
  xiiUInt32                  m_uiCurrentSlot = 0u;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "ReadbackAndTelemetryPass";
  bool      m_bActive = true;
};

void xiiPopulateReadbackAndTelemetryPass(xiiReadbackAndTelemetryPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

