#include <Foundation/FoundationPCH.h>

#if XII_DISABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Communication/Implementation/Mobile/MessageLoop_mobile.h>
#  include <Foundation/Communication/IpcChannel.h>

xiiMessageLoop_mobile::xiiMessageLoop_mobile() {}

xiiMessageLoop_mobile::~xiiMessageLoop_mobile()
{
  StopUpdateThread();
}

void xiiMessageLoop_mobile::WakeUp()
{
  // nothing to do
}

bool xiiMessageLoop_mobile::WaitForMessages(xiiInt32 iTimeout, xiiIpcChannel* pFilter)
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



XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Mobile_MessageLoop_mobile);
