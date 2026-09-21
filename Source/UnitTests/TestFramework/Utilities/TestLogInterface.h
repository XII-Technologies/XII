/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Foundation/Logging/Log.h>
#include <TestFramework/TestFrameworkDLL.h>

/// A xiiLogInterface that expects and handles error messages during test runs. Can be
/// used to ensure that expected error messages are produced by the tested functionality.
/// Expected error messages are not passed on and do not cause tests to fail.
class XII_TEST_DLL xiiTestLogInterface : public xiiLogInterface
{
public:
  xiiTestLogInterface() = default;
  ~xiiTestLogInterface();
  virtual void HandleLogMessage(const xiiLoggingEventData& le) override;

  /// Add expected message. Will fail the test when the expected message is not
  /// encountered. Can take an optional count, if messages are expected multiple times
  void ExpectMessage(const char* szMsg, xiiLogMsgType::Enum type = xiiLogMsgType::All, xiiInt32 iCount = 1);

  /// Set the log interface that unhandled messages are forwarded to.
  void SetParentLog(xiiLogInterface* pInterface) { m_pParentLog = pInterface; }

private:
  xiiLogInterface* m_pParentLog = nullptr;

  struct ExpectedMsg
  {
    xiiInt32            m_iCount = 0;
    xiiString           m_sMsgSubString;
    xiiLogMsgType::Enum m_Type = xiiLogMsgType::All;
  };

  mutable xiiMutex               m_Mutex;
  xiiHybridArray<ExpectedMsg, 8> m_ExpectedMessages;
};

/// A class that sets a custom xiiTestLogInterface as the thread local default log system,
/// and resets the previous system when it goes out of scope. The test version passes the previous
/// xiiLogInterface on to the xiiTestLogInterface to enable passing on unhandled messages.
///
/// If bCatchMessagesGlobally is false, the system only intercepts messages on the current thread.
/// If bCatchMessagesGlobally is true, it will also intercept messages from other threads, as long as they
/// go through xiiGlobalLog. See xiiGlobalLog::SetGlobalLogOverride().
class XII_TEST_DLL xiiTestLogSystemScope : public xiiLogSystemScope
{
public:
  explicit xiiTestLogSystemScope(xiiTestLogInterface* pInterface, bool bCatchMessagesGlobally = false) :
    xiiLogSystemScope(pInterface)
  {
    m_bCatchMessagesGlobally = bCatchMessagesGlobally;
    pInterface->SetParentLog(m_pPrevious);

    if (m_bCatchMessagesGlobally)
    {
      xiiGlobalLog::SetGlobalLogOverride(pInterface);
    }
  }

  ~xiiTestLogSystemScope()
  {
    if (m_bCatchMessagesGlobally)
    {
      xiiGlobalLog::SetGlobalLogOverride(nullptr);
    }
  }

private:
  bool m_bCatchMessagesGlobally = false;
};
