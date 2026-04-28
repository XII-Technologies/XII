/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Platform/Implementation/NoImpl/MessageLoop_NoImpl.h>

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
  XII_IGNORE_UNUSED(pFilter);

  // nothing to do

  if (iTimeout < 0)
  {
    // If timeout is 'indefinite' wait a little.
    xiiThreadUtils::YieldTimeSlice();
  }

  return false;
}

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Null_MessageLoop_NoImpl);
