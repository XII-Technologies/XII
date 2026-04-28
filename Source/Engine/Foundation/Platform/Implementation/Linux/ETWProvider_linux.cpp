/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Platform/Implementation/Linux/ETWProvider_linux.h>

#if XII_ENABLED(XII_PLATFORM_LINUX) && defined(BUILDSYSTEM_ENABLE_TRACELOGGING_LTTNG_SUPPORT)

#  include <tracelogging/TraceLoggingProvider.h>

TRACELOGGING_DECLARE_PROVIDER(g_xiiETWLogProvider);

// Define the GUID to use for the XII ETW Logger.
// {BFD4350A-BA77-463D-B4BE-E30374E42494}
#  define XII_LOGGER_GUID (0xbfd4350a, 0xba77, 0x463d, 0xb4, 0xbe, 0xe3, 0x3, 0x74, 0xe4, 0x24, 0x94)

TRACELOGGING_DEFINE_PROVIDER(g_xiiETWLogProvider, "xiiLogProvider", XII_LOGGER_GUID);

xiiETWProvider::xiiETWProvider()
{
  TraceLoggingRegister(g_xiiETWLogProvider);
}

xiiETWProvider::~xiiETWProvider()
{
  TraceLoggingUnregister(g_xiiETWLogProvider);
}

void xiiETWProvider::LogMessage(xiiLogMsgType::Enum eventType, xiiUInt8 uiIndentation, xiiStringView sText)
{
  const xiiStringBuilder sTemp = sText;

  TraceLoggingWrite(g_xiiETWLogProvider, "LogMessage", TraceLoggingValue((int)eventType, "Type"), TraceLoggingValue(uiIndentation, "Indentation"), TraceLoggingValue(sTemp.GetData(), "Text"));
}

xiiETWProvider& xiiETWProvider::GetInstance()
{
  static xiiETWProvider instance;
  return instance;
}
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_Linux_ETWProvider_linux);
