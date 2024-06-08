#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Utilities/Stats.h>

static xiiAssertHandler g_PreviousAssertHandler = nullptr;

static bool TelemetryAssertHandler(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  if (xiiTelemetry::IsConnectedToClient())
  {
    xiiTelemetryMessage msg;
    msg.SetMessageID(' APP', 'ASRT');
    msg.GetWriter() << szSourceFile;
    msg.GetWriter() << uiLine;
    msg.GetWriter() << szFunction;
    msg.GetWriter() << szExpression;
    msg.GetWriter() << szAssertMsg;

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);

    // messages might not arrive, if the network does not get enough time to transmit them
    // since we are crashing the application in (half) 'a second', we need to make sure the network traffic has indeed been sent
    for (xiiUInt32 i = 0; i < 5; ++i)
    {
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(100));
      xiiTelemetry::UpdateNetwork();
    }
  }

  if (g_PreviousAssertHandler)
    return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

  return true;
}

void AddTelemetryAssertHandler()
{
  g_PreviousAssertHandler = xiiGetAssertHandler();
  xiiSetAssertHandler(TelemetryAssertHandler);
}

void RemoveTelemetryAssertHandler()
{
  xiiSetAssertHandler(g_PreviousAssertHandler);
  g_PreviousAssertHandler = nullptr;
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

XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_App);
