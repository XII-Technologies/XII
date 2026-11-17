/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Network/Implementation/NetworkIOInternal.h>
#  include <Foundation/Platform/Implementation/NetworkPlatform.h>
#  include <Foundation/Platform/Implementation/NetworkSocketAccess.h>
#  include <Foundation/Strings/StringConversion.h>

#  include <aclapi.h>
#  include <mswsock.h>
#  include <sddl.h>

#  include <algorithm>
#  include <climits>
#  include <cstddef>
#  include <cstdlib>
#  include <cstring>
#  include <type_traits>

namespace
{
  struct NetworkIocpState
  {
    NetworkIocpState()                                             = default;
    NetworkIocpState(const NetworkIocpState&)                      = delete;
    NetworkIocpState& operator=(const NetworkIocpState&)           = delete;
    NetworkIocpState(NetworkIocpState&&)                           = delete;
    NetworkIocpState&                operator=(NetworkIocpState&&) = delete;
    xiiNetworkInternal::RuntimeLease m_Runtime;
    HANDLE                           m_hPort = nullptr;
    ~NetworkIocpState()
    {
      if (m_hPort != nullptr)
      {
        ::CloseHandle(m_hPort);
      }
    }
  };

  struct NetworkIocpRegistration
  {
    LPFN_ACCEPTEX  m_pAccept  = nullptr;
    LPFN_CONNECTEX m_pConnect = nullptr;
  };

  struct NetworkIocpOperation
  {
    OVERLAPPED                        m_Overlapped  = {};
    xiiNetworkIOInternal::Operation*  m_pOperation  = nullptr;
    WSABUF                            m_Buffers[64] = {};
    DWORD                             m_uiFlags     = 0;
    xiiNetworkInternal::NativeAddress m_Address;
    char                              m_AcceptAddresses[2 * (sizeof(sockaddr_storage) + 16)] = {};
    xiiUInt8                          m_uiScratch                                            = 0;
  };
  static_assert(std::is_standard_layout_v<NetworkIocpOperation> && offsetof(NetworkIocpOperation, m_Overlapped) == 0);

  struct NetworkWindowsThread
  {
    HANDLE m_hThread           = nullptr;
    void (*m_pFunction)(void*) = nullptr;
    void* m_pUserData          = nullptr;
  };

  DWORD WINAPI NetworkWindowsThreadMain(void* pData)
  {
    auto* pThread = static_cast<NetworkWindowsThread*>(pData);
    pThread->m_pFunction(pThread->m_pUserData);
    return 0;
  }

  xiiNetworkError NetworkCompletionError(DWORD uiError)
  {
    if (uiError == ERROR_SUCCESS)
    {
      return {};
    }
    xiiNetworkErrorCode code = xiiNetworkErrorCode::Unknown;
    switch (uiError)
    {
      case ERROR_OPERATION_ABORTED:
        code = xiiNetworkErrorCode::Cancelled;
        break;
      case ERROR_NETNAME_DELETED:
        code = xiiNetworkErrorCode::ConnectionReset;
        break;
      case ERROR_CONNECTION_ABORTED:
        code = xiiNetworkErrorCode::ConnectionAborted;
        break;
      case ERROR_CONNECTION_REFUSED:
        code = xiiNetworkErrorCode::ConnectionRefused;
        break;
      case ERROR_BROKEN_PIPE:
      case ERROR_NO_DATA:
      case ERROR_PIPE_NOT_CONNECTED:
        code = xiiNetworkErrorCode::ConnectionClosed;
        break;
      case ERROR_PIPE_BUSY:
      case ERROR_FILE_NOT_FOUND:
        code = xiiNetworkErrorCode::ConnectionRefused;
        break;
      case ERROR_ACCESS_DENIED:
        code = xiiNetworkErrorCode::PermissionDenied;
        break;
      case ERROR_SEM_TIMEOUT:
      case ERROR_TIMEOUT:
        code = xiiNetworkErrorCode::TimedOut;
        break;
      case ERROR_NETWORK_UNREACHABLE:
        code = xiiNetworkErrorCode::NetworkUnreachable;
        break;
      case ERROR_HOST_UNREACHABLE:
        code = xiiNetworkErrorCode::HostUnreachable;
        break;
      case ERROR_NOT_ENOUGH_MEMORY:
      case ERROR_OUTOFMEMORY:
        code = xiiNetworkErrorCode::ResourceExhausted;
        break;
      default:
        code = xiiNetworkInternal::TranslateSocketError(static_cast<int>(uiError)).m_Code;
        break;
    }
    return xiiNetworkError(code, static_cast<xiiInt32>(uiError), xiiNetworkErrorDomain::NativeCompletion);
  }

  xiiNetworkError SidText(PSID pSid, xiiString& out_sSid)
  {
    LPSTR szSid = nullptr;
    if (!::ConvertSidToStringSidA(pSid, &szSid))
    {
      return NetworkCompletionError(::GetLastError());
    }
    out_sSid = szSid;
    ::LocalFree(szSid);
    return {};
  }

