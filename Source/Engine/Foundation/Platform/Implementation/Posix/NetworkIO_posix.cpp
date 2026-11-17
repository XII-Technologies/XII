/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)

#  include <Foundation/Network/Implementation/NetworkIOInternal.h>
#  include <Foundation/Platform/Implementation/NetworkPlatform.h>
#  include <Foundation/Strings/StringBuilder.h>

#  include <cerrno>
#  include <cstring>
#  include <fcntl.h>
#  include <pthread.h>
#  include <sys/stat.h>
#  include <sys/uio.h>
#  include <sys/un.h>
#  include <unistd.h>

#  include <climits>
#  include <variant>

namespace
{
  using namespace xiiNetworkIOInternal;
  using namespace xiiNetworkInternal;

  xiiStringBuilder LocalDirectory(const xiiNetworkLocalEndpoint& endpoint)
  {
    xiiStringBuilder path;
    path.SetFormat("/tmp/xii-ipc-{0}-{1}", static_cast<xiiUInt32>(::geteuid()), endpoint.GetName());
    return path;
  }

  sockaddr_un LocalAddress(const xiiStringBuilder& directory)
  {
    sockaddr_un address = {};
    address.sun_family  = AF_UNIX;
#  if XII_ENABLED(XII_PLATFORM_OSX)
    address.sun_len = sizeof(address);
#  endif
    xiiStringBuilder path = directory;
    path.Append("/s");
    XII_ASSERT_DEV(path.GetElementCount() < sizeof(address.sun_path), "The endpoint name must fit the native address.");
    std::memcpy(address.sun_path, path.GetData(), path.GetElementCount() + 1);
    return address;
  }

  NativeAttempt SubmitLocalPosix(Operation& operation)
  {
    auto&                     socket  = operation.m_pRegistration->GetLocalSocket();
    const int                 iSocket = static_cast<int>(LocalSocketAccess::Native(socket));
    auto&                     result  = operation.m_Result;
    xiiNetworkOperationResult transfer;
    if (result.m_Kind == xiiNetworkIOOperationKind::Accept)
    {
      const int iAccepted = AcceptNative(iSocket, nullptr, nullptr, true, transfer.m_Error);
      if (iAccepted >= 0)
      {
        auto& accepted                      = result.m_AcceptedLocalSocket;
        LocalSocketAccess::Native(accepted) = static_cast<xiiUInt64>(iAccepted);
        LocalSocketAccess::SetConnected(accepted);
        LocalSocketAccess::SetEndpoint(accepted, result.m_LocalEndpoint);
        xiiString sIdentity;
        transfer.m_Error = LocalSocketAccess::PeerIdentity(accepted, sIdentity);
      }
    }
    else if (result.m_Kind == xiiNetworkIOOperationKind::Connect)
    {
      int iError = 0;
      if (!operation.m_bConnectInitiated)
      {
        const auto address = LocalAddress(LocalDirectory(result.m_LocalEndpoint));
        if (::connect(iSocket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0)
        {
          iError = errno;
          // AF_UNIX can report EAGAIN for a full listen backlog without starting a connect.
          if (iError == EAGAIN || iError == ENOENT)
          {
            return {{xiiNetworkError(xiiNetworkErrorCode::ConnectionRefused, iError, xiiNetworkErrorDomain::Socket), 0}, false, false};
          }
        }
        operation.m_bConnectInitiated = iError == EINPROGRESS || iError == EINTR || iError == EALREADY;
      }
      else
      {
        socklen_t uiSize = sizeof(iError);
        if (::getsockopt(iSocket, SOL_SOCKET, SO_ERROR, &iError, &uiSize) != 0)
        {
          iError = errno;
        }
      }
      transfer.m_Error = TranslateSocketError(iError);
      if (transfer.m_Error.Succeeded())
      {
        xiiString sIdentity;
        transfer.m_Error = LocalSocketAccess::PeerIdentity(socket, sIdentity);
        if (transfer.m_Error.Succeeded())
        {
          LocalSocketAccess::SetConnected(socket);
          LocalSocketAccess::SetEndpoint(socket, result.m_LocalEndpoint);
        }
      }
    }
    else
    {
      iovec  vectors[64];
      msghdr message     = {};
      message.msg_iov    = vectors;
      const auto uiLimit = operation.m_pWorker->m_pRuntime->m_Configuration.m_uiMaxTransferBytesPerAttempt;
      const bool bSend   = result.m_Kind == xiiNetworkIOOperationKind::Send;
      if (bSend)
      {
        xiiArrayPtr<const xiiUInt8> spans[64];
        message.msg_iovlen = static_cast<decltype(message.msg_iovlen)>(BuildSendSpans(operation, spans, uiLimit));
        for (int i = 0; i < static_cast<int>(message.msg_iovlen); ++i)
        {
          vectors[i] = {const_cast<xiiUInt8*>(spans[i].GetPtr()), spans[i].GetCount()};
        }
      }
      else
      {
        xiiArrayPtr<xiiUInt8> spans[64];
        message.msg_iovlen = static_cast<decltype(message.msg_iovlen)>(BuildReceiveSpans(operation, spans, uiLimit));
        for (int i = 0; i < static_cast<int>(message.msg_iovlen); ++i)
        {
          vectors[i] = {spans[i].GetPtr(), spans[i].GetCount()};
        }
      }
#  if XII_ENABLED(XII_PLATFORM_LINUX)
      constexpr int SendFlags = MSG_NOSIGNAL;
#  else
      constexpr int SendFlags = 0; // Socket creation and accept set SO_NOSIGPIPE.
#  endif
      const ssize_t iBytes = bSend ? ::sendmsg(iSocket, &message, SendFlags) : ::recvmsg(iSocket, &message, 0);
      if (iBytes < 0)
      {
        transfer.m_Error = TranslateSocketError(errno);
      }
      else if (iBytes == 0)
      {
        transfer.m_Error = xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed);
      }
      else
      {
        transfer.m_uiBytesTransferred = static_cast<xiiUInt32>(iBytes);
      }
    }
    const auto code = transfer.m_Error.m_Code;
    return {transfer, false, code == xiiNetworkErrorCode::WouldBlock || code == xiiNetworkErrorCode::InProgress || code == xiiNetworkErrorCode::Interrupted};
  }

