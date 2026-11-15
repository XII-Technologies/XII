/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Implementation/NetworkIOInternal.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Lock.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace xiiNetworkIOInternal
{
  Registration* Worker::Find(xiiNetworkIORegistration handle)
  {
    if (handle.m_uiContextId != m_pRuntime->m_uiContextId || handle.m_uiWorker != m_uiIndex || handle.m_uiSlot >= m_Registrations.GetCount())
    {
      return nullptr;
    }
    Registration& registration = m_Registrations[handle.m_uiSlot];
    return registration.m_bActive && registration.m_Handle == handle ? &registration : nullptr;
  }

  Operation* Worker::Find(xiiNetworkIOOperation handle)
  {
    if (handle.m_uiContextId != m_pRuntime->m_uiContextId || handle.m_uiWorker != m_uiIndex || handle.m_uiSlot >= m_pRuntime->m_Operations.GetCount())
    {
      return nullptr;
    }
    XII_LOCK(m_pRuntime->m_PoolMutex);
    Operation& operation = m_pRuntime->m_Operations[handle.m_uiSlot];
    return operation.m_bInUse && operation.m_pWorker == this && operation.m_uiGeneration == handle.m_uiGeneration && !operation.m_bCompleted ? &operation : nullptr;
  }

  void Worker::Ready(Registration& registration, bool bWake)
  {
    if (!registration.m_bActive || registration.m_bReadyQueued)
    {
      return;
    }
    registration.m_bReadyQueued   = true;
    registration.m_pReadyPrevious = m_pReadyTail;
    registration.m_pReadyNext     = nullptr;
    if (m_pReadyTail != nullptr)
    {
      m_pReadyTail->m_pReadyNext = &registration;
    }
    else
    {
      m_pReadyHead = &registration;
    }
    m_pReadyTail = &registration;
    ++m_uiReadyCount;
    XII_ASSERT_DEV(m_uiReadyCount <= m_Registrations.GetCount(), "A registration has at most one intrusive ready-queue entry.");
    if (bWake)
    {
      Wake(*this);
    }
  }

  void Worker::Unready(Registration& registration)
  {
    if (!registration.m_bReadyQueued)
    {
      return;
    }
    if (registration.m_pReadyPrevious != nullptr)
    {
      registration.m_pReadyPrevious->m_pReadyNext = registration.m_pReadyNext;
    }
    else
    {
      m_pReadyHead = registration.m_pReadyNext;
    }
    if (registration.m_pReadyNext != nullptr)
    {
      registration.m_pReadyNext->m_pReadyPrevious = registration.m_pReadyPrevious;
    }
    else
    {
      m_pReadyTail = registration.m_pReadyPrevious;
    }
    registration.m_pReadyPrevious = nullptr;
    registration.m_pReadyNext     = nullptr;
    registration.m_bReadyQueued   = false;
    --m_uiReadyCount;
  }

  void Worker::CloseIfDrained(Registration& registration)
  {
    if (!registration.m_bActive || !registration.m_bClosing || registration.m_uiPendingCount != 0)
    {
      return;
    }
    Unready(registration);
    Remove(*this, registration);
    registration.GetSocket().Close().IgnoreResult();
    registration.m_bActive      = false;
    registration.m_bReadyQueued = false;
    m_NativeTokens.Remove(registration.m_uiNativeToken);
    m_FreeRegistrations.PushBack(registration.m_Handle.m_uiSlot);
    --m_Statistics.m_uiRegisteredSockets;
    XII_LOCK(m_pRuntime->m_PoolMutex);
    --m_pRuntime->m_uiRegisteredCount;
  }

  void Worker::Complete(Operation& operation, xiiNetworkError error)
  {
    if (operation.m_bCompleted)
    {
      return;
    }
    XII_ASSERT_DEV(!operation.m_bNativePending || operation.m_Result.m_Kind == xiiNetworkIOOperationKind::Resolve,
                   "Native ownership must end before publishing socket completions.");
    if (operation.m_CancelReason.Succeeded() && !operation.m_Deadline.IsNegative() && xiiTime::Now() >= operation.m_Deadline)
    {
      operation.m_CancelReason = xiiNetworkError(xiiNetworkErrorCode::TimedOut);
    }
    if (operation.m_CancelReason.Failed())
    {
      const xiiNetworkError native = error;
      error                        = operation.m_CancelReason;
      if (native.m_iNativeCode != 0)
      {
        error.m_iNativeCode = native.m_iNativeCode;
        error.m_Domain      = native.m_Domain;
      }
    }
    RemoveDeadline(operation);
    Registration* pRegistration = operation.m_pRegistration;
    if (pRegistration != nullptr)
    {
      (operation.m_bReadQueue ? pRegistration->m_Read : pRegistration->m_Write).Remove(operation);
      --pRegistration->m_uiPendingCount;
      if (operation.m_bAcceptIssued)
      {
        --pRegistration->m_uiActiveAccepts;
        operation.m_bAcceptIssued = false;
      }
      operation.m_pRegistration = nullptr;
    }
    if (operation.m_Result.m_Kind == xiiNetworkIOOperationKind::Resolve)
    {
      m_Resolves.Remove(operation);
    }
    operation.m_bCompleted                  = true;
    operation.m_Result.m_Error              = error;
    operation.m_Result.m_uiBytesTransferred = operation.m_uiProgress;
    if (error.Failed())
    {
      operation.m_Result.m_AcceptedSocket.Close().IgnoreResult();
      if (error.m_Code == xiiNetworkErrorCode::Cancelled || error.m_Code == xiiNetworkErrorCode::TimedOut)
      {
        operation.m_Result.m_ResolvedEndpoints.Clear();
      }
    }
    const xiiTime latency = xiiTime::Now() - operation.m_SubmissionTime;
    m_Statistics.m_TotalCompletionLatency += latency;
    m_Statistics.m_MaxCompletionLatency = std::max(m_Statistics.m_MaxCompletionLatency, latency);
    --m_Statistics.m_uiPendingOperations;
    ++m_Statistics.m_uiCompletedOperations;
    if (!operation.m_bStarted)
    {
      --m_Statistics.m_uiSubmissionQueueDepth;
    }
    if (error.m_Code == xiiNetworkErrorCode::Cancelled)
    {
      ++m_Statistics.m_uiCanceledOperations;
    }
    else if (error.m_Code == xiiNetworkErrorCode::TimedOut)
    {
      ++m_Statistics.m_uiTimedOutOperations;
    }
    else if (error.Failed())
    {
      ++m_Statistics.m_uiFailedOperations;
    }
    if (error.m_iNativeCode != 0 && error.m_Code != xiiNetworkErrorCode::Cancelled && error.m_Code != xiiNetworkErrorCode::TimedOut)
    {
      ++m_Statistics.m_uiNativeErrors;
    }
    {
      XII_LOCK(m_pRuntime->m_PoolMutex);
      if (!operation.m_bStarted)
      {
        --m_pRuntime->m_uiSubmissionCount;
      }
      if (operation.m_bSendCredit)
      {
        m_pRuntime->m_uiQueuedSendBytes -= operation.m_uiTotalBytes;
        operation.m_bSendCredit = false;
      }
      if (operation.m_bReceiveCredit)
      {
        --m_pRuntime->m_uiReceiveRequests;
        operation.m_bReceiveCredit = false;
      }
      if (operation.m_bAcceptCredit)
      {
        --m_pRuntime->m_uiAcceptRequests;
        operation.m_bAcceptCredit = false;
      }
      if (operation.m_bResolverCredit && !operation.m_bNativePending)
      {
        --m_pRuntime->m_uiResolverRequests;
        operation.m_bResolverCredit = false;
      }
    }
    // Publish only after accounting and unlinking; a consumer may immediately recycle a quiescent slot.
    m_pRuntime->Publish(operation);
    if (pRegistration != nullptr)
    {
      Ready(*pRegistration, false);
      CloseIfDrained(*pRegistration);
    }
  }

  void Worker::Cancel(Operation& operation, xiiNetworkError reason)
  {
    if (operation.m_bCompleted || operation.m_CancelReason.Failed())
    {
      return;
    }
    operation.m_CancelReason = reason;
    RemoveDeadline(operation);
    Registration* pRegistration = operation.m_pRegistration;
    if (operation.m_Result.m_Kind == xiiNetworkIOOperationKind::Connect && pRegistration != nullptr)
    {
      // A canceled connect is not a reusable stream; retire its registration after native drain.
      pRegistration->m_bClosing = true;
    }
    if (operation.m_bNativePending && operation.m_Result.m_Kind != xiiNetworkIOOperationKind::Resolve)
    {
      CancelNative(operation);
    }
    else
    {
      Complete(operation, reason);
    }
    if (pRegistration != nullptr)
    {
      Ready(*pRegistration, false);
    }
  }

  void Worker::Cancel(Registration& registration, bool bClose, xiiNetworkError reason)
  {
    registration.m_bClosing = registration.m_bClosing || bClose;
    for (Operation* pOperation = registration.m_Read.m_pHead; pOperation != nullptr;)
    {
      Operation* pNext = pOperation->m_pNext;
      Cancel(*pOperation, reason);
      pOperation = pNext;
    }
    for (Operation* pOperation = registration.m_Write.m_pHead; pOperation != nullptr;)
    {
      Operation* pNext = pOperation->m_pNext;
      Cancel(*pOperation, reason);
      pOperation = pNext;
    }
    CloseIfDrained(registration);
  }

  void Worker::ApplyTransfer(Operation& operation, const xiiNetworkOperationResult& transfer)
  {
    const auto kind = operation.m_Result.m_Kind;
    if (transfer.m_uiBytesTransferred > operation.m_uiTotalBytes - operation.m_uiProgress)
    {
      Complete(operation, xiiNetworkError(xiiNetworkErrorCode::Unknown));
      return;
    }
    operation.m_uiProgress += transfer.m_uiBytesTransferred;
    if (kind == xiiNetworkIOOperationKind::Send || kind == xiiNetworkIOOperationKind::SendTo)
    {
      m_Statistics.m_uiBytesSent += transfer.m_uiBytesTransferred;
    }
    else if (kind == xiiNetworkIOOperationKind::Receive || kind == xiiNetworkIOOperationKind::ReceiveFrom)
    {
      m_Statistics.m_uiBytesReceived += transfer.m_uiBytesTransferred;
    }
    if (operation.m_CancelReason.Failed() || transfer.Failed())
    {
      Complete(operation, transfer.m_Error);
      return;
    }
    if ((kind != xiiNetworkIOOperationKind::Send && kind != xiiNetworkIOOperationKind::Receive) ||
        operation.m_Options.m_TransferMode == xiiNetworkIOTransferMode::Some || operation.m_uiProgress == operation.m_uiTotalBytes)
    {
      Complete(operation, {});
      return;
    }
    if (transfer.m_uiBytesTransferred == 0)
    {
      Complete(operation, xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed));
    }
  }

  void Worker::ProcessOperation(Operation& operation)
  {
    if (operation.m_bNativePending || operation.m_bCompleted)
    {
      return;
    }
    if (!operation.m_bStarted)
    {
      operation.m_bStarted = true;
      --m_Statistics.m_uiSubmissionQueueDepth;
      XII_LOCK(m_pRuntime->m_PoolMutex);
      --m_pRuntime->m_uiSubmissionCount;
    }
    if (operation.m_CancelReason.Failed())
    {
      Complete(operation, operation.m_CancelReason);
      return;
    }
    const auto kind = operation.m_Result.m_Kind;
    if ((kind == xiiNetworkIOOperationKind::Send || kind == xiiNetworkIOOperationKind::Receive) && operation.m_uiTotalBytes == 0)
    {
      Complete(operation, {});
      return;
    }
    const NativeAttempt attempt = Submit(operation);
    if (attempt.m_bPending)
    {
      operation.m_bNativePending = true;
      if (kind == xiiNetworkIOOperationKind::Accept)
      {
        operation.m_bAcceptIssued = true;
        ++operation.m_pRegistration->m_uiActiveAccepts;
      }
      return;
    }
    // Only readiness backends retain an operation after an uninitiated would-block/interrupted call.
    // A completion backend must either own a native request or publish the immediate failure.
    if (!attempt.m_bWaitForReadiness)
    {
      ApplyTransfer(operation, attempt.m_Transfer);
    }
  }

  void Worker::ProcessRegistration(Registration& registration)
  {
    if (registration.m_bClosing)
    {
      Cancel(registration, true);
      return;
    }
    const auto&     configuration    = m_pRuntime->m_Configuration;
    bool            bMoreProgress    = false;
    xiiUInt32       uiAttempts       = 0;
    const xiiUInt32 uiFirstDirection = registration.m_bWriteFirst ? 1U : 0U;
    registration.m_bWriteFirst       = !registration.m_bWriteFirst;
    const xiiTime budgetEnd          = xiiTime::Now() + configuration.m_SocketProcessingBudget;
    for (xiiUInt32 uiPass = 0; uiPass < 2 && registration.m_bActive; ++uiPass)
    {
      const xiiUInt32 uiDirection = (uiFirstDirection + uiPass) % 2;
      OperationQueue& queue       = uiDirection == 0 ? registration.m_Read : registration.m_Write;
      Operation*      pCurrent    = queue.m_pHead;
      while (pCurrent != nullptr && uiAttempts < configuration.m_uiAttemptsPerSocket &&
             (!configuration.m_SocketProcessingBudget.IsPositive() || xiiTime::Now() < budgetEnd))
      {
        if (pCurrent->m_bNativePending)
        {
          if (registration.m_Kind == SocketKind::Listener)
          {
            pCurrent = pCurrent->m_pNext;
            continue;
          }
          pCurrent = nullptr;
          break;
        }
        if (registration.m_Kind == SocketKind::Listener && registration.m_uiActiveAccepts >= configuration.m_uiAcceptConcurrencyPerSocket)
        {
          pCurrent = nullptr;
          break;
        }
        Operation*      pNext      = pCurrent->m_pNext;
        const xiiUInt32 uiPrevious = pCurrent->m_uiProgress;
        ++uiAttempts;
        ProcessOperation(*pCurrent);
        if (pCurrent->m_bCompleted)
        {
          pCurrent = queue.m_pHead;
        }
        else if (pCurrent->m_bNativePending)
        {
          pCurrent = registration.m_Kind == SocketKind::Listener ? pNext : nullptr;
        }
        else if (pCurrent->m_uiProgress == uiPrevious)
        {
          pCurrent = nullptr;
          break;
        }
      }
      bMoreProgress = bMoreProgress || (pCurrent != nullptr && !pCurrent->m_bNativePending);
    }
    if (!registration.m_bActive)
    {
      return;
    }
    const xiiNetworkError error = Arm(*this, registration);
    if (error.Failed())
    {
      ++m_Statistics.m_uiNativeErrors;
      m_FatalError = error;
      m_pRuntime->m_State.store(xiiNetworkIOContextState::Stopping);
      for (auto& pWorker : m_pRuntime->m_Workers)
      {
        Wake(*pWorker);
      }
    }
    else if (bMoreProgress)
    {
      Ready(registration, false);
    }
  }

  void Worker::ProcessReady()
  {
    const xiiUInt32 uiCount = std::min(m_pRuntime->m_Configuration.m_uiSocketsPerIteration, m_uiReadyCount);
    for (xiiUInt32 uiIndex = 0; uiIndex < uiCount && m_pReadyHead != nullptr; ++uiIndex)
    {
      Registration& registration = *m_pReadyHead;
      Unready(registration);
      ProcessRegistration(registration);
    }
  }

  void Worker::Run()
  {
    const xiiTime cpuBegin = CurrentThreadCpuTime();
    if (m_pRuntime->m_Configuration.m_bEnableProfiling)
    {
      xiiProfilingSystem::SetThreadName("Network I/O");
    }
    while (true)
    {
      xiiInt32      iWait = -1;
      const xiiTime begin = xiiTime::Now();
      {
        XII_LOCK(m_Mutex);
        if (m_pRuntime->m_State.load() == xiiNetworkIOContextState::Stopping && !m_bStopping)
        {
          m_bStopping = true;
          for (auto& registration : m_Registrations)
          {
            if (registration.m_bActive)
            {
              Cancel(registration, true, m_FatalError.Failed() ? m_FatalError : xiiNetworkError(xiiNetworkErrorCode::Cancelled));
            }
          }
          for (Operation* pOperation = m_Resolves.m_pHead; pOperation != nullptr;)
          {
            Operation* pNext = pOperation->m_pNext;
            Cancel(*pOperation, xiiNetworkError(xiiNetworkErrorCode::Cancelled));
            pOperation = pNext;
          }
        }
        Expire();
        if (m_pRuntime->m_Configuration.m_bEnableProfiling)
        {
          XII_PROFILE_SCOPE("NetworkIO.Ready");
          ProcessReady();
        }
        else
        {
          ProcessReady();
        }
        if (m_bStopping && m_Statistics.m_uiPendingOperations == 0 && m_Statistics.m_uiRegisteredSockets == 0)
        {
          m_Statistics.m_WorkerActiveTime += xiiTime::Now() - begin;
          break;
        }
        iWait = GetWaitMilliseconds();
        m_Statistics.m_WorkerActiveTime += xiiTime::Now() - begin;
        m_WaitBegin = xiiTime::Now();
        m_bWaiting  = true;
      }
      const xiiTime         waitBegin = m_WaitBegin;
      xiiUInt32             uiEvents  = 0;
      const xiiNetworkError error     = Wait(*this, iWait, uiEvents);
      const xiiTime         waitEnd   = xiiTime::Now();
      {
        XII_LOCK(m_Mutex);
        m_bWaiting = false;
        m_Statistics.m_WorkerWaitTime += waitEnd - waitBegin;
        ++m_Statistics.m_uiEventLoopWakeups;
        if (error.Failed() && error.m_Code != xiiNetworkErrorCode::Interrupted)
        {
          ++m_Statistics.m_uiNativeErrors;
          m_FatalError = error;
          m_pRuntime->m_State.store(xiiNetworkIOContextState::Stopping);
          for (auto& pWorker : m_pRuntime->m_Workers)
          {
            Wake(*pWorker);
          }
        }
        Expire();
        for (xiiUInt32 uiIndex = 0; uiIndex < uiEvents; ++uiIndex)
        {
          const NativeEvent& event = m_Events[uiIndex];
          if (event.m_pOperation != nullptr)
          {
            Operation& operation        = *event.m_pOperation;
            operation.m_bNativePending  = false;
            const auto    transfer      = FinishNative(operation, event);
            Registration* pRegistration = operation.m_pRegistration;
            ApplyTransfer(operation, transfer);
            if (pRegistration != nullptr && pRegistration->m_bActive)
            {
              Ready(*pRegistration, false);
            }
          }
          else if (event.m_uiToken != 0)
          {
            xiiUInt32 uiSlot = 0;
            if (m_NativeTokens.TryGetValue(event.m_uiToken, uiSlot))
            {
              Ready(m_Registrations[uiSlot], false);
            }
          }
        }
        m_Statistics.m_WorkerActiveTime += xiiTime::Now() - waitEnd;
      }
    }
    {
      XII_LOCK(m_Mutex);
      m_Statistics.m_WorkerCpuTime = CurrentThreadCpuTime() - cpuBegin;
    }
    if (m_pRuntime->m_Configuration.m_bEnableProfiling)
    {
      xiiProfilingSystem::RemoveThread();
    }
  }
} // namespace xiiNetworkIOInternal