  xiiNetworkError TokenSid(bool bThread, xiiString& out_sSid)
  {
    HANDLE hToken = nullptr;
    if (!(bThread ? ::OpenThreadToken(::GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken) :
                    ::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken)))
    {
      return NetworkCompletionError(::GetLastError());
    }
    alignas(TOKEN_USER) xiiUInt8 token[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE] = {};
    DWORD                        uiSize                                            = 0;
    const BOOL                   bSuccess                                          = ::GetTokenInformation(hToken, TokenUser, token, sizeof(token), &uiSize);
    const DWORD                  uiError                                           = bSuccess ? ERROR_SUCCESS : ::GetLastError();
    const auto                   error                                             = bSuccess ? SidText(reinterpret_cast<TOKEN_USER*>(token)->User.Sid, out_sSid) : NetworkCompletionError(uiError);
    ::CloseHandle(hToken);
    return error;
  }

  xiiStringBuilder LocalPipeName(const xiiNetworkLocalEndpoint& endpoint, xiiStringView sSid)
  {
    xiiStringBuilder name;
    name.SetFormat(R"(\\.\pipe\xii-ipc-{0}-{1})", sSid, endpoint.GetName());
    return name;
  }

  HANDLE CreateLocalPipe(const xiiNetworkLocalEndpoint& endpoint, bool bFirst, xiiNetworkError& out_error)
  {
    xiiString sSid;
    out_error = TokenSid(false, sSid);
    if (out_error.Failed())
    {
      return INVALID_HANDLE_VALUE;
    }
    xiiStringBuilder descriptor;
    descriptor.SetFormat("O:{0}D:P(A;;GA;;;{0})", sSid);
    PSECURITY_DESCRIPTOR pDescriptor = nullptr;
    if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(xiiStringWChar(descriptor).GetData(), SDDL_REVISION_1, &pDescriptor, nullptr))
    {
      out_error = NetworkCompletionError(::GetLastError());
      return INVALID_HANDLE_VALUE;
    }
    SECURITY_ATTRIBUTES attributes = {sizeof(attributes), pDescriptor, FALSE};
    const HANDLE        hPipe      = ::CreateNamedPipeW(xiiStringWChar(LocalPipeName(endpoint, sSid)).GetData(),
                                                        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | (bFirst ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),
                                                        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, PIPE_UNLIMITED_INSTANCES, 65536, 65536, 0, &attributes);
    const DWORD         uiError    = hPipe == INVALID_HANDLE_VALUE ? ::GetLastError() : ERROR_SUCCESS;
    ::LocalFree(pDescriptor);
    out_error = NetworkCompletionError(uiError);
    if (bFirst && uiError == ERROR_ACCESS_DENIED)
    {
      out_error.m_Code = xiiNetworkErrorCode::AddressInUse;
    }
    return hPipe;
  }

  xiiNetworkError VerifyLocalPipeOwner(HANDLE hPipe, xiiStringView sExpected)
  {
    PSID                 pOwner      = nullptr;
    PSECURITY_DESCRIPTOR pDescriptor = nullptr;
    const DWORD          uiError     = ::GetSecurityInfo(hPipe, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pOwner, nullptr, nullptr, nullptr, &pDescriptor);
    if (uiError != ERROR_SUCCESS)
    {
      return xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed, static_cast<xiiInt32>(uiError), xiiNetworkErrorDomain::NativeCompletion);
    }
    xiiString  sOwner;
    const auto error = SidText(pOwner, sOwner);
    ::LocalFree(pDescriptor);
    return error.Succeeded() && sOwner == sExpected ? xiiNetworkError() : xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed);
  }

  xiiNetworkError VerifyLocalPipeClient(HANDLE hPipe, xiiString& out_sSid)
  {
    xiiString sExpected;
    if (TokenSid(false, sExpected).Failed() || !::ImpersonateNamedPipeClient(hPipe))
    {
      return xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed);
    }
    const auto error = TokenSid(true, out_sSid);
    if (!::RevertToSelf())
    {
      std::abort(); // A native worker must never run another request with an impersonated token.
    }
    return error.Succeeded() && out_sSid == sExpected ? xiiNetworkError() : xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed);
  }

  xiiNetworkError AssociateLocalPipe(xiiNetworkIOInternal::Worker& worker, HANDLE hPipe)
  {
    const auto& state = *static_cast<NetworkIocpState*>(worker.m_pNativeData);
    return ::CreateIoCompletionPort(hPipe, state.m_hPort, 1, 0) != nullptr ? xiiNetworkError() : NetworkCompletionError(::GetLastError());
  }

  xiiNetworkOperationResult FinishLocalPipe(xiiNetworkIOInternal::Operation& operation, xiiNetworkError error, xiiUInt32 uiBytes)
  {
    using namespace xiiNetworkIOInternal;
    auto&      socket = operation.m_pRegistration->GetLocalSocket();
    const auto hPipe  = reinterpret_cast<HANDLE>(LocalSocketAccess::Native(socket));
    if (operation.m_Result.m_Kind == xiiNetworkIOOperationKind::Accept)
    {
      if (error.Succeeded() && operation.m_CancelReason.Succeeded())
      {
        // Keep a listening instance alive before transferring the completed instance.
        const HANDLE hNext = CreateLocalPipe(socket.GetEndpoint(), false, error);
        if (error.Succeeded())
        {
          error = AssociateLocalPipe(*operation.m_pWorker, hNext);
          if (error.Failed())
          {
            ::CloseHandle(hNext);
          }
          else
          {
            auto& accepted                      = operation.m_Result.m_AcceptedLocalSocket;
            LocalSocketAccess::Native(accepted) = reinterpret_cast<xiiUInt64>(hPipe);
            LocalSocketAccess::SetConnected(accepted);
            LocalSocketAccess::SetEndpoint(accepted, socket.GetEndpoint());
            LocalSocketAccess::SetAffinity(accepted, operation.m_pWorker->m_pRuntime->m_uiContextId, operation.m_pWorker->m_uiIndex);
            LocalSocketAccess::Native(socket) = reinterpret_cast<xiiUInt64>(hNext);
            return {};
          }
        }
      }
      if (hPipe != INVALID_HANDLE_VALUE)
      {
        ::DisconnectNamedPipe(hPipe);
      }
      return {error, 0};
    }
    if (operation.m_Result.m_Kind == xiiNetworkIOOperationKind::Receive && error.Succeeded())
    {
      if (uiBytes == 0)
      {
        return {xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed), 0};
      }
      auto& sIdentity = LocalSocketAccess::Identity(socket);
      if (sIdentity.IsEmpty() && operation.m_CancelReason.Succeeded())
      {
        ULONG uiPeerProcessId = 0;
        error = VerifyLocalPipeClient(hPipe, sIdentity);
        if (error.Failed())
        {
          xiiArrayPtr<xiiUInt8> spans[64];
          BuildReceiveSpans(operation, spans, uiBytes);
          std::memset(spans[0].GetPtr(), 0, uiBytes);
          sIdentity.Clear();
          operation.m_pRegistration->m_bClosing = true;
          return {error, 0};
        }
        if (::GetNamedPipeClientProcessId(hPipe, &uiPeerProcessId))
        {
          LocalSocketAccess::PeerProcessId(socket) = uiPeerProcessId;
        }
      }
    }
    return {error, uiBytes};
  }

  xiiNetworkIOInternal::NativeAttempt SubmitLocalPipe(xiiNetworkIOInternal::Operation& operation, OVERLAPPED& ref_overlapped)
  {
    using namespace xiiNetworkIOInternal;
    auto&      socket = operation.m_pRegistration->GetLocalSocket();
    const auto kind   = operation.m_Result.m_Kind;
    if (kind == xiiNetworkIOOperationKind::Connect)
    {
      xiiString sSid;
      auto      error = TokenSid(false, sSid);
      if (error.Failed())
      {
        return {{error, 0}, false};
      }
      const auto&  endpoint = operation.m_Result.m_LocalEndpoint;
      const HANDLE hPeer    = ::CreateFileW(xiiStringWChar(LocalPipeName(endpoint, sSid)).GetData(),
                                            FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | READ_CONTROL | SYNCHRONIZE,
                                            0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
      if (hPeer == INVALID_HANDLE_VALUE)
      {
        return {{NetworkCompletionError(::GetLastError()), 0}, false};
      }
      error = VerifyLocalPipeOwner(hPeer, sSid);
      if (error.Succeeded())
      {
        error = AssociateLocalPipe(*operation.m_pWorker, hPeer);
      }
      if (error.Failed())
      {
        ::CloseHandle(hPeer);
        return {{error, 0}, false};
      }
      LocalSocketAccess::Native(socket) = reinterpret_cast<xiiUInt64>(hPeer);
      LocalSocketAccess::SetEndpoint(socket, endpoint);
      LocalSocketAccess::SetConnected(socket);
      LocalSocketAccess::Identity(socket) = std::move(sSid);
      ULONG uiPeerProcessId               = 0;
      if (::GetNamedPipeServerProcessId(hPeer, &uiPeerProcessId))
      {
        LocalSocketAccess::PeerProcessId(socket) = uiPeerProcessId;
      }
      return {};
    }
    const auto hPipe    = reinterpret_cast<HANDLE>(LocalSocketAccess::Native(socket));
    BOOL       bSuccess = FALSE;
    if (kind == xiiNetworkIOOperationKind::Accept)
    {
      bSuccess = ::ConnectNamedPipe(hPipe, &ref_overlapped);
    }
    else if (kind == xiiNetworkIOOperationKind::Send)
    {
      xiiArrayPtr<const xiiUInt8> spans[64];
      BuildSendSpans(operation, spans, operation.m_pWorker->m_pRuntime->m_Configuration.m_uiMaxTransferBytesPerAttempt);
      bSuccess = ::WriteFile(hPipe, spans[0].GetPtr(), spans[0].GetCount(), nullptr, &ref_overlapped);
    }
    else
    {
      xiiArrayPtr<xiiUInt8> spans[64];
      BuildReceiveSpans(operation, spans, operation.m_pWorker->m_pRuntime->m_Configuration.m_uiMaxTransferBytesPerAttempt);
      bSuccess = ::ReadFile(hPipe, spans[0].GetPtr(), spans[0].GetCount(), nullptr, &ref_overlapped);
    }
    const DWORD uiError = bSuccess ? ERROR_SUCCESS : ::GetLastError();
    if (bSuccess || uiError == ERROR_IO_PENDING)
    {
      return {{}, true};
    }
    if (kind == xiiNetworkIOOperationKind::Accept)
    {
      return {FinishLocalPipe(operation, uiError == ERROR_PIPE_CONNECTED ? xiiNetworkError() : NetworkCompletionError(uiError), 0), false};
    }
    return {{NetworkCompletionError(uiError), 0}, false};
  }

  template <typename Function>
  xiiNetworkError NetworkExtension(SOCKET hSocket, GUID guid, Function& out_pFunction)
  {
    DWORD uiBytes = 0;
    if (::WSAIoctl(hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &out_pFunction, sizeof(out_pFunction), &uiBytes, nullptr, nullptr) != 0)
    {
      return xiiNetworkInternal::TranslateSocketError(::WSAGetLastError());
    }
    return {};
  }
} // namespace

