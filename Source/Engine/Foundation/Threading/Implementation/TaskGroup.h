/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/ConditionVariable.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/SharedPtr.h>

/// \internal Represents the state of a group of tasks that can be waited on
class xiiTaskGroup
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTaskGroup);

public:
  xiiTaskGroup();
  ~xiiTaskGroup();

private:
  friend class xiiTaskSystem;

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  static void DebugCheckTaskGroup(xiiTaskGroupID groupID, xiiMutex& mutex);
#else
  XII_ALWAYS_INLINE static void DebugCheckTaskGroup(xiiTaskGroupID groupID, xiiMutex& mutex)
  {
    XII_IGNORE_UNUSED(groupID);
    XII_IGNORE_UNUSED(mutex);
  }
#endif

  /// Puts the calling thread to sleep until this group is fully finished.
  void WaitForFinish(xiiTaskGroupID group) const;
  void Reuse(xiiTaskPriority::Enum priority, xiiOnTaskGroupFinishedCallback callback);

  bool                                      m_bInUse           = true;
  bool                                      m_bStartedByUser   = false;
  xiiUInt16                                 m_uiTaskGroupIndex = 0xFFFF; // only there as a debugging aid
  xiiUInt32                                 m_uiGroupCounter   = 1;
  xiiHybridArray<xiiSharedPtr<xiiTask>, 16> m_Tasks;
  xiiHybridArray<xiiTaskGroupID, 4>         m_DependsOnGroups;
  xiiHybridArray<xiiTaskGroupID, 8>         m_OthersDependingOnMe;
  xiiAtomicInteger32                        m_iNumActiveDependencies;
  xiiAtomicInteger32                        m_iNumRemainingTasks;
  xiiOnTaskGroupFinishedCallback            m_OnFinishedCallback;
  xiiTaskPriority::Enum                     m_Priority = xiiTaskPriority::ThisFrame;
  mutable xiiConditionVariable              m_CondVarGroupFinished;
};
