/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Diagnostics/TelemetryService.h>

namespace
{
  void Collect(xiiTelemetryService& service, xiiUInt64 id)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = id;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();

    xiiHybridArray<xiiPlugin::PluginInfo, 16> infos;
    xiiPlugin::GetAllPluginInfos(infos);
    for (const auto& plugin : infos)
    {
      if (sample.m_Records.GetCount() == 4096)
      {
        ++sample.m_uiDropped;
        continue;
      }
      auto& row = sample.m_Records.ExpandAndGetRef().m_Fields;
      row.Insert("name", plugin.m_sName);
      xiiStringBuilder dependencies;
      for (const auto& dependency : plugin.m_sDependencies)
        dependencies.AppendWithSeparator(" | ", dependency);
      row.Insert("dependencies", xiiString(dependencies));
    }
    service.Publish(std::move(sample)).IgnoreResult();
  }
} // namespace

void AddPluginEventHandler()
{
  auto&                  service = xiiApplication::GetApplicationInstance()->CreateTelemetryService();
  xiiTelemetrySourceInfo info;
  info.m_uiId                          = xiiTelemetrySources::Plugins;
  info.m_sName                         = "Plugins";
  info.m_sCategory                     = "Runtime";
  info.m_uiMinimumIntervalMilliseconds = 1000;
  info.m_bSnapshot                     = true;
  info.m_bLatestOnly                   = true;
  XII_VERIFY(service.RegisterSource(info, Collect).Succeeded(), "Duplicate diagnostics source.");
}

void RemovePluginEventHandler()
{
  if (auto* service = xiiApplication::GetApplicationInstance()->GetTelemetryService())
    service->UnregisterSource(xiiTelemetrySources::Plugins);
}
