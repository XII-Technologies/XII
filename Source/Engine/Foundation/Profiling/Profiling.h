/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Time.h>

class xiiStreamWriter;
class xiiThread;

/// This class encapsulates a profiling scope.
///
/// The constructor creates a new scope in the profiling system and the destructor pops the scope.
/// You shouldn't need to use this directly, just use the macro XII_PROFILE_SCOPE provided below.
class XII_FOUNDATION_DLL xiiProfilingScope
{
public:
  xiiProfilingScope(xiiStringView sName, xiiStringView sFunctionName, xiiTime timeout);
  ~xiiProfilingScope();

protected:
  xiiStringView m_sName;
  xiiStringView m_sFunction;
  xiiTime       m_BeginTime;
  xiiTime       m_Timeout;
};

/// This class implements a profiling scope similar to xiiProfilingScope, but with additional sub-scopes which can be added easily without
/// introducing actual C++ scopes.
///
/// The constructor pushes one surrounding scope on the stack and then a nested scope as the first section.
/// The function StartNextSection() will end the nested scope and start a new inner scope.
/// This allows to end one scope and start a new one, without having to add actual C++ scopes for starting/stopping profiling scopes.
///
/// You shouldn't need to use this directly, just use the macro XII_PROFILE_LIST_SCOPE provided below.
class xiiProfilingListScope
{
public:
  XII_FOUNDATION_DLL xiiProfilingListScope(xiiStringView sListName, xiiStringView sFirstSectionName, xiiStringView sFunctionName);
  XII_FOUNDATION_DLL ~xiiProfilingListScope();

  XII_FOUNDATION_DLL static void StartNextSection(xiiStringView sNextSectionName);

protected:
  static thread_local xiiProfilingListScope* s_pCurrentList;

  xiiProfilingListScope* m_pPreviousList;

  xiiStringView m_sListName;
  xiiStringView m_sListFunction;
  xiiTime       m_ListBeginTime;

  xiiStringView m_sCurSectionName;
  xiiTime       m_CurSectionBeginTime;
};

/// Helper functionality of the profiling system.
class XII_FOUNDATION_DLL xiiProfilingSystem
{
public:
  struct ThreadInfo
  {
    xiiUInt64 m_uiThreadId;
    xiiString m_sName;
  };

  struct CPUScope
  {
    XII_DECLARE_POD_TYPE();

    static constexpr xiiUInt32 NAME_SIZE = 40;

    xiiStringView m_sFunctionName;
    xiiTime       m_BeginTime;
    xiiTime       m_EndTime;
    char          m_szName[NAME_SIZE];
  };

  struct CPUScopesBufferFlat
  {
    xiiDynamicArray<CPUScope> m_Data;
    xiiUInt64                 m_uiThreadId = 0;
  };

  /// Helper struct to hold GPU profiling data.
  struct GPUScope
  {
    XII_DECLARE_POD_TYPE();

    static constexpr xiiUInt32 NAME_SIZE = 48;

    xiiTime m_BeginTime;
    xiiTime m_EndTime;
    char    m_szName[NAME_SIZE];
  };

  struct XII_FOUNDATION_DLL ProfilingData
  {
    xiiUInt32      m_uiFramesThreadID   = 0;
    xiiUInt32      m_uiProcessSortIndex = 0;
    xiiOsProcessID m_uiProcessID        = 0;

    xiiHybridArray<ThreadInfo, 16> m_ThreadInfos;

    xiiDynamicArray<CPUScopesBufferFlat> m_AllEventBuffers;

    xiiUInt64                m_uiFrameCount = 0;
    xiiDynamicArray<xiiTime> m_FrameStartTimes;

    xiiDynamicArray<xiiDynamicArray<GPUScope>> m_GPUScopes;

    /// Writes profiling data as JSON to the output stream.
    xiiResult Write(xiiStreamWriter& ref_outputStream) const;

    void Clear();

    /// Concatenates all given ProfilingData instances into one merge struct
    static void Merge(ProfilingData& out_merged, xiiArrayPtr<const ProfilingData*> inputs);
  };

public:
  static void Clear();

  static void Capture(xiiProfilingSystem::ProfilingData& out_capture, bool bClearAfterCapture = false);

  /// Scopes are discarded if their duration is shorter than the specified threshold. Default is 0.1ms.
  static void SetDiscardThreshold(xiiTime threshold);