  struct NetworkPosixThread
  {
    pthread_t m_hThread        = {};
    void (*m_pFunction)(void*) = nullptr;
    void* m_pUserData          = nullptr;
  };

  void* NetworkPosixThreadMain(void* pData)
  {
    auto* pThread = static_cast<NetworkPosixThread*>(pData);
    pThread->m_pFunction(pThread->m_pUserData);
    return nullptr;
  }
} // namespace

namespace xiiNetworkIOInternal
{
  xiiNetworkError LocalSocketAccess::Open(xiiNetworkLocalSocket& ref_socket)
  {
    xiiNetworkError error;
    const int       iSocket = CreateNative(AF_UNIX, SOCK_STREAM, true, error);
    if (error.Succeeded())
    {
      ref_socket.m_uiNativeHandle = static_cast<xiiUInt64>(iSocket);
      ref_socket.m_bOpen          = true;
    }
    return error;
  }

  xiiNetworkError LocalSocketAccess::Listen(xiiNetworkLocalSocket& ref_socket, const xiiNetworkLocalEndpoint& endpoint, xiiInt32 iBacklog)
  {
    if (!endpoint.IsValid() || iBacklog <= 0)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    const auto error = Open(ref_socket);
    if (error.Failed())
    {
      return error;
    }
    const auto directory = LocalDirectory(endpoint);
    if (::mkdir(directory.GetData(), S_IRWXU) != 0)
    {
      return errno == EEXIST ? xiiNetworkError(xiiNetworkErrorCode::AddressInUse) : TranslateSocketError(errno);
    }
    const int iDirectory = ::open(directory.GetData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (iDirectory < 0)
    {
      const int iError = errno;
      ::rmdir(directory.GetData());
      return TranslateSocketError(iError);
    }
    ref_socket.m_uiDirectoryHandle = static_cast<xiiUInt64>(iDirectory);
    ref_socket.m_Endpoint          = endpoint;
    struct stat entry              = {};
    if (::fstat(iDirectory, &entry) != 0 || entry.st_uid != ::geteuid() || (entry.st_mode & (S_IRWXG | S_IRWXO)) != 0)
    {
      return xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed);
    }
    const auto address = LocalAddress(directory);
    const auto iSocket = static_cast<int>(ref_socket.m_uiNativeHandle);
    if (::bind(iSocket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0)
    {
      return TranslateSocketError(errno);
    }
    if (::fstatat(iDirectory, "s", &entry, AT_SYMLINK_NOFOLLOW) != 0 || !S_ISSOCK(entry.st_mode))
    {
      return xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed);
    }
    ref_socket.m_uiSocketDevice = static_cast<xiiUInt64>(entry.st_dev);
    ref_socket.m_uiSocketInode  = static_cast<xiiUInt64>(entry.st_ino);
    if (::listen(iSocket, iBacklog) != 0)
    {
      return TranslateSocketError(errno);
    }
    ref_socket.m_bListener = true;
    return {};
  }

