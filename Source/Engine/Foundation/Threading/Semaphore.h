/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// A semaphore is used to synchronize threads, similar to a mutex (see xiiMutex).
///
/// There are three main differences to a mutex:
/// 1. The thread that acquires a token from a semaphore and the one that returns a token, don't have to be the same.
/// 2. A semaphore can be locked up to N times by multiple threads in parallel. So it can be used to implement constructs like single-writer / multiple/readers.
/// 3. A 'named' semaphore can be opened by other processes as well, which allows you to implement the basics for inter process communication (IPC).
///
/// Semaphores are quite a bit slower than mutexes (10x or so), so don't use them unless you need the added flexibility.
///
/// \sa xiiMutex, xiiConditionVariable
class XII_FOUNDATION_DLL xiiSemaphore
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSemaphore);

public:
  xiiSemaphore();
  ~xiiSemaphore();

  /// Attempts to create a new semaphore with an initial number of available tokens.
  ///
  /// If sSharedName is a non-empty string, a 'named' semaphore is created, which can be opened on other processes as well.
  ///
  /// This call can fail, if a semaphore with the same name already exists. Use xiiSemaphore::Open() instead.
  xiiResult Create(xiiUInt32 uiInitialTokenCount = 0, xiiStringView sSharedName = xiiStringView());

  /// Attempts to open an existing named semaphore.
  ///
  /// Fails if no such semaphore exists.
  xiiResult Open(xiiStringView sSharedName);

  /// Waits until a token is available and acquires it.
  ///
  /// Use TryAcquireToken() to prevent blocking if desired.
  /// AcquireToken() and ReturnToken() may be called from different threads.
  void AcquireToken();

  /// Returns a single token. If another thread is currently waiting for a token, this will wake it up.
  ///
  /// AcquireToken() and ReturnToken() may be called from different threads.
  void ReturnToken();

  /// Same as AcquireToken() but returns immediately with XII_FAILURE, if currently not tokens are available.
  xiiResult TryAcquireToken(xiiTime timeout = xiiTime::MakeZero());

private:
  xiiSemaphoreHandle m_hSemaphore = {};
};
