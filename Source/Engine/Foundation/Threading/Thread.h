/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/Threading/Implementation/OSThread.h>

// Warning: 'this' used in member initialization list (is fine here since it is just stored and not
// accessed in the constructor (so no operations on a not completely initialized object happen)

XII_WARNING_PUSH()
XII_WARNING_DISABLE_MSVC(4355)

#ifndef XII_THREAD_CLASS_ENTRY_POINT
#  error "Definition for xiiThreadClassEntryPoint is missing on this platform!"
#endif

XII_THREAD_CLASS_ENTRY_POINT;

struct xiiThreadEvent
{
  enum class Type
  {
    ThreadCreated,     ///< Called on the thread that creates the xiiThread instance (not the xiiThread itself).
    ThreadDestroyed,   ///< Called on the thread that destroys the xiiThread instance (not the xiiThread itself).
    StartingExecution, ///< Called on the xiiThread before the Run() method is executed.
    FinishedExecution, ///< Called on the xiiThread after the Run() method was executed.
    ClearThreadLocals, ///< Potentially called on the xiiThread (currently only for task system threads) at a time when plugins should clean up thread-local storage.
  };

  Type       m_Type;
  xiiThread* m_pThread = nullptr;
};

/// This class is the base class for platform independent long running threads
///
/// Used by deriving from this class and overriding the Run() method.
class XII_FOUNDATION_DLL xiiThread : public xiiOSThread
{
public:
  /// Returns the current xiiThread if the current platform thread is a xiiThread. Returns nullptr otherwise.
  static const xiiThread* GetCurrentThread();

  /// Describes the thread status
  enum xiiThreadStatus
  {
    Created = 0,
    Running,
    Finished
  };

  /// Initializes the runnable class
  xiiThread(xiiStringView sName = "xiiThread", xiiUInt32 uiStackSize = 128 * 1024);

  /// Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~xiiThread();

  /// Returns the thread status
  inline xiiThreadStatus GetThreadStatus() const { return m_ThreadStatus; }

  /// Helper function to determine if the thread is running
  inline bool IsRunning() const { return m_ThreadStatus == Running; }

  /// Returns the thread name
  inline xiiStringView GetThreadName() const { return m_sName; }

  /// These events inform about threads starting and finishing.
  ///
  /// The events are raised on the executing thread! That means thread-specific code may be executed during the event callback,
  /// e.g. to set up thread-local functionality.
  static xiiEvent<const xiiThreadEvent&, xiiMutex> s_ThreadEvents;

private:
  /// The run function can be used to implement a long running task in a thread in a platform independent way
  virtual xiiUInt32 Run() = 0;

  volatile xiiThreadStatus m_ThreadStatus = Created;

  xiiString m_sName;

  friend xiiUInt32 RunThread(xiiThread* pThread);
};

XII_WARNING_POP()