namespace xiiNetworkIOInternal
{
  xiiNetworkError LocalSocketAccess::Open(xiiNetworkLocalSocket& ref_socket)
  {
    ref_socket.m_bOpen = true;
    return {};
  }

  xiiNetworkError LocalSocketAccess::Listen(xiiNetworkLocalSocket& ref_socket, const xiiNetworkLocalEndpoint& endpoint, xiiInt32 iBacklog)
  {
    if (!endpoint.IsValid() || iBacklog <= 0)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    xiiNetworkError error;
    const HANDLE    hPipe = CreateLocalPipe(endpoint, true, error);
    if (error.Succeeded())
    {
      ref_socket.m_uiNativeHandle = reinterpret_cast<xiiUInt64>(hPipe);
      ref_socket.m_Endpoint       = endpoint;
      ref_socket.m_bOpen = ref_socket.m_bListener = true;
    }
    return error;
  }

  xiiNetworkError LocalSocketAccess::PeerIdentity(const xiiNetworkLocalSocket& socket, xiiString& out_sIdentity, xiiUInt32* out_pProcessId)
  {
    if (socket.m_sPeerIdentity.IsEmpty())
    {
      return xiiNetworkError(xiiNetworkErrorCode::InProgress);
    }
    out_sIdentity = socket.m_sPeerIdentity;
    if (out_pProcessId != nullptr)
    {
      *out_pProcessId = socket.m_uiPeerProcessId;
    }
    return {};
  }

