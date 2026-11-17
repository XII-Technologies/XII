/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Diagnostics/TelemetryService.h>

namespace
{
  void Collect(xiiTelemetryService& service, xiiUInt64 source)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = source;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
    for (auto* variable = xiiCVar::GetFirstInstance(); variable; variable = variable->GetNextInstance())
    {
      if (sample.m_Records.GetCount() == 4096)
      {
        ++sample.m_uiDropped;
        continue;
      }
      auto& row = sample.m_Records.ExpandAndGetRef().m_Fields;
      row.Insert("name", xiiString(variable->GetName()));
      row.Insert("plugin", xiiString(variable->GetPluginName()));
      row.Insert("description", xiiString(variable->GetDescription()));
      row.Insert("type", static_cast<xiiUInt8>(variable->GetType()));
      switch (variable->GetType())
      {
        case xiiCVarType::Bool:
          row.Insert("value", static_cast<xiiCVarBool*>(variable)->GetValue());
          break;
        case xiiCVarType::Int:
          row.Insert("value", static_cast<xiiCVarInt*>(variable)->GetValue());
          break;
        case xiiCVarType::Float:
          row.Insert("value", static_cast<xiiCVarFloat*>(variable)->GetValue());
          break;
        case xiiCVarType::Double:
          row.Insert("value", static_cast<xiiCVarDouble*>(variable)->GetValue());
          break;
        case xiiCVarType::String:
          row.Insert("value", xiiString(static_cast<xiiCVarString*>(variable)->GetValue()));
          break;
        default:
          break;
      }
    }
    service.Publish(std::move(sample)).IgnoreResult();
  }
} // namespace
void AddCVarEventHandler()
{
  xiiTelemetrySourceInfo source;
  source.m_uiId                          = xiiTelemetrySources::CVars;
  source.m_sName                         = "Configuration variables";
  source.m_sCategory                     = "Configuration";
  source.m_uiMinimumIntervalMilliseconds = 500;
  source.m_bSnapshot = source.m_bLatestOnly = true;
  XII_VERIFY(xiiApplication::GetApplicationInstance()->CreateTelemetryService().RegisterSource(source, Collect).Succeeded(), "Duplicate CVar source.");
}
void RemoveCVarEventHandler()
{
  xiiApplication::GetApplicationInstance()->GetTelemetryService()->UnregisterSource(xiiTelemetrySources::CVars);
}