namespace
{
  template <typename Function>
  xiiNetworkError WithRegistration(xiiNetworkIORuntime& runtime, xiiNetworkIORegistration handle, Function&& function)
  {
    if (handle.m_uiContextId != runtime.m_uiContextId || handle.m_uiWorker >= runtime.m_Workers.GetCount())
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidHandle);
    }
    auto& worker = *runtime.m_Workers[handle.m_uiWorker];
    XII_LOCK(worker.m_Mutex);
    auto* pRegistration = worker.Find(handle);
    return pRegistration != nullptr ? std::forward<Function>(function)(worker, *pRegistration) : xiiNetworkError(xiiNetworkErrorCode::InvalidHandle);
  }
} // namespace

xiiNetworkError xiiNetworkIOContext::Deregister(xiiNetworkIORegistration registration)
{
  return WithRegistration(*m_pRuntime, registration, [](auto& worker, auto& entry) {
    worker.Cancel(entry, true);
    xiiNetworkIOInternal::Wake(worker);
    return xiiNetworkError();
  });
}

xiiNetworkError xiiNetworkIOContext::Cancel(xiiNetworkIORegistration registration)
{
  return WithRegistration(*m_pRuntime, registration, [](auto& worker, auto& entry) {
    worker.Cancel(entry, false);
    xiiNetworkIOInternal::Wake(worker);
    return xiiNetworkError();
  });
}

