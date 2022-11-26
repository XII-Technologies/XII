#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

static_assert(sizeof(CONDITION_VARIABLE) == sizeof(xiiConditionVariableHandle), "not equal!");

xiiConditionVariable::xiiConditionVariable()
{
  InitializeConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable));
}

xiiConditionVariable::~xiiConditionVariable()
{
  XII_ASSERT_DEV(m_iLockCount == 0, "Thread-signal must be unlocked during destruction.");
}

void xiiConditionVariable::SignalOne()
{
  WakeConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable));
}

void xiiConditionVariable::SignalAll()
{
  WakeAllConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable));
}

void xiiConditionVariable::UnlockWaitForSignalAndLock() const
{
  XII_ASSERT_DEV(m_iLockCount > 0, "xiiConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  SleepConditionVariableCS(
    reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable), (CRITICAL_SECTION*)&m_Mutex.GetMutexHandle(), INFINITE);
}

xiiConditionVariable::WaitResult xiiConditionVariable::UnlockWaitForSignalAndLock(xiiTime timeout) const
{
  XII_ASSERT_DEV(m_iLockCount > 0, "xiiConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  // inside the lock
  --m_iLockCount;
  DWORD result = SleepConditionVariableCS(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable),
                                          (CRITICAL_SECTION*)&m_Mutex.GetMutexHandle(), static_cast<DWORD>(timeout.GetMilliseconds()));

  if (result == WAIT_TIMEOUT)
  {
    // inside the lock at this point
    ++m_iLockCount;
    return WaitResult::Timeout;
  }

  // inside the lock
  ++m_iLockCount;
  return WaitResult::Signaled;
}
