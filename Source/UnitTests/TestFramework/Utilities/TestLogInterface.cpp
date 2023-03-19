#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestLogInterface.h>

#include <TestFramework/Framework/TestFramework.h>

xiiTestLogInterface::~xiiTestLogInterface()
{
  for (const ExpectedMsg& msg : m_ExpectedMessages)
  {
    xiiInt32 count = msg.m_iCount;
    XII_TEST_BOOL_MSG(count == 0, "Message \"%s\" was logged %d times %s than expected.", msg.m_sMsgSubString.GetData(), count < 0 ? -count : count,
                      count < 0 ? "more" : "less");
  }
}

void xiiTestLogInterface::HandleLogMessage(const xiiLoggingEventData& le)
{
  {
    // in case this interface is used with xiiTestLogSystemScope to override the xiiGlobalLog (see xiiGlobalLog::SetGlobalLogOverride)
    // it must be thread-safe
    XII_LOCK(m_Mutex);

    for (ExpectedMsg& msg : m_ExpectedMessages)
    {
      if (msg.m_Type != xiiLogMsgType::All && le.m_EventType != msg.m_Type)
        continue;

      if (le.m_sText.FindSubString(msg.m_sMsgSubString))
      {
        --msg.m_iCount;

        // filter out error and warning messages entirely
        if (le.m_EventType >= xiiLogMsgType::ErrorMsg && le.m_EventType <= xiiLogMsgType::WarningMsg)
          return;

        // pass all other messages along to the parent log
        break;
      }
    }
  }

  if (m_pParentLog)
  {
    m_pParentLog->HandleLogMessage(le);
  }
}

void xiiTestLogInterface::ExpectMessage(const char* msg, xiiLogMsgType::Enum type /*= xiiLogMsgType::All*/, xiiInt32 count /*= 1*/)
{
  XII_LOCK(m_Mutex);

  // Do not allow initial count to be less than 1, but use signed int to keep track
  // of error messages that were encountered more often than expected.
  XII_ASSERT_DEV(count >= 1, "Message needs to be expected at least once");

  ExpectedMsg& em    = m_ExpectedMessages.ExpandAndGetRef();
  em.m_sMsgSubString = msg;
  em.m_iCount        = count;
  em.m_Type          = type;
}


XII_STATICLINK_FILE(TestFramework, TestFramework_Utilities_TestLogInterface);