  void LocalSocketAccess::Close(xiiNetworkLocalSocket& ref_socket)
  {
    if (ref_socket.m_uiNativeHandle != xiiNetworkLocalSocket::InvalidHandle)
    {
      ::CloseHandle(reinterpret_cast<HANDLE>(ref_socket.m_uiNativeHandle));
      ref_socket.m_uiNativeHandle = xiiNetworkLocalSocket::InvalidHandle;
    }
  }

  xiiTime CurrentThreadCpuTime()
  {
    FILETIME creation = {}, exit = {}, kernel = {}, user = {};
    if (!::GetThreadTimes(::GetCurrentThread(), &creation, &exit, &kernel, &user))
    {
      return {};
    }
    const xiiUInt64 uiKernel = (static_cast<xiiUInt64>(kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime;
    const xiiUInt64 uiUser   = (static_cast<xiiUInt64>(user.dwHighDateTime) << 32) | user.dwLowDateTime;
    return xiiTime::MakeFromSeconds(static_cast<double>(uiKernel + uiUser) / 10000000.0);
  }

  xiiNetworkError StartThread(NativeThread& thread, void (*pFunction)(void*), void* pUserData)
  {
    NetworkWindowsThread* pThread = XII_DEFAULT_NEW(NetworkWindowsThread);
    pThread->m_pFunction          = pFunction;
    pThread->m_pUserData          = pUserData;
    pThread->m_hThread            = ::CreateThread(nullptr, 0, NetworkWindowsThreadMain, pThread, 0, nullptr);
    if (pThread->m_hThread == nullptr)
    {
      auto error     = NetworkCompletionError(::GetLastError());
      error.m_Domain = xiiNetworkErrorDomain::Initialization;
      if (error.m_Code != xiiNetworkErrorCode::ResourceExhausted)
      {
        error.m_Code = xiiNetworkErrorCode::InitializationFailed;
      }
      XII_DEFAULT_DELETE(pThread);
      return error;
    }
    thread.m_pData = pThread;
    return {};
  }

  void JoinThread(NativeThread& thread)
  {
    if (thread.m_pData != nullptr)
    {
      auto*       pThread  = static_cast<NetworkWindowsThread*>(thread.m_pData);
      const DWORD uiResult = ::WaitForSingleObject(pThread->m_hThread, INFINITE);
      XII_ASSERT_ALWAYS(uiResult == WAIT_OBJECT_0, "Owned worker threads must be joined before native state is released.");
      ::CloseHandle(pThread->m_hThread);
      XII_DEFAULT_DELETE(pThread);
      thread.m_pData = nullptr;
    }
  }

  xiiNetworkError CreateWorker(Worker& worker)
  {
    xiiUniquePtr<NetworkIocpState> pState = XII_DEFAULT_NEW(NetworkIocpState);
    if (pState->m_Runtime.GetError().Failed())
    {
      return pState->m_Runtime.GetError();
    }
    // Bounded affinity shards, each serving many sockets. Never one port or thread per connection.
    pState->m_hPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
    if (pState->m_hPort == nullptr)
    {
      return NetworkCompletionError(::GetLastError());
    }
    worker.m_Statistics.m_uiInternalMemoryBytes += sizeof(NetworkIocpState);
    worker.m_pNativeData = pState.Release();
    return {};
  }

  void DestroyWorker(Worker& worker)
  {
    auto* pState = static_cast<NetworkIocpState*>(worker.m_pNativeData);
    if (pState != nullptr)
    {
      worker.m_Statistics.m_uiInternalMemoryBytes -= sizeof(NetworkIocpState);
    }
    XII_DEFAULT_DELETE(pState);
    worker.m_pNativeData = nullptr;
  }

  void Wake(Worker& worker)
  {
    if (worker.m_pNativeData == nullptr || worker.m_bWakePending.exchange(true))
    {
      return;
    }
    auto&      state   = *static_cast<NetworkIocpState*>(worker.m_pNativeData);
    const BOOL bPosted = ::PostQueuedCompletionStatus(state.m_hPort, 0, 0, nullptr);
    XII_ASSERT_ALWAYS(bPosted != FALSE, "The owned completion port must remain available until every worker joins.");
  }

  xiiNetworkError Wait(Worker& worker, xiiInt32 iMilliseconds, xiiUInt32& out_uiCount)
  {
    auto& state = *static_cast<NetworkIocpState*>(worker.m_pNativeData);
    out_uiCount = 0;
    for (xiiUInt32 uiIndex = 0; uiIndex < worker.m_Events.GetCount(); ++uiIndex)
    {
      DWORD       uiBytes     = 0;
      ULONG_PTR   uiKey       = 0;
      OVERLAPPED* pOverlapped = nullptr;
      const DWORD uiTimeout   = uiIndex != 0 ? 0 : (iMilliseconds < 0 ? INFINITE : static_cast<DWORD>(iMilliseconds));
      const BOOL  bSuccess    = ::GetQueuedCompletionStatus(state.m_hPort, &uiBytes, &uiKey, &pOverlapped, uiTimeout);
      const DWORD uiError     = bSuccess ? ERROR_SUCCESS : ::GetLastError();
      if (pOverlapped == nullptr)
      {
        if (!bSuccess && uiError != WAIT_TIMEOUT)
        {
          return NetworkCompletionError(uiError);
        }
        if (!bSuccess)
        {
          break;
        }
        worker.m_bWakePending.store(false);
        // Null-overlapped packets are coalesced control wakeups, never operation completions.
        continue;
      }
      auto*        pNative = reinterpret_cast<NetworkIocpOperation*>(pOverlapped);
      NativeEvent& event   = worker.m_Events[out_uiCount++];
      event                = {};
      event.m_pOperation   = pNative->m_pOperation;
      event.m_uiBytes      = uiBytes;
      event.m_Error        = NetworkCompletionError(uiError);
    }
    return {};
  }

  xiiNetworkError Associate(Worker& worker, Registration& registration)
  {
    if (registration.IsLocal())
    {
      const auto& socket = registration.GetLocalSocket();
      const auto  hPipe  = reinterpret_cast<HANDLE>(registration.GetNativeHandle());
      return hPipe == INVALID_HANDLE_VALUE || LocalSocketAccess::GetContext(socket) != 0 ? xiiNetworkError() : AssociateLocalPipe(worker, hPipe);
    }
    if (registration.m_pNativeData == nullptr)
    {
      NetworkIocpRegistration* pData = XII_DEFAULT_NEW(NetworkIocpRegistration);
      registration.m_pNativeData     = pData;
      worker.m_Statistics.m_uiInternalMemoryBytes += sizeof(NetworkIocpRegistration);
    }
    auto& native         = *static_cast<NetworkIocpRegistration*>(registration.m_pNativeData);
    native               = {};
    auto&        state   = *static_cast<NetworkIocpState*>(worker.m_pNativeData);
    const SOCKET hSocket = static_cast<SOCKET>(xiiNetworkSocketAccess::BorrowNativeHandle(registration.GetSocket()));
    if (::CreateIoCompletionPort(reinterpret_cast<HANDLE>(hSocket), state.m_hPort, 1, 0) == nullptr)
    {
      return NetworkCompletionError(::GetLastError());
    }
    return {};
  }

  void Remove(Worker&, Registration& registration)
  {
    // Windows association ends when the owning socket closes, after all OVERLAPPED packets drain.
    registration.m_bNativeRegistered = false;
  }
  xiiNetworkError Arm(Worker&, Registration&) { return {}; }

  NativeAttempt Submit(Operation& operation)
  {
    using namespace xiiNetworkInternal;
    Registration& registration = *operation.m_pRegistration;
    const auto    kind         = operation.m_Result.m_Kind;
    if (operation.m_pNativeData == nullptr)
    {
      NetworkIocpOperation* pData = XII_DEFAULT_NEW(NetworkIocpOperation);
      operation.m_pNativeData     = pData;
      operation.m_pWorker->m_Statistics.m_uiInternalMemoryBytes += sizeof(NetworkIocpOperation);
    }
    auto& native        = *static_cast<NetworkIocpOperation*>(operation.m_pNativeData);
    native.m_Overlapped = {};
    native.m_pOperation = &operation;
    native.m_uiFlags    = 0;
    native.m_Address    = {};
    if (registration.IsLocal())
    {
      return SubmitLocalPipe(operation, native.m_Overlapped);
    }
    const SOCKET hSocket = static_cast<SOCKET>(xiiNetworkSocketAccess::BorrowNativeHandle(registration.GetSocket()));
    DWORD        uiBytes = 0;
    int          iResult = SOCKET_ERROR;
    if (kind == xiiNetworkIOOperationKind::Accept)
    {
      auto& extensions = *static_cast<NetworkIocpRegistration*>(registration.m_pNativeData);
      if (extensions.m_pAccept == nullptr)
      {
        const auto error = NetworkExtension(hSocket, WSAID_ACCEPTEX, extensions.m_pAccept);
        if (error.Failed())
        {
          return {{error, 0}, false};
        }
      }
      auto error = operation.m_Result.m_AcceptedSocket.Open(registration.GetSocket().GetFamily());
      if (error.Failed())
      {
        return {{error, 0}, false};
      }
      if (registration.GetSocket().GetFamily() == xiiNetworkAddressFamily::Ipv6)
      {
        int iOnly = 1;
        int iSize = sizeof(iOnly);
        if (::getsockopt(hSocket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&iOnly), &iSize) != 0)
        {
          return {{TranslateSocketError(::WSAGetLastError()), 0}, false};
        }
        error = operation.m_Result.m_AcceptedSocket.SetIpv6Only(iOnly != 0);
        if (error.Failed())
        {
          return {{error, 0}, false};
        }
      }
      const SOCKET hAccepted = static_cast<SOCKET>(xiiNetworkSocketAccess::BorrowNativeHandle(operation.m_Result.m_AcceptedSocket));
      iResult                = extensions.m_pAccept(hSocket, hAccepted, native.m_AcceptAddresses, 0,
                                                    sizeof(sockaddr_storage) + 16, sizeof(sockaddr_storage) + 16, &uiBytes, &native.m_Overlapped) ?
        0 :
        SOCKET_ERROR;
    }
    else if (kind == xiiNetworkIOOperationKind::Connect)
    {
      auto& extensions = *static_cast<NetworkIocpRegistration*>(registration.m_pNativeData);
      if (extensions.m_pConnect == nullptr)
      {
        const auto error = NetworkExtension(hSocket, WSAID_CONNECTEX, extensions.m_pConnect);
        if (error.Failed())
        {
          return {{error, 0}, false};
        }
      }
      auto&              socket = *std::get_if<xiiTcpSocket>(&registration.m_Socket);
      xiiNetworkEndpoint local;
      const auto         localError = socket.GetLocalEndpoint(local);
      if ((localError.Succeeded() && local.GetPort() == 0) || localError.m_Code == xiiNetworkErrorCode::InvalidArgument)
      {
        const auto bound = socket.Bind(xiiNetworkEndpoint(xiiNetworkAddress::MakeWildcard(socket.GetFamily()), 0));
        if (bound.Failed())
        {
          return {{bound, 0}, false};
        }
      }
      else if (localError.Failed())
      {
        return {{localError, 0}, false};
      }
      const auto error = ToNative(operation.m_Result.m_Endpoint, native.m_Address);
      if (error.Failed())
      {
        return {{error, 0}, false};
      }
      xiiNetworkSocketAccess::SetState(socket, xiiNetworkSocketState::Connecting, xiiNetworkError(xiiNetworkErrorCode::InProgress));
      iResult = extensions.m_pConnect(hSocket, native.m_Address.Get(), native.m_Address.m_uiLength, nullptr, 0, &uiBytes, &native.m_Overlapped) ? 0 : SOCKET_ERROR;
    }
    else
    {
      DWORD           uiCount = 0;
      const bool      bSend   = kind == xiiNetworkIOOperationKind::Send || kind == xiiNetworkIOOperationKind::SendTo;
      const xiiUInt32 uiLimit = kind == xiiNetworkIOOperationKind::SendTo || kind == xiiNetworkIOOperationKind::ReceiveFrom ?
        static_cast<xiiUInt32>(INT_MAX) :
        operation.m_pWorker->m_pRuntime->m_Configuration.m_uiMaxTransferBytesPerAttempt;
      if (bSend)
      {
        xiiArrayPtr<const xiiUInt8> spans[64];
        uiCount = BuildSendSpans(operation, spans, uiLimit);
        for (DWORD uiIndex = 0; uiIndex < uiCount; ++uiIndex)
        {
          native.m_Buffers[uiIndex].buf = reinterpret_cast<char*>(const_cast<xiiUInt8*>(spans[uiIndex].GetPtr()));
          native.m_Buffers[uiIndex].len = spans[uiIndex].GetCount();
        }
      }
      else
      {
        xiiArrayPtr<xiiUInt8> spans[64];
        uiCount = BuildReceiveSpans(operation, spans, uiLimit);
        for (DWORD uiIndex = 0; uiIndex < uiCount; ++uiIndex)
        {
          native.m_Buffers[uiIndex].buf = reinterpret_cast<char*>(spans[uiIndex].GetPtr());
          native.m_Buffers[uiIndex].len = spans[uiIndex].GetCount();
        }
      }
      if (uiCount == 0)
      {
        native.m_Buffers[0].buf = reinterpret_cast<char*>(&native.m_uiScratch);
        native.m_Buffers[0].len = bSend ? 0 : 1;
        uiCount                 = 1;
      }
      switch (kind)
      {
        case xiiNetworkIOOperationKind::Send:
          iResult = ::WSASend(hSocket, native.m_Buffers, uiCount, &uiBytes, 0, &native.m_Overlapped, nullptr);
          break;
        case xiiNetworkIOOperationKind::Receive:
          iResult = ::WSARecv(hSocket, native.m_Buffers, uiCount, &uiBytes, &native.m_uiFlags, &native.m_Overlapped, nullptr);
          break;
        case xiiNetworkIOOperationKind::SendTo:
        {
          const auto error = ToNative(operation.m_Result.m_Endpoint, native.m_Address);
          if (error.Failed())
          {
            return {{error, 0}, false};
          }
          iResult = ::WSASendTo(hSocket, native.m_Buffers, uiCount, &uiBytes, 0, native.m_Address.Get(), native.m_Address.m_uiLength, &native.m_Overlapped, nullptr);
          break;
        }
        case xiiNetworkIOOperationKind::ReceiveFrom:
          iResult = ::WSARecvFrom(hSocket, native.m_Buffers, uiCount, &uiBytes, &native.m_uiFlags, native.m_Address.Get(), &native.m_Address.m_uiLength, &native.m_Overlapped, nullptr);
          break;
        default:
          return {{xiiNetworkError(xiiNetworkErrorCode::InvalidArgument), 0}, false};
      }
    }
    if (iResult == 0 || ::WSAGetLastError() == WSA_IO_PENDING)
    {
      // Even immediate success posts a completion packet. Never complete or recycle it inline.
      return {{}, true};
    }
    const auto error = TranslateSocketError(::WSAGetLastError());
    if (kind == xiiNetworkIOOperationKind::ReceiveFrom && error.m_iNativeCode == WSAEMSGSIZE)
    {
      // An immediate error did not initiate an overlapped request, but an oversized UDP
      // datagram was consumed and its prefix copied. Normalize it through the same result path.
      NativeEvent event;
      event.m_Error   = error;
      event.m_uiBytes = operation.m_uiTotalBytes;
      return {FinishNative(operation, event), false};
    }
    if (kind == xiiNetworkIOOperationKind::Connect)
    {
      xiiNetworkSocketAccess::SetState(registration.GetSocket(), xiiNetworkSocketState::Failed, error);
    }
    return {{error, 0}, false};
  }

  xiiNetworkOperationResult FinishNative(Operation& operation, const NativeEvent& event)
  {
    using namespace xiiNetworkInternal;
    auto&         native       = *static_cast<NetworkIocpOperation*>(operation.m_pNativeData);
    Registration& registration = *operation.m_pRegistration;
    auto          error        = event.m_Error;
    xiiUInt32     uiBytes      = event.m_uiBytes;
    const auto    kind         = operation.m_Result.m_Kind;
    if (registration.IsLocal())
    {
      return FinishLocalPipe(operation, error, uiBytes);
    }
    if (kind == xiiNetworkIOOperationKind::ReceiveFrom &&
        (error.m_iNativeCode == WSAEMSGSIZE || error.m_iNativeCode == ERROR_MORE_DATA || (native.m_uiFlags & MSG_PARTIAL) != 0))
    {
      error.m_Code = xiiNetworkErrorCode::Truncated;
      uiBytes      = operation.m_uiTotalBytes;
    }
    if (kind == xiiNetworkIOOperationKind::ReceiveFrom && operation.m_uiTotalBytes == 0)
    {
      if (uiBytes != 0 && error.Succeeded())
      {
        error = xiiNetworkError(xiiNetworkErrorCode::Truncated);
      }
      uiBytes = 0;
    }
    if (kind == xiiNetworkIOOperationKind::ReceiveFrom && (error.Succeeded() || error.m_Code == xiiNetworkErrorCode::Truncated))
    {
      const auto address = FromNative(native.m_Address.Get(), native.m_Address.m_uiLength, operation.m_Result.m_Endpoint);
      if (address.Failed())
      {
        error = address;
      }
    }
    if (kind == xiiNetworkIOOperationKind::Accept && error.Succeeded())
    {
      const SOCKET hListener = static_cast<SOCKET>(xiiNetworkSocketAccess::BorrowNativeHandle(registration.GetSocket()));
      auto&        accepted  = operation.m_Result.m_AcceptedSocket;
      const SOCKET hAccepted = static_cast<SOCKET>(xiiNetworkSocketAccess::BorrowNativeHandle(accepted));
      if (::setsockopt(hAccepted, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<const char*>(&hListener), sizeof(hListener)) != 0)
      {
        error = TranslateSocketError(::WSAGetLastError());
      }
      else
      {
        xiiNetworkSocketAccess::SetState(accepted, xiiNetworkSocketState::Connected);
        error = accepted.GetPeerEndpoint(operation.m_Result.m_Endpoint);
      }
      uiBytes = 0;
    }
    if (kind == xiiNetworkIOOperationKind::Connect)
    {
      if (error.Succeeded())
      {
        const SOCKET hSocket = static_cast<SOCKET>(xiiNetworkSocketAccess::BorrowNativeHandle(registration.GetSocket()));
        if (::setsockopt(hSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0) != 0)
        {
          error = TranslateSocketError(::WSAGetLastError());
        }
      }
      xiiNetworkSocketAccess::SetState(registration.GetSocket(), error.Succeeded() ? xiiNetworkSocketState::Connected : xiiNetworkSocketState::Failed, error);
      uiBytes = 0;
    }
    if (kind == xiiNetworkIOOperationKind::Receive && error.Succeeded() && uiBytes == 0)
    {
      error = xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed);
    }
    if (kind == xiiNetworkIOOperationKind::SendTo && error.Succeeded() && uiBytes != operation.m_uiTotalBytes)
    {
      error = xiiNetworkError(xiiNetworkErrorCode::Unknown);
    }
    return {error, uiBytes};
  }

