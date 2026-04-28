/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

xiiConditionVariable::xiiConditionVariable()
{
  pthread_cond_init(&m_Data.m_ConditionVariable, nullptr);
}

xiiConditionVariable::~xiiConditionVariable()
{
  XII_ASSERT_DEV(m_iLockCount == 0, "Thread-signal must be unlocked during destruction.");

  pthread_cond_destroy(&m_Data.m_ConditionVariable);
}

void xiiConditionVariable::SignalOne()
{
  pthread_cond_signal(&m_Data.m_ConditionVariable);
}

void xiiConditionVariable::SignalAll()
{
  pthread_cond_broadcast(&m_Data.m_ConditionVariable);
}

void xiiConditionVariable::UnlockWaitForSignalAndLock() const
{
  XII_ASSERT_DEV(m_iLockCount > 0, "xiiConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  pthread_cond_wait(&m_Data.m_ConditionVariable, &m_Mutex.GetMutexHandle());
}

xiiConditionVariable::WaitResult xiiConditionVariable::UnlockWaitForSignalAndLock(xiiTime timeout) const
{
  XII_ASSERT_DEV(m_iLockCount > 0, "xiiConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  // inside the lock
  --m_iLockCount;

  timeval now;
  gettimeofday(&now, nullptr);

  // pthread_cond_timedwait needs an absolute time value, so compute it from the current time.
  struct timespec timeToWait;

  const xiiInt64 iNanoSecondsPerSecond      = 1000000000LL;
  const xiiInt64 iMicroSecondsPerNanoSecond = 1000LL;

  xiiInt64 endTime = now.tv_sec * iNanoSecondsPerSecond + now.tv_usec * iMicroSecondsPerNanoSecond + static_cast<xiiInt64>(timeout.GetNanoseconds());

  timeToWait.tv_sec  = endTime / iNanoSecondsPerSecond;
  timeToWait.tv_nsec = endTime % iNanoSecondsPerSecond;

  if (pthread_cond_timedwait(&m_Data.m_ConditionVariable, &m_Mutex.GetMutexHandle(), &timeToWait) == ETIMEDOUT)
  {
    // inside the lock
    ++m_iLockCount;
    return WaitResult::Timeout;
  }

  // inside the lock
  ++m_iLockCount;
  return WaitResult::Signaled;
}
