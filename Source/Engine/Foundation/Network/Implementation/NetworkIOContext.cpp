/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Implementation/NetworkIOContextAccess.h>

#include <Foundation/Network/NetworkIOContext.h>

#include <Foundation/Network/Implementation/NetworkIOInternal.h>
#include <Foundation/Threading/Lock.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <utility>

namespace
{
  std::atomic<xiiUInt64> s_uiNextContext{1};
  std::atomic<xiiUInt64> s_uiActiveContexts{0};

  xiiUInt64 NextIdentity(std::atomic<xiiUInt64>& counter)
  {
    xiiUInt64 uiValue = counter.load();
    while (uiValue != 0)
    {
      if (counter.compare_exchange_weak(uiValue, uiValue + 1))
      {
        return uiValue;
      }
    }
    return 0; // Exhaustion is permanent; never wrap back into a live identity namespace.
  }

  void RunNetworkWorker(void* pData) { static_cast<xiiNetworkIOInternal::Worker*>(pData)->Run(); }
  void RunNetworkResolver(void* pData) { static_cast<xiiNetworkIORuntime*>(pData)->ResolverRun(); }

  bool ValidTimeout(xiiTime timeout)
  {
    return std::isfinite(timeout.GetSeconds()) && (timeout.GetSeconds() == -1 || !timeout.IsNegative());
  }
} // namespace

using namespace xiiNetworkIOInternal;

xiiNetworkIORuntime::xiiNetworkIORuntime() : m_uiContextId(NextIdentity(s_uiNextContext)) {}

xiiNetworkIORuntime::~xiiNetworkIORuntime()
{
  Shutdown();
  for (auto& operation : m_Operations)
  {
    DestroyOperation(operation);
  }
  for (auto& pWorker : m_Workers)
  {
    for (auto& registration : pWorker->m_Registrations)
    {
      DestroyRegistration(registration);
    }
  }
}

xiiNetworkError xiiNetworkIORuntime::Start(const xiiNetworkIOConfiguration& configuration, xiiAllocator* pBufferAllocator)
{
  XII_LOCK(m_LifecycleMutex);
  if (m_State.load() != xiiNetworkIOContextState::Created)
  {
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyOpen);
  }
  if (configuration.m_uiWorkerThreadCount == 0 || configuration.m_uiResolverThreadCount == 0 ||
      configuration.m_uiWorkerThreadCount > configuration.m_uiMaxRegisteredSockets ||
      configuration.m_uiResolverThreadCount > configuration.m_uiMaxResolverRequests ||
      configuration.m_uiMaxOutstandingOperations == 0 || configuration.m_uiMaxOperationsPerSocket == 0 ||
      configuration.m_uiMaxQueuedCompletions == 0 || configuration.m_uiMaxQueuedSubmissions == 0 ||
      configuration.m_uiMaxReceiveRequests == 0 || configuration.m_uiMaxAcceptOperations == 0 ||
      configuration.m_uiAcceptConcurrencyPerSocket == 0 || configuration.m_uiNativeEventBatchSize == 0 ||
      configuration.m_uiNativeEventBatchSize > static_cast<xiiUInt32>(INT_MAX) ||
      configuration.m_uiSocketsPerIteration == 0 || configuration.m_uiAttemptsPerSocket == 0 ||
      configuration.m_uiMaxTransferBytesPerAttempt == 0 || configuration.m_uiMaxTransferBytesPerAttempt > INT_MAX ||
      configuration.m_uiMaxQueuedSendBytes == 0 || !ValidTimeout(configuration.m_DefaultTimeout) || !std::isfinite(configuration.m_SocketProcessingBudget.GetSeconds()) || configuration.m_SocketProcessingBudget.IsNegative() || m_uiContextId == 0)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  xiiNetworkError error = m_BufferPool.Initialize(configuration.m_BufferPool, pBufferAllocator);
  if (error.Failed())
  {
    return error;
  }
  m_Configuration = configuration;
  m_Operations.SetCount(configuration.m_uiMaxOutstandingOperations);
  m_FreeOperations.Reserve(configuration.m_uiMaxOutstandingOperations);
  for (xiiUInt32 uiIndex = configuration.m_uiMaxOutstandingOperations; uiIndex > 0; --uiIndex)
  {
    m_Operations[uiIndex - 1].m_uiPoolSlot = uiIndex - 1;
    m_FreeOperations.PushBack(uiIndex - 1);
  }
  m_CompletionRing.SetCount(configuration.m_uiMaxQueuedCompletions);
  m_ResolverThreads.SetCount(configuration.m_uiResolverThreadCount);
  m_Workers.Reserve(configuration.m_uiWorkerThreadCount);
  const xiiUInt32 uiSocketsPerWorker = (configuration.m_uiMaxRegisteredSockets / configuration.m_uiWorkerThreadCount) +
    (configuration.m_uiMaxRegisteredSockets % configuration.m_uiWorkerThreadCount != 0 ? 1U : 0U);
  for (xiiUInt32 uiIndex = 0; uiIndex < configuration.m_uiWorkerThreadCount; ++uiIndex)
  {
    xiiUniquePtr<Worker> pWorker = XII_DEFAULT_NEW(Worker);
    pWorker->m_pRuntime          = this;
    pWorker->m_uiIndex           = uiIndex;
    pWorker->m_Registrations.SetCount(uiSocketsPerWorker);
    pWorker->m_FreeRegistrations.Reserve(uiSocketsPerWorker);
    pWorker->m_NativeTokens.Reserve(uiSocketsPerWorker);
    pWorker->m_DeadlineHeap.Reserve(configuration.m_uiMaxOutstandingOperations);
    pWorker->m_Events.SetCount(configuration.m_uiNativeEventBatchSize);
    for (xiiUInt32 uiSlot = uiSocketsPerWorker; uiSlot > 0; --uiSlot)
    {
      pWorker->m_FreeRegistrations.PushBack(uiSlot - 1);
      pWorker->m_Registrations[uiSlot - 1].m_Handle.m_uiSlot = uiSlot - 1;
    }
    error = CreateWorker(*pWorker);
    m_Workers.PushBack(std::move(pWorker));
    if (error.Failed())
    {
      ShutdownUnlocked();
      return error;
    }
  }
  for (auto& thread : m_ResolverThreads)
  {
    error = StartThread(thread, RunNetworkResolver, this);
    if (error.Failed())
    {
      ShutdownUnlocked();
      return error;
    }
  }
  for (auto& pWorker : m_Workers)
  {
    error = StartThread(pWorker->m_Thread, RunNetworkWorker, pWorker.Borrow());
    if (error.Failed())
    {
      ShutdownUnlocked();
      return error;
    }
  }
  m_bCountedActive = true;
  ++s_uiActiveContexts;
  m_State.store(xiiNetworkIOContextState::Running);
  return {};
}

