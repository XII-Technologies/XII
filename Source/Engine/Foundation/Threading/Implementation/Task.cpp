#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/Task.h>

xiiTask::xiiTask()  = default;
xiiTask::~xiiTask() = default;

void xiiTask::Reset()
{
  m_iRemainingRuns    = (int)xiiMath::Max(1u, m_uiMultiplicity);
  m_bCancelExecution  = false;
  m_bTaskIsScheduled  = false;
  m_bUsesMultiplicity = m_uiMultiplicity > 0;
}

void xiiTask::ConfigureTask(xiiStringView sTaskName, xiiTaskNesting nestingMode, xiiOnTaskFinishedCallback callback /*= xiiOnTaskFinishedCallback()*/)
{
  XII_ASSERT_DEV(IsTaskFinished(), "This function must be called before the task is started.");

  m_sTaskName      = sTaskName;
  m_NestingMode    = nestingMode;
  m_OnTaskFinished = callback;
}

void xiiTask::SetMultiplicity(xiiUInt32 uiMultiplicity)
{
  m_uiMultiplicity    = uiMultiplicity;
  m_bUsesMultiplicity = m_uiMultiplicity > 0;
}

void xiiTask::Run(xiiUInt32 uiInvocation)
{
  // actually this should not be possible to happen
  if (m_iRemainingRuns == 0 || m_bCancelExecution)
  {
    m_iRemainingRuns = 0;
    return;
  }

  {
    xiiStringBuilder scopeName = m_sTaskName;

    if (m_bUsesMultiplicity)
      scopeName.AppendFormat("-{}", uiInvocation);

    XII_PROFILE_SCOPE(scopeName.GetData());

    if (m_bUsesMultiplicity)
    {
      ExecuteWithMultiplicity(uiInvocation);
    }
    else
    {
      Execute();
    }
  }

  m_iRemainingRuns.Decrement();
}

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Task);
