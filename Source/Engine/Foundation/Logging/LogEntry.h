/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiLogMsgType);

/// A persistent log entry created from a xiiLoggingEventData.
/// Allows for a log event to survive for longer than just the event
/// and is reflected, allowing for it to be sent to remote targets.
struct XII_FOUNDATION_DLL xiiLogEntry
{
  xiiLogEntry();
  xiiLogEntry(const xiiLoggingEventData& le);

  xiiString              m_sMsg;
  xiiString              m_sTag;
  xiiEnum<xiiLogMsgType> m_Type;
  xiiUInt8               m_uiIndentation = 0;
  double                 m_fSeconds      = 0;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiLogEntry);

/// A log interface implementation that converts a log event into
/// a xiiLogEntry and calls a delegate with it.
///
/// A typical use case is to re-route and store log messages in a scope:
/// \code{.cpp}
///   {
///     xiiLogEntryDelegate logger(([&array](xiiLogEntry& entry) -> void
///     {
///       array.PushBack(std::move(entry));
///     }));
///     xiiLogSystemScope logScope(&logger);
///     *log something*
///   }
/// \endcode
class XII_FOUNDATION_DLL xiiLogEntryDelegate : public xiiLogInterface
{
public:
  using Callback = xiiDelegate<void(xiiLogEntry&)>;

  /// Log events will be delegated to the given callback.
  xiiLogEntryDelegate(Callback callback, xiiLogMsgType::Enum logLevel = xiiLogMsgType::All);
  virtual void HandleLogMessage(const xiiLoggingEventData& le) override;

private:
  Callback m_Callback;
};