void xiiNetworkIORuntime::Shutdown()
{
  XII_LOCK(m_LifecycleMutex);
  ShutdownUnlocked();
}

void xiiNetworkIORuntime::ShutdownUnlocked()
{
  if (m_State.load() == xiiNetworkIOContextState::Stopped)
  {
    return;
  }
  m_State.store(xiiNetworkIOContextState::Stopping);
  {
    XII_LOCK(m_ResolverCondition);
    m_bResolverStop = true;
    m_ResolverCondition.SignalAll();
  }
  for (auto& pWorker : m_Workers)
  {
    XII_LOCK(pWorker->m_Mutex);
    if (pWorker->m_pNativeData != nullptr)
    {
      Wake(*pWorker);
    }
  }
  for (auto& pWorker : m_Workers)
  {
    JoinThread(pWorker->m_Thread);
  }
  for (auto& thread : m_ResolverThreads)
  {
    JoinThread(thread);
  }
  for (auto& pWorker : m_Workers)
  {
    XII_LOCK(pWorker->m_Mutex);
    // Start can fail before a worker thread exists; no operation was admitted in that case.
    for (auto& registration : pWorker->m_Registrations)
    {
      if (registration.m_bActive)
      {
        pWorker->Cancel(registration, true);
        pWorker->CloseIfDrained(registration);
      }
    }
    DestroyWorker(*pWorker);
  }
  if (m_bCountedActive)
  {
    --s_uiActiveContexts;
    m_bCountedActive = false;
  }
  m_State.store(xiiNetworkIOContextState::Stopped);
  XII_LOCK(m_Completions);
  m_Completions.SignalAll();
}

