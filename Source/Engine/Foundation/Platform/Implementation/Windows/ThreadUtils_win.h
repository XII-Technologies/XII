#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

// Windows implementation of thread helper functions

static DWORD g_uiMainThreadID = 0xFFFFFFFF;

void xiiThreadUtils::Initialize()
{
  g_uiMainThreadID = GetCurrentThreadId();
}

void xiiThreadUtils::YieldTimeSlice()
{
  ::Sleep(0);
}

void xiiThreadUtils::YieldHardwareThread()
{
  YieldProcessor();
}

void xiiThreadUtils::Sleep(const xiiTime& duration)
{
  ::Sleep((DWORD)duration.GetMilliseconds());
}

xiiThreadID xiiThreadUtils::GetCurrentThreadID()
{
  return ::GetCurrentThreadId();
}

bool xiiThreadUtils::IsMainThread()
{
  return GetCurrentThreadID() == g_uiMainThreadID;
}
