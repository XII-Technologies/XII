/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)

#  include <Foundation/Network/Implementation/NetworkIOInternal.h>
#  include <Foundation/Platform/Implementation/NetworkPlatform.h>
#  include <Foundation/Platform/Implementation/NetworkSocketAccess.h>

#  include <cerrno>
#  include <pthread.h>

#  include <climits>

namespace
{
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
    return {result, false};
  }
} // namespace xiiNetworkIOInternal

#endif
