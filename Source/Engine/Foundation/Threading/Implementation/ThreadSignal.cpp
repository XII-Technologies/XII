/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/ThreadSignal.h>

xiiThreadSignal::xiiThreadSignal(Mode mode /*= Mode::AutoReset*/)
{
  m_Mode = mode;
}

xiiThreadSignal::~xiiThreadSignal() = default;

void xiiThreadSignal::WaitForSignal() const
{
  XII_LOCK(m_ConditionVariable);

  while (!m_bSignalState)
  {
    m_ConditionVariable.UnlockWaitForSignalAndLock();
  }

  if (m_Mode == Mode::AutoReset)
  {
    m_bSignalState = false;
  }
}

xiiThreadSignal::WaitResult xiiThreadSignal::WaitForSignal(xiiTime timeout) const
{
  XII_LOCK(m_ConditionVariable);

  const xiiTime tStart   = xiiTime::Now();
  xiiTime       tElapsed = xiiTime::MakeZero();

  while (!m_bSignalState)
  {
    if (m_ConditionVariable.UnlockWaitForSignalAndLock(timeout - tElapsed) == xiiConditionVariable::WaitResult::Timeout)
    {
      return WaitResult::Timeout;
    }

    tElapsed = xiiTime::Now() - tStart;
    if (tElapsed >= timeout)
    {
      return WaitResult::Timeout;
    }
  }

  if (m_Mode == Mode::AutoReset)
  {
    m_bSignalState = false;
  }

  return WaitResult::Signaled;
}

void xiiThreadSignal::RaiseSignal()
{
  {
    XII_LOCK(m_ConditionVariable);
    m_bSignalState = true;
  }

  if (m_Mode == Mode::AutoReset)
  {
    // with auto-reset there is no need to wake up more than one
    m_ConditionVariable.SignalOne();
  }
  else
  {
    m_ConditionVariable.SignalAll();
  }
}

void xiiThreadSignal::ClearSignal()
{
  XII_LOCK(m_ConditionVariable);
  m_bSignalState = false;
}

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ThreadSignal);
