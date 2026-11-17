/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/ScopeExit.h>

namespace
{
  struct Measurement
  {
    xiiTime m_At, m_Raw, m_Smoothed;
  };
  xiiMutex                       s_Mutex;
  xiiMap<xiiString, Measurement> s_Clocks;
  void                           ClockUpdated(const xiiClock::EventData& event)
  {
    auto* service = xiiApplication::GetApplicationInstance()->GetTelemetryService();
    if (!service || !service->IsCollecting(xiiTelemetrySources::Time) || s_Mutex.TryLock().Failed())
      return;
    XII_SCOPE_EXIT(s_Mutex.Unlock());
    if (event.m_sClockName.GetElementCount() > 512 || (s_Clocks.GetCount() >= 64 && !s_Clocks.Contains(event.m_sClockName)))
      return;
    s_Clocks[event.m_sClockName] = {xiiTime::Now(), event.m_RawTimeStep, event.m_SmoothedTimeStep};
  }
  void Collect(xiiTelemetryService& service, xiiUInt64 source)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = source;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
    {
      XII_LOCK(s_Mutex);
      for (auto it = s_Clocks.GetIterator(); it.IsValid(); ++it)
      {
        auto& fields = sample.m_Records.ExpandAndGetRef().m_Fields;
        fields.Insert("name", it.Key());
        fields.Insert("at", it.Value().m_At);
        fields.Insert("raw", it.Value().m_Raw);
        fields.Insert("smoothed", it.Value().m_Smoothed);
      }
      s_Clocks.Clear();
    }
    service.Publish(std::move(sample)).IgnoreResult();
  }
} // namespace

void AddTimeEventHandler()
{
  xiiTelemetrySourceInfo source;
  source.m_uiId                          = xiiTelemetrySources::Time;
  source.m_sName                         = "Clocks";
  source.m_sCategory                     = "Timing";
  source.m_uiMinimumIntervalMilliseconds = 33;
  source.m_bLatestOnly                   = true;
  XII_VERIFY(xiiApplication::GetApplicationInstance()->CreateTelemetryService().RegisterSource(source, Collect).Succeeded(), "Duplicate clock source.");
  xiiClock::AddEventHandler(ClockUpdated);
}

void RemoveTimeEventHandler()
{
  xiiClock::RemoveEventHandler(ClockUpdated);
  xiiApplication::GetApplicationInstance()->GetTelemetryService()->UnregisterSource(xiiTelemetrySources::Time);
  XII_LOCK(s_Mutex);
  s_Clocks.Clear();
}
