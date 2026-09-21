/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// Manages a lock (e.g. a mutex) and ensures that it is properly released as the lock object goes out of scope.
template <typename T>
class xiiLock
{
public:
  XII_ALWAYS_INLINE explicit xiiLock(T& ref_lock) :
    m_Lock(ref_lock)
  {
    m_Lock.Lock();
  }

  XII_ALWAYS_INLINE ~xiiLock() { m_Lock.Unlock(); }

private:
  xiiLock();
  xiiLock(const xiiLock<T>& rhs);
  void operator=(const xiiLock<T>& rhs);

  T& m_Lock;
};

/// Shortcut for xiiLock<Type> l(lock)
#define XII_LOCK(lock) xiiLock<decltype(lock)> XII_PP_CONCAT(l_, XII_SOURCE_LINE)(lock)
