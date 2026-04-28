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
    m_FrameRing[i].m_pFrameDurationQuery.Clear();
    m_FrameRing[i].m_uiFrameIndex = xiiInvalidIndex;
  }
  m_pDevice = nullptr;

  XII_LOCK(m_ResultMutex);
  m_ResolvedDurationsMs.Clear();
}

void xiiRenderGraphTimestampProfiler::OnPassBegin(xiiGALCommandList& commandList, xiiStringView sPassName, xiiUInt32 uiPassIndex)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "Profiler not initialized.");

  FrameData& frame = m_FrameRing[m_uiCurrentRingSlot];

  if (uiPassIndex >= frame.m_PassQueries.GetCount())
  {
    frame.m_PassQueries.SetCount(uiPassIndex + 1U);
  }

  // If this is the first pass recorded for the frame, create & begin the frame-wide duration query.
  if (uiPassIndex == 0U)
  {
    if (frame.m_pFrameDurationQuery == nullptr)
    {
      xiiGALQueryCreationDescription frameQueryDescription;
      frameQueryDescription.m_Type = xiiGALQueryType::Duration;

      frame.m_pFrameDurationQuery = m_pDevice->CreateQuery(frameQueryDescription);
    }

    if (frame.m_pFrameDurationQuery != nullptr)
    {
      commandList.BeginQuery(frame.m_pFrameDurationQuery);
    }
  }

  PassQueries& pass = frame.m_PassQueries[uiPassIndex];
  pass.m_bActive    = true;
  pass.m_sPassName.Assign(sPassName);

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

  // If this is the last pass for the frame (based on current known count), end the frame-wide query.
  // Note: This assumes passes are recorded in order and that the final pass index equals GetCount() - 1.
  if (uiPassIndex == frame.m_PassQueries.GetCount() - 1)
  {
    if (frame.m_pFrameDurationQuery != nullptr)
    {
      commandList.EndQuery(frame.m_pFrameDurationQuery);
    }
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

  // Read back the frame-wide duration query if present.
  if (frameData.m_pFrameDurationQuery != nullptr)
  {
    xiiGALQueryDataDuration frameDurationData;
    if (frameData.m_pFrameDurationQuery->GetData(&frameDurationData, sizeof(frameDurationData), /*bAutoInvalidate=*/false))
    {
      if (frameDurationData.m_uiFrequency > 0ULL)
      {
        const float fFrameDurationMs = static_cast<float>(frameDurationData.m_uiDuration) / static_cast<float>(frameDurationData.m_uiFrequency) * 1000.0f;

        // Use a reserved name for the total frame duration so callers can query it like a pass.
        static const xiiHashedString s_sFrameTotalName = xiiMakeHashedString("__FrameTotal__");
        m_ResolvedDurationsMs.Insert(s_sFrameTotalName, fFrameDurationMs);
      }
    }

    // Reset the frame query so it will be recreated on the next frame.
    frameData.m_pFrameDurationQuery = nullptr;
  }

  // Clear pass queries array for this frame so it can be reused.
  frameData.m_PassQueries.Clear();
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
