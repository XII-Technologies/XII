/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <GraphicsFoundation/Device/Device.h>

xiiRenderGraphTimestampProfiler::xiiRenderGraphTimestampProfiler() = default;

xiiRenderGraphTimestampProfiler::~xiiRenderGraphTimestampProfiler()
{
  Shutdown();
}

void xiiRenderGraphTimestampProfiler::Initialize(xiiSharedPtr<xiiGALDevice> pDevice)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Device must not be null.");

  m_pDevice = pDevice;
}

void xiiRenderGraphTimestampProfiler::Shutdown()
{
  for (xiiUInt32 i = 0U; i < s_uiRingFrameCount; ++i)
  {
    m_FrameRing[i].m_PassQueries.Clear();
    m_FrameRing[i].m_SubmissionDurationQueries.Clear();
    m_FrameRing[i].m_SubmissionQueryActive.Clear();
    m_FrameRing[i].m_SubmissionGraphIds.Clear();
    m_FrameRing[i].m_SubmissionQueueIndices.Clear();
    m_FrameRing[i].m_uiFrameIndex = xiiInvalidIndex;
  }
  m_pDevice = nullptr;

  XII_LOCK(m_ResultMutex);
  m_ResolvedDurationsMs.Clear();
  m_ResolvedGraphDurationsMs.Clear();
  m_ResolvedQueueDurationsMs.Clear();
}

bool xiiRenderGraphTimestampProfiler::SupportsDurationQueries(const xiiGALCommandList& commandList) const
{
  if (m_pDevice == nullptr || m_pDevice->GetFeatures().m_DurationQueries != xiiGALDeviceFeatureState::Enabled)
    return false;

  const xiiBitflags<xiiGALCommandQueueFlags> queueFlags = commandList.GetDescription().m_QueueFlags;
  const bool bTransferOnly = queueFlags.AreAllSet(xiiGALCommandQueueFlags::Transfer) && !queueFlags.AreAllSet(xiiGALCommandQueueFlags::Compute);
  return !bTransferOnly || m_pDevice->GetFeatures().m_TransferQueueTimestampQueries == xiiGALDeviceFeatureState::Enabled;
}

void xiiRenderGraphTimestampProfiler::OnGraphBegin(xiiGALCommandList& commandList, xiiUInt64 uiGraphId, xiiUInt32 uiSubmissionIndex, xiiUInt32 uiQueueIndex)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "Profiler not initialized.");

  FrameData& frame = m_FrameRing[m_uiCurrentRingSlot];

  if (uiSubmissionIndex >= frame.m_SubmissionDurationQueries.GetCount())
  {
    frame.m_SubmissionDurationQueries.SetCount(uiSubmissionIndex + 1U);
    frame.m_SubmissionQueryActive.SetCount(uiSubmissionIndex + 1U, false);
    frame.m_SubmissionGraphIds.SetCount(uiSubmissionIndex + 1U, 0ULL);
    frame.m_SubmissionQueueIndices.SetCount(uiSubmissionIndex + 1U, 0U);
  }
  frame.m_SubmissionGraphIds[uiSubmissionIndex]     = uiGraphId;
  frame.m_SubmissionQueueIndices[uiSubmissionIndex] = uiQueueIndex;

  if (!SupportsDurationQueries(commandList))
  {
    frame.m_SubmissionQueryActive[uiSubmissionIndex] = false;
    return;
  }

  xiiSharedPtr<xiiGALQuery>& pSubmissionQuery = frame.m_SubmissionDurationQueries[uiSubmissionIndex];
  if (pSubmissionQuery == nullptr)
  {
    xiiGALQueryCreationDescription queryDescription;
    queryDescription.m_Type = xiiGALQueryType::Duration;

    pSubmissionQuery = m_pDevice->CreateQuery(queryDescription);
  }

  if (pSubmissionQuery != nullptr)
  {
    commandList.BeginQuery(pSubmissionQuery);

    frame.m_SubmissionQueryActive[uiSubmissionIndex] = true;
  }
}

