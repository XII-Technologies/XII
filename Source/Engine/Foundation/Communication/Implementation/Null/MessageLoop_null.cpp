#include <Foundation/FoundationPCH.h>

#if XII_DISABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Communication/Implementation/Null/MessageLoop_null.h>
#  include <Foundation/Communication/IpcChannel.h>

xiiMessageLoop_null::xiiMessageLoop_null() {}

xiiMessageLoop_null::~xiiMessageLoop_null()
{
  StopUpdateThread();
}

void xiiMessageLoop_null::WakeUp()
{
  // nothing to do
}

bool xiiMessageLoop_null::WaitForMessages(xiiInt32 iTimeout, xiiIpcChannel* pFilter)
{
  // nothing to do

  if (iTimeout < 0)
  {
    // if timeout is 'indefinite' wait a little
    xiiThreadUtils::YieldTimeSlice();
  }

  return false;
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Null_MessageLoop_null);
