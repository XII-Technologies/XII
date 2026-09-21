/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS) || XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Logging/ETWWriter.h>
#endif

#include <stdarg.h>

xiiLogMsgType::Enum      xiiLog::s_DefaultLogLevel     = xiiLogMsgType::All;
xiiLog::PrintFunction    xiiLog::s_CustomPrintFunction = nullptr;
xiiAtomicInteger32       xiiGlobalLog::s_uiMessageCount[xiiLogMsgType::ENUM_COUNT];
xiiLoggingEvent          xiiGlobalLog::s_LoggingEvent;
xiiLogInterface*         xiiGlobalLog::s_pOverrideLog = nullptr;
static thread_local bool s_bAllowOverrideLog          = true;
static xiiMutex          s_OverrideLogMutex;

/// The log system that messages are sent to when the user specifies no system himself.
static thread_local xiiLogInterface* s_DefaultLogSystem = nullptr;

xiiEventSubscriptionID xiiGlobalLog::AddLogWriter(xiiLoggingEvent::Handler handler)
{
  if (s_LoggingEvent.HasEventHandler(handler))
    return 0;

  return s_LoggingEvent.AddEventHandler(handler);
}

void xiiGlobalLog::RemoveLogWriter(xiiLoggingEvent::Handler handler)
{
  if (!s_LoggingEvent.HasEventHandler(handler))
    return;

  s_LoggingEvent.RemoveEventHandler(handler);
}

void xiiGlobalLog::RemoveLogWriter(xiiEventSubscriptionID& ref_subscriptionID)
{
  s_LoggingEvent.RemoveEventHandler(ref_subscriptionID);
}

void xiiGlobalLog::SetGlobalLogOverride(xiiLogInterface* pInterface)
{
  XII_LOCK(s_OverrideLogMutex);

  XII_ASSERT_DEV(pInterface == nullptr || s_pOverrideLog == nullptr, "Only one override log can be set at a time");
  s_pOverrideLog = pInterface;
}

void xiiGlobalLog::HandleLogMessage(const xiiLoggingEventData& le)
{
  if (s_pOverrideLog != nullptr && s_pOverrideLog != this && s_bAllowOverrideLog)
  {
    // only enter the lock when really necessary
    XII_LOCK(s_OverrideLogMutex);

    // since s_bAllowOverrideLog is thread_local we do not need to re-check it

    // check this again under the lock, to be safe
    if (s_pOverrideLog != nullptr && s_pOverrideLog != this)
    {
      // disable the override log for the period in which it handles the event
      // to prevent infinite recursions
      s_bAllowOverrideLog = false;
      s_pOverrideLog->HandleLogMessage(le);
      s_bAllowOverrideLog = true;

      return;
    }
  }

  // else
  {
    const xiiLogMsgType::Enum ThisType = le.m_EventType;

    if ((ThisType > xiiLogMsgType::None) && (ThisType < xiiLogMsgType::All))
      s_uiMessageCount[ThisType].Increment();

    s_LoggingEvent.Broadcast(le);
  }
}

xiiLogBlock::xiiLogBlock(xiiStringView sName, xiiStringView sContextInfo)
{
  m_pLogInterface = xiiLog::GetThreadLocalLogSystem();

  if (!m_pLogInterface)
    return;

  m_sName        = sName;
  m_sContextInfo = sContextInfo;
  m_bWritten     = false;

  m_pParentBlock                   = m_pLogInterface->m_pCurrentBlock;
  m_pLogInterface->m_pCurrentBlock = this;

  m_uiBlockDepth = m_pParentBlock ? (m_pParentBlock->m_uiBlockDepth + 1) : 0;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = xiiTime::Now().GetSeconds();
#endif
}

xiiLogBlock::xiiLogBlock(xiiLogInterface* pInterface, xiiStringView sName, xiiStringView sContextInfo)
{
  m_pLogInterface = pInterface;

  if (!m_pLogInterface)
    return;

  m_sName        = sName;
  m_sContextInfo = sContextInfo;
  m_bWritten     = false;

  m_pParentBlock                   = m_pLogInterface->m_pCurrentBlock;
  m_pLogInterface->m_pCurrentBlock = this;

  m_uiBlockDepth = m_pParentBlock ? (m_pParentBlock->m_uiBlockDepth + 1) : 0;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = xiiTime::Now().GetSeconds();
#endif
}

xiiLogBlock::~xiiLogBlock()
{
  if (!m_pLogInterface)
    return;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = xiiTime::Now().GetSeconds() - m_fSeconds;
#endif

  m_pLogInterface->m_pCurrentBlock = m_pParentBlock;

  xiiLog::EndLogBlock(m_pLogInterface, this);
}


