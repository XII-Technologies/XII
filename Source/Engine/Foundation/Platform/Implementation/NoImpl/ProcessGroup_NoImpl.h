
///
/// Implements xiiProcessGroup by using xiiProcess
///

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/ProcessGroup.h>

struct xiiProcessGroupImpl
{
  XII_DECLARE_POD_TYPE();
};

xiiProcessGroup::xiiProcessGroup(xiiStringView sGroupName)
{
}

xiiProcessGroup::~xiiProcessGroup()
{
  TerminateAll().IgnoreResult();
}

xiiResult xiiProcessGroup::Launch(const xiiProcessOptions& opt)
{
  xiiProcess& process = m_Processes.ExpandAndGetRef();
  return process.Launch(opt);
}

xiiResult xiiProcessGroup::WaitToFinish(xiiTime timeout /*= xiiTime::MakeZero()*/)
{
  for (auto& process : m_Processes)
  {
    if (process.GetState() != xiiProcessState::Finished && process.WaitToFinish(timeout).Failed())
    {
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiProcessGroup::TerminateAll(xiiInt32 iForcedExitCode /*= -2*/)
{
  auto result = XII_SUCCESS;
  for (auto& process : m_Processes)
  {
    if (process.GetState() == xiiProcessState::Running && process.Terminate().Failed())
    {
      result = XII_FAILURE;
    }
  }

  return result;
}
