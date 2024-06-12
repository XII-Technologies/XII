#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Implementation/Task.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Lock.h>

xiiTaskGroup::xiiTaskGroup()  = default;
xiiTaskGroup::~xiiTaskGroup() = default;

void xiiTaskGroup::WaitForFinish(xiiTaskGroupID group) const
{
  if (m_uiGroupCounter != group.m_uiGroupCounter)
    return;

  XII_LOCK(m_CondVarGroupFinished);

  while (m_uiGroupCounter == group.m_uiGroupCounter)
  {
    m_CondVarGroupFinished.UnlockWaitForSignalAndLock();
  }
}

void xiiTaskGroup::Reuse(xiiTaskPriority::Enum priority, xiiOnTaskGroupFinishedCallback callback)
{
  m_bInUse         = true;
  m_bStartedByUser = false;
  m_uiGroupCounter += 2; // even if it wraps around, it will never be zero, thus zero stays an invalid group counter
  m_Tasks.Clear();
  m_DependsOnGroups.Clear();
  m_OthersDependingOnMe.Clear();
  m_Priority           = priority;
  m_OnFinishedCallback = callback;
}

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
void xiiTaskGroup::DebugCheckTaskGroup(xiiTaskGroupID groupID, xiiMutex& mutex)
{
  XII_LOCK(mutex);

  const xiiTaskGroup* pGroup = groupID.m_pTaskGroup;

  XII_ASSERT_DEV(pGroup != nullptr, "TaskGroupID is invalid.");
  XII_ASSERT_DEV(pGroup->m_uiGroupCounter == groupID.m_uiGroupCounter, "The given TaskGroupID is not valid anymore.");
  XII_ASSERT_DEV(!pGroup->m_bStartedByUser, "The given TaskGroupID is already started, you cannot modify it anymore.");
  XII_ASSERT_DEV(pGroup->m_iNumActiveDependencies == 0, "Invalid active dependenices");
}
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskGroup);
