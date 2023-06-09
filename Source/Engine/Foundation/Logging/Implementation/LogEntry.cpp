#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/LogEntry.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiLogMsgType, 1)
  XII_BITFLAGS_CONSTANTS(xiiLogMsgType::Flush, xiiLogMsgType::BeginGroup, xiiLogMsgType::EndGroup, xiiLogMsgType::None)
  XII_BITFLAGS_CONSTANTS(xiiLogMsgType::ErrorMsg, xiiLogMsgType::SeriousWarningMsg, xiiLogMsgType::WarningMsg, xiiLogMsgType::SuccessMsg, xiiLogMsgType::InfoMsg, xiiLogMsgType::DevMsg, xiiLogMsgType::DebugMsg, xiiLogMsgType::All)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiLogEntry, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiLogEntry>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Msg", m_sMsg),
    XII_MEMBER_PROPERTY("Tag", m_sTag),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiLogMsgType, m_Type),
    XII_MEMBER_PROPERTY("Indentation", m_uiIndentation),
    XII_MEMBER_PROPERTY("Time", m_fSeconds),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiLogEntry::xiiLogEntry() = default;

xiiLogEntry::xiiLogEntry(const xiiLoggingEventData& le)
{
  m_sMsg          = le.m_sText;
  m_sTag          = le.m_sTag;
  m_Type          = le.m_EventType;
  m_uiIndentation = le.m_uiIndentation;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = le.m_fSeconds;
#else
  m_fSeconds = 0.0f;
#endif
}

xiiLogEntryDelegate::xiiLogEntryDelegate(Callback callback, xiiLogMsgType::Enum logLevel) :
  m_Callback(callback)
{
  SetLogLevel(logLevel);
}

void xiiLogEntryDelegate::HandleLogMessage(const xiiLoggingEventData& le)
{
  xiiLogEntry e(le);
  m_Callback(e);
}

XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_LogEntry);
