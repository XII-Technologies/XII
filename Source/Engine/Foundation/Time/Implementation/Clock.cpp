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

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiClock, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    XII_ACCESSOR_PROPERTY("Speed", GetSpeed, SetSpeed),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GetGlobalClock),
    XII_SCRIPT_FUNCTION_PROPERTY(GetAccumulatedTime),
    XII_SCRIPT_FUNCTION_PROPERTY(GetTimeDiff)
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiClock::xiiClock(xiiStringView sName)
{
  SetClockName(sName);

  Reset(true);
}

void xiiClock::Reset(bool bEverything)
{
  if (bEverything)
  {
    m_pTimeStepSmoother = nullptr;
    m_MinTimeStep       = xiiTime::MakeFromSeconds(0.001); // 1000 FPS
    m_MaxTimeStep       = xiiTime::MakeFromSeconds(0.1);   //   10 FPS, many simulations will be instable at that rate already
    m_FixedTimeStep     = xiiTime::MakeFromSeconds(0.0);
  }

  m_AccumulatedTime = xiiTime::MakeFromSeconds(0.0);
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
    m_LastTimeDiff = xiiTime::MakeFromSeconds(0.0);
  }
  else if (m_FixedTimeStep > xiiTime::MakeFromSeconds(0.0))
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
  ed.m_sClockName       = m_sName;
  ed.m_RawTimeStep      = tDiff;
  ed.m_SmoothedTimeStep = m_LastTimeDiff;

  s_TimeEvents.Broadcast(ed);
}

void xiiClock::SetAccumulatedTime(xiiTime t)
{
  m_AccumulatedTime = t;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = xiiTime::Now() - xiiTime::MakeFromSeconds(0.01);
  m_LastTimeDiff   = xiiTime::MakeFromSeconds(0.01);
}

void xiiClock::Save(xiiStreamWriter& ref_stream) const
{
  const xiiUInt8 uiVersion = 1;

  ref_stream << uiVersion;
  ref_stream << m_AccumulatedTime;
  ref_stream << m_LastTimeDiff;
  ref_stream << m_FixedTimeStep;
  ref_stream << m_MinTimeStep;
  ref_stream << m_MaxTimeStep;
  ref_stream << m_fSpeed;
  ref_stream << m_bPaused;
}

void xiiClock::Load(xiiStreamReader& ref_stream)
{
  xiiUInt8 uiVersion = 0;
  ref_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion == 1, "Wrong version for xiiClock: {0}", uiVersion);

  ref_stream >> m_AccumulatedTime;
  ref_stream >> m_LastTimeDiff;
  ref_stream >> m_FixedTimeStep;
  ref_stream >> m_MinTimeStep;
  ref_stream >> m_MaxTimeStep;
  ref_stream >> m_fSpeed;
  ref_stream >> m_bPaused;

  // make sure we continue properly
  m_LastTimeUpdate = xiiTime::Now() - m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

XII_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Clock);
