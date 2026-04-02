#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 64 — Readbacks and Telemetry.
///
/// Copy queue. Issues asynchronous GPU readbacks of selected debug targets and
/// per-pass performance counters — NEVER blocks the frame. Results are consumed
/// CPU-side on a ring-buffer two or three frames later. Writes GPU frame timing
/// into the m_pFrameTimingBuffer consumed by DynamicResolutionPass next frame.
class XII_GRAPHICSCORE_DLL xiiReadbackAndTelemetryPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReadbackAndTelemetryPass, xiiRenderPipelinePass);

public:
  xiiReadbackAndTelemetryPass();
  virtual ~xiiReadbackAndTelemetryPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

private:
  static constexpr xiiUInt32 k_uiRingSize = 3u;
  xiiSharedPtr<xiiGALBuffer> m_pReadbackBuffers[k_uiRingSize]; ///< CPU-visible readback ring.
  xiiUInt32                  m_uiCurrentSlot = 0u;
};
