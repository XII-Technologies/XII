/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Events/GlobalEvent.h>

namespace
{
  void Collect(xiiTelemetryService& service, xiiUInt64 id)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = id;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();

    xiiGlobalEvent::UpdateGlobalEventStatistics();
    for (auto it = xiiGlobalEvent::GetEventStatistics().GetIterator(); it.IsValid(); ++it)
    {
      if (sample.m_Records.GetCount() == 4096)
      {
        ++sample.m_uiDropped;
        continue;
      }
      auto& row = sample.m_Records.ExpandAndGetRef().m_Fields;
      row.Insert("name", it.Key());
      row.Insert("fired", it.Value().m_uiNumTimesFired);
      row.Insert("handlers", it.Value().m_uiNumEventHandlersRegular);
      row.Insert("once", it.Value().m_uiNumEventHandlersOnce);
    }
    service.Publish(std::move(sample)).IgnoreResult();
  }
} // namespace

void AddGlobalEventHandler()
{
  auto&                  service = xiiApplication::GetApplicationInstance()->CreateTelemetryService();
  xiiTelemetrySourceInfo info;
  info.m_uiId                          = xiiTelemetrySources::GlobalEvents;
  info.m_sName                         = "Global events";
  info.m_sCategory                     = "Runtime";
  info.m_uiMinimumIntervalMilliseconds = 500;
  info.m_bSnapshot                     = true;
  info.m_bLatestOnly                   = true;
  XII_VERIFY(service.RegisterSource(info, Collect).Succeeded(), "Duplicate diagnostics source.");
}

void RemoveGlobalEventHandler()
{
  if (auto* service = xiiApplication::GetApplicationInstance()->GetTelemetryService())
    service->UnregisterSource(xiiTelemetrySources::GlobalEvents);
}
