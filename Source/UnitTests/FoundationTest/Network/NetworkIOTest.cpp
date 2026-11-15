/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/NetworkIOContext.h>
#include <Foundation/Threading/Implementation/OSThread.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#include <atomic>

#include <cstring>
#include <source_location>
#include <utility>

XII_CREATE_SIMPLE_TEST_GROUP(NetworkAsync);

namespace
{
  class AsyncTestThread
  {
  public:
    explicit AsyncTestThread(xiiDelegate<void()> function) : m_Function(std::move(function)), m_Thread(Entry, this, "Network test", 512 * 1024) { m_Thread.Start(); }
    ~AsyncTestThread() { m_Thread.Join(); }

  private:
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    static xiiMinWindows::DWORD __stdcall Entry(void* pData)
#else
    static void* Entry(void* pData)
#endif
    {
      static_cast<AsyncTestThread*>(pData)->m_Function();
      return {};
    }
    xiiDelegate<void()> m_Function;
    xiiOSThread         m_Thread;
  };

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
  const xiiUInt32     uiBytes = xiiMath::Clamp(xiiCommandLineUtils::GetGlobalInstance()->GetUIntOption("-networkStreamBytes", 256 * 1024), 2U, 16U * 1024 * 1024);
  const xiiTime       begin   = xiiTime::Now();
  xiiNetworkIOBuffers send;
  xiiNetworkIOBuffers receive;
  if (!AsyncBuffer(context, uiBytes / 2, send, 17) || !AsyncBuffer(context, uiBytes - uiBytes / 2, send, 29) ||
      !AsyncBuffer(context, uiBytes, receive))
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
    XII_TEST_INT(result.m_uiBytesTransferred, uiBytes);
    if (result.m_Operation == received)
    {
      XII_TEST_INT(result.m_Buffers.GetCount(), 1);
      const auto bytes = result.m_Buffers[0].GetBytes();
      for (xiiUInt32 uiByte = 0; uiByte < bytes.GetCount(); ++uiByte)
      {
        if (!XII_TEST_INT(bytes[uiByte], uiByte < uiBytes / 2 ? 17 : 29))
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
  XII_TEST_INT(statistics.m_uiBytesSent, uiBytes);
  XII_TEST_INT(statistics.m_uiBytesReceived, uiBytes);
  XII_TEST_BOOL(statistics.m_uiEventLoopWakeups > 0);
  XII_TEST_INT(statistics.m_uiRetainedOperations, 0);
  xiiTestFramework::Output(xiiTestOutput::Details, "Stream: %u bytes, %.3f MiB/s (submission, transfer and payload validation)", uiBytes,
                           static_cast<double>(uiBytes) / (1024 * 1024 * (xiiTime::Now() - begin).GetSeconds()));
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

XII_CREATE_SIMPLE_TEST(NetworkAsync, ConcurrentCancellation)
{
  xiiNetworkIOContext context;
  if (!AsyncCheck(context.Start()))
    return;
  xiiUdpSocket socket;
  if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(socket.Bind(AsyncLoopback())))
    return;
  xiiNetworkEndpoint endpoint;
  AsyncCheck(socket.GetLocalEndpoint(endpoint));
  xiiNetworkIORegistration registration;
  if (!AsyncCheck(context.Register(std::move(socket), registration)))
    return;
  std::atomic<xiiUInt32> uiAccepted{0};
  std::atomic<xiiUInt32> uiErrors{0};
  const xiiTime          begin = xiiTime::Now();
  {
    xiiDynamicArray<xiiUniquePtr<AsyncTestThread>> threads;
    for (xiiUInt32 uiThread = 0; uiThread < 4; ++uiThread)
    {
      threads.PushBack(XII_DEFAULT_NEW(AsyncTestThread, [&, uiThread]() {
        xiiUdpSocket sender;
        if (sender.Open(xiiNetworkAddressFamily::Ipv4).Failed())
        {
          ++uiErrors;
          return;
        }
        for (xiiUInt32 uiIndex = 0; uiIndex < 64; ++uiIndex)
        {
          xiiNetworkIOBuffer buffer;
          if (context.AcquireBuffer(64, buffer).Failed())
          {
            ++uiErrors;
            return;
          }
          xiiNetworkIOBuffers buffers;
          buffers.PushBack(std::move(buffer));
          xiiNetworkIOOptions options;
          options.m_uiUserData = uiThread * 64 + uiIndex;
          xiiNetworkIOOperation operation;
          if (context.ReceiveFrom(registration, std::move(buffers), operation, options).Failed())
          {
            ++uiErrors;
            return;
          }
          ++uiAccepted;
          const xiiUInt8 payload[1] = {1};
          if (sender.SendTo(endpoint, payload).Failed())
            ++uiErrors;
          const auto canceled = context.Cancel(operation);
          if (canceled.Failed() && canceled.m_Code != xiiNetworkErrorCode::InvalidHandle)
            ++uiErrors;
        }
      }));
    }
  }
  XII_TEST_INT(uiErrors.load(), 0);
  XII_TEST_INT(uiAccepted.load(), 256);
  bool bSeen[256] = {};
  for (xiiUInt32 uiIndex = 0; uiIndex < uiAccepted; ++uiIndex)
  {
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result))
      return;
    XII_TEST_BOOL(result.Succeeded() || result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
    if (!XII_TEST_BOOL(result.m_uiUserData < 256))
      return;
    XII_TEST_BOOL(!bSeen[result.m_uiUserData]);
    bSeen[result.m_uiUserData] = true;
  }
  xiiNetworkIOResult extra;
  XII_TEST_BOOL(!context.TryGetCompletion(extra));
  const auto statistics = context.GetStatistics();
  XII_TEST_INT(statistics.m_uiRetainedOperations, 0);
  xiiTestFramework::Output(xiiTestOutput::Details, "Cancellation race: 256 operations, %.3f ms, %llu canceled, %.3f us mean terminal latency",
                           (xiiTime::Now() - begin).GetMilliseconds(), static_cast<unsigned long long>(statistics.m_uiCanceledOperations), statistics.m_TotalCompletionLatency.GetMicroseconds() / 256);
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, DeadlinesAndGenerationReuse)
{
  xiiNetworkIOContext       context;
  xiiNetworkIOConfiguration configuration;
  configuration.m_uiMaxOperationsPerSocket = 256;
  if (!AsyncCheck(context.Start(configuration)))
    return;
  xiiUdpSocket socket;
  if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(socket.Bind(AsyncLoopback())))
    return;
  xiiNetworkIORegistration registration;
  if (!AsyncCheck(context.Register(std::move(socket), registration)))
    return;
  xiiNetworkIOOperation operations[128];
  for (xiiUInt32 uiIndex = 0; uiIndex < 128; ++uiIndex)
  {
    xiiNetworkIOBuffers buffers;
    xiiNetworkIOOptions options;
    options.m_Timeout    = xiiTime::MakeFromSeconds(30);
    options.m_uiUserData = uiIndex;
    if (!AsyncBuffer(context, 1, buffers) || !AsyncCheck(context.ReceiveFrom(registration, std::move(buffers), operations[uiIndex], options)))
      return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 128; ++uiIndex)
    AsyncCheck(context.SetDeadline(operations[(uiIndex * 37) % 128], xiiTime::MakeFromMilliseconds(200 + uiIndex % 11)));
  for (xiiUInt32 uiIndex = 0; uiIndex < 128; uiIndex += 2)
  {
    AsyncCheck(context.SetDeadline(operations[uiIndex], xiiTime::MakeFromSeconds(-1)));
    AsyncCheck(context.Cancel(operations[uiIndex]));
  }
  bool bSeen[128] = {};
  for (xiiUInt32 uiIndex = 0; uiIndex < 128; ++uiIndex)
  {
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result))
      return;
    if (!XII_TEST_BOOL(result.m_uiUserData < 128))
      return;
    XII_TEST_BOOL(!bSeen[result.m_uiUserData]);
    bSeen[result.m_uiUserData] = true;
    XII_TEST_BOOL(result.m_Error.m_Code == (result.m_uiUserData % 2 == 0 ? xiiNetworkErrorCode::Cancelled : xiiNetworkErrorCode::TimedOut));
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 128; ++uiIndex)
    XII_TEST_BOOL(context.Cancel(operations[uiIndex]).m_Code == xiiNetworkErrorCode::InvalidHandle);
  xiiNetworkIOBuffers buffers;
  if (!AsyncBuffer(context, 1, buffers))
    return;
  xiiNetworkIOOperation replacement;
  if (!AsyncCheck(context.ReceiveFrom(registration, std::move(buffers), replacement)))
    return;
  for (auto operation : operations)
    XII_TEST_BOOL(operation != replacement);
  AsyncCheck(context.Cancel(replacement));
  xiiNetworkIOResult result;
  AsyncWait(context, result);
  AsyncCheck(context.Deregister(registration));
  xiiUdpSocket next;
  if (!AsyncCheck(next.Open(xiiNetworkAddressFamily::Ipv4)))
    return;
  xiiNetworkIORegistration newRegistration;
  if (!AsyncCheck(context.Register(std::move(next), newRegistration)))
    return;
  XII_TEST_BOOL(newRegistration != registration);
  XII_TEST_BOOL(context.Cancel(registration).m_Code == xiiNetworkErrorCode::InvalidHandle);
  xiiNetworkIOContext other;
  if (!AsyncCheck(other.Start()))
    return;
  XII_TEST_BOOL(other.Cancel(newRegistration).m_Code == xiiNetworkErrorCode::InvalidHandle);
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, ShutdownWithProducers)
{
  xiiNetworkIOResult surviving;
  {
    xiiNetworkIOContext context;
    if (!AsyncCheck(context.Start()))
      return;
    xiiUdpSocket socket;
    if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(socket.Bind(AsyncLoopback())))
      return;
    xiiNetworkIORegistration registration;
    if (!AsyncCheck(context.Register(std::move(socket), registration)))
      return;
    std::atomic<xiiUInt32> uiAccepted{0};
    std::atomic<xiiUInt32> uiErrors{0};
    std::atomic<bool>      bReady{false};
    {
      AsyncTestThread producer([&]() {
        for (xiiUInt32 uiIndex = 0; uiIndex < 1000; ++uiIndex)
        {
          xiiNetworkIOBuffer buffer;
          auto               error = context.AcquireBuffer(64, buffer);
          if (error.m_Code == xiiNetworkErrorCode::NotRunning)
            break;
          if (error.Failed())
          {
            ++uiErrors;
            break;
          }
          xiiNetworkIOBuffers buffers;
          buffers.PushBack(std::move(buffer));
          xiiNetworkIOOperation operation;
          error = context.ReceiveFrom(registration, std::move(buffers), operation);
          if (error.Succeeded())
            ++uiAccepted;
          else if (error.m_Code != xiiNetworkErrorCode::Backpressure && error.m_Code != xiiNetworkErrorCode::ShuttingDown &&
                   error.m_Code != xiiNetworkErrorCode::NotRunning && error.m_Code != xiiNetworkErrorCode::InvalidHandle)
            ++uiErrors;
          bReady.store(true);
        }
        bReady.store(true);
      });
      const xiiTime   deadline = xiiTime::Now() + AsyncTimeout;
      while (!bReady.load() && xiiTime::Now() < deadline)
        xiiThreadUtils::YieldTimeSlice();
      context.Shutdown();
    }
    XII_TEST_INT(uiErrors.load(), 0);
    XII_TEST_BOOL(uiAccepted.load() > 0);
    for (xiiUInt32 uiIndex = 0; uiIndex < uiAccepted; ++uiIndex)
    {
      if (!AsyncWait(context, surviving))
        return;
      XII_TEST_BOOL(surviving.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
    }
    XII_TEST_INT(context.GetStatistics().m_uiPendingOperations, 0);
    XII_TEST_INT(context.GetStatistics().m_uiRegisteredSockets, 0);
    XII_TEST_BOOL(!context.TryGetCompletion(surviving));
  }
  XII_TEST_BOOL(surviving.m_Buffers[0].IsValid());
  surviving.m_Buffers[0].GetBytes()[0] = 42;
  XII_TEST_INT(surviving.m_Buffers[0].GetBytes()[0], 42);
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, RefusalAcceptCancellationAndEof)
{
  xiiNetworkIOContext context;
  if (!AsyncCheck(context.Start()))
    return;
  xiiTcpListener listener;
  if (!AsyncCheck(listener.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(listener.Bind(AsyncLoopback())))
    return;
  xiiNetworkEndpoint endpoint;
  AsyncCheck(listener.GetLocalEndpoint(endpoint));
  // A bound TCP socket that is not listening reserves the port without accepting connections.
  xiiTcpSocket client;
  if (!AsyncCheck(client.Open(xiiNetworkAddressFamily::Ipv4)))
    return;
  xiiNetworkIORegistration registration;
  if (!AsyncCheck(context.Register(std::move(client), registration)))
    return;
  xiiNetworkIOOperation operation;
  if (!AsyncCheck(context.Connect(registration, endpoint, operation)))
    return;
  xiiNetworkIOResult result;
  if (!AsyncWait(context, result))
    return;
  XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::ConnectionRefused);
  AsyncCheck(context.Deregister(registration));
  if (!AsyncCheck(listener.Listen()) || !AsyncCheck(context.Register(std::move(listener), registration)))
    return;
  if (!AsyncCheck(context.Accept(registration, operation)))
    return;
  AsyncCheck(context.Cancel(operation));
  if (!AsyncWait(context, result))
    return;
  XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
  AsyncCheck(context.Deregister(registration));
  xiiNetworkIORegistration sender;
  xiiNetworkIORegistration receiver;
  if (!AsyncCheck(AsyncMakePair(context, sender, receiver)))
    return;
  xiiNetworkIOBuffers buffers;
  if (!AsyncBuffer(context, 16, buffers) || !AsyncCheck(context.Receive(receiver, std::move(buffers), operation)))
    return;
  AsyncCheck(context.Deregister(sender));
  if (!AsyncWait(context, result))
    return;
  XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::ConnectionClosed);
  XII_TEST_INT(result.m_uiBytesTransferred, 0);
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, ConnectionStress)
{
  const xiiUInt32           uiConnections = xiiMath::Clamp(xiiCommandLineUtils::GetGlobalInstance()->GetUIntOption("-networkStressConnections", 64), 1U, 4096U);
  xiiNetworkIOConfiguration configuration;
  configuration.m_uiMaxRegisteredSockets     = uiConnections * 2 + 8;
  configuration.m_uiMaxOutstandingOperations = uiConnections * 2 + 16;
  configuration.m_uiMaxQueuedCompletions     = configuration.m_uiMaxOutstandingOperations;
  configuration.m_uiMaxQueuedSubmissions     = configuration.m_uiMaxOutstandingOperations;
  configuration.m_uiMaxReceiveRequests       = uiConnections;
  configuration.m_uiMaxOperationsPerSocket   = xiiMath::Max(uiConnections, 128U);
  configuration.m_uiMaxAcceptOperations      = uiConnections;
  xiiNetworkIOContext context;
  if (!AsyncCheck(context.Start(configuration)))
    return;
  xiiTcpListener listener;
  if (!AsyncCheck(listener.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(listener.Bind(AsyncLoopback())) || !AsyncCheck(listener.Listen(128)))
    return;
  xiiNetworkEndpoint endpoint;
  AsyncCheck(listener.GetLocalEndpoint(endpoint));
  xiiNetworkIORegistration listening;
  if (!AsyncCheck(context.Register(std::move(listener), listening)))
    return;
  xiiDynamicArray<xiiNetworkIORegistration> clients;
  xiiDynamicArray<xiiNetworkIORegistration> servers;
  clients.SetCount(uiConnections);
  servers.SetCount(uiConnections);
  const xiiTime begin = xiiTime::Now();
  for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections; ++uiIndex)
  {
    xiiTcpSocket socket;
    const auto   error = socket.Open(xiiNetworkAddressFamily::Ipv4);
    if (error.m_Code == xiiNetworkErrorCode::ResourceExhausted)
    {
      xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped requested %u-connection stress: socket resources exhausted at %u; reduce -networkStressConnections.", uiConnections, uiIndex);
      return;
    }
    if (!AsyncCheck(error) || !AsyncCheck(context.Register(std::move(socket), clients[uiIndex])))
      return;
    xiiNetworkIOOptions options;
    options.m_uiUserData = uiIndex;
    options.m_Timeout    = xiiTime::MakeFromSeconds(30);
    xiiNetworkIOOperation operation;
    if (!AsyncCheck(context.Accept(listening, operation, options)) || !AsyncCheck(context.Connect(clients[uiIndex], endpoint, operation, options)))
      return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections * 2; ++uiIndex)
  {
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
      return;
    if (result.m_Kind == xiiNetworkIOOperationKind::Accept)
    {
      if (!XII_TEST_BOOL(result.m_uiUserData < uiConnections) || !AsyncCheck(context.Register(std::move(result.m_AcceptedSocket), servers[static_cast<xiiUInt32>(result.m_uiUserData)])))
        return;
    }
    else
      XII_TEST_BOOL(result.m_Kind == xiiNetworkIOOperationKind::Connect);
  }
  // Accept completion order need not match connect order; every peer sends the same validated payload.
  const xiiTime connected = xiiTime::Now();
  for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections; ++uiIndex)
  {
    xiiNetworkIOBuffers   send;
    xiiNetworkIOBuffers   receive;
    xiiNetworkIOOperation operation;
    if (!AsyncBuffer(context, 64, send, 91) || !AsyncBuffer(context, 64, receive) ||
        !AsyncCheck(context.Receive(servers[uiIndex], std::move(receive), operation)) || !AsyncCheck(context.Send(clients[uiIndex], std::move(send), operation)))
      return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections * 2; ++uiIndex)
  {
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
      return;
    XII_TEST_INT(result.m_uiBytesTransferred, 64);
    if (result.m_Kind == xiiNetworkIOOperationKind::Receive)
      for (xiiUInt8 uiByte : result.m_Buffers[0].GetBytes())
        XII_TEST_INT(uiByte, 91);
  }
  const auto live = context.GetStatistics();
  XII_TEST_INT(live.m_uiRegisteredSockets, uiConnections * 2 + 1);
  XII_TEST_INT(live.m_uiRetainedOperations, 0);
  const xiiTime finished = xiiTime::Now();
  context.Shutdown();
  const auto final = context.GetStatistics();
  xiiTestFramework::Output(xiiTestOutput::Details, "Connections: %u concurrent, %.3f ms establishment, %.0f transfer ops/s, %.3f ms worker CPU, %.2f%% of %u-worker elapsed capacity",
                           uiConnections, (connected - begin).GetMilliseconds(), uiConnections * 2 / (finished - connected).GetSeconds(), final.m_WorkerCpuTime.GetMilliseconds(),
                           100 * final.m_WorkerCpuTime.GetSeconds() / ((finished - begin).GetSeconds() * configuration.m_uiWorkerThreadCount), configuration.m_uiWorkerThreadCount);
  xiiTestFramework::Output(xiiTestOutput::Details, "Runtime tracked storage: %llu bytes; pool committed %llu bytes; %llu allocations, %llu reuses; %.3f us mean completion latency",
                           static_cast<unsigned long long>(live.m_uiInternalMemoryBytes), static_cast<unsigned long long>(live.m_BufferPool.m_uiCommittedBytes),
                           static_cast<unsigned long long>(live.m_BufferPool.m_uiAllocations), static_cast<unsigned long long>(live.m_BufferPool.m_uiReuses),
                           live.m_TotalCompletionLatency.GetMicroseconds() / static_cast<double>(live.m_uiCompletedOperations));
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, SmallMessageBenchmarkAndFifo)
{
  const xiiUInt32     uiMessages = xiiMath::Clamp(xiiCommandLineUtils::GetGlobalInstance()->GetUIntOption("-networkBenchmarkMessages", 1000), 1U, 100000U);
  xiiNetworkIOContext context;
  if (!AsyncCheck(context.Start()))
    return;
  xiiNetworkIORegistration client;
  xiiNetworkIORegistration server;
  if (!AsyncCheck(AsyncMakePair(context, client, server)))
    return;
  xiiNetworkIOBuffers receive;
  if (!AsyncBuffer(context, uiMessages * 64, receive))
    return;
  xiiNetworkIOOperation operation;
  if (!AsyncCheck(context.Receive(server, std::move(receive), operation)))
    return;
  const xiiTime begin       = xiiTime::Now();
  xiiUInt32     uiSubmitted = 0;
  xiiUInt32     uiCompleted = 0;
  bool          bReceived   = false;
  while (uiCompleted < uiMessages || !bReceived)
  {
    while (uiSubmitted < uiMessages && uiSubmitted - uiCompleted < 32)
    {
      xiiNetworkIOBuffers send;
      if (!AsyncBuffer(context, 64, send, static_cast<xiiUInt8>(uiSubmitted % 251)) || !AsyncCheck(context.Send(client, std::move(send), operation)))
        return;
      ++uiSubmitted;
    }
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
      return;
    if (result.m_Kind == xiiNetworkIOOperationKind::Send)
    {
      ++uiCompleted;
      XII_TEST_INT(result.m_uiBytesTransferred, 64);
    }
    else
    {
      XII_TEST_BOOL(!bReceived);
      bReceived = true;
      XII_TEST_INT(result.m_uiBytesTransferred, uiMessages * 64);
      const auto bytes = result.m_Buffers[0].GetBytes();
      for (xiiUInt32 uiIndex = 0; uiIndex < bytes.GetCount(); ++uiIndex)
        if (!XII_TEST_INT(bytes[uiIndex], (uiIndex / 64) % 251))
          break;
    }
  }
  const xiiTime elapsed = xiiTime::Now() - begin;
  context.Shutdown();
  const auto statistics = context.GetStatistics();
  XII_TEST_INT(statistics.m_uiPendingOperations, 0);
  XII_TEST_BOOL(statistics.m_BufferPool.m_uiReuses > 0 || uiMessages <= 32);
  xiiTestFramework::Output(xiiTestOutput::Details, "Small messages: %u x 64 bytes, %.0f messages/s, %.3f MiB/s, %.3f ms worker CPU, %llu pool allocations, %llu reuses",
                           uiMessages, uiMessages / elapsed.GetSeconds(), uiMessages * 64 / (1024 * 1024 * elapsed.GetSeconds()), statistics.m_WorkerCpuTime.GetMilliseconds(),
                           static_cast<unsigned long long>(statistics.m_BufferPool.m_uiAllocations), static_cast<unsigned long long>(statistics.m_BufferPool.m_uiReuses));
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, CrossContextAndIpv6)
{
  for (auto family : {xiiNetworkAddressFamily::Ipv4, xiiNetworkAddressFamily::Ipv6})
  {
    xiiTcpListener listener;
    auto           error = listener.Open(family);
    if (error.Succeeded())
      error = listener.Bind(AsyncLoopback(family));
    if (family == xiiNetworkAddressFamily::Ipv6 && (error.m_Code == xiiNetworkErrorCode::UnsupportedAddressFamily || error.m_Code == xiiNetworkErrorCode::AddressNotAvailable))
    {
      xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped async IPv6: %s (%d)", error.GetCodeName(), error.m_iNativeCode);
      continue;
    }
    if (!AsyncCheck(error) || !AsyncCheck(listener.Listen()))
      return;
    xiiNetworkEndpoint endpoint;
    AsyncCheck(listener.GetLocalEndpoint(endpoint));
    xiiNetworkIOContext acceptContext;
    xiiNetworkIOContext transferContext;
    if (!AsyncCheck(acceptContext.Start()) || !AsyncCheck(transferContext.Start()))
      return;
    xiiNetworkIORegistration listening;
    if (!AsyncCheck(acceptContext.Register(std::move(listener), listening)))
      return;
    xiiNetworkIOOperation operation;
    if (!AsyncCheck(acceptContext.Accept(listening, operation)))
      return;
    xiiTcpSocket client;
    if (!AsyncCheck(client.Open(family)))
      return;
    AsyncCheck(client.SetSendTimeout(xiiTime::MakeFromSeconds(1)));
    AsyncCheck(client.SetReceiveTimeout(xiiTime::MakeFromSeconds(1)));
    error                         = client.Connect(endpoint);
    const xiiTime connectDeadline = xiiTime::Now() + AsyncTimeout;
    while (error.m_Code == xiiNetworkErrorCode::InProgress && xiiTime::Now() < connectDeadline)
    {
      xiiThreadUtils::YieldTimeSlice();
      error = client.FinishConnect();
    }
    if (!AsyncCheck(error))
      return;
    xiiNetworkIOResult accepted;
    if (!AsyncWait(acceptContext, accepted) || !AsyncCheck(accepted.m_Error))
      return;
    xiiNetworkIORegistration server;
    if (!AsyncCheck(transferContext.Register(std::move(accepted.m_AcceptedSocket), server)))
      return;
    XII_TEST_BOOL(!accepted.m_AcceptedSocket.IsOpen());
    acceptContext.Shutdown();
    xiiNetworkIOBuffers buffers;
    if (!AsyncBuffer(transferContext, 1024, buffers))
      return;
    xiiNetworkIOOptions options;
    options.m_TransferMode = xiiNetworkIOTransferMode::Some;
    if (!AsyncCheck(transferContext.Receive(server, std::move(buffers), operation, options)))
      return;
    const xiiUInt8 payload[] = {7, 8, 9};
    if (!AsyncCheck(client.Send(payload).m_Error))
      return;
    xiiNetworkIOResult result;
    if (!AsyncWait(transferContext, result) || !AsyncCheck(result.m_Error))
      return;
    XII_TEST_BOOL(result.m_uiBytesTransferred > 0 && result.m_uiBytesTransferred <= 3);
    XII_TEST_INT(result.m_Buffers[0].GetBytes()[0], 7);
    xiiNetworkIOBuffers remaining;
    if (!AsyncBuffer(transferContext, 16, remaining) || !AsyncCheck(transferContext.Receive(server, std::move(remaining), operation)))
      return;
    AsyncCheck(client.Shutdown(xiiNetworkShutdown::Send));
    if (!AsyncWait(transferContext, result))
      return;
    XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::ConnectionClosed);
    XII_TEST_BOOL(result.m_uiBytesTransferred <= 2);
  }
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, AsyncUdpAndResourceLimits)
{
  for (auto family : {xiiNetworkAddressFamily::Ipv4, xiiNetworkAddressFamily::Ipv6})
  {
    xiiUdpSocket receiver;
    auto         error = receiver.Open(family);
    if (error.Succeeded())
      error = receiver.Bind(AsyncLoopback(family));
    if (family == xiiNetworkAddressFamily::Ipv6 && (error.m_Code == xiiNetworkErrorCode::UnsupportedAddressFamily || error.m_Code == xiiNetworkErrorCode::AddressNotAvailable))
    {
      xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped async IPv6 datagrams: %s (%d)", error.GetCodeName(), error.m_iNativeCode);
      continue;
    }
    if (!AsyncCheck(error))
      return;
    xiiNetworkEndpoint destination;
    AsyncCheck(receiver.GetLocalEndpoint(destination));
    xiiNetworkIOConfiguration configuration;
    configuration.m_uiMaxQueuedSendBytes = 64;
    configuration.m_uiMaxReceiveRequests = 1;
    xiiNetworkIOContext context;
    if (!AsyncCheck(context.Start(configuration)))
      return;
    xiiUdpSocket sender;
    if (!AsyncCheck(sender.Open(family)) || !AsyncCheck(sender.Bind(AsyncLoopback(family))))
      return;
    xiiNetworkIORegistration receiving;
    xiiNetworkIORegistration sending;
    if (!AsyncCheck(context.Register(std::move(receiver), receiving)) || !AsyncCheck(context.Register(std::move(sender), sending)))
      return;
    for (xiiUInt32 uiSize : {0U, 16U})
    {
      xiiNetworkIOBuffers send;
      xiiNetworkIOBuffers receive;
      if (!AsyncBuffer(context, uiSize, send, 19) || !AsyncBuffer(context, uiSize, send, 37) ||
          !AsyncBuffer(context, uiSize, receive) || !AsyncBuffer(context, uiSize, receive))
        return;
      xiiNetworkIOOperation operation;
      if (!AsyncCheck(context.ReceiveFrom(receiving, std::move(receive), operation)))
        return;
      xiiNetworkIOBuffers rejected;
      if (!AsyncBuffer(context, 128, rejected))
        return;
      XII_TEST_BOOL(context.ReceiveFrom(receiving, std::move(rejected), operation).m_Code == xiiNetworkErrorCode::Backpressure);
      XII_TEST_INT(rejected.GetCount(), 1);
      XII_TEST_BOOL(context.SendTo(sending, destination, std::move(rejected), operation).m_Code == xiiNetworkErrorCode::Backpressure);
      XII_TEST_INT(rejected.GetCount(), 1);
      if (!AsyncCheck(context.SendTo(sending, destination, std::move(send), operation)))
        return;
      for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
      {
        xiiNetworkIOResult result;
        if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
          return;
        XII_TEST_INT(result.m_uiBytesTransferred, uiSize * 2);
        if (result.m_Kind == xiiNetworkIOOperationKind::ReceiveFrom && uiSize != 0)
        {
          XII_TEST_INT(result.m_Buffers[0].GetBytes()[0], 19);
          XII_TEST_INT(result.m_Buffers[1].GetBytes()[0], 37);
          XII_TEST_BOOL(result.m_Endpoint.GetAddress().IsLoopback());
        }
      }
    }
  }
  xiiNetworkIOConfiguration configuration;
  configuration.m_uiMaxRegisteredSockets = 2;
  xiiNetworkIOContext context;
  if (!AsyncCheck(context.Start(configuration)))
    return;
  xiiNetworkIORegistration handles[2];
  for (auto& handle : handles)
  {
    xiiUdpSocket socket;
    if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(context.Register(std::move(socket), handle)))
      return;
  }
  xiiUdpSocket extra;
  if (!AsyncCheck(extra.Open(xiiNetworkAddressFamily::Ipv4)))
    return;
  xiiNetworkIORegistration handle;
  XII_TEST_BOOL(context.Register(std::move(extra), handle).m_Code == xiiNetworkErrorCode::Backpressure);
  XII_TEST_BOOL(extra.IsOpen());
  xiiNetworkIOOperation operation;
  const char            invalid[] = {'a', '\0', 'b'};
  XII_TEST_BOOL(context.Resolve(xiiStringView(invalid, 3), "80", operation).m_Code == xiiNetworkErrorCode::InvalidArgument);
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, PendingStreamCancellationAndChurn)
{
  xiiNetworkIOContext context;
  if (!AsyncCheck(context.Start()))
    return;
  for (xiiUInt32 uiIteration = 0; uiIteration < 16; ++uiIteration)
  {
    xiiNetworkIORegistration client;
    xiiNetworkIORegistration server;
    if (!AsyncCheck(AsyncMakePair(context, client, server)))
      return;
    xiiNetworkIOBuffers buffers;
    if (!AsyncBuffer(context, 4 * 1024 * 1024, buffers))
      return;
    xiiNetworkIOOperation send;
    if (!AsyncCheck(context.Send(client, std::move(buffers), send)))
      return;
    const auto canceled = context.Cancel(send);
    XII_TEST_BOOL(canceled.Succeeded() || canceled.m_Code == xiiNetworkErrorCode::InvalidHandle);
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result))
      return;
    XII_TEST_BOOL(result.Succeeded() || result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
    XII_TEST_BOOL(result.m_uiBytesTransferred <= 4 * 1024 * 1024);
    XII_TEST_INT(result.m_Buffers.GetCount(), 1);
    AsyncCheck(context.Deregister(client));
    AsyncCheck(context.Deregister(server));
  }
  // Race a loopback connect against cancellation; either native completion or cancellation wins once.
  xiiTcpListener listener;
  if (!AsyncCheck(listener.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(listener.Bind(AsyncLoopback())) || !AsyncCheck(listener.Listen()))
    return;
  xiiNetworkEndpoint endpoint;
  AsyncCheck(listener.GetLocalEndpoint(endpoint));
  xiiTcpSocket socket;
  if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)))
    return;
  xiiNetworkIORegistration registration;
  if (!AsyncCheck(context.Register(std::move(socket), registration)))
    return;
  xiiNetworkIOOperation operation;
  if (!AsyncCheck(context.Connect(registration, endpoint, operation)))
    return;
  const auto canceled = context.Cancel(operation);
  XII_TEST_BOOL(canceled.Succeeded() || canceled.m_Code == xiiNetworkErrorCode::InvalidHandle);
  xiiNetworkIOResult result;
  if (!AsyncWait(context, result))
    return;
  XII_TEST_BOOL(result.Succeeded() || result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
  context.Shutdown();
  XII_TEST_BOOL(!context.TryGetCompletion(result));
  XII_TEST_INT(context.GetStatistics().m_uiRegisteredSockets, 0);
  XII_TEST_INT(context.GetStatistics().m_uiRetainedOperations, 0);
}