template <typename Socket>
xiiNetworkError xiiNetworkIORuntime::Register(Socket&& socket, xiiNetworkIORegistration& out_registration)
{
  if (m_State.load() != xiiNetworkIOContextState::Running)
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotRunning);
  }
  if (!socket.IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  if constexpr (std::is_same_v<Socket, xiiTcpListener>)
  {
    if (socket.GetState() != xiiNetworkSocketState::Listening)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
  }
  else if (socket.GetState() != xiiNetworkSocketState::Open && socket.GetState() != xiiNetworkSocketState::Connected)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  const xiiNetworkError mode = socket.SetNonBlocking(true);
  if (mode.Failed())
  {
    return mode;
  }
  {
    XII_LOCK(m_PoolMutex);
    if (m_uiRegisteredCount >= m_Configuration.m_uiMaxRegisteredSockets)
    {
      ++m_uiBackpressureEvents;
      return xiiNetworkError(xiiNetworkErrorCode::Backpressure);
    }
    ++m_uiRegisteredCount;
  }
  const xiiUInt32 uiFirst = m_uiNextWorker.fetch_add(1) % m_Workers.GetCount();
  xiiNetworkError error(xiiNetworkErrorCode::Backpressure);
  for (xiiUInt32 uiAttempt = 0; uiAttempt < m_Workers.GetCount(); ++uiAttempt)
  {
    Worker& worker = *m_Workers[(uiFirst + uiAttempt) % m_Workers.GetCount()];
    XII_LOCK(worker.m_Mutex);
    if (m_State.load() != xiiNetworkIOContextState::Running || worker.m_bStopping)
    {
      error = xiiNetworkError(xiiNetworkErrorCode::ShuttingDown);
      break;
    }
    if (worker.m_FreeRegistrations.IsEmpty())
    {
      continue;
    }
    const xiiUInt32 uiSlot       = worker.m_FreeRegistrations.PeekBack();
    Registration&   registration = worker.m_Registrations[uiSlot];
    if (registration.m_Handle.m_uiGeneration == 0xFFFFFFFFFFFFFFFFULL)
    {
      continue;
    }
    const xiiUInt64 uiToken = NextIdentity(m_uiNextToken);
    if (uiToken == 0)
    {
      error = xiiNetworkError(xiiNetworkErrorCode::ResourceExhausted);
      break;
    }
    registration.m_Socket.template emplace<Socket>(std::forward<Socket>(socket));
    if constexpr (std::is_same_v<Socket, xiiTcpSocket>)
    {
      registration.m_Kind = SocketKind::Tcp;
    }
    else if constexpr (std::is_same_v<Socket, xiiTcpListener>)
    {
      registration.m_Kind = SocketKind::Listener;
    }
    else
    {
      registration.m_Kind = SocketKind::Udp;
    }
    ++registration.m_Handle.m_uiGeneration;
    registration.m_Handle.m_uiContextId = m_uiContextId;
    registration.m_Handle.m_uiWorker    = worker.m_uiIndex;
    registration.m_uiNativeToken        = uiToken;
    registration.m_bClosing             = false;
    registration.m_bWriteFirst          = false;
    registration.m_bReadyQueued         = false;
    registration.m_uiNativeInterest     = 0;
    registration.m_bNativeRegistered    = false;
    error                               = Associate(worker, registration);
    if (error.Failed())
    {
      socket = std::move(*std::get_if<Socket>(&registration.m_Socket));
      break;
    }
    registration.m_bActive = true;
    worker.m_FreeRegistrations.PopBack();
    worker.m_NativeTokens.Insert(uiToken, uiSlot);
    ++worker.m_Statistics.m_uiRegisteredSockets;
    out_registration = registration.m_Handle;
    worker.Ready(registration, true);
    return {};
  }
  XII_LOCK(m_PoolMutex);
  --m_uiRegisteredCount;
  return error;
}

xiiNetworkError xiiNetworkIORuntime::ValidateOptions(const xiiNetworkIOOptions& options, xiiTime& out_deadline) const
{
  if (options.m_TransferMode != xiiNetworkIOTransferMode::Some && options.m_TransferMode != xiiNetworkIOTransferMode::All)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  const xiiTime timeout = options.m_Timeout.GetSeconds() == -2 ? m_Configuration.m_DefaultTimeout : options.m_Timeout;
  if (!ValidTimeout(timeout))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  out_deadline = timeout.IsNegative() ? xiiTime::MakeFromSeconds(-1) : xiiTime::Now() + timeout;
  return std::isfinite(out_deadline.GetSeconds()) ? xiiNetworkError() : xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
}