void xiiLog::EndLogBlock(xiiLogInterface* pInterface, xiiLogBlock* pBlock)
{
  if (pBlock->m_bWritten)
  {
    xiiLoggingEventData le;
    le.m_EventType     = xiiLogMsgType::EndGroup;
    le.m_sText         = pBlock->m_sName;
    le.m_uiIndentation = pBlock->m_uiBlockDepth;
    le.m_sTag          = pBlock->m_sContextInfo;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    le.m_fSeconds = pBlock->m_fSeconds;
#endif

    pInterface->HandleLogMessage(le);
  }
}

void xiiLog::WriteBlockHeader(xiiLogInterface* pInterface, xiiLogBlock* pBlock)
{
  if (!pBlock || pBlock->m_bWritten)
    return;

  pBlock->m_bWritten = true;

  WriteBlockHeader(pInterface, pBlock->m_pParentBlock);

  xiiLoggingEventData le;
  le.m_EventType     = xiiLogMsgType::BeginGroup;
  le.m_sText         = pBlock->m_sName;
  le.m_uiIndentation = pBlock->m_uiBlockDepth;
  le.m_sTag          = pBlock->m_sContextInfo;

  pInterface->HandleLogMessage(le);
}

void xiiLog::BroadcastLoggingEvent(xiiLogInterface* pInterface, xiiLogMsgType::Enum type, xiiStringView sString)
{
  xiiLogBlock* pTopBlock     = pInterface->m_pCurrentBlock;
  xiiUInt8     uiIndentation = 0;

  if (pTopBlock)
  {
    uiIndentation = pTopBlock->m_uiBlockDepth + 1;

    WriteBlockHeader(pInterface, pTopBlock);
  }

  char szTag[32] = "";

  if (sString.StartsWith("["))
  {
    const char* szAfterTag = sString.GetStartPointer();

    ++szAfterTag;

    xiiInt32 iPos = 0;

    // only treat it as a tag, if it is properly enclosed in square brackets and doesn't contain spaces
    while ((*szAfterTag != '\0') && (*szAfterTag != '[') && (*szAfterTag != ']') && (*szAfterTag != ' ') && (iPos < 31))
    {
      szTag[iPos] = *szAfterTag;
      ++szAfterTag;
      ++iPos;
    }

    if (*szAfterTag == ']')
    {
      szTag[iPos] = '\0';
      sString.SetStartPosition(szAfterTag + 1);
    }
    else
    {
      szTag[0] = '\0';
    }
  }

  xiiLoggingEventData le;
  le.m_EventType     = type;
  le.m_sText         = sString;
  le.m_uiIndentation = uiIndentation;
  le.m_sTag          = szTag;

  pInterface->HandleLogMessage(le);
  pInterface->m_uiLoggedMsgsSinceFlush++;
}

void xiiLog::Print(const char* szText)
{
  printf("%s", szText);

#if XII_ENABLED(XII_PLATFORM_WINDOWS) || XII_ENABLED(XII_PLATFORM_LINUX)
  xiiLogWriter::ETW::LogMessage(xiiLogMsgType::ErrorMsg, 0, szText);
#endif
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  OutputDebugStringW(xiiStringWChar(szText).GetData());
#endif

  if (s_CustomPrintFunction)
  {
    s_CustomPrintFunction(szText);
  }

  fflush(stdout);
  fflush(stderr);
}

void xiiLog::Printf(const char* szFormat, ...)
{
  va_list args;
  va_start(args, szFormat);

  char buffer[4096];
  xiiStringUtils::vsnprintf(buffer, XII_ARRAY_SIZE(buffer), szFormat, args);

  Print(buffer);

  va_end(args);
}

void xiiLog::SetCustomPrintFunction(PrintFunction func)
{
  s_CustomPrintFunction = func;
}

void xiiLog::OsMessageBox(const xiiFormatString& text)
{
  xiiStringBuilder tmp;
  xiiStringBuilder display = text.GetText(tmp);
  display.Trim(" \n\r\t");

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const char* title = "";
  if (xiiApplication::GetApplicationInstance())
  {
    title = xiiApplication::GetApplicationInstance()->GetApplicationName();
  }

  MessageBoxW(nullptr, xiiStringWChar(display).GetData(), xiiStringWChar(title), MB_OK);
#else
  xiiLog::Print(display);
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}

void xiiLog::GenerateFormattedTimestamp(TimestampMode mode, xiiStringBuilder& ref_sTimestampOut)
{
  // if mode is 'None', early out to not even retrieve a timestamp
  if (mode == TimestampMode::None)
    return;

  const xiiDateTime dateTime = xiiDateTime::MakeFromTimestamp(xiiTimestamp::CurrentTimestamp());

  switch (mode)
  {
    case TimestampMode::Numeric:
      ref_sTimestampOut.SetFormat("[{}] ", xiiArgDateTime(dateTime, xiiArgDateTime::ShowDate | xiiArgDateTime::ShowMilliseconds | xiiArgDateTime::ShowTimeZone));
      break;
    case TimestampMode::TimeOnly:
      ref_sTimestampOut.SetFormat("[{}] ", xiiArgDateTime(dateTime, xiiArgDateTime::ShowMilliseconds));
      break;
    case TimestampMode::Textual:
      ref_sTimestampOut.SetFormat("[{}] ", xiiArgDateTime(dateTime, xiiArgDateTime::TextualDate | xiiArgDateTime::ShowMilliseconds | xiiArgDateTime::ShowTimeZone));
      break;
    default:
      XII_ASSERT_DEV(false, "Unknown timestamp mode.");
      break;
  }
}

