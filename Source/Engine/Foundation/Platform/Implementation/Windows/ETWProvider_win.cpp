#include <Foundation/FoundationPCH.h>

#include <Foundation/Platform/Implementation/Windows/ETWProvider_win.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <TraceLoggingProvider.h>

// Workaround to support TraceLoggingProvider.h and /utf-8 compiler switch.
#  undef _TlgPragmaUtf8Begin
#  undef _TlgPragmaUtf8End
#  define _TlgPragmaUtf8Begin
#  define _TlgPragmaUtf8End
#  undef _tlgPragmaUtf8Begin
#  undef _tlgPragmaUtf8End
#  define _tlgPragmaUtf8Begin
#  define _tlgPragmaUtf8End

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

  TraceLoggingWrite(g_xiiETWLogProvider, "LogMessge", TraceLoggingValue((xiiInt32)eventType, "Type"), TraceLoggingValue(uiIndentation, "Indentation"), TraceLoggingValue(sTemp.GetData(), "Text"));
}

xiiETWProvider& xiiETWProvider::GetInstance()
{
  static xiiETWProvider instance;
  return instance;
}
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_Win_ETWProvider_win);
