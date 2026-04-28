/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Time/Stopwatch.h>

xiiStopwatch::xiiStopwatch()
{
  m_LastCheckpoint = xiiTime::Now();

  StopAndReset();
  Resume();
}

void xiiStopwatch::StopAndReset()
{
  m_TotalDuration = xiiTime::MakeZero();
  m_bRunning      = false;
}

void xiiStopwatch::Resume()
{
  if (m_bRunning)
    return;

  m_bRunning   = true;
  m_LastUpdate = xiiTime::Now();
}

void xiiStopwatch::Pause()
{
  if (!m_bRunning)
    return;

  m_bRunning = false;

  m_TotalDuration += xiiTime::Now() - m_LastUpdate;
}

xiiTime xiiStopwatch::GetRunningTotal() const
{
  if (m_bRunning)
  {
    const xiiTime tNow = xiiTime::Now();

    m_TotalDuration += tNow - m_LastUpdate;
    m_LastUpdate = tNow;
  }

  return m_TotalDuration;
}

xiiTime xiiStopwatch::Checkpoint()
{
  const xiiTime tNow = xiiTime::Now();

  const xiiTime tDiff = tNow - m_LastCheckpoint;
  m_LastCheckpoint    = tNow;

  return tDiff;
}

XII_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Stopwatch);
