#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Mutex.h>

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

template <xiiUInt32 a, xiiUInt32 b>
struct SameSize
{
  static_assert(a == b, "Critical section has incorrect size");
};

template <xiiUInt32 a, xiiUInt32 b>
struct SameAlignment
{
  static_assert(a == b, "Critical section has incorrect alignment");
};


xiiMutex::xiiMutex()
{
  SameSize<sizeof(xiiMutexHandle), sizeof(CRITICAL_SECTION)> check1;
  (void)check1;
  SameAlignment<alignof(xiiMutexHandle), alignof(CRITICAL_SECTION)> check2;
  (void)check2;
  InitializeCriticalSection((CRITICAL_SECTION*)&m_hHandle);
}

xiiMutex::~xiiMutex()
{
  DeleteCriticalSection((CRITICAL_SECTION*)&m_hHandle);
}
#endif


XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Win_Mutex_win);