  using ScopeTimeoutDelegate = xiiDelegate<void(xiiStringView sName, xiiStringView sFunctionName, xiiTime duration)>;

  /// Sets a callback that is triggered when a profiling scope takes longer than desired.
  static void SetScopeTimeoutCallback(ScopeTimeoutDelegate callback);

  /// Should be called once per frame to capture the timestamp of the new frame.
  static void StartNewFrame();

  /// Adds a new scoped event for the calling thread in the profiling system
  static void AddCPUScope(xiiStringView sName, xiiStringView sFunctionName, xiiTime beginTime, xiiTime endTime, xiiTime scopeTimeout);

  /// Get current frame counter
  static xiiUInt64 GetFrameCount();

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ProfilingSystem);
  friend xiiUInt32 RunThread(xiiThread* pThread);

  static void Initialize();
  /// Removes profiling data of dead threads.
  static void Reset();

  /// Sets the name of the current thread.
  static void SetThreadName(xiiStringView sThreadName);
  /// Removes the current thread from the profiling system.
  ///  Needs to be called before the thread exits to be able to release profiling memory of dead threads on Reset.
  static void RemoveThread();

public:
  /// Initialized internal data structures for GPU profiling data. Needs to be called before adding any data.
  static void InitializeGPUData(xiiUInt32 uiGpuCount = 1);

  /// Adds a GPU profiling scope in the internal event ringbuffer.
  static void AddGPUScope(xiiStringView sName, xiiTime beginTime, xiiTime endTime, xiiUInt32 uiGpuIndex = 0);
};

#if XII_ENABLED(XII_USE_PROFILING) || defined(XII_DOCS)

/// Profiles the current scope using the given name.
///
/// It is allowed to nest XII_PROFILE_SCOPE, also with XII_PROFILE_LIST_SCOPE. However XII_PROFILE_SCOPE should start and end within the same list scope
/// section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa xiiProfilingScope
/// \sa XII_PROFILE_LIST_SCOPE
#  define XII_PROFILE_SCOPE(szScopeName) \
    xiiProfilingScope XII_PP_CONCAT(_xiiProfilingScope, XII_SOURCE_LINE)(szScopeName, XII_SOURCE_FUNCTION, xiiTime::MakeZero())

/// Same as XII_PROFILE_SCOPE but if the scope takes longer than 'Timeout', the xiiProfilingSystem's timeout callback is executed.
///
/// This can be used to log an error or save a callstack, etc. when a scope exceeds an expected amount of time.
///
/// \sa xiiProfilingSystem::SetScopeTimeoutCallback()
#  define XII_PROFILE_SCOPE_WITH_TIMEOUT(szScopeName, Timeout) xiiProfilingScope XII_PP_CONCAT(_xiiProfilingScope, XII_SOURCE_LINE)(szScopeName, XII_SOURCE_FUNCTION, Timeout)

/// Profiles the current scope using the given name as the overall list scope name and the section name for the first section in the list.
///
/// Use XII_PROFILE_LIST_NEXT_SECTION to start a new section in the list scope.
///
/// It is allowed to nest XII_PROFILE_SCOPE, also with XII_PROFILE_LIST_SCOPE. However XII_PROFILE_SCOPE should start and end within the same list scope
/// section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa xiiProfilingListScope
/// \sa XII_PROFILE_LIST_NEXT_SECTION
#  define XII_PROFILE_LIST_SCOPE(szListName, szFirstSectionName) \
    xiiProfilingListScope XII_PP_CONCAT(_xiiProfilingScope, XII_SOURCE_LINE)(szListName, szFirstSectionName, XII_SOURCE_FUNCTION)

/// Starts a new section in a XII_PROFILE_LIST_SCOPE
///
/// \sa xiiProfilingListScope
/// \sa XII_PROFILE_LIST_SCOPE
#  define XII_PROFILE_LIST_NEXT_SECTION(szNextSectionName) xiiProfilingListScope::StartNextSection(szNextSectionName)

#else

#  define XII_PROFILE_SCOPE(Name) /*empty*/

#  define XII_PROFILE_SCOPE_WITH_TIMEOUT(szScopeName, Timeout) /*empty*/

#  define XII_PROFILE_LIST_SCOPE(szListName, szFirstSectionName) /*empty*/

#  define XII_PROFILE_LIST_NEXT_SECTION(szNextSectionName) /*empty*/

#endif
