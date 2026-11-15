/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/NetworkIOContext.h>
#include <Foundation/Time/Time.h>

#include <cstring>
#include <source_location>
#include <utility>

XII_CREATE_SIMPLE_TEST_GROUP(NetworkAsync);

namespace
{
  const xiiTime AsyncTimeout = xiiTime::MakeFromSeconds(5);

  bool AsyncCheck(xiiNetworkError error, const std::source_location& location = std::source_location::current())
  {
    return XII_TEST_BOOL_MSG(error.Succeeded(), "%s (%d) at %s:%u", error.GetCodeName(), error.m_iNativeCode, location.file_name(), location.line());
  }

  bool AsyncWait(xiiNetworkIOContext& context, xiiNetworkIOResult& out_result)
  {
    return AsyncCheck(context.WaitForCompletion(out_result, AsyncTimeout));
  }

  xiiNetworkEndpoint AsyncLoopback(xiiNetworkAddressFamily family = xiiNetworkAddressFamily::Ipv4)
  {
    return {xiiNetworkAddress::MakeLoopback(family), 0};
  }

  xiiNetworkError AsyncMakePair(xiiNetworkIOContext& context, xiiNetworkIORegistration& out_client,
                                xiiNetworkIORegistration& out_server, xiiNetworkAddressFamily family = xiiNetworkAddressFamily::Ipv4)
  {
    xiiTcpListener listener;
    auto           error = listener.Open(family);
    if (error.Failed())
    {
      return error;
    }
    error = listener.Bind(AsyncLoopback(family));
    if (error.Failed())
    {
      return error;
    }
    error = listener.Listen();
    if (error.Failed())
    {
      return error;
    }
    xiiNetworkEndpoint endpoint;
    error = listener.GetLocalEndpoint(endpoint);
    if (error.Failed())
    {
      return error;
    }
    xiiNetworkIORegistration listening;
    error = context.Register(std::move(listener), listening);
    if (error.Failed())
    {
      return error;
    }
    xiiTcpSocket client;
    error = client.Open(family);
    if (error.Failed())
    {
      return error;
    }
    error = context.Register(std::move(client), out_client);
    if (error.Failed())
    {
      return error;
    }
    xiiNetworkIOOperation accepted;
    xiiNetworkIOOperation connected;
    error = context.Accept(listening, accepted);
    if (error.Failed())
    {
      return error;
    }
    error = context.Connect(out_client, endpoint, connected);
    if (error.Failed())
    {
      return error;
    }
    for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
    {
      xiiNetworkIOResult result;
      error = context.WaitForCompletion(result, AsyncTimeout);
      if (error.Failed())
      {
        return error;
      }
      if (result.Failed())
      {
        return result.m_Error;
      }
      if (result.m_Operation == accepted)
      {
        error = context.Register(std::move(result.m_AcceptedSocket), out_server);
        if (error.Failed())
        {
          return error;
        }
      }
      else if (!XII_TEST_BOOL(result.m_Operation == connected))
      {
        return xiiNetworkError(xiiNetworkErrorCode::Unknown);
      }
    }
    return context.Deregister(listening);
  }

