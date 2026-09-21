/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// Implementation of a thread.
///
/// Since the thread class needs a platform specific entry-point it is usually
/// recommended to use the xiiThread class instead as the base for long running threads.
class XII_FOUNDATION_DLL xiiOSThread
{
public:
  /// Initializes the thread instance (e.g. thread creation etc.)
  ///
  /// Note that the thread won't start execution until Start() is called. Please note that szName must be valid until Start() has been called!
  xiiOSThread(xiiOSThreadEntryPoint threadEntryPoint, void* pUserData = nullptr, xiiStringView sName = "xiiOSThread", xiiUInt32 uiStackSize = 128 * 1024);

  /// Destructor.
  virtual ~xiiOSThread();

  /// Starts the thread
  void Start(); // [tested]

  /// Waits in the calling thread until the thread has finished execution (e.g. returned from the thread function)
  void Join(); // [tested]

  /// Returns the thread ID of the thread object, may be used in comparison operations with xiiThreadUtils::GetCurrentThreadID() for
  /// example.
  const xiiThreadID& GetThreadID() const { return m_ThreadID; }

  /// Returns how many xiiOSThreads are currently active.
  static xiiInt32 GetThreadCount() { return s_iThreadCount; }

protected:
  xiiThreadHandle m_hHandle;
  xiiThreadID     m_ThreadID;

  xiiOSThreadEntryPoint m_EntryPoint;

  void* m_pUserData;

  xiiString m_sName;

  xiiUInt32 m_uiStackSize;

private:
  /// Stores how many xiiOSThread are currently active.
  static xiiAtomicInteger32 s_iThreadCount;

  XII_DISALLOW_COPY_AND_ASSIGN(xiiOSThread);
};