xiiNetworkError xiiNetworkIOContext::GetLocalEndpoint(xiiNetworkIORegistration registration, xiiNetworkEndpoint& out_endpoint) const
{
  return WithRegistration(*m_pRuntime, registration, [&](auto&, auto& entry) { return entry.GetSocket().GetLocalEndpoint(out_endpoint); });
}

xiiNetworkError xiiNetworkIOContext::GetPeerEndpoint(xiiNetworkIORegistration registration, xiiNetworkEndpoint& out_endpoint) const
{
  return WithRegistration(*m_pRuntime, registration, [&](auto&, auto& entry) { return entry.GetSocket().GetPeerEndpoint(out_endpoint); });
}

xiiNetworkError xiiNetworkIOContext::Cancel(xiiNetworkIOOperation operation)
{
  if (operation.m_uiContextId != m_pRuntime->m_uiContextId || operation.m_uiWorker >= m_pRuntime->m_Workers.GetCount())
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidHandle);
  }
  auto& worker = *m_pRuntime->m_Workers[operation.m_uiWorker];
  XII_LOCK(worker.m_Mutex);
  auto* pOperation = worker.Find(operation);
  if (pOperation == nullptr)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidHandle);
  }
  worker.Cancel(*pOperation, xiiNetworkError(xiiNetworkErrorCode::Cancelled));
  xiiNetworkIOInternal::Wake(worker);
  return {};
}

xiiNetworkError xiiNetworkIOContext::SetDeadline(xiiNetworkIOOperation operation, xiiTime timeout)
{
  if (!std::isfinite(timeout.GetSeconds()) || (timeout.IsNegative() && timeout.GetSeconds() != -1))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  const xiiTime deadline = timeout.IsNegative() ? xiiTime::MakeFromSeconds(-1) : xiiTime::Now() + timeout;
  if (!std::isfinite(deadline.GetSeconds()))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  if (operation.m_uiContextId != m_pRuntime->m_uiContextId || operation.m_uiWorker >= m_pRuntime->m_Workers.GetCount())
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidHandle);
  }
  auto& worker = *m_pRuntime->m_Workers[operation.m_uiWorker];
  XII_LOCK(worker.m_Mutex);
  auto* pOperation = worker.Find(operation);
  if (pOperation == nullptr || pOperation->m_CancelReason.Failed())
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidHandle);
  }
  worker.RemoveDeadline(*pOperation);
  pOperation->m_Deadline = deadline;
  worker.AddDeadline(*pOperation);
  xiiNetworkIOInternal::Wake(worker);
  return {};
}
