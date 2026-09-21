/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// Manages a lock (e.g. a mutex) and ensures that it is properly released as the lock object goes out of scope. The lock/unlock will
/// only be done if the boolean condition is satisfied at scope creation time.
template <typename T>
class xiiConditionalLock
{
public:
  XII_ALWAYS_INLINE explicit xiiConditionalLock(T& lock, bool bCondition) :
    m_lock(lock), m_bCondition(bCondition)
  {
    if (m_bCondition)
    {
      m_lock.Lock();
    }
  }

  XII_ALWAYS_INLINE ~xiiConditionalLock()
  {
    if (m_bCondition)
    {
      m_lock.Unlock();
    }
  }

private:
  xiiConditionalLock();
  xiiConditionalLock(const xiiConditionalLock<T>& rhs);
  void operator=(const xiiConditionalLock<T>& rhs);

  T&   m_lock;
  bool m_bCondition;
};
