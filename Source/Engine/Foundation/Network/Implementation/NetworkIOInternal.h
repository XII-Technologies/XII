/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/Implementation/NetworkLocalSocketAccess.h>
#include <Foundation/Network/Implementation/NetworkNativeThread.h>
#include <Foundation/Network/NetworkIOContext.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/ConditionVariable.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

#include <atomic>
#include <variant>

XII_FOUNDATION_INTERNAL_HEADER

class xiiNetworkIOBufferAccess
{
public:
  static bool IsFrom(const xiiNetworkIOBuffer& buffer, const xiiNetworkBufferPool& pool)
  {
    return buffer.m_pState.Borrow() == pool.m_pState.Borrow();
  }
};

namespace xiiNetworkIOInternal
{
  struct Worker;
  struct Registration;
  struct Operation;

  enum class SocketKind : xiiUInt8
  {
    Tcp,
    Listener,
    Udp
  };

  struct OperationQueue
  {
    Operation* m_pHead = nullptr;
    Operation* m_pTail = nullptr;
    void       Push(Operation& operation);
    void       Remove(Operation& operation);
  };

  struct Registration
  {
    std::variant<xiiTcpSocket, xiiTcpListener, xiiUdpSocket, xiiNetworkLocalSocket> m_Socket;
    SocketKind                                                                      m_Kind = SocketKind::Tcp;
    xiiNetworkIORegistration                                                        m_Handle;
    xiiUInt64                                                                       m_uiNativeToken     = 0;
    xiiUInt32                                                                       m_uiPendingCount    = 0;
    xiiUInt32                                                                       m_uiNativeInterest  = 0;
    xiiUInt32                                                                       m_uiActiveAccepts   = 0;
    bool                                                                            m_bActive           = false;
    bool                                                                            m_bClosing          = false;
    bool                                                                            m_bWriteFirst       = false;
    bool                                                                            m_bReadyQueued      = false;
    bool                                                                            m_bNativeRegistered = false;
    void*                                                                           m_pNativeData       = nullptr;
    Registration*                                                                   m_pReadyPrevious    = nullptr;
    Registration*                                                                   m_pReadyNext        = nullptr;
    OperationQueue                                                                  m_Read;
    OperationQueue                                                                  m_Write;
    xiiNetworkSocket&                                                               GetSocket();
    const xiiNetworkSocket&                                                         GetSocket() const;
    bool                                                                            IsLocal() const { return std::holds_alternative<xiiNetworkLocalSocket>(m_Socket); }
    xiiNetworkLocalSocket&                                                          GetLocalSocket() { return std::get<xiiNetworkLocalSocket>(m_Socket); }
    const xiiNetworkLocalSocket&                                                    GetLocalSocket() const { return std::get<xiiNetworkLocalSocket>(m_Socket); }
    xiiUInt64                                                                       GetNativeHandle() const;
  };

  /// Stable pool slot: never moved while an executor or the OS can reference it.
  struct Operation
  {
    Worker*                 m_pWorker       = nullptr;
    Registration*           m_pRegistration = nullptr;
    Operation*              m_pPrevious     = nullptr;
    Operation*              m_pNext         = nullptr;
    xiiNetworkIOResult      m_Result;
    xiiNetworkIOOptions     m_Options;
    xiiString               m_sHost;
    xiiString               m_sService;
    xiiNetworkAddressFamily m_ResolveFamily    = xiiNetworkAddressFamily::Unspecified;
    xiiNetworkTransport     m_ResolveTransport = xiiNetworkTransport::Tcp;
    xiiTime                 m_SubmissionTime;
    xiiTime                 m_Deadline;
    xiiNetworkError         m_CancelReason;
    xiiUInt64               m_uiGeneration      = 0;
    xiiUInt32               m_uiPoolSlot        = 0;
    xiiUInt32               m_uiTotalBytes      = 0;
    xiiUInt32               m_uiProgress        = 0;
    xiiUInt32               m_uiDeadlineIndex   = xiiInvalidIndex;
    bool                    m_bInUse            = false;
    bool                    m_bCompleted        = false;
    bool                    m_bConsumed         = false;
    bool                    m_bStarted          = false;
    bool                    m_bNativePending    = false;
    bool                    m_bConnectInitiated = false;
    bool                    m_bResolverCredit   = false;
    bool                    m_bReceiveCredit    = false;
    bool                    m_bAcceptCredit     = false;
    bool                    m_bAcceptIssued     = false;
    bool                    m_bSendCredit       = false;
    bool                    m_bReadQueue        = false;
    void*                   m_pNativeData       = nullptr;
  };

