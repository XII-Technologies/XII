/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Utilities/Stats.h>

static xiiAssertHandler g_PreviousAssertHandler = nullptr;

static bool TelemetryAssertHandler(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  if (auto* service = xiiApplication::GetApplicationInstance()->GetTelemetryService(); service && service->IsCollecting(xiiTelemetrySources::Application))
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = xiiTelemetrySources::Application;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
    auto& fields                    = sample.m_Records.ExpandAndGetRef().m_Fields;
    fields.Insert("file", xiiString(szSourceFile));
    fields.Insert("line", uiLine);
    fields.Insert("function", xiiString(szFunction));
    fields.Insert("expression", xiiString(szExpression));
    fields.Insert("message", xiiString(szAssertMsg));
    service->Publish(std::move(sample)).IgnoreResult();
  }

  if (g_PreviousAssertHandler)
    return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

  return true;
}

void AddTelemetryAssertHandler()
{
  xiiTelemetrySourceInfo source;
  source.m_uiId      = xiiTelemetrySources::Application;
  source.m_sName     = "Application assertions";
  source.m_sCategory = "Application";
  XII_VERIFY(xiiApplication::GetApplicationInstance()->CreateTelemetryService().RegisterSource(source).Succeeded(), "Duplicate application source.");
  g_PreviousAssertHandler = xiiGetAssertHandler();
  xiiSetAssertHandler(TelemetryAssertHandler);
}

void RemoveTelemetryAssertHandler()
{
  xiiSetAssertHandler(g_PreviousAssertHandler);
  g_PreviousAssertHandler = nullptr;
  xiiApplication::GetApplicationInstance()->GetTelemetryService()->UnregisterSource(xiiTelemetrySources::Application);
}

void SetAppStats()
{
  xiiStringBuilder           sOut;
  const xiiSystemInformation info = xiiSystemInformation::Get();

  xiiStats::SetStat("Platform/Name", info.GetPlatformName());

  xiiStats::SetStat("Hardware/CPU Cores", info.GetCPUCoreCount());

  xiiStats::SetStat("Hardware/RAM[GB]", info.GetInstalledMainMemory() / 1024.0f / 1024.0f / 1024.0f);

  sOut = info.Is64BitOS() ? "64 Bit" : "32 Bit";
  xiiStats::SetStat("Platform/Architecture", sOut.GetData());

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  sOut = "Debug";
#elif XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  sOut = "Dev";
#else
  sOut = "Release";
#endif
  xiiStats::SetStat("Platform/Build", sOut.GetData());

#if XII_ENABLED(XII_USE_PROFILING)
  sOut = "Enabled";
#else
  sOut = "Disabled";
#endif
  xiiStats::SetStat("Features/Profiling", sOut.GetData());

  if constexpr (xiiAllocatorTrackingMode::Default >= xiiAllocatorTrackingMode::AllocationStats)
    sOut = "Enabled";
  else
    sOut = "Disabled";

  xiiStats::SetStat("Features/Allocation Tracking", sOut.GetData());

  if constexpr (xiiAllocatorTrackingMode::Default >= xiiAllocatorTrackingMode::AllocationStatsAndStacktraces)
    sOut = "Enabled";
  else
    sOut = "Disabled";

  xiiStats::SetStat("Features/Allocation Stack Tracing", sOut.GetData());

#if XII_ENABLED(XII_PLATFORM_LITTLE_ENDIAN)
  sOut = "Little";
#else
  sOut = "Big";
#endif
  xiiStats::SetStat("Platform/Endianess", sOut.GetData());
}