void xiiLog::SetThreadLocalLogSystem(xiiLogInterface* pInterface)
{
  XII_ASSERT_DEV(pInterface != nullptr, "You cannot set a nullptr logging system. If you want to discard all log information, set a dummy system that does not do anything.");

  s_DefaultLogSystem = pInterface;
}

xiiLogInterface* xiiLog::GetThreadLocalLogSystem()
{
  if (s_DefaultLogSystem == nullptr)
  {
    // use new, not XII_DEFAULT_NEW, to prevent tracking
    s_DefaultLogSystem = new xiiGlobalLog;
  }

  return s_DefaultLogSystem;
}

void xiiLog::SetDefaultLogLevel(xiiLogMsgType::Enum logLevel)
{
  XII_ASSERT_DEV(logLevel >= xiiLogMsgType::None && logLevel <= xiiLogMsgType::All, "Invalid default log level {}", (xiiInt32)logLevel);

  s_DefaultLogLevel = logLevel;
}

xiiLogMsgType::Enum xiiLog::GetDefaultLogLevel()
{
  return s_DefaultLogLevel;
}

#define LOG_LEVEL_FILTER(MaxLevel)                                                                                                    \
  if (pInterface == nullptr)                                                                                                          \
    return;                                                                                                                           \
  if ((pInterface->GetLogLevel() == xiiLogMsgType::GlobalDefault ? xiiLog::s_DefaultLogLevel : pInterface->GetLogLevel()) < MaxLevel) \
    return;


void xiiLog::Error(xiiLogInterface* pInterface, const xiiFormatString& string)
{
  LOG_LEVEL_FILTER(xiiLogMsgType::ErrorMsg);

  xiiStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, xiiLogMsgType::ErrorMsg, string.GetText(tmp));
}

void xiiLog::SeriousWarning(xiiLogInterface* pInterface, const xiiFormatString& string)
{
  LOG_LEVEL_FILTER(xiiLogMsgType::SeriousWarningMsg);

  xiiStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, xiiLogMsgType::SeriousWarningMsg, string.GetText(tmp));
}

void xiiLog::Warning(xiiLogInterface* pInterface, const xiiFormatString& string)
{
  LOG_LEVEL_FILTER(xiiLogMsgType::WarningMsg);

  xiiStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, xiiLogMsgType::WarningMsg, string.GetText(tmp));
}

void xiiLog::Success(xiiLogInterface* pInterface, const xiiFormatString& string)
{
  LOG_LEVEL_FILTER(xiiLogMsgType::SuccessMsg);

  xiiStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, xiiLogMsgType::SuccessMsg, string.GetText(tmp));
}

void xiiLog::Info(xiiLogInterface* pInterface, const xiiFormatString& string)
{
  LOG_LEVEL_FILTER(xiiLogMsgType::InfoMsg);

  xiiStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, xiiLogMsgType::InfoMsg, string.GetText(tmp));
}

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)

void xiiLog::Dev(xiiLogInterface* pInterface, const xiiFormatString& string)
{
  LOG_LEVEL_FILTER(xiiLogMsgType::DevMsg);

  xiiStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, xiiLogMsgType::DevMsg, string.GetText(tmp));
}

#endif

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)

void xiiLog::Debug(xiiLogInterface* pInterface, const xiiFormatString& string)
{
  LOG_LEVEL_FILTER(xiiLogMsgType::DebugMsg);

  xiiStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, xiiLogMsgType::DebugMsg, string.GetText(tmp));
}

#endif

bool xiiLog::Flush(xiiUInt32 uiNumNewMsgThreshold, xiiTime timeIntervalThreshold, xiiLogInterface* pInterface /*= GetThreadLocalLogSystem()*/)
{
  if (pInterface == nullptr || pInterface->m_uiLoggedMsgsSinceFlush == 0) // if really nothing was logged, don't execute a flush
    return false;

  const xiiTime tNow = xiiTime::Now();

  if (pInterface->m_uiLoggedMsgsSinceFlush <= uiNumNewMsgThreshold && tNow - pInterface->m_LastFlushTime < timeIntervalThreshold)
    return false;

  BroadcastLoggingEvent(pInterface, xiiLogMsgType::Flush, nullptr);

  pInterface->m_uiLoggedMsgsSinceFlush = 0;
  pInterface->m_LastFlushTime          = tNow;

  return true;
}

XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_Log);