  xiiNetworkError LocalSocketAccess::PeerIdentity(const xiiNetworkLocalSocket& socket, xiiString& out_sIdentity)
  {
    const int iSocket = static_cast<int>(socket.m_uiNativeHandle);
    uid_t     uid     = {};
#  if XII_ENABLED(XII_PLATFORM_LINUX)
    struct ucred credentials = {};
    socklen_t    uiSize      = sizeof(credentials);
    if (::getsockopt(iSocket, SOL_SOCKET, SO_PEERCRED, &credentials, &uiSize) != 0 || uiSize != sizeof(credentials))
    {
      return xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed);
    }
    uid = credentials.uid;
#  else
    gid_t gid = {};
    if (::getpeereid(iSocket, &uid, &gid) != 0)
    {
      return xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed);
    }
#  endif
    if (uid != ::geteuid())
    {
      return xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed);
    }
    xiiStringBuilder identity;
    identity.SetFormat("uid:{0}", static_cast<xiiUInt32>(uid));
    out_sIdentity = identity;
    return {};
  }

  void LocalSocketAccess::Close(xiiNetworkLocalSocket& ref_socket)
  {
    if (ref_socket.m_uiNativeHandle != xiiNetworkLocalSocket::InvalidHandle)
    {
      ::close(static_cast<int>(ref_socket.m_uiNativeHandle));
      ref_socket.m_uiNativeHandle = xiiNetworkLocalSocket::InvalidHandle;
    }
    if (ref_socket.m_uiDirectoryHandle == xiiNetworkLocalSocket::InvalidHandle)
    {
      return;
    }
    const int   iDirectory = static_cast<int>(ref_socket.m_uiDirectoryHandle);
    struct stat entry      = {};
    if (ref_socket.m_uiSocketInode != 0 && ::fstatat(iDirectory, "s", &entry, AT_SYMLINK_NOFOLLOW) == 0 &&
        S_ISSOCK(entry.st_mode) && static_cast<xiiUInt64>(entry.st_dev) == ref_socket.m_uiSocketDevice &&
        static_cast<xiiUInt64>(entry.st_ino) == ref_socket.m_uiSocketInode)
    {
      ::unlinkat(iDirectory, "s", 0);
    }
    const auto  directory = LocalDirectory(ref_socket.m_Endpoint);
    struct stat owned     = {};
    if (::fstat(iDirectory, &owned) == 0 && ::lstat(directory.GetData(), &entry) == 0 &&
        S_ISDIR(entry.st_mode) && entry.st_dev == owned.st_dev && entry.st_ino == owned.st_ino)
    {
      ::rmdir(directory.GetData());
    }
    ::close(iDirectory);
    ref_socket.m_uiDirectoryHandle = xiiNetworkLocalSocket::InvalidHandle;
    ref_socket.m_uiSocketInode     = 0;
  }

  xiiNetworkError StartThread(NativeThread& thread, void (*pFunction)(void*), void* pUserData)
  {
    NetworkPosixThread* pThread = XII_DEFAULT_NEW(NetworkPosixThread);
    pThread->m_pFunction        = pFunction;
    pThread->m_pUserData        = pUserData;
    const int iError            = ::pthread_create(&pThread->m_hThread, nullptr, NetworkPosixThreadMain, pThread);
    if (iError != 0)
    {
      XII_DEFAULT_DELETE(pThread);
      return xiiNetworkError(iError == EAGAIN || iError == ENOMEM ? xiiNetworkErrorCode::ResourceExhausted : xiiNetworkErrorCode::InitializationFailed, iError, xiiNetworkErrorDomain::Initialization);
    }
    thread.m_pData = pThread;
    return {};
  }

  void JoinThread(NativeThread& thread)
  {
    if (thread.m_pData != nullptr)
    {
      auto*     pThread = static_cast<NetworkPosixThread*>(thread.m_pData);
      const int iError  = ::pthread_join(pThread->m_hThread, nullptr);
      XII_ASSERT_ALWAYS(iError == 0, "An owned worker must be joined exactly once from another thread.");
      XII_DEFAULT_DELETE(pThread);
      thread.m_pData = nullptr;
    }
  }

  xiiNetworkError Associate(Worker&, Registration&) { return {}; }
  void            CancelNative(Operation&) {}
  void            DestroyOperation(Operation&) {}
  void            DestroyRegistration(Registration&) {}

  xiiNetworkOperationResult FinishNative(Operation&, const NativeEvent&) { return {xiiNetworkError(xiiNetworkErrorCode::UnsupportedOperation), 0}; }

  NativeAttempt Submit(Operation& operation)
  {
    Registration&             registration = *operation.m_pRegistration;
    if (registration.IsLocal())
    {
      return SubmitLocalPosix(operation);
    }
    const auto                kind         = operation.m_Result.m_Kind;
    xiiNetworkOperationResult result;
    switch (kind)
    {
      case xiiNetworkIOOperationKind::Accept:
        result.m_Error = std::get_if<xiiTcpListener>(&registration.m_Socket)->Accept(operation.m_Result.m_AcceptedSocket, &operation.m_Result.m_Endpoint, true);
        break;
      case xiiNetworkIOOperationKind::Connect:
      {
        auto& socket = *std::get_if<xiiTcpSocket>(&registration.m_Socket);
        if (!operation.m_bConnectInitiated)
        {
          result.m_Error                = socket.Connect(operation.m_Result.m_Endpoint);
          operation.m_bConnectInitiated = true;
        }
        else
        {
          result.m_Error = socket.FinishConnect();
        }
        break;
      }
      case xiiNetworkIOOperationKind::Send:
      case xiiNetworkIOOperationKind::SendTo:
      {
        xiiArrayPtr<const xiiUInt8> spans[64];
        const xiiUInt32             uiCount = BuildSendSpans(operation, spans,
                                                             kind == xiiNetworkIOOperationKind::Send ? operation.m_pWorker->m_pRuntime->m_Configuration.m_uiMaxTransferBytesPerAttempt : static_cast<xiiUInt32>(INT_MAX));
        result                              = kind == xiiNetworkIOOperationKind::Send ? std::get_if<xiiTcpSocket>(&registration.m_Socket)->SendGather({spans, uiCount}) :
                                                                                        std::get_if<xiiUdpSocket>(&registration.m_Socket)->SendToGather(operation.m_Result.m_Endpoint, {spans, uiCount});
        break;
      }
      case xiiNetworkIOOperationKind::Receive:
      case xiiNetworkIOOperationKind::ReceiveFrom:
      {
        xiiArrayPtr<xiiUInt8> spans[64];
        const xiiUInt32       uiCount = BuildReceiveSpans(operation, spans,
                                                          kind == xiiNetworkIOOperationKind::Receive ? operation.m_pWorker->m_pRuntime->m_Configuration.m_uiMaxTransferBytesPerAttempt : static_cast<xiiUInt32>(INT_MAX));
        result                        = kind == xiiNetworkIOOperationKind::Receive ? std::get_if<xiiTcpSocket>(&registration.m_Socket)->ReceiveScatter({spans, uiCount}) :
                                                                                     std::get_if<xiiUdpSocket>(&registration.m_Socket)->ReceiveFromScatter({spans, uiCount}, operation.m_Result.m_Endpoint);
        break;
      }
      case xiiNetworkIOOperationKind::Resolve:
        result.m_Error = xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
        break;
    }
    const auto code = result.m_Error.m_Code;
    return {result, false, code == xiiNetworkErrorCode::WouldBlock || code == xiiNetworkErrorCode::InProgress || code == xiiNetworkErrorCode::Interrupted};
  }
} // namespace xiiNetworkIOInternal

#endif
