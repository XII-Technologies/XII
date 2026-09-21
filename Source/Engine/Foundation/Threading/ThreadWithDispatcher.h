/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Thread.h>

/// This class is the base class for threads which need dispatching of calls.
///
/// Used by deriving from this class and overriding the Run() method. Call DispatchQueue() regurarely so that the collected messages can be
/// dispatched.
class XII_FOUNDATION_DLL xiiThreadWithDispatcher : public xiiThread
{
public:
  using DispatchFunction = xiiDelegate<void(), 128>;

  /// Initializes the runnable class
  xiiThreadWithDispatcher(xiiStringView sName = "xiiThreadWithDispatcher", xiiUInt32 uiStackSize = 128 * 1024);

  /// Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~xiiThreadWithDispatcher();

  /// Use this to enqueue a function call to the given delegate at some later point running in the given thread context.
  void Dispatch(DispatchFunction&& delegate);

protected:
  /// Needs to be called by derived thread implementations to dispatch the function calls.
  void DispatchQueue();

private:
  /// The run function can be used to implement a long running task in a thread in a platform independent way
  virtual xiiUInt32 Run() = 0;

  xiiDynamicArray<DispatchFunction> m_ActiveQueue;
  xiiDynamicArray<DispatchFunction> m_CurrentlyBeingDispatchedQueue;

  xiiMutex m_QueueMutex;
};