Operation* xiiNetworkIORuntime::Allocate(Worker& worker, xiiNetworkIOOperationKind kind, xiiUInt32 uiBytes, xiiNetworkError& out_error)
{
  XII_LOCK(m_PoolMutex);
  const bool bSend    = kind == xiiNetworkIOOperationKind::Send || kind == xiiNetworkIOOperationKind::SendTo;
  const bool bReceive = kind == xiiNetworkIOOperationKind::Receive || kind == xiiNetworkIOOperationKind::ReceiveFrom;
  if (m_State.load() != xiiNetworkIOContextState::Running)
  {
    out_error = xiiNetworkError(xiiNetworkErrorCode::ShuttingDown);
    return nullptr;
  }
  if (m_FreeOperations.IsEmpty() || m_uiRetainedCount >= m_Configuration.m_uiMaxQueuedCompletions ||
      m_uiSubmissionCount >= m_Configuration.m_uiMaxQueuedSubmissions ||
      (bSend && uiBytes > m_Configuration.m_uiMaxQueuedSendBytes - m_uiQueuedSendBytes) ||
      (bReceive && m_uiReceiveRequests >= m_Configuration.m_uiMaxReceiveRequests) ||
      (kind == xiiNetworkIOOperationKind::Accept && m_uiAcceptRequests >= m_Configuration.m_uiMaxAcceptOperations) ||
      (kind == xiiNetworkIOOperationKind::Resolve && m_uiResolverRequests >= m_Configuration.m_uiMaxResolverRequests))
  {
    ++m_uiBackpressureEvents;
    out_error = xiiNetworkError(xiiNetworkErrorCode::Backpressure);
    return nullptr;
  }
  const xiiUInt32 uiSlot    = m_FreeOperations.PeekBack();
  Operation&      operation = m_Operations[uiSlot];
  if (operation.m_uiGeneration == 0xFFFFFFFFFFFFFFFFULL)
  {
    out_error = xiiNetworkError(xiiNetworkErrorCode::ResourceExhausted);
    return nullptr;
  }
  m_FreeOperations.PopBack();
  ++operation.m_uiGeneration;
  operation.m_Result             = xiiNetworkIOResult();
  operation.m_Result.m_Operation = {m_uiContextId, operation.m_uiGeneration, uiSlot, worker.m_uiIndex};
  operation.m_Result.m_Kind      = kind;
  operation.m_pWorker            = &worker;
  operation.m_pRegistration      = nullptr;
  operation.m_pPrevious          = nullptr;
  operation.m_pNext              = nullptr;
  operation.m_uiProgress         = 0;
  operation.m_uiTotalBytes       = uiBytes;
  operation.m_uiDeadlineIndex    = xiiInvalidIndex;
  operation.m_Deadline           = xiiTime::MakeFromSeconds(-1);
  operation.m_SubmissionTime     = xiiTime::Now();
  operation.m_CancelReason       = {};
  operation.m_bInUse             = true;
  operation.m_bStarted           = false;
  operation.m_bCompleted         = false;
  operation.m_bConsumed          = false;
  operation.m_bNativePending     = false;
  operation.m_bConnectInitiated  = false;
  operation.m_bReadQueue         = false;
  operation.m_bAcceptIssued      = false;
  operation.m_bSendCredit        = bSend;
  operation.m_bReceiveCredit     = bReceive;
  operation.m_bAcceptCredit      = kind == xiiNetworkIOOperationKind::Accept;
  operation.m_bResolverCredit    = kind == xiiNetworkIOOperationKind::Resolve;
  m_uiQueuedSendBytes += bSend ? uiBytes : 0;
  m_uiReceiveRequests += bReceive ? 1 : 0;
  m_uiAcceptRequests += operation.m_bAcceptCredit ? 1 : 0;
  m_uiResolverRequests += operation.m_bResolverCredit ? 1 : 0;
  ++m_uiRetainedCount;
  ++m_uiSubmissionCount;
  ++worker.m_Statistics.m_uiPendingOperations;
  ++worker.m_Statistics.m_uiSubmissionQueueDepth;
  return &operation;
}

void xiiNetworkIORuntime::Retire(Operation& operation)
{
  if (!operation.m_bConsumed || operation.m_bNativePending)
  {
    return;
  }
  operation.m_Result = xiiNetworkIOResult();
  operation.m_sHost.Clear();
  operation.m_sService.Clear();
  XII_LOCK(m_PoolMutex);
  operation.m_bInUse  = false;
  operation.m_pWorker = nullptr;
  if (operation.m_bResolverCredit)
  {
    --m_uiResolverRequests;
    operation.m_bResolverCredit = false;
  }
  --m_uiRetainedCount;
  m_FreeOperations.PushBack(operation.m_uiPoolSlot);
}

