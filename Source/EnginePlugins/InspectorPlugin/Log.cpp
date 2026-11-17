/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Logging/Log.h>

namespace
{
  void CollectLog(const xiiLoggingEventData& event)
  {
    const auto* application = xiiApplication::GetApplicationInstance();
    auto*       service     = application != nullptr ? application->GetTelemetryService() : nullptr;
    if (service == nullptr || !service->IsCollecting(xiiTelemetrySources::Logs))
      return;

    if (event.m_sText.GetElementCount() > 32768 || event.m_sTag.GetElementCount() > 1024)
    {
      service->RecordDropped(xiiTelemetrySources::Logs);
      return;
    }

    xiiTelemetrySample sample;
    sample.m_uiSource               = xiiTelemetrySources::Logs;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
    sample.m_Fields.Insert("type", static_cast<xiiInt32>(event.m_EventType));
    sample.m_Fields.Insert("indentation", static_cast<xiiUInt32>(event.m_uiIndentation));
    sample.m_Fields.Insert("tag", xiiString(event.m_sTag));
    sample.m_Fields.Insert("text", xiiString(event.m_sText));
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (event.m_EventType == xiiLogMsgType::EndGroup)
      sample.m_Fields.Insert("seconds", static_cast<double>(event.m_fSeconds));
#endif
    service->Publish(std::move(sample)).IgnoreResult();
  }
} // namespace

void AddLogWriter()
{
  if (auto* application = xiiApplication::GetApplicationInstance())
  {
    xiiTelemetrySourceInfo source;
    source.m_uiId                          = xiiTelemetrySources::Logs;
    source.m_sName                         = "Application log";
    source.m_sCategory                     = "Application";
    source.m_uiMinimumIntervalMilliseconds = 1;
    application->CreateTelemetryService().RegisterSource(source).AssertSuccess("Could not register the log source.");
  }
  xiiGlobalLog::AddLogWriter(CollectLog);
}

void RemoveLogWriter()
{
  xiiGlobalLog::RemoveLogWriter(CollectLog);
  if (auto* application = xiiApplication::GetApplicationInstance())
    if (auto* service = application->GetTelemetryService())
      service->UnregisterSource(xiiTelemetrySources::Logs);
}
