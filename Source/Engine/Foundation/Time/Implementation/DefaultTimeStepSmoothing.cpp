#include <Foundation/FoundationPCH.h>

#include <Foundation/Time/DefaultTimeStepSmoothing.h>

xiiDefaultTimeStepSmoothing::xiiDefaultTimeStepSmoothing()
{
  m_fLerpFactor = 0.2f;
}

void xiiDefaultTimeStepSmoothing::Reset(const xiiClock* pClock)
{
  m_LastTimeSteps.Clear();
}

xiiTime xiiDefaultTimeStepSmoothing::GetSmoothedTimeStep(xiiTime RawTimeStep, const xiiClock* pClock)
{
  RawTimeStep = xiiMath::Clamp(RawTimeStep * pClock->GetSpeed(), pClock->GetMinimumTimeStep(), pClock->GetMaximumTimeStep());

  if (m_LastTimeSteps.GetCount() < 10)
  {
    m_LastTimeSteps.PushBack(RawTimeStep);
    m_LastTimeStepTaken = RawTimeStep;
    return m_LastTimeStepTaken;
  }

  if (!m_LastTimeSteps.CanAppend(1))
    m_LastTimeSteps.PopFront(1);

  m_LastTimeSteps.PushBack(RawTimeStep);

  xiiStaticArray<xiiTime, 11> Sorted;
  Sorted.SetCountUninitialized(m_LastTimeSteps.GetCount());

  for (xiiUInt32 i = 0; i < m_LastTimeSteps.GetCount(); ++i)
    Sorted[i] = m_LastTimeSteps[i];

  Sorted.Sort();

  xiiUInt32 uiFirstSample = 2;
  xiiUInt32 uiLastSample  = 8;

  xiiTime tAvg;

  for (xiiUInt32 i = uiFirstSample; i <= uiLastSample; ++i)
  {
    tAvg = tAvg + Sorted[i];
  }

  tAvg = tAvg / (double)((uiLastSample - uiFirstSample) + 1.0);


  m_LastTimeStepTaken = xiiMath::Lerp(m_LastTimeStepTaken, tAvg, m_fLerpFactor);

  return m_LastTimeStepTaken;
}



XII_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_DefaultTimeStepSmoothing);