xiiNetworkError xiiNetworkIORuntime::Submit(xiiNetworkIORegistration handle, xiiNetworkIOOperationKind kind,
                                            xiiNetworkIOBuffers* pBuffers, const xiiNetworkEndpoint* pEndpoint, const xiiNetworkIOOptions& options, xiiNetworkIOOperation& out_operation)
{
  if (m_State.load() != xiiNetworkIOContextState::Running)
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotRunning);
  }
  if (handle.m_uiContextId != m_uiContextId || handle.m_uiWorker >= m_Workers.GetCount())
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidHandle);
  }
  xiiTime         deadline;
  xiiNetworkError error = ValidateOptions(options, deadline);
  if (error.Failed())
  {
    return error;
  }
  Worker& worker = *m_Workers[handle.m_uiWorker];
  XII_LOCK(worker.m_Mutex);
  Registration* pRegistration = worker.Find(handle);
  if (pRegistration == nullptr)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidHandle);
  }
  if (pRegistration->m_bClosing)
  {
    return xiiNetworkError(xiiNetworkErrorCode::ShuttingDown);
  }
  const SocketKind socketKind = pRegistration->m_Kind;
  const bool       bTcp       = kind == xiiNetworkIOOperationKind::Send || kind == xiiNetworkIOOperationKind::Receive || kind == xiiNetworkIOOperationKind::Connect;
  const bool       bUdp       = kind == xiiNetworkIOOperationKind::SendTo || kind == xiiNetworkIOOperationKind::ReceiveFrom;
  if ((bTcp && socketKind != SocketKind::Tcp) || (bUdp && socketKind != SocketKind::Udp) ||
      (kind == xiiNetworkIOOperationKind::Accept && socketKind != SocketKind::Listener))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  const auto state = pRegistration->GetSocket().GetState();
  if ((kind == xiiNetworkIOOperationKind::Accept && state != xiiNetworkSocketState::Listening) ||
      (kind == xiiNetworkIOOperationKind::Connect && (state != xiiNetworkSocketState::Open || pRegistration->m_uiPendingCount != 0)))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  if ((kind == xiiNetworkIOOperationKind::Send || kind == xiiNetworkIOOperationKind::Receive) && state != xiiNetworkSocketState::Connected)
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotConnected);
  }
  if (pEndpoint != nullptr && (!pEndpoint->IsValid() || pEndpoint->GetAddress().GetFamily() != pRegistration->GetSocket().GetFamily()))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
  }
  xiiUInt32 uiBytes = 0;
  if (pBuffers != nullptr)
  {
    if (pBuffers->GetCount() > 64)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    for (const auto& buffer : *pBuffers)
    {
      if (!buffer.IsValid() || !xiiNetworkIOBufferAccess::IsFrom(buffer, m_BufferPool))
      {
        return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
      }
      if (buffer.GetCount() > static_cast<xiiUInt32>(INT_MAX) - uiBytes)
      {
        return xiiNetworkError(xiiNetworkErrorCode::MessageTooLarge);
      }
      uiBytes += buffer.GetCount();
    }
    if (kind == xiiNetworkIOOperationKind::SendTo && uiBytes > (pRegistration->GetSocket().GetFamily() == xiiNetworkAddressFamily::Ipv4 ? 65507U : 65527U))
    {
      return xiiNetworkError(xiiNetworkErrorCode::MessageTooLarge);
    }
  }
  if (pRegistration->m_uiPendingCount >= m_Configuration.m_uiMaxOperationsPerSocket)
  {
    ++worker.m_Statistics.m_uiBackpressureEvents;
    return xiiNetworkError(xiiNetworkErrorCode::Backpressure);
  }
  Operation* pOperation = Allocate(worker, kind, uiBytes, error);
  if (pOperation == nullptr)
  {
    return error;
  }
  pOperation->m_pRegistration         = pRegistration;
  pOperation->m_Result.m_Registration = handle;
  pOperation->m_Result.m_uiUserData   = options.m_uiUserData;
  pOperation->m_Options               = options;
  pOperation->m_Deadline              = deadline;
  if (pEndpoint != nullptr)
  {
    pOperation->m_Result.m_Endpoint = *pEndpoint;
  }
  if (pBuffers != nullptr)
  {
    pOperation->m_Result.m_Buffers = std::move(*pBuffers);
  }
  pOperation->m_bReadQueue = kind == xiiNetworkIOOperationKind::Accept || kind == xiiNetworkIOOperationKind::Receive || kind == xiiNetworkIOOperationKind::ReceiveFrom;
  (pOperation->m_bReadQueue ? pRegistration->m_Read : pRegistration->m_Write).Push(*pOperation);
  ++pRegistration->m_uiPendingCount;
  worker.AddDeadline(*pOperation);
  out_operation = pOperation->m_Result.m_Operation;
  worker.Ready(*pRegistration, true);
  return {};
}