void xiiRenderGraphTimestampProfiler::OnGraphEnd(xiiGALCommandList& commandList, xiiUInt64 uiGraphId, xiiUInt32 uiSubmissionIndex, xiiUInt32 uiQueueIndex)
{
  XII_IGNORE_UNUSED(uiGraphId);
  XII_IGNORE_UNUSED(uiQueueIndex);
  FrameData& frame = m_FrameRing[m_uiCurrentRingSlot];

  if (uiSubmissionIndex >= frame.m_SubmissionDurationQueries.GetCount())
    return;

  if (!frame.m_SubmissionQueryActive[uiSubmissionIndex])
    return;

  xiiSharedPtr<xiiGALQuery>& pSubmissionQuery = frame.m_SubmissionDurationQueries[uiSubmissionIndex];
  if (pSubmissionQuery != nullptr)
  {
    commandList.EndQuery(pSubmissionQuery);
  }
}

void xiiRenderGraphTimestampProfiler::OnPassBegin(xiiGALCommandList& commandList, xiiStringView sPassName, xiiUInt32 uiPassIndex)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "Profiler not initialized.");

  FrameData& frame = m_FrameRing[m_uiCurrentRingSlot];

  if (uiPassIndex >= frame.m_PassQueries.GetCount())
  {
    frame.m_PassQueries.SetCount(uiPassIndex + 1U);
  }

  PassQueries& pass = frame.m_PassQueries[uiPassIndex];
  pass.m_bActive    = SupportsDurationQueries(commandList);
  pass.m_sPassName.Assign(sPassName);

  if (!pass.m_bActive)
    return;

  if (pass.m_pDurationQuery == nullptr)
  {
    xiiGALQueryCreationDescription queryDescription;
    queryDescription.m_Type = xiiGALQueryType::Duration;

    pass.m_pDurationQuery = m_pDevice->CreateQuery(queryDescription);
  }

  if (pass.m_pDurationQuery != nullptr)
  {
    commandList.BeginQuery(pass.m_pDurationQuery);
  }
}

void xiiRenderGraphTimestampProfiler::OnPassEnd(xiiGALCommandList& commandList, xiiStringView sPassName, xiiUInt32 uiPassIndex)
{
  XII_IGNORE_UNUSED(sPassName);

  FrameData& frame = m_FrameRing[m_uiCurrentRingSlot];

  if (uiPassIndex >= frame.m_PassQueries.GetCount())
    return;

  PassQueries& pass = frame.m_PassQueries[uiPassIndex];
  if (pass.m_pDurationQuery != nullptr && pass.m_bActive)
  {
    commandList.EndQuery(pass.m_pDurationQuery);
  }
}

void xiiRenderGraphTimestampProfiler::OnFrameEnd(xiiUInt64 uiFrameIndex)
{
  m_FrameRing[m_uiCurrentRingSlot].m_uiFrameIndex = uiFrameIndex;
  m_uiCurrentRingSlot                             = (m_uiCurrentRingSlot + 1U) % s_uiRingFrameCount;

  // Attempt readback on the oldest slot (2-frame delay minimum before read).
  FrameData& oldestFrame = m_FrameRing[m_uiCurrentRingSlot];
  if (oldestFrame.m_uiFrameIndex != xiiInvalidIndex)
  {
    ReadbackFrame(oldestFrame);
  }
}

