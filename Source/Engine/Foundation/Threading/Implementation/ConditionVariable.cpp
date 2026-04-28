/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/ConditionVariable.h>

void xiiConditionVariable::Lock()
{
  m_Mutex.Lock();
  ++m_iLockCount;
}

xiiResult xiiConditionVariable::TryLock()
{
  if (m_Mutex.TryLock().Succeeded())
  {
    ++m_iLockCount;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiConditionVariable::Unlock()
{
  XII_ASSERT_DEV(m_iLockCount > 0, "Cannot unlock a thread-signal that was not locked before.");
  --m_iLockCount;
  m_Mutex.Unlock();
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/ConditionVariable_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Posix/ConditionVariable_posix.h>
#else
#  error "Unsupported Platform."
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ConditionVariable);