  struct NativeEvent
  {
    xiiUInt64       m_uiToken    = 0;
    Operation*      m_pOperation = nullptr;
    xiiUInt32       m_uiBytes    = 0;
    xiiNetworkError m_Error;
  };

  struct NativeAttempt
  {
    xiiNetworkOperationResult m_Transfer;
    bool                      m_bPending          = false;
    bool                      m_bWaitForReadiness = false;
  };

  /// One affinity shard, native wait resource and I/O worker. All mutable operation state is shard-locked.
  /// Windows shards use completion ports; POSIX shards use readiness with one-shot/dispatch filters.
  struct Worker
  {
    xiiNetworkIORuntime*               m_pRuntime = nullptr;
    xiiUInt32                          m_uiIndex  = 0;
    mutable xiiMutex                   m_Mutex;
    xiiDeque<Registration>             m_Registrations;
    xiiDynamicArray<xiiUInt32>         m_FreeRegistrations;
    xiiHashTable<xiiUInt64, xiiUInt32> m_NativeTokens;
    Registration*                      m_pReadyHead   = nullptr;
    Registration*                      m_pReadyTail   = nullptr;
    xiiUInt32                          m_uiReadyCount = 0;
    std::atomic<bool>                  m_bWakePending{false};
    OperationQueue                     m_Resolves;
    xiiDynamicArray<Operation*>        m_DeadlineHeap;
    xiiDynamicArray<NativeEvent>       m_Events;
    NativeThread                       m_Thread;
    void*                              m_pNativeData = nullptr;
    xiiNetworkError                    m_FatalError;
    xiiNetworkIOStatistics             m_Statistics;
    xiiTime                            m_WaitBegin;
    bool                               m_bWaiting  = false;
    bool                               m_bStopping = false;

    Registration* Find(xiiNetworkIORegistration handle);
    Operation*    Find(xiiNetworkIOOperation handle);
    void          Ready(Registration& registration, bool bWake);
    void          Unready(Registration& registration);
    void          ProcessReady();
    void          ProcessRegistration(Registration& registration);
    void          ProcessOperation(Operation& operation);
    void          ApplyTransfer(Operation& operation, const xiiNetworkOperationResult& transfer);
    void          Complete(Operation& operation, xiiNetworkError error);
    void          Cancel(Operation& operation, xiiNetworkError reason);
    void          Cancel(Registration& registration, bool bClose, xiiNetworkError reason = xiiNetworkError(xiiNetworkErrorCode::Cancelled));
    void          CloseIfDrained(Registration& registration);
    void          AddDeadline(Operation& operation);
    void          RemoveDeadline(Operation& operation);
    void          FixDeadline(xiiUInt32 uiIndex);
    void          Expire();
    xiiInt32      GetWaitMilliseconds() const;
    void          Run();
  };

  xiiTime                   CurrentThreadCpuTime();
  xiiNetworkError           CreateWorker(Worker& worker);
  void                      DestroyWorker(Worker& worker);
  void                      Wake(Worker& worker);
  xiiNetworkError           Wait(Worker& worker, xiiInt32 iMilliseconds, xiiUInt32& out_uiCount);
  xiiNetworkError           Associate(Worker& worker, Registration& registration);
  void                      Remove(Worker& worker, Registration& registration);
  xiiNetworkError           Arm(Worker& worker, Registration& registration);
  NativeAttempt             Submit(Operation& operation);
  xiiNetworkOperationResult FinishNative(Operation& operation, const NativeEvent& event);
  void                      CancelNative(Operation& operation);
  void                      DestroyOperation(Operation& operation);
  void                      DestroyRegistration(Registration& registration);

  xiiUInt32 BuildSendSpans(const Operation& operation, xiiArrayPtr<const xiiUInt8>* out_pSpans, xiiUInt32 uiLimit);
  xiiUInt32 BuildReceiveSpans(Operation& operation, xiiArrayPtr<xiiUInt8>* out_pSpans, xiiUInt32 uiLimit);
} // namespace xiiNetworkIOInternal