void xiiRenderGraphTimestampProfiler::ReadbackFrame(FrameData& frameData)
{
  XII_LOCK(m_ResultMutex);

  float                          fFrameDurationMs = 0.0f;
  xiiHashTable<xiiUInt64, float> frameGraphDurations;
  xiiHashTable<xiiUInt64, float> frameQueueDurations;

  for (PassQueries& pass : frameData.m_PassQueries)
  {
    if (!pass.m_bActive || pass.m_pDurationQuery == nullptr)
      continue;

    xiiGALQueryDataDuration durationData;
    if (pass.m_pDurationQuery->GetData(&durationData, sizeof(durationData), /*bAutoInvalidate=*/false))
    {
      if (durationData.m_uiFrequency > 0ULL)
      {
        const float fDurationMs = static_cast<float>(durationData.m_uiDuration) / static_cast<float>(durationData.m_uiFrequency) * 1000.0f;

        m_ResolvedDurationsMs.Insert(pass.m_sPassName, fDurationMs);
      }
    }
    pass.m_bActive = false;
  }

  // Read back all submission duration queries for this frame and accumulate them.
  for (xiiUInt32 uiSubmissionIndex = 0U; uiSubmissionIndex < frameData.m_SubmissionDurationQueries.GetCount(); ++uiSubmissionIndex)
  {
    if (!frameData.m_SubmissionQueryActive[uiSubmissionIndex])
      continue;

    xiiSharedPtr<xiiGALQuery>& pSubmissionQuery = frameData.m_SubmissionDurationQueries[uiSubmissionIndex];
    if (pSubmissionQuery == nullptr)
      continue;

    xiiGALQueryDataDuration submissionDurationData;
    if (pSubmissionQuery->GetData(&submissionDurationData, sizeof(submissionDurationData), /*bAutoInvalidate=*/false))
    {
      if (submissionDurationData.m_uiFrequency > 0ULL)
      {
        fFrameDurationMs += static_cast<float>(submissionDurationData.m_uiDuration) / static_cast<float>(submissionDurationData.m_uiFrequency) * 1000.0f;
        const float     fSubmissionMs = static_cast<float>(submissionDurationData.m_uiDuration) / static_cast<float>(submissionDurationData.m_uiFrequency) * 1000.0f;
        const xiiUInt64 uiGraphId     = frameData.m_SubmissionGraphIds[uiSubmissionIndex];
        const xiiUInt64 uiQueueKey    = uiGraphId ^ (0x9E3779B97F4A7C15ULL + static_cast<xiiUInt64>(frameData.m_SubmissionQueueIndices[uiSubmissionIndex]));

        float fGraphMs = 0.0f;
        frameGraphDurations.TryGetValue(uiGraphId, fGraphMs);
        frameGraphDurations.Insert(uiGraphId, fGraphMs + fSubmissionMs);
        float fQueueMs = 0.0f;
        frameQueueDurations.TryGetValue(uiQueueKey, fQueueMs);
        frameQueueDurations.Insert(uiQueueKey, fQueueMs + fSubmissionMs);
      }
    }

    frameData.m_SubmissionQueryActive[uiSubmissionIndex] = false;
  }

  // Use a reserved name for the total frame duration so callers can query it like a pass.
  static const xiiHashedString s_sFrameTotalName = xiiMakeHashedString("__FrameTotal__");
  m_ResolvedDurationsMs.Insert(s_sFrameTotalName, fFrameDurationMs);
  for (auto it = frameGraphDurations.GetIterator(); it.IsValid(); ++it)
    m_ResolvedGraphDurationsMs.Insert(it.Key(), it.Value());
  for (auto it = frameQueueDurations.GetIterator(); it.IsValid(); ++it)
    m_ResolvedQueueDurationsMs.Insert(it.Key(), it.Value());

  // Clear per-frame arrays so the slot can be reused.
  frameData.m_PassQueries.Clear();
  frameData.m_SubmissionDurationQueries.Clear();
  frameData.m_SubmissionQueryActive.Clear();
  frameData.m_SubmissionGraphIds.Clear();
  frameData.m_SubmissionQueueIndices.Clear();
  frameData.m_uiFrameIndex = xiiInvalidIndex;
}

float xiiRenderGraphTimestampProfiler::GetPassDurationMs(xiiStringView sPassName) const
{
  XII_LOCK(m_ResultMutex);

  float fResult = 0.0f;
  m_ResolvedDurationsMs.TryGetValue(xiiTempHashedString(sPassName), fResult);

  return fResult;
}

float xiiRenderGraphTimestampProfiler::GetFrameDurationMs() const
{
  // Use the reserved name for the total frame duration.
  static const xiiHashedString s_sFrameTotalName = xiiMakeHashedString("__FrameTotal__");

  XII_LOCK(m_ResultMutex);

  float fResult = 0.0f;
  m_ResolvedDurationsMs.TryGetValue(s_sFrameTotalName, fResult);

  return fResult;
}

float xiiRenderGraphTimestampProfiler::GetGraphDurationMs(xiiUInt64 uiGraphId) const
{
  XII_LOCK(m_ResultMutex);
  float fResult = 0.0f;
  m_ResolvedGraphDurationsMs.TryGetValue(uiGraphId, fResult);
  return fResult;
}

float xiiRenderGraphTimestampProfiler::GetQueueDurationMs(xiiUInt64 uiGraphId, xiiUInt32 uiQueueIndex) const
{
  XII_LOCK(m_ResultMutex);
  const xiiUInt64 uiQueueKey = uiGraphId ^ (0x9E3779B97F4A7C15ULL + static_cast<xiiUInt64>(uiQueueIndex));
  float           fResult    = 0.0f;
  m_ResolvedQueueDurationsMs.TryGetValue(uiQueueKey, fResult);
  return fResult;
}
