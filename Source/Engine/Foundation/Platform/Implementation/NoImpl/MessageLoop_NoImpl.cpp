#include <Foundation/FoundationPCH.h>

#if XII_DISABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Communication/Implementation/Null/MessageLoop_NoImpl.h>
#  include <Foundation/Communication/IpcChannel.h>

xiiMessageLoop_NoImpl::xiiMessageLoop_NoImpl() {}

xiiMessageLoop_NoImpl::~xiiMessageLoop_NoImpl()
{
  StopUpdateThread();
}

void xiiMessageLoop_NoImpl::WakeUp()
{
  // nothing to do
}

bool xiiMessageLoop_NoImpl::WaitForMessages(xiiInt32 iTimeout, xiiIpcChannel* pFilter)
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

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Null_MessageLoop_NoImpl);
