#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass for Frame Setup and GPU Markers. Must be the first pass in every frame.
///
/// Responsibilities are:
/// - Push top-of-frame GPU debug group so all subsequent passes appear nested in profiling tools.
/// - Begin a GPU timestamp query covering the entire frame.
/// - Write xiiPreviousFrameStats (retrieved from last frame's readback) and xiiFrameTimestampRange to the per-view blackboard so downstream passes can budget their work.
/// - Manages a ring of GPU timestamp query buffers for CPU readback with a 2-frame delay to get accurate GPU timing data for the previous frame.
/// - Note that the timestamp query is started in this pass, but ended in the last pass of the frame (e.g. PresentPass) to ensure it covers the entire frame.
class XII_GRAPHICSCORE_DLL xiiFrameSetupPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFrameSetupPass, xiiRenderPipelinePass);

public:
  xiiFrameSetupPass();
  virtual ~xiiFrameSetupPass();

  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

private:
  static constexpr xiiUInt32 s_uiTimestampRingSize = 3U;                 ///< The number of timestamp query buffers in the ring. Must be at least 3 to cover the 2-frame readback delay and ensure a free slot for the current frame.
  xiiSharedPtr<xiiGALBuffer> m_pTimestampBuffers[s_uiTimestampRingSize]; ///< Ring of GPU timestamp query buffers for CPU readback with 2-frame delay.
  xiiUInt32                  m_uiCurrentTimestampSlot = 0U;              ///< Index of the current timestamp buffer slot in the ring.
};