/// Private context implementation. Lock order: shard -> pool/completion/resolver (never the reverse).
/// Blocking native waits, resolver calls and thread joins occur without shard or queue locks.
class xiiNetworkIORuntime
{
public:
  xiiNetworkIORuntime();
  ~xiiNetworkIORuntime();
  xiiNetworkIORuntime(const xiiNetworkIORuntime&)                             = delete;
  xiiNetworkIORuntime& operator=(const xiiNetworkIORuntime&)                  = delete;
  xiiNetworkIORuntime(xiiNetworkIORuntime&&)                                  = delete;
  xiiNetworkIORuntime&                       operator=(xiiNetworkIORuntime&&) = delete;
  xiiNetworkError                            Start(const xiiNetworkIOConfiguration& configuration, xiiAllocator* pBufferAllocator);
  void                                       Shutdown();
  void                                       ShutdownUnlocked();
  template <typename Socket> xiiNetworkError Register(Socket&& socket, xiiNetworkIORegistration& out_registration);
  xiiNetworkError                            Submit(xiiNetworkIORegistration handle, xiiNetworkIOOperationKind kind, xiiNetworkIOBuffers* pBuffers,
                                                    const xiiNetworkEndpoint* pEndpoint, const xiiNetworkIOOptions& options, xiiNetworkIOOperation& out_operation, const xiiNetworkLocalEndpoint* pLocalEndpoint = nullptr);
  xiiNetworkError                            Resolve(xiiStringView sHost, xiiStringView sService, xiiNetworkAddressFamily family, xiiNetworkTransport transport,
                                                     const xiiNetworkIOOptions& options, xiiNetworkIOOperation& out_operation);
  xiiNetworkError                            ValidateOptions(const xiiNetworkIOOptions& options, xiiTime& out_deadline) const;
  xiiNetworkIOInternal::Operation*           Allocate(xiiNetworkIOInternal::Worker& worker, xiiNetworkIOOperationKind kind, xiiUInt32 uiBytes, xiiNetworkError& out_error);
  void                                       Retire(xiiNetworkIOInternal::Operation& operation);
  void                                       Publish(xiiNetworkIOInternal::Operation& operation);
  bool                                       TryGetCompletion(xiiNetworkIOResult& out_result);
  xiiNetworkError                            WaitForCompletion(xiiNetworkIOResult& out_result, xiiTime timeout);
  void                                       ResolverRun();
  xiiNetworkIOStatistics                     GetStatistics() const;

  std::atomic<xiiNetworkIOContextState>                m_State{xiiNetworkIOContextState::Created};
  xiiNetworkIOConfiguration                            m_Configuration;
  const xiiUInt64                                      m_uiContextId;
  std::atomic<xiiUInt64>                               m_uiNextToken{1};
  std::atomic<xiiUInt32>                               m_uiNextWorker{0};
  mutable xiiMutex                                     m_LifecycleMutex;
  mutable xiiMutex                                     m_PoolMutex;
  xiiDeque<xiiNetworkIOInternal::Operation>            m_Operations;
  xiiDynamicArray<xiiUInt32>                           m_FreeOperations;
  xiiDeque<xiiUniquePtr<xiiNetworkIOInternal::Worker>> m_Workers;
  xiiNetworkBufferPool                                 m_BufferPool;
  xiiUInt64                                            m_uiRegisteredCount    = 0;
  xiiUInt64                                            m_uiRetainedCount      = 0;
  xiiUInt64                                            m_uiQueuedSendBytes    = 0;
  xiiUInt32                                            m_uiReceiveRequests    = 0;
  xiiUInt32                                            m_uiResolverRequests   = 0;
  xiiUInt32                                            m_uiAcceptRequests     = 0;
  xiiUInt32                                            m_uiSubmissionCount    = 0;
  xiiUInt64                                            m_uiBackpressureEvents = 0;
  xiiThreadSignal*                                     m_pCompletionSignal    = nullptr;
  mutable xiiConditionVariable                         m_Completions;
  xiiDynamicArray<xiiNetworkIOInternal::Operation*>    m_CompletionRing;
  xiiUInt32                                            m_uiCompletionHead  = 0;
  xiiUInt32                                            m_uiCompletionCount = 0;
  xiiConditionVariable                                 m_ResolverCondition;
  xiiDeque<xiiNetworkIOInternal::Operation*>           m_ResolverQueue;
  xiiDynamicArray<xiiNetworkIOInternal::NativeThread>  m_ResolverThreads;
  bool                                                 m_bResolverStop  = false;
  bool                                                 m_bCountedActive = false;
};