xiiNetworkError xiiNetworkIORuntime::Resolve(xiiStringView sHost, xiiStringView sService, xiiNetworkAddressFamily family,
                                             xiiNetworkTransport transport, const xiiNetworkIOOptions& options, xiiNetworkIOOperation& out_operation)
{
  if (m_State.load() != xiiNetworkIOContextState::Running)
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotRunning);
  }
  // Bound retained text independently of the OS resolver's native name limits.
  if (sHost.GetElementCount() > 1024 || sService.GetElementCount() > 256 ||
      (!sHost.IsEmpty() && std::memchr(sHost.GetStartPointer(), 0, sHost.GetElementCount()) != nullptr) ||
      (!sService.IsEmpty() && std::memchr(sService.GetStartPointer(), 0, sService.GetElementCount()) != nullptr) ||
      (family != xiiNetworkAddressFamily::Unspecified && family != xiiNetworkAddressFamily::Ipv4 && family != xiiNetworkAddressFamily::Ipv6) ||
      (transport != xiiNetworkTransport::Tcp && transport != xiiNetworkTransport::Udp))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  xiiTime         deadline;
  xiiNetworkError error = ValidateOptions(options, deadline);
  if (error.Failed())
  {
    return error;
  }
  Worker& worker = *m_Workers[m_uiNextWorker.fetch_add(1) % m_Workers.GetCount()];
  XII_LOCK(worker.m_Mutex);
  Operation* pOperation = Allocate(worker, xiiNetworkIOOperationKind::Resolve, 0, error);
  if (pOperation == nullptr)
  {
    return error;
  }
  pOperation->m_Options             = options;
  pOperation->m_Result.m_uiUserData = options.m_uiUserData;
  pOperation->m_Deadline            = deadline;
  pOperation->m_sHost               = sHost;
  pOperation->m_sService            = sService;
  pOperation->m_ResolveFamily       = family;
  pOperation->m_ResolveTransport    = transport;
  pOperation->m_bNativePending      = true; // The executor queue pins this slot before a resolver starts.
  out_operation                     = pOperation->m_Result.m_Operation;
  worker.AddDeadline(*pOperation);
  worker.m_Resolves.Push(*pOperation);
  bool bQueued = false;
  {
    XII_LOCK(m_ResolverCondition);
    if (!m_bResolverStop)
    {
      m_ResolverQueue.PushBack(pOperation);
      m_ResolverCondition.SignalOne();
      bQueued = true;
    }
  }
  if (!bQueued)
  {
    // Shutdown may close executor admission after Allocate accepted this operation.
    // Do not leave a pinned queue entry after the last resolver has already exited.
    pOperation->m_bNativePending = false;
    worker.Complete(*pOperation, xiiNetworkError(xiiNetworkErrorCode::Cancelled));
  }
  Wake(worker);
  return {};
}

void xiiNetworkIORuntime::Publish(Operation& operation)
{
  XII_LOCK(m_Completions);
  XII_ASSERT_DEV(m_uiCompletionCount < m_CompletionRing.GetCount(), "Every accepted operation reserves completion capacity.");
  const xiiUInt32 uiTail   = static_cast<xiiUInt32>((static_cast<xiiUInt64>(m_uiCompletionHead) + m_uiCompletionCount) % m_CompletionRing.GetCount());
  m_CompletionRing[uiTail] = &operation;
  ++m_uiCompletionCount;
  m_Completions.SignalOne();
  if (m_pCompletionSignal != nullptr)
  {
    m_pCompletionSignal->RaiseSignal();
  }
}

bool xiiNetworkIORuntime::TryGetCompletion(xiiNetworkIOResult& out_result)
{
  Operation* pOperation = nullptr;
  {
    XII_LOCK(m_Completions);
    if (m_uiCompletionCount == 0)
    {
      return false;
    }
    pOperation         = m_CompletionRing[m_uiCompletionHead];
    m_uiCompletionHead = (m_uiCompletionHead + 1) % m_CompletionRing.GetCount();
    --m_uiCompletionCount;
  }
  xiiNetworkIOResult result;
  {
    Worker& worker = *pOperation->m_pWorker;
    XII_LOCK(worker.m_Mutex);
    result                  = std::move(pOperation->m_Result);
    pOperation->m_bConsumed = true;
    Retire(*pOperation);
  }
  out_result = std::move(result);
  return true;
}

xiiNetworkError xiiNetworkIORuntime::WaitForCompletion(xiiNetworkIOResult& out_result, xiiTime timeout)
{
  if (!std::isfinite(timeout.GetSeconds()) || timeout.IsNegative())
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  const xiiTime deadline = xiiTime::Now() + timeout;
  if (!std::isfinite(deadline.GetSeconds()))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  while (true)
  {
    if (TryGetCompletion(out_result))
    {
      return {};
    }
    XII_LOCK(m_Completions);
    if (m_uiCompletionCount != 0)
    {
      continue;
    }
    if (m_State.load() == xiiNetworkIOContextState::Stopped || m_State.load() == xiiNetworkIOContextState::Created)
    {
      return xiiNetworkError(xiiNetworkErrorCode::NotRunning);
    }
    const xiiTime remaining = deadline - xiiTime::Now();
    if (remaining.IsZeroOrNegative())
    {
      return xiiNetworkError(xiiNetworkErrorCode::TimedOut);
    }
    m_Completions.UnlockWaitForSignalAndLock(remaining);
  }
}

