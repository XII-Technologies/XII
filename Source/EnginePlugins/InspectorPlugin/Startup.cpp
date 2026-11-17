/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Diagnostics/TelemetryService.h>

namespace
{
  void Collect(xiiTelemetryService& service, xiiUInt64 id)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = id;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();

    for (auto* system = xiiSubSystem::GetFirstInstance(); system; system = system->GetNextInstance())
    {
      if (sample.m_Records.GetCount() == 4096)
      {
        ++sample.m_uiDropped;
        continue;
      }
      auto& row = sample.m_Records.ExpandAndGetRef().m_Fields;
      row.Insert("group", xiiString(system->GetGroupName()));
      row.Insert("name", xiiString(system->GetSubSystemName()));
      row.Insert("plugin", xiiString(system->GetPluginName()));
      xiiUInt32 stages = 0;
      for (xiiUInt32 stage = 0; stage < xiiStartupStage::ENUM_COUNT; ++stage)
        if (system->IsStartupPhaseDone(static_cast<xiiStartupStage::Enum>(stage)))
          stages |= 1U << stage;
      row.Insert("stages", stages);
      xiiStringBuilder dependencies;
      for (xiiUInt32 dependency = 0; dependency < 256 && !system->GetDependency(dependency).IsEmpty(); ++dependency)
        dependencies.AppendWithSeparator(" | ", system->GetDependency(dependency));
      row.Insert("dependencies", xiiString(dependencies));
    }
    service.Publish(std::move(sample)).IgnoreResult();
  }
} // namespace

void AddStartupEventHandler()
{
  auto&                  service = xiiApplication::GetApplicationInstance()->CreateTelemetryService();
  xiiTelemetrySourceInfo info;
  info.m_uiId                          = xiiTelemetrySources::Subsystems;
  info.m_sName                         = "Subsystem lifecycle";
  info.m_sCategory                     = "Runtime";
  info.m_uiMinimumIntervalMilliseconds = 1000;
  info.m_bSnapshot                     = true;
  info.m_bLatestOnly                   = true;
  XII_VERIFY(service.RegisterSource(info, Collect).Succeeded(), "Duplicate diagnostics source.");
}

void RemoveStartupEventHandler()
{
  if (auto* service = xiiApplication::GetApplicationInstance()->GetTelemetryService())
    service->UnregisterSource(xiiTelemetrySources::Subsystems);
}
