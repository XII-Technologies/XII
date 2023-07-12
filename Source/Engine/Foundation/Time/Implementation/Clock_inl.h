#pragma once

#include <Foundation/Time/Clock.h>

inline void xiiClock::SetClockName(xiiStringView sName)
{
  m_sName = sName;
}

inline xiiStringView xiiClock::GetClockName() const
{
  return m_sName.GetView();
}

inline void xiiClock::SetTimeStepSmoothing(xiiTimeStepSmoothing* pSmoother)
{
  m_pTimeStepSmoother = pSmoother;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

inline xiiTimeStepSmoothing* xiiClock::GetTimeStepSmoothing() const
{
  return m_pTimeStepSmoother;
}

inline void xiiClock::SetPaused(bool bPaused)
{
  m_bPaused = bPaused;

  // when we enter a pause, inform the time step smoother to throw away his statistics
  if (bPaused && m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

inline bool xiiClock::GetPaused() const
{
  return m_bPaused;
}

inline xiiTime xiiClock::GetFixedTimeStep() const
{
  return m_FixedTimeStep;
}

inline xiiTime xiiClock::GetAccumulatedTime() const
{
  return m_AccumulatedTime;
}

inline xiiTime xiiClock::GetTimeDiff() const
{
  return m_LastTimeDiff;
}

inline double xiiClock::GetSpeed() const
{
  return m_fSpeed;
}

inline void xiiClock::SetMinimumTimeStep(xiiTime min)
{
  XII_ASSERT_DEV(min >= xiiTime::Seconds(0.0), "Time flows in one direction only.");

  m_MinTimeStep = min;
}

inline void xiiClock::SetMaximumTimeStep(xiiTime max)
{
  XII_ASSERT_DEV(max >= xiiTime::Seconds(0.0), "Time flows in one direction only.");

  m_MaxTimeStep = max;
}

inline xiiTime xiiClock::GetMinimumTimeStep() const
{
  return m_MinTimeStep;
}

inline xiiTime xiiClock::GetMaximumTimeStep() const
{
  return m_MaxTimeStep;
}

inline void xiiClock::SetFixedTimeStep(xiiTime diff)
{
  XII_ASSERT_DEV(m_FixedTimeStep.GetSeconds() >= 0.0, "Fixed Time Stepping cannot reverse time!");

  m_FixedTimeStep = diff;
}

inline void xiiClock::SetSpeed(double fFactor)
{
  XII_ASSERT_DEV(fFactor >= 0.0, "Time cannot run backwards.");

  m_fSpeed = fFactor;
}
