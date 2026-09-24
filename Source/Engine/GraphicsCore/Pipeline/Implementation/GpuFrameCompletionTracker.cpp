/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/GpuFrameCompletionTracker.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuFrameCompletionStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuFrameCompletionStats>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LastCapturedFrame", m_uiLastCapturedFrame),
    XII_MEMBER_PROPERTY("LastCompletedFrame", m_uiLastCompletedFrame),
    XII_MEMBER_PROPERTY("PendingFrameCount", m_uiPendingFrameCount),
    XII_MEMBER_PROPERTY("TrackedQueueCount", m_uiTrackedQueueCount),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

void xiiGpuFrameCompletionTracker::Initialize(xiiGALDevice* pDevice)
{
  XII_ASSERT_DEV(pDevice != nullptr, "GPU frame completion tracking requires a device.");

  Reset();
  m_pDevice = pDevice;
}

void xiiGpuFrameCompletionTracker::Reset()
{
  m_PendingFrames.Clear();
  m_Stats = {};
  m_pDevice = nullptr;
}

void xiiGpuFrameCompletionTracker::CaptureSubmittedFrame(xiiUInt64 uiFrameIndex)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "GPU frame completion tracker is not initialized.");
  XII_ASSERT_DEV(uiFrameIndex > m_Stats.m_uiLastCapturedFrame, "Frames must be captured in increasing order.");

  FramePoint& frame = m_PendingFrames.ExpandAndGetRef();
  frame.m_uiFrameIndex = uiFrameIndex;

  constexpr xiiGALCommandQueueFlags::Enum queueTypes[] = {
    xiiGALCommandQueueFlags::Graphics,
    xiiGALCommandQueueFlags::Compute,
    xiiGALCommandQueueFlags::Transfer,
  };

  for (xiiGALCommandQueueFlags::Enum queueType : queueTypes)
  {
    xiiGALCommandQueue* pQueue = m_pDevice->GetCommandQueue(queueType);
    if (pQueue == nullptr)
      continue;

    bool bAlreadyTracked = false;
    for (const QueuePoint& queuePoint : frame.m_QueuePoints)
    {
      if (queuePoint.m_pQueue == pQueue)
      {
        bAlreadyTracked = true;
        break;
      }
    }
    if (bAlreadyTracked)
      continue;

    QueuePoint& queuePoint = frame.m_QueuePoints.ExpandAndGetRef();
    queuePoint.m_pQueue = pQueue;
    const xiiUInt64 uiNextFenceValue = pQueue->GetNextFenceValue();
    queuePoint.m_uiSubmittedValue = uiNextFenceValue > 0ULL ? uiNextFenceValue - 1ULL : 0ULL;
  }

  m_Stats.m_uiLastCapturedFrame = uiFrameIndex;
  m_Stats.m_uiPendingFrameCount = m_PendingFrames.GetCount();
  m_Stats.m_uiTrackedQueueCount = frame.m_QueuePoints.GetCount();
}

xiiUInt64 xiiGpuFrameCompletionTracker::PollCompletedFrames()
{
  while (!m_PendingFrames.IsEmpty())
  {
    const FramePoint& frame = m_PendingFrames.PeekFront();
    bool bComplete = true;
    for (const QueuePoint& queuePoint : frame.m_QueuePoints)
    {
      if (queuePoint.m_pQueue->GetCompletedFenceValue() < queuePoint.m_uiSubmittedValue)
      {
        bComplete = false;
        break;
      }
    }

    if (!bComplete)
      break;

    m_Stats.m_uiLastCompletedFrame = frame.m_uiFrameIndex;
    m_PendingFrames.PopFront();
  }

  m_Stats.m_uiPendingFrameCount = m_PendingFrames.GetCount();
  return m_Stats.m_uiLastCompletedFrame;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_GpuFrameCompletionTracker);