  void CancelNative(Operation& operation)
  {
    auto&      native       = *static_cast<NetworkIocpOperation*>(operation.m_pNativeData);
    auto&      registration = *operation.m_pRegistration;
    const auto hSocket      = registration.GetNativeHandle();
    if (!::CancelIoEx(reinterpret_cast<HANDLE>(hSocket), &native.m_Overlapped) && ::GetLastError() != ERROR_NOT_FOUND)
    {
      // A provider that cannot cancel an individual request must be aborted at socket scope.
      // The shard's Winsock lease and operation storage remain alive until IOCP packets drain.
      registration.m_bClosing = true;
      if (registration.IsLocal())
      {
        registration.GetLocalSocket().Close();
      }
      else
      {
        registration.GetSocket().Close().IgnoreResult();
      }
    }
  }

  void DestroyOperation(Operation& operation)
  {
    auto* pData = static_cast<NetworkIocpOperation*>(operation.m_pNativeData);
    XII_DEFAULT_DELETE(pData);
    operation.m_pNativeData = nullptr;
  }

  void DestroyRegistration(Registration& registration)
  {
    auto* pData = static_cast<NetworkIocpRegistration*>(registration.m_pNativeData);
    XII_DEFAULT_DELETE(pData);
    registration.m_pNativeData = nullptr;
  }

} // namespace xiiNetworkIOInternal

#endif
