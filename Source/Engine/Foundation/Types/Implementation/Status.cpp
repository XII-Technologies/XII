#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Status.h>

void xiiResult::AssertSuccess(const char* szMsg /*= nullptr*/, const char* szDetails /*= nullptr*/) const
{
  if (Succeeded())
    return;

  if (szMsg)
  {
    XII_REPORT_FAILURE(szMsg, szDetails);
  }
  else
  {
    XII_REPORT_FAILURE("An operation failed unexpectedly.");
  }
}

xiiStatus::xiiStatus(const xiiFormatString& fmt) :
  m_Result(XII_FAILURE)
{
  xiiStringBuilder sMsg;
  m_sMessage = fmt.GetText(sMsg);
}

void xiiStatus::LogFailure(xiiLogInterface* pLog)
{
  if (Failed())
  {
    xiiLogInterface* pInterface = pLog ? pLog : xiiLog::GetThreadLocalLogSystem();
    xiiLog::Error(pInterface, "{0}", m_sMessage);
  }
}


XII_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_Status);