  bool AsyncBuffer(xiiNetworkIOContext& context, xiiUInt32 uiSize, xiiNetworkIOBuffers& out_buffers, xiiUInt8 uiFill = 0)
  {
    xiiNetworkIOBuffer buffer;
    if (!AsyncCheck(context.AcquireBuffer(uiSize, buffer)))
    {
      return false;
    }
    if (uiSize > 0)
    {
      std::memset(buffer.GetBytes().GetPtr(), uiFill, uiSize);
    }
    out_buffers.PushBack(std::move(buffer));
    return true;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(NetworkAsync, LifecycleAndDns)
{
  const xiiUInt64 uiActiveBefore = xiiNetworkIOContext::GetActiveContextCount();
  for (xiiUInt32 uiIteration = 0; uiIteration < 4; ++uiIteration)
  {
    xiiNetworkIOContext       context;
    xiiNetworkIOConfiguration configuration;
    configuration.m_bEnableProfiling = true;
    if (!AsyncCheck(context.Start(configuration)))
    {
      return;
    }
    XII_TEST_INT(xiiNetworkIOContext::GetActiveContextCount(), uiActiveBefore + 1);
    xiiNetworkIOOperation operation;
    if (!AsyncCheck(context.Resolve("localhost", "80", operation)))
    {
      return;
    }
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
    {
      return;
    }
    XII_TEST_BOOL(result.m_Operation == operation);
    XII_TEST_BOOL(!result.m_ResolvedEndpoints.IsEmpty());
    for (const auto& endpoint : result.m_ResolvedEndpoints)
    {
      XII_TEST_BOOL(endpoint.GetAddress().IsLoopback());
      XII_TEST_INT(endpoint.GetPort(), 80);
    }
    context.Shutdown();
    context.Shutdown();
    XII_TEST_BOOL(context.GetState() == xiiNetworkIOContextState::Stopped);
    XII_TEST_BOOL(context.Resolve("localhost", "80", operation).m_Code == xiiNetworkErrorCode::NotRunning);
    XII_TEST_BOOL(context.Start().m_Code == xiiNetworkErrorCode::AlreadyOpen);
    XII_TEST_INT(xiiNetworkIOContext::GetActiveContextCount(), uiActiveBefore);
  }
  xiiNetworkIOContext       invalid;
  xiiNetworkIOConfiguration configuration;
  configuration.m_uiWorkerThreadCount = 0;
  XII_TEST_BOOL(invalid.Start(configuration).m_Code == xiiNetworkErrorCode::InvalidArgument);
  invalid.Shutdown();
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, TcpTransfers)
{
  xiiNetworkIOContext       context;
  xiiNetworkIOConfiguration configuration;
  configuration.m_uiMaxTransferBytesPerAttempt = 4096;
  if (!AsyncCheck(context.Start(configuration)))
  {
    return;
  }
  xiiNetworkIORegistration client;
  xiiNetworkIORegistration server;
  if (!AsyncCheck(AsyncMakePair(context, client, server)))
  {
    return;
  }
  xiiNetworkIOBuffers send;
  xiiNetworkIOBuffers receive;
  if (!AsyncBuffer(context, 128 * 1024, send, 17) || !AsyncBuffer(context, 128 * 1024, send, 29) ||
      !AsyncBuffer(context, 256 * 1024, receive))
  {
    return;
  }
  xiiNetworkIOOperation sent;
  xiiNetworkIOOperation received;
  if (!AsyncCheck(context.Receive(server, std::move(receive), received)) || !AsyncCheck(context.Send(client, std::move(send), sent)))
  {
    return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
  {
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
    {
      return;
    }
    XII_TEST_INT(result.m_uiBytesTransferred, 256 * 1024);
    if (result.m_Operation == received)
    {
      XII_TEST_INT(result.m_Buffers.GetCount(), 1);
      const auto bytes = result.m_Buffers[0].GetBytes();
      for (xiiUInt32 uiByte = 0; uiByte < bytes.GetCount(); ++uiByte)
      {
        if (!XII_TEST_INT(bytes[uiByte], uiByte < 128 * 1024 ? 17 : 29))
        {
          break;
        }
      }
    }
    else
    {
      XII_TEST_BOOL(result.m_Operation == sent);
    }
  }
  const auto statistics = context.GetStatistics();
  XII_TEST_INT(statistics.m_uiBytesSent, 256 * 1024);
  XII_TEST_INT(statistics.m_uiBytesReceived, 256 * 1024);
  XII_TEST_BOOL(statistics.m_uiEventLoopWakeups > 0);
  XII_TEST_INT(statistics.m_uiRetainedOperations, 0);
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, UdpDatagrams)
{
  xiiNetworkIOContext context;
  if (!AsyncCheck(context.Start()))
  {
    return;
  }
  xiiUdpSocket socket;
  if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(socket.Bind(AsyncLoopback())))
  {
    return;
  }
  xiiNetworkEndpoint destination;
  AsyncCheck(socket.GetLocalEndpoint(destination));
  xiiNetworkIORegistration receiver;
  if (!AsyncCheck(context.Register(std::move(socket), receiver)))
  {
    return;
  }
  XII_TEST_BOOL(!socket.IsOpen());
  xiiUdpSocket sender;
  if (!AsyncCheck(sender.Open(xiiNetworkAddressFamily::Ipv4)))
  {
    return;
  }
  const xiiUInt32 uiSizes[] = {0, 1, 32, 1024};
  for (xiiUInt32 uiSize : uiSizes)
  {
    xiiNetworkIOBuffers buffers;
    if (!AsyncBuffer(context, uiSize == 1024 ? 3 : uiSize, buffers))
    {
      return;
    }
    xiiNetworkIOOperation operation;
    if (!AsyncCheck(context.ReceiveFrom(receiver, std::move(buffers), operation)))
    {
      return;
    }
    xiiUInt8 uiPayload[1024] = {};
    if (!AsyncCheck(sender.SendTo(destination, {uiPayload, uiSize}).m_Error))
    {
      return;
    }
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result))
    {
      return;
    }
    XII_TEST_BOOL(result.m_Operation == operation);
    XII_TEST_INT(result.m_uiBytesTransferred, uiSize == 1024 ? 3 : uiSize);
    if (uiSize == 1024)
    {
      XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::Truncated);
    }
    else
    {
      AsyncCheck(result.m_Error);
    }
    XII_TEST_BOOL(result.m_Endpoint.GetAddress().IsLoopback());
  }
  // Queue the datagram first to exercise immediate native completions (including truncation).
  xiiUInt8 uiPayload[16] = {};
  AsyncCheck(sender.SendTo(destination, uiPayload).m_Error);
  xiiNetworkIOBuffers   buffers;
  xiiNetworkIOOperation operation;
  if (!AsyncBuffer(context, 2, buffers) || !AsyncCheck(context.ReceiveFrom(receiver, std::move(buffers), operation)))
  {
    return;
  }
  xiiNetworkIOResult result;
  if (!AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::Truncated);
  XII_TEST_INT(result.m_uiBytesTransferred, 2);
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, CancellationAndBackpressure)
{
  xiiNetworkIOContext       context;
  xiiNetworkIOConfiguration configuration;
  configuration.m_uiMaxOutstandingOperations = 4;
  configuration.m_uiMaxQueuedCompletions     = 2;
  if (!AsyncCheck(context.Start(configuration)))
  {
    return;
  }
  xiiUdpSocket socket;
  if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(socket.Bind(AsyncLoopback())))
  {
    return;
  }
  xiiNetworkIORegistration registration;
  if (!AsyncCheck(context.Register(std::move(socket), registration)))
  {
    return;
  }
  xiiNetworkIOOperation operations[2];
  for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
  {
    xiiNetworkIOBuffers buffers;
    if (!AsyncBuffer(context, 128, buffers) || !AsyncCheck(context.ReceiveFrom(registration, std::move(buffers), operations[uiIndex])))
    {
      return;
    }
  }
  xiiNetworkIOBuffers rejected;
  if (!AsyncBuffer(context, 1, rejected))
  {
    return;
  }
  xiiNetworkIOOperation operation;
  XII_TEST_BOOL(context.ReceiveFrom(registration, std::move(rejected), operation).m_Code == xiiNetworkErrorCode::Backpressure);
  XII_TEST_INT(rejected.GetCount(), 1);
  AsyncCheck(context.Cancel(operations[0]));
  AsyncCheck(context.Cancel(registration));
  xiiNetworkIOResult result;
  for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
  {
    if (!AsyncWait(context, result))
    {
      return;
    }
    XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
    XII_TEST_BOOL(result.m_Operation == operations[0] || result.m_Operation == operations[1]);
  }
  XII_TEST_BOOL(!context.TryGetCompletion(result));
  XII_TEST_BOOL(context.Cancel(operations[0]).m_Code == xiiNetworkErrorCode::InvalidHandle);
  xiiNetworkIOOptions options;
  options.m_Timeout = xiiTime::MakeFromMilliseconds(10);
  if (!AsyncCheck(context.ReceiveFrom(registration, std::move(rejected), operation, options)))
  {
    return;
  }
  if (!AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::TimedOut);
  AsyncCheck(context.Deregister(registration));
  context.Shutdown();
  XII_TEST_INT(context.GetStatistics().m_uiPendingOperations, 0);
  XII_TEST_INT(context.GetStatistics().m_uiRegisteredSockets, 0);
}
