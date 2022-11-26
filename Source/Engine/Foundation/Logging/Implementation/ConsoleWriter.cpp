#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Time/Timestamp.h>

#if XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <android/log.h>
#  define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "xiiEngine", __VA_ARGS__)
#endif

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

static void SetConsoleColor(WORD ui)
{
#  if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
#  endif
}
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
static void SetConsoleColor(xiiUInt8 ui) {}
#else
#  error "Unknown Platform."
static void SetConsoleColor(xiiUInt8 ui) {}
#endif

xiiLog::TimestampMode xiiLogWriter::Console::s_TimestampMode = xiiLog::TimestampMode::None;

void xiiLogWriter::Console::LogMessageHandler(const xiiLoggingEventData& eventData)
{
  xiiStringBuilder sTimestamp;
  xiiLog::GenerateFormattedTimestamp(s_TimestampMode, sTimestamp);

  static xiiMutex WriterLock; // will only be created if this writer is used at all
  XII_LOCK(WriterLock);

  if (eventData.m_EventType == xiiLogMsgType::BeginGroup)
    printf("\n");

  for (xiiUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    printf(" ");

  switch (eventData.m_EventType)
  {
    case xiiLogMsgType::Flush:
      fflush(stdout);
      break;

    case xiiLogMsgType::BeginGroup:
      SetConsoleColor(0x02);
      printf("+++++ %s (%s) +++++\n", eventData.m_szText, eventData.m_szTag);
      break;

    case xiiLogMsgType::EndGroup:
      SetConsoleColor(0x02);
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      printf("----- %s (%.6f sec)-----\n\n", eventData.m_szText, eventData.m_fSeconds);
#else
      printf("----- %s (%s)-----\n\n", eventData.m_szText, "timing info not available");
#endif
      break;

    case xiiLogMsgType::ErrorMsg:
      SetConsoleColor(0x0C);
      printf("%sError: %s\n", sTimestamp.GetData(), eventData.m_szText);
      fflush(stdout);
      break;

    case xiiLogMsgType::SeriousWarningMsg:
      SetConsoleColor(0x0C);
      printf("%sSeriously: %s\n", sTimestamp.GetData(), eventData.m_szText);
      break;

    case xiiLogMsgType::WarningMsg:
      SetConsoleColor(0x0E);
      printf("%sWarning: %s\n", sTimestamp.GetData(), eventData.m_szText);
      break;

    case xiiLogMsgType::SuccessMsg:
      SetConsoleColor(0x0A);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_szText);
      fflush(stdout);
      break;

    case xiiLogMsgType::InfoMsg:
      SetConsoleColor(0x07);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_szText);
      break;

    case xiiLogMsgType::DevMsg:
      SetConsoleColor(0x08);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_szText);
      break;

    case xiiLogMsgType::DebugMsg:
      SetConsoleColor(0x09);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_szText);
      break;

    default:
      SetConsoleColor(0x0D);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_szText);

      xiiLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }

  SetConsoleColor(0x07);
}

void xiiLogWriter::Console::SetTimestampMode(xiiLog::TimestampMode mode)
{
  s_TimestampMode = mode;
}
#if XII_ENABLED(XII_PLATFORM_ANDROID)
#  undef printf
#endif


XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_ConsoleWriter);