void xiiNetworkIORuntime::ResolverRun()
{
  while (true)
  {
    Operation* pOperation = nullptr;
    {
      XII_LOCK(m_ResolverCondition);
      while (m_ResolverQueue.IsEmpty() && !m_bResolverStop)
      {
        m_ResolverCondition.UnlockWaitForSignalAndLock();
      }
      if (m_ResolverQueue.IsEmpty())
      {
        return;
      }
      pOperation = m_ResolverQueue.PeekFront();
      m_ResolverQueue.PopFront();
    }
    Worker& worker = *pOperation->m_pWorker;
    {
      XII_LOCK(worker.m_Mutex);
      if (pOperation->m_bCompleted || m_State.load() == xiiNetworkIOContextState::Stopping)
      {
        pOperation->m_bNativePending = false;
        if (!pOperation->m_bCompleted)
        {
          worker.Complete(*pOperation, xiiNetworkError(xiiNetworkErrorCode::Cancelled));
        }
        Retire(*pOperation);
        continue;
      }
      pOperation->m_bStarted = true;
      --worker.m_Statistics.m_uiSubmissionQueueDepth;
      XII_LOCK(m_PoolMutex);
      --m_uiSubmissionCount;
    }
    xiiDynamicArray<xiiNetworkEndpoint> endpoints;
    const xiiNetworkError               error = xiiNetworkResolver::Resolve(pOperation->m_sHost, pOperation->m_sService, endpoints,
                                                                            pOperation->m_ResolveFamily, pOperation->m_ResolveTransport);
    {
      XII_LOCK(worker.m_Mutex);
      pOperation->m_bNativePending = false;
      if (!pOperation->m_bCompleted)
      {
        pOperation->m_Result.m_ResolvedEndpoints = std::move(endpoints);
        worker.Complete(*pOperation, error);
      }
      Retire(*pOperation);
    }
  }
}

xiiNetworkIOStatistics xiiNetworkIORuntime::GetStatistics() const
{
  xiiNetworkIOStatistics statistics;
  {
    XII_LOCK(m_PoolMutex);
    statistics.m_uiRetainedOperations  = m_uiRetainedCount;
    statistics.m_uiBackpressureEvents  = m_uiBackpressureEvents;
    statistics.m_uiInternalMemoryBytes = m_Operations.GetHeapMemoryUsage() + m_FreeOperations.GetHeapMemoryUsage() +
      m_CompletionRing.GetHeapMemoryUsage() + m_Workers.GetHeapMemoryUsage() + m_ResolverThreads.GetHeapMemoryUsage();
  }
  {
    XII_LOCK(m_Completions);
    statistics.m_uiCompletionQueueDepth = m_uiCompletionCount;
  }
  for (const auto& pWorker : m_Workers)
  {
    XII_LOCK(pWorker->m_Mutex);
    const auto& source = pWorker->m_Statistics;
    statistics.m_uiRegisteredSockets += source.m_uiRegisteredSockets;
    statistics.m_uiPendingOperations += source.m_uiPendingOperations;
    statistics.m_uiCompletedOperations += source.m_uiCompletedOperations;
    statistics.m_uiCanceledOperations += source.m_uiCanceledOperations;
    statistics.m_uiTimedOutOperations += source.m_uiTimedOutOperations;
    statistics.m_uiFailedOperations += source.m_uiFailedOperations;
    statistics.m_uiSubmissionQueueDepth += source.m_uiSubmissionQueueDepth;
    statistics.m_uiBytesSent += source.m_uiBytesSent;
    statistics.m_uiBytesReceived += source.m_uiBytesReceived;
    statistics.m_uiInternalMemoryBytes += source.m_uiInternalMemoryBytes;
    statistics.m_uiBackpressureEvents += source.m_uiBackpressureEvents;
    statistics.m_uiEventLoopWakeups += source.m_uiEventLoopWakeups;
    statistics.m_uiNativeErrors += source.m_uiNativeErrors;
    statistics.m_WorkerCpuTime += source.m_WorkerCpuTime;
    statistics.m_WorkerActiveTime += source.m_WorkerActiveTime;
    statistics.m_WorkerWaitTime += source.m_WorkerWaitTime;
    if (pWorker->m_bWaiting)
    {
      statistics.m_WorkerWaitTime += xiiTime::Now() - pWorker->m_WaitBegin;
    }
    statistics.m_TotalCompletionLatency += source.m_TotalCompletionLatency;
    statistics.m_MaxCompletionLatency = std::max(statistics.m_MaxCompletionLatency, source.m_MaxCompletionLatency);
    statistics.m_uiInternalMemoryBytes += sizeof(Worker) + pWorker->m_Registrations.GetHeapMemoryUsage() + pWorker->m_NativeTokens.GetHeapMemoryUsage() +
      pWorker->m_FreeRegistrations.GetHeapMemoryUsage() +
      pWorker->m_DeadlineHeap.GetHeapMemoryUsage() + pWorker->m_Events.GetHeapMemoryUsage();
  }
  statistics.m_BufferPool = m_BufferPool.GetStatistics();
  return statistics;
}

