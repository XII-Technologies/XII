/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

struct xiiTime;
class xiiThread;

/// Contains general thread functions.
class XII_FOUNDATION_DLL xiiThreadUtils
{
public:
  /// Suspends execution of the current thread.
  static void YieldTimeSlice();

  /// Give resources to other hardware threads on the same processor. Does nothing if the processor has no hardware threads.
  static void YieldHardwareThread();

  /// Suspends the execution of the current thread for the given amount of time. (Precision may vary according to OS)
  static void Sleep(const xiiTime& duration); // [tested]

  /// Helper function to check if the current thread is the main thread (e.g. the thread which initialized the foundation library)
  static bool IsMainThread();

  /// Returns an identifier for the currently running thread.
  static xiiThreadID GetCurrentThreadID();

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ThreadUtils);

  /// Initialization functionality of the threading system (called by foundation startup and thus private)
  static void Initialize();
};
