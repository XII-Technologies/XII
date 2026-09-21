/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>

namespace xiiLogWriter
{

  /// A log writer that writes out log messages to an HTML file.
  ///
  /// Create an instance of this class, register the LogMessageHandler at xiiLog and pass the pointer
  /// to the instance as the pPassThrough argument to it.
  class XII_FOUNDATION_DLL HTML
  {
  public:
    ~HTML();

    /// Register this at xiiLog to write all log messages to an HTML file.
    void LogMessageHandler(const xiiLoggingEventData& eventData);

    /// Opens the given file for writing the log. From now on all incoming log messages are written into it.
    void BeginLog(xiiStringView sFile, xiiStringView sAppTitle);

    /// Closes the HTML file and stops logging the incoming message.
    void EndLog();

    /// Returns the name of the log-file that was really opened. Might be slightly different than what was given to BeginLog, to allow parallel
    /// execution of the same application.
    const xiiFileWriter& GetOpenedLogFile() const;

    /// Allows to indicate in what form timestamps should be added to log messages.
    void SetTimestampMode(xiiLog::TimestampMode mode);

  private:
    void WriteString(xiiStringView sText, xiiUInt32 uiColor);

    xiiFileWriter m_File;

    xiiLog::TimestampMode m_TimestampMode = xiiLog::TimestampMode::None;
  };
} // namespace xiiLogWriter
