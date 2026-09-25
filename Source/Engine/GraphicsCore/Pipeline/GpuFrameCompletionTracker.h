/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>

class xiiGALCommandQueue;
class xiiGALDevice;

/// Reflection-friendly state exposed to diagnostics and frame-resource tooling.
struct XII_GRAPHICSCORE_DLL xiiGpuFrameCompletionStats
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64 m_uiLastCapturedFrame  = 0ULL;
  xiiUInt64 m_uiLastCompletedFrame = 0ULL;
  xiiUInt32 m_uiPendingFrameCount  = 0U;
  xiiUInt32 m_uiTrackedQueueCount  = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuFrameCompletionStats);

/// Converts per-queue GAL fence progress into a conservative completed-frame index.
///
/// CaptureSubmittedFrame() is called after every queue submission belonging to a frame.
/// PollCompletedFrames() advances only when every unique queue used by that frame has
/// completed the value captured for it. The result is safe for deferred destruction,
/// bindless index recycling, streaming eviction, and frame-ring reuse.
class XII_GRAPHICSCORE_DLL xiiGpuFrameCompletionTracker
{
public:
  void Initialize(xiiGALDevice* pDevice);
  void Reset();

  /// Snapshots the latest submitted value on graphics, compute, and transfer queues.
  /// Queue fallbacks that resolve to the same physical GAL queue are recorded once.
  void CaptureSubmittedFrame(xiiUInt64 uiFrameIndex);

  /// Polls queue fences without blocking and returns the last fully completed frame.
  [[nodiscard]] xiiUInt64 PollCompletedFrames();

  /// Blocks until every queue submission captured for the requested frame has completed.
  /// Use this before reusing a frame-ring slot. Frames must have been captured sequentially.
  void WaitForFrame(xiiUInt64 uiFrameIndex);

  [[nodiscard]] xiiUInt64                         GetLastCompletedFrame() const { return m_Stats.m_uiLastCompletedFrame; }
  [[nodiscard]] const xiiGpuFrameCompletionStats& GetStats() const { return m_Stats; }

private:
  struct QueuePoint
  {
    xiiGALCommandQueue* m_pQueue           = nullptr;
    xiiUInt64           m_uiSubmittedValue = 0ULL;
  };

  struct FramePoint
  {
    xiiUInt64                      m_uiFrameIndex = 0ULL;
    xiiHybridArray<QueuePoint, 3U> m_QueuePoints;
  };

  xiiGALDevice*              m_pDevice = nullptr;
  xiiDeque<FramePoint>       m_PendingFrames;
  xiiGpuFrameCompletionStats m_Stats;
};
