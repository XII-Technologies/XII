/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// Provides a simple mechanism for mutual exclusion to prevent multiple threads from accessing a shared resource simultaneously.
///
/// This can be used to protect code that is not thread-safe against race conditions.
/// To ensure that mutexes are always properly released, use the xiiLock class or XII_LOCK macro.
///
/// \sa xiiSemaphore, xiiConditionVariable
class XII_FOUNDATION_DLL xiiMutex
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiMutex);

public:
  xiiMutex();
  ~xiiMutex();

  /// Acquires an exclusive lock for this mutex object
  void Lock();

  /// Attempts to acquire an exclusive lock for this mutex object. Returns true on success.
  ///
  /// If the mutex is already acquired by another thread, the function returns immediately and returns false.
  xiiResult TryLock();

  /// Releases a lock that has been previously acquired
  void Unlock();

  /// Returns true, if the mutex is currently acquired. Can be used to assert that a lock was entered.
  ///
  /// Obviously, this check is not thread-safe and should not be used to check whether a mutex could be locked without blocking.
  /// Use TryLock for that instead.
  XII_ALWAYS_INLINE bool IsLocked() const { return m_iLockCount > 0; }

  xiiMutexHandle& GetMutexHandle() { return m_hHandle; }

private:
  xiiMutexHandle m_hHandle;
  xiiInt32       m_iLockCount = 0;
};

/// A dummy mutex that does no locking.
///
/// Used when a mutex object needs to be passed to some code (such as allocators), but thread-synchronization
/// is actually not necessary.
class XII_FOUNDATION_DLL xiiNoMutex
{
public:
  /// Implements the 'Acquire' interface function, but does nothing.
  XII_ALWAYS_INLINE void Lock() {}

  /// Implements the 'TryLock' interface function, but does nothing.
  XII_ALWAYS_INLINE xiiResult TryLock() { return XII_SUCCESS; }

  /// Implements the 'Release' interface function, but does nothing.
  XII_ALWAYS_INLINE void Unlock() {}

  XII_ALWAYS_INLINE bool IsLocked() const { return false; }
};

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/Mutex_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Threading/Implementation/Posix/Mutex_posix.h>
#else
#  error "Mutex is not implemented on current platform"
#endif
