#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 2 â€” Dynamic Resolution Decision.
///
/// Reads GPU frame time history from the blackboard (written by xiiFrameSetupPass via the timestamp readback)
/// and dispatches a single-thread compute shader to compute the render scale for this frame. The resulting
/// scale is published back to the blackboard as "DynamicResolutionScale" (float). Downstream passes that
/// size transient render targets must read this value during their setup callbacks.
struct XII_GRAPHICSCORE_DLL xiiDynamicResolutionPass
{
  /// \brief Minimum allowed render scale. Default 0.5 (50 % native).
  float m_fMinScale = 0.50f;
  /// \brief Maximum allowed render scale. Default 1.0 (100 % native).
  float m_fMaxScale = 1.00f;
  /// \brief Target GPU frame time in milliseconds. Typically 1000/targetFPS.
  float m_fTargetFrameTimeMs = 16.667f; // ~60 Hz
  xiiSharedPtr<xiiGALBuffer> m_pFrameTimingBuffer;   ///< Readback of previous GPU frame duration (float4).
  xiiSharedPtr<xiiGALBuffer> m_pCameraVelocityBuffer; ///< CPU-written camera angular/linear velocity (float4).
  xiiSharedPtr<xiiGALBuffer> m_pResolutionOutputBuffer; ///< RW buffer: x=scale, y=measuredMs, z=targetMs.
  xiiSharedPtr<xiiGALBuffer> m_pFrameConstantBuffer; ///< Per-frame constants for the compute shader.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "DynamicResolutionPass";
  bool      m_bActive = true;
};

void xiiPopulateDynamicResolutionPass(xiiDynamicResolutionPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