xiiNetworkIOContext::xiiNetworkIOContext() : m_pRuntime(XII_DEFAULT_NEW(xiiNetworkIORuntime)) {}
xiiNetworkIOContext::~xiiNetworkIOContext() { XII_DEFAULT_DELETE(m_pRuntime); }
xiiNetworkError          xiiNetworkIOContext::Start(const xiiNetworkIOConfiguration& configuration, xiiAllocator* pAllocator) { return m_pRuntime->Start(configuration, pAllocator); }
void                     xiiNetworkIOContext::Shutdown() { m_pRuntime->Shutdown(); }
xiiNetworkIOContextState xiiNetworkIOContext::GetState() const { return m_pRuntime->m_State.load(); }
xiiUInt64                xiiNetworkIOContext::GetActiveContextCount() { return s_uiActiveContexts.load(); }
xiiNetworkError          xiiNetworkIOContext::Register(xiiTcpSocket&& socket, xiiNetworkIORegistration& out_registration) { return m_pRuntime->Register(std::move(socket), out_registration); }
xiiNetworkError          xiiNetworkIOContext::Register(xiiTcpListener&& listener, xiiNetworkIORegistration& out_registration) { return m_pRuntime->Register(std::move(listener), out_registration); }
xiiNetworkError          xiiNetworkIOContext::Register(xiiUdpSocket&& socket, xiiNetworkIORegistration& out_registration) { return m_pRuntime->Register(std::move(socket), out_registration); }
xiiNetworkError          xiiNetworkIOContext::Accept(xiiNetworkIORegistration listener, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options) { return m_pRuntime->Submit(listener, xiiNetworkIOOperationKind::Accept, nullptr, nullptr, options, out_operation); }
xiiNetworkError          xiiNetworkIOContext::Connect(xiiNetworkIORegistration socket, const xiiNetworkEndpoint& endpoint, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options) { return m_pRuntime->Submit(socket, xiiNetworkIOOperationKind::Connect, nullptr, &endpoint, options, out_operation); }
xiiNetworkError          xiiNetworkIOContext::Send(xiiNetworkIORegistration socket, xiiNetworkIOBuffers&& buffers, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options) { return m_pRuntime->Submit(socket, xiiNetworkIOOperationKind::Send, &buffers, nullptr, options, out_operation); }
xiiNetworkError          xiiNetworkIOContext::Receive(xiiNetworkIORegistration socket, xiiNetworkIOBuffers&& buffers, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options) { return m_pRuntime->Submit(socket, xiiNetworkIOOperationKind::Receive, &buffers, nullptr, options, out_operation); }
xiiNetworkError          xiiNetworkIOContext::SendTo(xiiNetworkIORegistration socket, const xiiNetworkEndpoint& destination, xiiNetworkIOBuffers&& buffers, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options) { return m_pRuntime->Submit(socket, xiiNetworkIOOperationKind::SendTo, &buffers, &destination, options, out_operation); }
xiiNetworkError          xiiNetworkIOContext::ReceiveFrom(xiiNetworkIORegistration socket, xiiNetworkIOBuffers&& buffers, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options) { return m_pRuntime->Submit(socket, xiiNetworkIOOperationKind::ReceiveFrom, &buffers, nullptr, options, out_operation); }
xiiNetworkError          xiiNetworkIOContext::Resolve(xiiStringView sHost, xiiStringView sService, xiiNetworkIOOperation& out_operation, xiiNetworkAddressFamily family, xiiNetworkTransport transport, const xiiNetworkIOOptions& options) { return m_pRuntime->Resolve(sHost, sService, family, transport, options, out_operation); }
bool                     xiiNetworkIOContext::TryGetCompletion(xiiNetworkIOResult& out_result) { return m_pRuntime->TryGetCompletion(out_result); }
xiiNetworkError          xiiNetworkIOContext::WaitForCompletion(xiiNetworkIOResult& out_result, xiiTime timeout) { return m_pRuntime->WaitForCompletion(out_result, timeout); }
xiiNetworkIOStatistics   xiiNetworkIOContext::GetStatistics() const { return m_pRuntime->GetStatistics(); }
xiiNetworkError          xiiNetworkIOContext::AcquireBuffer(xiiUInt32 uiCount, xiiNetworkIOBuffer& out_buffer)
{
  return GetState() == xiiNetworkIOContextState::Running ? m_pRuntime->m_BufferPool.Acquire(uiCount, out_buffer) : xiiNetworkError(xiiNetworkErrorCode::NotRunning);
}

xiiNetworkError xiiNetworkIOContextAccess::SetCompletionSignal(xiiNetworkIOContext& context, xiiThreadSignal& signal)
{
  XII_LOCK(context.m_pRuntime->m_LifecycleMutex);
  if (context.GetState() != xiiNetworkIOContextState::Created)
  {
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyOpen);
  }
  context.m_pRuntime->m_pCompletionSignal = &signal;
  return {};
}
