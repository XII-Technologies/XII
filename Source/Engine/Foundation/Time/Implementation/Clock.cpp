#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Clock.h>

xiiClock::Event xiiClock::s_TimeEvents;
xiiClock*       xiiClock::s_pGlobalClock = nullptr;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Clock)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_BASESYSTEMS_STARTUP
  {
    xiiClock::s_pGlobalClock = new xiiClock("Global");
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiClock::xiiClock(const char* szName)
{
  SetClockName(szName);

  Reset(true);
}

void xiiClock::Reset(bool bEverything)
{
  if (bEverything)
  {
    m_pTimeStepSmoother = nullptr;
    m_MinTimeStep       = xiiTime::Seconds(0.001); // 1000 FPS
    m_MaxTimeStep       = xiiTime::Seconds(0.1);   //   10 FPS, many simulations will be instable at that rate already
    m_FixedTimeStep     = xiiTime::Seconds(0.0);
  }

  m_AccumulatedTime = xiiTime::Seconds(0.0);
  m_fSpeed          = 1.0;
  m_bPaused         = false;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = xiiTime::Now() - m_MinTimeStep;
  m_LastTimeDiff   = m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

void xiiClock::Update()
{
  const xiiTime tNow  = xiiTime::Now();
  const xiiTime tDiff = tNow - m_LastTimeUpdate;
  m_LastTimeUpdate    = tNow;

  if (m_bPaused)
  {
    // no change during pause
    m_LastTimeDiff = xiiTime::Seconds(0.0);
  }
  else if (m_FixedTimeStep > xiiTime::Seconds(0.0))
  {
    // scale the time step by the speed factor
    m_LastTimeDiff = m_FixedTimeStep * m_fSpeed;
  }
  else
  {
    // in variable time step mode, apply the time step smoother, if available
    if (m_pTimeStepSmoother)
      m_LastTimeDiff = m_pTimeStepSmoother->GetSmoothedTimeStep(tDiff, this);
    else
    {
      // scale the time step by the speed factor
      // and make sure the time step does not leave the predetermined bounds
      m_LastTimeDiff = xiiMath::Clamp(tDiff * m_fSpeed, m_MinTimeStep, m_MaxTimeStep);
    }
  }

  m_AccumulatedTime += m_LastTimeDiff;

  EventData ed;
  ed.m_szClockName      = m_sName.GetData();
  ed.m_RawTimeStep      = tDiff;
  ed.m_SmoothedTimeStep = m_LastTimeDiff;

  s_TimeEvents.Broadcast(ed);
}

void xiiClock::SetAccumulatedTime(xiiTime t)
{
  m_AccumulatedTime = t;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = xiiTime::Now() - xiiTime::Seconds(0.01);
  m_LastTimeDiff   = xiiTime::Seconds(0.01);
}

void xiiClock::Save(xiiStreamWriter& Stream) const
{
  const xiiUInt8 uiVersion = 1;

  Stream << uiVersion;
  Stream << m_AccumulatedTime;
  Stream << m_LastTimeDiff;
  Stream << m_FixedTimeStep;
  Stream << m_MinTimeStep;
  Stream << m_MaxTimeStep;
  Stream << m_fSpeed;
  Stream << m_bPaused;
}

void xiiClock::Load(xiiStreamReader& Stream)
{
  xiiUInt8 uiVersion = 0;
  Stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion == 1, "Wrong version for xiiClock: {0}", uiVersion);

  Stream >> m_AccumulatedTime;
  Stream >> m_LastTimeDiff;
  Stream >> m_FixedTimeStep;
  Stream >> m_MinTimeStep;
  Stream >> m_MaxTimeStep;
  Stream >> m_fSpeed;
  Stream >> m_bPaused;

  // make sure we continue properly
  m_LastTimeUpdate = xiiTime::Now() - m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}



XII_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Clock);
