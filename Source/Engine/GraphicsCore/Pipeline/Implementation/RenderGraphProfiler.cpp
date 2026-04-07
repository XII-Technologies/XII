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

  m_pDevice = std::move(pDevice);
}

void xiiRenderGraphTimestampProfiler::Shutdown()
{
  for (xiiUInt32 i = 0U; i < s_uiRingFrameCount; ++i)
  {
    m_FrameRing[i].m_PassQueries.Clear();
    m_FrameRing[i].m_uiFrameIndex = xiiInvalidIndex;
  }
  m_pDevice = nullptr;
}

void xiiRenderGraphTimestampProfiler::OnPassBegin(xiiGALCommandList& commandList, xiiHashedString sPassName, xiiUInt32 uiPassIndex)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "Profiler not initialized.");

  FrameData& frame = m_FrameRing[m_uiCurrentRingSlot];

  if (uiPassIndex >= frame.m_PassQueries.GetCount())
  {
    frame.m_PassQueries.SetCount(uiPassIndex + 1U);
  }

  PassQueries& pass = frame.m_PassQueries[uiPassIndex];
  pass.m_sPassName  = sPassName;
  pass.m_bActive    = true;

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

void xiiRenderGraphTimestampProfiler::OnPassEnd(xiiGALCommandList& commandList, xiiHashedString sPassName, xiiUInt32 uiPassIndex)
{
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

  // Attempt readback on the oldest slot (2-frame delay minimum before read)
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
}

float xiiRenderGraphTimestampProfiler::GetPassDurationMs(xiiHashedString sPassName) const
{
  XII_LOCK(m_ResultMutex);

  float fResult = 0.0f;
  m_ResolvedDurationsMs.TryGetValue(sPassName, fResult);

  return fResult;
}
