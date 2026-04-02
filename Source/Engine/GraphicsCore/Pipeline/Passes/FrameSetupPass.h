#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 1 — Frame Setup and GPU Markers.
///
/// Must be the first pass in every frame. Responsibilities:
/// - Push top-of-frame GPU debug group so all subsequent passes appear nested in profiling tools.
/// - Begin a GPU timestamp query covering the entire frame.
/// - Write xiiPreviousFrameStats (retrieved from last frame's readback) and xiiFrameTimestampRange
///   to the per-view blackboard so downstream passes can budget their work.
class XII_GRAPHICSCORE_DLL xiiFrameSetupPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFrameSetupPass, xiiRenderPipelinePass);

public:
  xiiFrameSetupPass();
  virtual ~xiiFrameSetupPass();

  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

private:
  // Ring of per-frame timestamp query buffers for CPU readback (2-frame delay).
  static constexpr xiiUInt32 k_uiTimestampRingSize = 3u;
  xiiSharedPtr<xiiGALBuffer> m_pTimestampBuffers[k_uiTimestampRingSize];
  xiiUInt32                  m_uiCurrentTimestampSlot = 0u;
};
