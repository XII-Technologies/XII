/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Network/NetworkIOContext.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Implementation/OSThread.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#include <FoundationTest/Network/Implementation/NetworkHostTestUtilities.h>
#include <FoundationTest/Network/Implementation/NetworkTestUtilities.h>

#include <atomic>

#include <cstring>
#include <source_location>
#include <utility>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>
#  include <sddl.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)
#  include <cstdio>
#  include <sys/socket.h>
#  include <sys/stat.h>
#  include <sys/un.h>
#  include <unistd.h>
#endif

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

  bool AsyncWait(xiiNetworkIOContext& context, xiiNetworkIOResult& out_result, const std::source_location& location = std::source_location::current())
  {
    return AsyncCheck(context.WaitForCompletion(out_result, AsyncTimeout), location);
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
  {
    return;
  }
  xiiUdpSocket socket;
  if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(socket.Bind(AsyncLoopback())))
  {
    return;
  }
  xiiNetworkEndpoint endpoint;
  AsyncCheck(socket.GetLocalEndpoint(endpoint));
  xiiNetworkIORegistration registration;
  if (!AsyncCheck(context.Register(std::move(socket), registration)))
  {
    return;
  }
  std::atomic<xiiUInt32> uiAccepted{0};
  std::atomic<xiiUInt32> uiErrors{0};
  const xiiTime          begin = xiiTime::Now();
  {
    xiiDeque<xiiUniquePtr<AsyncTestThread>> threads;
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
          {
            ++uiErrors;
          }
          const auto canceled = context.Cancel(operation);
          if (canceled.Failed() && canceled.m_Code != xiiNetworkErrorCode::InvalidHandle)
          {
            ++uiErrors;
          }
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
    {
      return;
    }
    XII_TEST_BOOL(result.Succeeded() || result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
    if (!XII_TEST_BOOL(result.m_uiUserData < 256))
    {
      return;
    }
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
  xiiNetworkIOOperation operations[128];
  for (xiiUInt32 uiIndex = 0; uiIndex < 128; ++uiIndex)
  {
    xiiNetworkIOBuffers buffers;
    xiiNetworkIOOptions options;
    options.m_Timeout    = xiiTime::MakeFromSeconds(30);
    options.m_uiUserData = uiIndex;
    if (!AsyncBuffer(context, 1, buffers) || !AsyncCheck(context.ReceiveFrom(registration, std::move(buffers), operations[uiIndex], options)))
    {
      return;
    }
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 128; ++uiIndex)
  {
    AsyncCheck(context.SetDeadline(operations[(uiIndex * 37) % 128], xiiTime::MakeFromMilliseconds(200 + uiIndex % 11)));
  }
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
    {
      return;
    }
    if (!XII_TEST_BOOL(result.m_uiUserData < 128))
    {
      return;
    }
    XII_TEST_BOOL(!bSeen[result.m_uiUserData]);
    bSeen[result.m_uiUserData] = true;
    XII_TEST_BOOL(result.m_Error.m_Code == (result.m_uiUserData % 2 == 0 ? xiiNetworkErrorCode::Cancelled : xiiNetworkErrorCode::TimedOut));
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 128; ++uiIndex)
  {
    XII_TEST_BOOL(context.Cancel(operations[uiIndex]).m_Code == xiiNetworkErrorCode::InvalidHandle);
  }
  xiiNetworkIOBuffers buffers;
  if (!AsyncBuffer(context, 1, buffers))
  {
    return;
  }
  xiiNetworkIOOperation replacement;
  if (!AsyncCheck(context.ReceiveFrom(registration, std::move(buffers), replacement)))
  {
    return;
  }
  for (auto operation : operations)
  {
    XII_TEST_BOOL(operation != replacement);
  }
  AsyncCheck(context.Cancel(replacement));
  xiiNetworkIOResult result;
  AsyncWait(context, result);
  AsyncCheck(context.Deregister(registration));
  xiiUdpSocket next;
  if (!AsyncCheck(next.Open(xiiNetworkAddressFamily::Ipv4)))
  {
    return;
  }
  xiiNetworkIORegistration newRegistration;
  if (!AsyncCheck(context.Register(std::move(next), newRegistration)))
  {
    return;
  }
  XII_TEST_BOOL(newRegistration != registration);
  XII_TEST_BOOL(context.Cancel(registration).m_Code == xiiNetworkErrorCode::InvalidHandle);
  xiiNetworkIOContext other;
  if (!AsyncCheck(other.Start()))
  {
    return;
  }
  XII_TEST_BOOL(other.Cancel(newRegistration).m_Code == xiiNetworkErrorCode::InvalidHandle);
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, ShutdownWithProducers)
{
  xiiNetworkIOResult surviving;
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
    xiiNetworkIORegistration registration;
    if (!AsyncCheck(context.Register(std::move(socket), registration)))
    {
      return;
    }
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
          {
            break;
          }
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
          {
            ++uiAccepted;
          }
          else if (error.m_Code != xiiNetworkErrorCode::Backpressure && error.m_Code != xiiNetworkErrorCode::ShuttingDown &&
                   error.m_Code != xiiNetworkErrorCode::NotRunning && error.m_Code != xiiNetworkErrorCode::InvalidHandle)
          {
            ++uiErrors;
          }
          bReady.store(true);
        }
        bReady.store(true);
      });
      const xiiTime   deadline = xiiTime::Now() + AsyncTimeout;
      while (!bReady.load() && xiiTime::Now() < deadline)
      {
        xiiThreadUtils::YieldTimeSlice();
      }
      context.Shutdown();
    }
    XII_TEST_INT(uiErrors.load(), 0);
    XII_TEST_BOOL(uiAccepted.load() > 0);
    for (xiiUInt32 uiIndex = 0; uiIndex < uiAccepted; ++uiIndex)
    {
      if (!AsyncWait(context, surviving))
      {
        return;
      }
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
  {
    return;
  }
  xiiTcpListener listener;
  if (!AsyncCheck(listener.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(listener.Bind(AsyncLoopback())))
  {
    return;
  }
  xiiNetworkEndpoint endpoint;
  AsyncCheck(listener.GetLocalEndpoint(endpoint));
  // Bind the client before releasing the reserved port, preventing accidental self-connect.
  // BSD can leave SYNs to bound non-listeners pending, so release the target before connecting.
  xiiTcpSocket client;
  if (!AsyncCheck(client.Open(xiiNetworkAddressFamily::Ipv4)))
  {
    return;
  }
  if (!AsyncCheck(client.Bind(AsyncLoopback())) || !AsyncCheck(listener.Close()))
  {
    return;
  }
  xiiNetworkIORegistration registration;
  if (!AsyncCheck(context.Register(std::move(client), registration)))
  {
    return;
  }
  xiiNetworkIOOperation operation;
  if (!AsyncCheck(context.Connect(registration, endpoint, operation)))
  {
    return;
  }
  xiiNetworkIOResult result;
  if (!AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::ConnectionRefused);
  AsyncCheck(context.Deregister(registration));
  if (!AsyncCheck(listener.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(listener.Bind(AsyncLoopback())) || !AsyncCheck(listener.Listen()) || !AsyncCheck(context.Register(std::move(listener), registration)))
  {
    return;
  }
  if (!AsyncCheck(context.Accept(registration, operation)))
  {
    return;
  }
  AsyncCheck(context.Cancel(operation));
  if (!AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
  AsyncCheck(context.Deregister(registration));
  xiiNetworkIORegistration sender;
  xiiNetworkIORegistration receiver;
  if (!AsyncCheck(AsyncMakePair(context, sender, receiver)))
  {
    return;
  }
  xiiNetworkIOBuffers buffers;
  if (!AsyncBuffer(context, 16, buffers) || !AsyncCheck(context.Receive(receiver, std::move(buffers), operation)))
  {
    return;
  }
  AsyncCheck(context.Deregister(sender));
  if (!AsyncWait(context, result))
  {
    return;
  }
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
  {
    return;
  }
  xiiTcpListener listener;
  if (!AsyncCheck(listener.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(listener.Bind(AsyncLoopback())) || !AsyncCheck(listener.Listen(128)))
  {
    return;
  }
  xiiNetworkEndpoint endpoint;
  AsyncCheck(listener.GetLocalEndpoint(endpoint));
  xiiNetworkIORegistration listening;
  if (!AsyncCheck(context.Register(std::move(listener), listening)))
  {
    return;
  }
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
    {
      return;
    }
    xiiNetworkIOOptions options;
    options.m_uiUserData = uiIndex;
    options.m_Timeout    = xiiTime::MakeFromSeconds(30);
    xiiNetworkIOOperation operation;
    if (!AsyncCheck(context.Accept(listening, operation, options)) || !AsyncCheck(context.Connect(clients[uiIndex], endpoint, operation, options)))
    {
      return;
    }
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections * 2; ++uiIndex)
  {
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
    {
      return;
    }
    if (result.m_Kind == xiiNetworkIOOperationKind::Accept)
    {
      if (!XII_TEST_BOOL(result.m_uiUserData < uiConnections) || !AsyncCheck(context.Register(std::move(result.m_AcceptedSocket), servers[static_cast<xiiUInt32>(result.m_uiUserData)])))
      {
        return;
      }
    }
    else
    {
      XII_TEST_BOOL(result.m_Kind == xiiNetworkIOOperationKind::Connect);
    }
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
    {
      return;
    }
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections * 2; ++uiIndex)
  {
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
    {
      return;
    }
    XII_TEST_INT(result.m_uiBytesTransferred, 64);
    if (result.m_Kind == xiiNetworkIOOperationKind::Receive)
    {
      for (xiiUInt8 uiByte : result.m_Buffers[0].GetBytes())
      {
        XII_TEST_INT(uiByte, 91);
      }
    }
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
  {
    return;
  }
  xiiNetworkIORegistration client;
  xiiNetworkIORegistration server;
  if (!AsyncCheck(AsyncMakePair(context, client, server)))
  {
    return;
  }
  xiiNetworkIOBuffers receive;
  if (!AsyncBuffer(context, uiMessages * 64, receive))
  {
    return;
  }
  xiiNetworkIOOperation operation;
  if (!AsyncCheck(context.Receive(server, std::move(receive), operation)))
  {
    return;
  }
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
      {
        return;
      }
      ++uiSubmitted;
    }
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
    {
      return;
    }
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
      {
        if (!XII_TEST_INT(bytes[uiIndex], (uiIndex / 64) % 251))
        {
          break;
        }
      }
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
    {
      error = listener.Bind(AsyncLoopback(family));
    }
    if (family == xiiNetworkAddressFamily::Ipv6 && (error.m_Code == xiiNetworkErrorCode::UnsupportedAddressFamily || error.m_Code == xiiNetworkErrorCode::AddressNotAvailable))
    {
      xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped async IPv6: %s (%d)", error.GetCodeName(), error.m_iNativeCode);
      continue;
    }
    if (!AsyncCheck(error) || !AsyncCheck(listener.Listen()))
    {
      return;
    }
    xiiNetworkEndpoint endpoint;
    AsyncCheck(listener.GetLocalEndpoint(endpoint));
    xiiNetworkIOContext acceptContext;
    xiiNetworkIOContext transferContext;
    if (!AsyncCheck(acceptContext.Start()) || !AsyncCheck(transferContext.Start()))
    {
      return;
    }
    xiiNetworkIORegistration listening;
    if (!AsyncCheck(acceptContext.Register(std::move(listener), listening)))
    {
      return;
    }
    xiiNetworkIOOperation operation;
    if (!AsyncCheck(acceptContext.Accept(listening, operation)))
    {
      return;
    }
    xiiTcpSocket client;
    if (!AsyncCheck(client.Open(family)))
    {
      return;
    }
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
    {
      return;
    }
    xiiNetworkIOResult accepted;
    if (!AsyncWait(acceptContext, accepted) || !AsyncCheck(accepted.m_Error))
    {
      return;
    }
    xiiNetworkIORegistration server;
    if (!AsyncCheck(transferContext.Register(std::move(accepted.m_AcceptedSocket), server)))
    {
      return;
    }
    XII_TEST_BOOL(!accepted.m_AcceptedSocket.IsOpen());
    acceptContext.Shutdown();
    xiiNetworkIOBuffers buffers;
    if (!AsyncBuffer(transferContext, 1024, buffers))
    {
      return;
    }
    xiiNetworkIOOptions options;
    options.m_TransferMode = xiiNetworkIOTransferMode::Some;
    if (!AsyncCheck(transferContext.Receive(server, std::move(buffers), operation, options)))
    {
      return;
    }
    const xiiUInt8 payload[] = {7, 8, 9};
    if (!AsyncCheck(client.Send(payload).m_Error))
    {
      return;
    }
    xiiNetworkIOResult result;
    if (!AsyncWait(transferContext, result) || !AsyncCheck(result.m_Error))
    {
      return;
    }
    XII_TEST_BOOL(result.m_uiBytesTransferred > 0 && result.m_uiBytesTransferred <= 3);
    XII_TEST_INT(result.m_Buffers[0].GetBytes()[0], 7);
    xiiNetworkIOBuffers remaining;
    if (!AsyncBuffer(transferContext, 16, remaining) || !AsyncCheck(transferContext.Receive(server, std::move(remaining), operation)))
    {
      return;
    }
    AsyncCheck(client.Shutdown(xiiNetworkShutdown::Send));
    if (!AsyncWait(transferContext, result))
    {
      return;
    }
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
    {
      error = receiver.Bind(AsyncLoopback(family));
    }
    if (family == xiiNetworkAddressFamily::Ipv6 && (error.m_Code == xiiNetworkErrorCode::UnsupportedAddressFamily || error.m_Code == xiiNetworkErrorCode::AddressNotAvailable))
    {
      xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped async IPv6 datagrams: %s (%d)", error.GetCodeName(), error.m_iNativeCode);
      continue;
    }
    if (!AsyncCheck(error))
    {
      return;
    }
    xiiNetworkEndpoint destination;
    AsyncCheck(receiver.GetLocalEndpoint(destination));
    xiiNetworkIOConfiguration configuration;
    configuration.m_uiMaxQueuedSendBytes = 64;
    configuration.m_uiMaxReceiveRequests = 1;
    xiiNetworkIOContext context;
    if (!AsyncCheck(context.Start(configuration)))
    {
      return;
    }
    xiiUdpSocket sender;
    if (!AsyncCheck(sender.Open(family)) || !AsyncCheck(sender.Bind(AsyncLoopback(family))))
    {
      return;
    }
    xiiNetworkIORegistration receiving;
    xiiNetworkIORegistration sending;
    if (!AsyncCheck(context.Register(std::move(receiver), receiving)) || !AsyncCheck(context.Register(std::move(sender), sending)))
    {
      return;
    }
    for (xiiUInt32 uiSize : {0U, 16U})
    {
      xiiNetworkIOBuffers send;
      xiiNetworkIOBuffers receive;
      if (!AsyncBuffer(context, uiSize, send, 19) || !AsyncBuffer(context, uiSize, send, 37) ||
          !AsyncBuffer(context, uiSize, receive) || !AsyncBuffer(context, uiSize, receive))
      {
        return;
      }
      xiiNetworkIOOperation operation;
      if (!AsyncCheck(context.ReceiveFrom(receiving, std::move(receive), operation)))
      {
        return;
      }
      xiiNetworkIOBuffers rejected;
      if (!AsyncBuffer(context, 128, rejected))
      {
        return;
      }
      XII_TEST_BOOL(context.ReceiveFrom(receiving, std::move(rejected), operation).m_Code == xiiNetworkErrorCode::Backpressure);
      XII_TEST_INT(rejected.GetCount(), 1);
      XII_TEST_BOOL(context.SendTo(sending, destination, std::move(rejected), operation).m_Code == xiiNetworkErrorCode::Backpressure);
      XII_TEST_INT(rejected.GetCount(), 1);
      if (!AsyncCheck(context.SendTo(sending, destination, std::move(send), operation)))
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
  {
    return;
  }
  xiiNetworkIORegistration handles[2];
  for (auto& handle : handles)
  {
    xiiUdpSocket socket;
    if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(context.Register(std::move(socket), handle)))
    {
      return;
    }
  }
  xiiUdpSocket extra;
  if (!AsyncCheck(extra.Open(xiiNetworkAddressFamily::Ipv4)))
  {
    return;
  }
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
  {
    return;
  }
  for (xiiUInt32 uiIteration = 0; uiIteration < 16; ++uiIteration)
  {
    xiiNetworkIORegistration client;
    xiiNetworkIORegistration server;
    if (!AsyncCheck(AsyncMakePair(context, client, server)))
    {
      return;
    }
    xiiNetworkIOBuffers buffers;
    if (!AsyncBuffer(context, 4 * 1024 * 1024, buffers))
    {
      return;
    }
    xiiNetworkIOOperation send;
    if (!AsyncCheck(context.Send(client, std::move(buffers), send)))
    {
      return;
    }
    const auto canceled = context.Cancel(send);
    XII_TEST_BOOL(canceled.Succeeded() || canceled.m_Code == xiiNetworkErrorCode::InvalidHandle);
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result))
    {
      return;
    }
    XII_TEST_BOOL(result.Succeeded() || result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
    XII_TEST_BOOL(result.m_uiBytesTransferred <= 4 * 1024 * 1024);
    XII_TEST_INT(result.m_Buffers.GetCount(), 1);
    AsyncCheck(context.Deregister(client));
    AsyncCheck(context.Deregister(server));
  }
  // Race a loopback connect against cancellation; either native completion or cancellation wins once.
  xiiTcpListener listener;
  if (!AsyncCheck(listener.Open(xiiNetworkAddressFamily::Ipv4)) || !AsyncCheck(listener.Bind(AsyncLoopback())) || !AsyncCheck(listener.Listen()))
  {
    return;
  }
  xiiNetworkEndpoint endpoint;
  AsyncCheck(listener.GetLocalEndpoint(endpoint));
  xiiTcpSocket socket;
  if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)))
  {
    return;
  }
  xiiNetworkIORegistration registration;
  if (!AsyncCheck(context.Register(std::move(socket), registration)))
  {
    return;
  }
  xiiNetworkIOOperation operation;
  if (!AsyncCheck(context.Connect(registration, endpoint, operation)))
  {
    return;
  }
  const auto canceled = context.Cancel(operation);
  XII_TEST_BOOL(canceled.Succeeded() || canceled.m_Code == xiiNetworkErrorCode::InvalidHandle);
  xiiNetworkIOResult result;
  if (!AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.Succeeded() || result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
  context.Shutdown();
  XII_TEST_BOOL(!context.TryGetCompletion(result));
  XII_TEST_INT(context.GetStatistics().m_uiRegisteredSockets, 0);
  XII_TEST_INT(context.GetStatistics().m_uiRetainedOperations, 0);
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, ResolverCancellationAndShutdown)
{
  xiiNetworkIOContext       context;
  xiiNetworkIOConfiguration configuration;
  configuration.m_uiResolverThreadCount = 1;
  configuration.m_uiMaxResolverRequests = 16;
  if (!AsyncCheck(context.Start(configuration)))
  {
    return;
  }
  xiiNetworkIOOperation operation;
  xiiNetworkIOOptions   options;
  options.m_Timeout = xiiTime::MakeZero();
  if (!AsyncCheck(context.Resolve("localhost", "80", operation, xiiNetworkAddressFamily::Ipv4, xiiNetworkTransport::Tcp, options)))
  {
    return;
  }
  xiiNetworkIOResult result;
  if (!AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::TimedOut);
  XII_TEST_BOOL(result.m_ResolvedEndpoints.IsEmpty());
  // A canceled query can still pin executor capacity until the bounded OS resolver call returns.
  xiiNetworkIOOperation operations[8];
  for (xiiUInt32 uiIndex = 0; uiIndex < 8; ++uiIndex)
  {
    if (!AsyncCheck(context.Resolve("localhost", "80", operations[uiIndex])))
    {
      return;
    }
    const auto canceled = context.Cancel(operations[uiIndex]);
    XII_TEST_BOOL(canceled.Succeeded() || canceled.m_Code == xiiNetworkErrorCode::InvalidHandle);
  }
  context.Shutdown();
  bool bSeen[8] = {};
  for (xiiUInt32 uiIndex = 0; uiIndex < 8; ++uiIndex)
  {
    if (!AsyncWait(context, result))
    {
      return;
    }
    XII_TEST_BOOL(result.Succeeded() || result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
    bool bFound = false;
    for (xiiUInt32 uiSlot = 0; uiSlot < 8; ++uiSlot)
    {
      if (operations[uiSlot] == result.m_Operation)
      {
        XII_TEST_BOOL(!bSeen[uiSlot]);
        bSeen[uiSlot] = true;
        bFound        = true;
      }
    }
    XII_TEST_BOOL(bFound);
  }
  XII_TEST_INT(context.GetStatistics().m_uiRetainedOperations, 0);
  XII_TEST_BOOL(!context.TryGetCompletion(result));
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, CancellationBenchmark)
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
  xiiNetworkIORegistration registration;
  if (!AsyncCheck(context.Register(std::move(socket), registration)))
  {
    return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 64; ++uiIndex)
  {
    xiiNetworkIOBuffers   buffers;
    xiiNetworkIOOperation operation;
    if (!AsyncBuffer(context, 64, buffers) || !AsyncCheck(context.ReceiveFrom(registration, std::move(buffers), operation)))
    {
      return;
    }
  }
  const xiiTime begin = xiiTime::Now();
  AsyncCheck(context.Cancel(registration));
  const xiiTime requested = xiiTime::Now();
  for (xiiUInt32 uiIndex = 0; uiIndex < 64; ++uiIndex)
  {
    xiiNetworkIOResult result;
    if (!AsyncWait(context, result))
    {
      return;
    }
    XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
  }
  const xiiTime finished = xiiTime::Now();
  XII_TEST_INT(context.GetStatistics().m_uiRetainedOperations, 0);
  xiiTestFramework::Output(xiiTestOutput::Details, "Socket cancellation: 64 queued receives, %.3f us request, %.3f us through final dequeue",
                           (requested - begin).GetMicroseconds(), (finished - begin).GetMicroseconds());
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, RegistrationChurnIsBounded)
{
  xiiNetworkIOContext       context;
  xiiNetworkIOConfiguration configuration;
  configuration.m_uiMaxRegisteredSockets = 2;
  if (!AsyncCheck(context.Start(configuration)))
  {
    return;
  }
  xiiUInt64 uiWarmMemory = 0;
  for (xiiUInt32 uiIndex = 0; uiIndex < 1004; ++uiIndex)
  {
    xiiUdpSocket socket;
    if (!AsyncCheck(socket.Open(xiiNetworkAddressFamily::Ipv4)))
    {
      return;
    }
    xiiNetworkIORegistration registration;
    if (!AsyncCheck(context.Register(std::move(socket), registration)) || !AsyncCheck(context.Deregister(registration)))
    {
      return;
    }
    if (uiIndex == 3)
    {
      uiWarmMemory = context.GetStatistics().m_uiInternalMemoryBytes;
    }
  }
  const auto statistics = context.GetStatistics();
  XII_TEST_INT(statistics.m_uiRegisteredSockets, 0);
  XII_TEST_INT(statistics.m_uiInternalMemoryBytes, uiWarmMemory);
  context.Shutdown();
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, NativeResourceLifetime)
{
  xiiUdpSocket runtimeOwner;
  if (!AsyncCheck(runtimeOwner.Open(xiiNetworkAddressFamily::Ipv4)))
  {
    return;
  }
  xiiUInt32 uiWarmCount = 0;
  for (xiiUInt32 uiIteration = 0; uiIteration < 20; ++uiIteration)
  {
    {
      xiiNetworkIOContext context;
      if (!AsyncCheck(context.Start()))
      {
        return;
      }
      xiiNetworkIORegistration client;
      xiiNetworkIORegistration server;
      if (!AsyncCheck(AsyncMakePair(context, client, server)))
      {
        return;
      }
      xiiNetworkIOOperation operation;
      if (!AsyncCheck(context.Resolve("localhost", "80", operation)))
      {
        return;
      }
      xiiNetworkIOBuffers receive;
      if (!AsyncBuffer(context, 256, receive) || !AsyncCheck(context.Receive(server, std::move(receive), operation)))
      {
        return;
      }
      // Leave both completions undrained. Destruction must join native workers and reclaim records.
      context.Shutdown();
      XII_TEST_INT(context.GetStatistics().m_uiRegisteredSockets, 0);
      XII_TEST_INT(context.GetStatistics().m_uiPendingOperations, 0);
    }
    if (uiIteration == 3)
    {
      uiWarmCount = xiiNetworkTestUtilities::CountOpenResources();
      if (!XII_TEST_BOOL(uiWarmCount != 0))
      {
        return;
      }
    }
    else if (uiIteration > 3)
    {
      XII_TEST_INT(xiiNetworkTestUtilities::CountOpenResources(), uiWarmCount);
    }
  }
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, ResolverAdmissionDuringShutdown)
{
  for (xiiUInt32 uiIteration = 0; uiIteration < 32; ++uiIteration)
  {
    xiiNetworkIOConfiguration configuration;
    configuration.m_uiWorkerThreadCount   = 1;
    configuration.m_uiResolverThreadCount = 1;
    xiiNetworkIOContext context;
    if (!AsyncCheck(context.Start(configuration)))
    {
      return;
    }
    std::atomic<xiiUInt32> uiAccepted{0};
    std::atomic<xiiUInt32> uiErrors{0};
    std::atomic<bool>      bReady{false};
    {
      AsyncTestThread producer([&]() {
        for (xiiUInt32 uiIndex = 0; uiIndex < 256; ++uiIndex)
        {
          xiiNetworkIOOperation operation;
          const auto            error = context.Resolve("127.0.0.1", "80", operation);
          if (error.Succeeded())
          {
            ++uiAccepted;
          }
          else if (error.m_Code != xiiNetworkErrorCode::NotRunning && error.m_Code != xiiNetworkErrorCode::ShuttingDown && error.m_Code != xiiNetworkErrorCode::Backpressure)
          {
            ++uiErrors;
          }
          bReady.store(true);
        }
      });
      const xiiTime   deadline = xiiTime::Now() + AsyncTimeout;
      while (!bReady.load() && xiiTime::Now() < deadline)
      {
        xiiThreadUtils::YieldTimeSlice();
      }
      context.Shutdown();
    }
    XII_TEST_INT(uiErrors.load(), 0);
    XII_TEST_BOOL(uiAccepted.load() > 0);
    xiiNetworkIOResult result;
    xiiUInt32          uiCompletions = 0;
    while (context.TryGetCompletion(result))
    {
      ++uiCompletions;
      XII_TEST_BOOL(result.Succeeded() || result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
    }
    XII_TEST_INT(uiCompletions, uiAccepted.load());
    XII_TEST_INT(context.GetStatistics().m_uiRetainedOperations, 0);
  }
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, TcpRegisteredHalfClose)
{
  xiiNetworkIOContext context;
  if (!AsyncCheck(context.Start()))
  {
    return;
  }
  xiiNetworkIORegistration client;
  xiiNetworkIORegistration server;
  if (!AsyncCheck(AsyncMakePair(context, client, server)))
  {
    return;
  }
  xiiNetworkIOBuffers   receive;
  xiiNetworkIOOperation operation;
  if (!AsyncBuffer(context, 1, receive) || !AsyncCheck(context.Receive(server, std::move(receive), operation)) ||
      !AsyncCheck(context.ShutdownSocket(client, xiiNetworkShutdown::Send)))
  {
    return;
  }
  xiiNetworkIOResult result;
  if (!AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::ConnectionClosed);
  xiiNetworkIOBuffers send;
  if (!AsyncBuffer(context, 1, send, 42) || !AsyncCheck(context.Send(server, std::move(send), operation)))
  {
    return;
  }
  if (!AsyncWait(context, result) || !XII_TEST_BOOL(result.Succeeded()))
  {
    return;
  }
  if (!AsyncBuffer(context, 1, receive) || !AsyncCheck(context.Receive(client, std::move(receive), operation)) || !AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.Succeeded());
  XII_TEST_INT(result.m_uiBytesTransferred, 1);
  XII_TEST_INT(result.m_Buffers[0].GetBytes()[0], 42);
  XII_TEST_BOOL(context.ShutdownSocket({}, xiiNetworkShutdown::Send).m_Code == xiiNetworkErrorCode::InvalidHandle);
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, LocalOperations)
{
  const auto                endpoint = xiiNetworkHostTestUtilities::MakeLocalTestEndpoint("local-io");
  xiiNetworkIOConfiguration configuration;
  configuration.m_uiMaxOperationsPerSocket     = 1;
  configuration.m_uiMaxTransferBytesPerAttempt = 17;
  xiiNetworkIOContext      context;
  xiiNetworkIORegistration listener, client, server;
  xiiNetworkIOOperation    accept, connect;
  if (!AsyncCheck(context.Start(configuration)) || !AsyncCheck(context.ListenLocal(endpoint, listener)))
  {
    return;
  }

  xiiNetworkIOOptions timed;
  timed.m_Timeout = xiiTime::MakeFromMilliseconds(20);
  xiiNetworkIOResult result;
  if (!AsyncCheck(context.Accept(listener, accept, timed)) || !AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.m_Operation == accept);
  XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkErrorCode::TimedOut);
  XII_TEST_BOOL(!result.m_AcceptedLocalSocket.IsOpen());
  XII_TEST_BOOL(context.Cancel(accept).m_Code == xiiNetworkErrorCode::InvalidHandle);

  if (!AsyncCheck(context.OpenLocal(client)) || !AsyncCheck(context.Accept(listener, accept)) || !AsyncCheck(context.Connect(client, endpoint, connect)))
  {
    return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
  {
    if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
    {
      return;
    }
    XII_TEST_BOOL(result.m_LocalEndpoint == endpoint);
    if (result.m_Operation == accept)
    {
      XII_TEST_BOOL(result.m_AcceptedLocalSocket.IsOpen());
      if (!AsyncCheck(context.Register(std::move(result.m_AcceptedLocalSocket), server)))
      {
        return;
      }
      XII_TEST_BOOL(!result.m_AcceptedLocalSocket.IsOpen());
    }
    else
    {
      XII_TEST_BOOL(result.m_Operation == connect);
    }
  }

  xiiNetworkIOBuffer    lease;
  xiiNetworkIOOperation read;
  xiiNetworkIOOperation rejected;
  xiiNetworkIOBuffers   readBuffers;
  xiiNetworkIOBuffers   held;
  if (!AsyncCheck(context.AcquireBuffer(513, lease)))
  {
    return;
  }
  readBuffers.PushBack(std::move(lease));
  if (!AsyncCheck(context.AcquireBuffer(1, lease)))
  {
    return;
  }
  held.PushBack(std::move(lease));
  if (!AsyncCheck(context.Receive(server, std::move(readBuffers), read)))
  {
    return;
  }
  XII_TEST_BOOL(context.Receive(server, std::move(held), rejected).m_Code == xiiNetworkErrorCode::Backpressure);
  XII_TEST_BOOL(held.GetCount() == 1 && held[0].IsValid());
  if (!AsyncCheck(context.Cancel(read)) || !AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.m_Operation == read && result.m_Error.m_Code == xiiNetworkErrorCode::Cancelled);
  XII_TEST_BOOL(result.m_Buffers.GetCount() == 1 && result.m_Buffers[0].GetCount() == 513);
  XII_TEST_BOOL(!context.TryGetCompletion(result));

  xiiNetworkIOBuffers writeBuffers;
  constexpr xiiUInt32 PayloadBytes = 513;
  if (!AsyncCheck(context.AcquireBuffer(PayloadBytes, lease)))
  {
    return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < PayloadBytes; ++uiIndex)
  {
    lease.GetBytes()[uiIndex] = static_cast<xiiUInt8>(uiIndex % 173);
  }
  writeBuffers.PushBack(std::move(lease));
  xiiNetworkIOOperation write;
  if (!AsyncCheck(context.Receive(server, std::move(result.m_Buffers), read)) ||
      !AsyncCheck(context.Send(client, std::move(writeBuffers), write)))
  {
    return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
  {
    if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error))
    {
      return;
    }
    XII_TEST_INT(result.m_uiBytesTransferred, PayloadBytes);
    if (result.m_Operation == read)
    {
      for (xiiUInt32 uiByte = 0; uiByte < PayloadBytes; ++uiByte)
      {
        XII_TEST_INT(result.m_Buffers[0].GetBytes()[uiByte], uiByte % 173);
      }
    }
    else
    {
      XII_TEST_BOOL(result.m_Operation == write);
    }
  }
  xiiString sIdentity;
  XII_TEST_BOOL(context.GetLocalPeerIdentity(server, sIdentity).Succeeded() && !sIdentity.IsEmpty());

  if (!AsyncCheck(context.Receive(server, std::move(held), read)) || !AsyncCheck(context.Deregister(client)) || !AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.m_Operation == read && result.m_Error.m_Code == xiiNetworkErrorCode::ConnectionClosed);
  XII_TEST_BOOL(context.Cancel(client).m_Code == xiiNetworkErrorCode::InvalidHandle);
  if (!AsyncCheck(context.Accept(listener, accept)))
  {
    return;
  }
  context.Shutdown();
  if (AsyncWait(context, result))
  {
    XII_TEST_BOOL(result.m_Operation == accept && result.Failed());
  }
  XII_TEST_BOOL(!context.TryGetCompletion(result));
  context.Shutdown();

  xiiNetworkIOContext rebound;
  XII_TEST_BOOL(rebound.Start(configuration).Succeeded());
  XII_TEST_BOOL(rebound.ListenLocal(endpoint, listener).Succeeded());
}

xiiInt32 xiiNetworkHostTestUtilities::RunLocalTransferClient(xiiStringView sName)
{
  xiiNetworkLocalEndpoint  endpoint;
  xiiNetworkIOContext      context;
  xiiNetworkIORegistration client;
  xiiNetworkIOOperation    operation;
  const auto               timeout = xiiTime::MakeFromSeconds(10);
  xiiNetworkIOResult       result;
  if (xiiNetworkLocalEndpoint::Make(sName, endpoint).Failed() || context.Start().Failed() ||
      context.OpenLocal(client).Failed() || context.Connect(client, endpoint, operation).Failed() ||
      context.WaitForCompletion(result, timeout).Failed() || result.Failed())
  {
    return 1;
  }

  xiiNetworkIOBuffer marker;
  if (context.AcquireBuffer(1, marker).Failed())
  {
    return 2;
  }
  marker.GetBytes()[0] = 42;
  xiiNetworkIOBuffers buffers;
  buffers.PushBack(std::move(marker));
  if (context.Send(client, std::move(buffers), operation).Failed() || context.WaitForCompletion(result, timeout).Failed() || result.Failed())
  {
    return 3;
  }

  // Wait without reading, so the parent's large write has pending native work when it terminates us.
  context.WaitForCompletion(result, timeout).IgnoreResult();
  return 4;
}

XII_CREATE_SIMPLE_TEST(NetworkAsync, LocalProcessTermination)
{
  const auto               endpoint = xiiNetworkHostTestUtilities::MakeLocalTestEndpoint("local-child");
  xiiNetworkIOContext      context;
  xiiNetworkIORegistration listener, server;
  xiiNetworkIOOperation    operation;
  if (!AsyncCheck(context.Start()) || !AsyncCheck(context.ListenLocal(endpoint, listener)) || !AsyncCheck(context.Accept(listener, operation)))
  {
    return;
  }

  xiiProcessOptions options;
  options.m_sProcess = xiiOSFile::GetApplicationPath();
  options.m_Arguments.PushBack("-localTransferClient");
  options.m_Arguments.PushBack(xiiString(endpoint.GetName()));
  xiiProcess         child;
  xiiNetworkIOResult result;
  if (!XII_TEST_BOOL(child.Launch(options).Succeeded()) || !AsyncWait(context, result) || !AsyncCheck(result.m_Error) ||
      !AsyncCheck(context.Register(std::move(result.m_AcceptedLocalSocket), server)))
  {
    return;
  }

  xiiNetworkIOBuffer lease;
  if (!AsyncCheck(context.AcquireBuffer(1, lease)))
  {
    return;
  }
  xiiNetworkIOBuffers buffers;
  buffers.PushBack(std::move(lease));
  if (!AsyncCheck(context.Receive(server, std::move(buffers), operation)) || !AsyncWait(context, result) ||
      !AsyncCheck(result.m_Error) || !XII_TEST_BOOL(result.m_Buffers[0].GetBytes()[0] == 42))
  {
    return;
  }

  constexpr xiiUInt32 PayloadBytes = 4 * 1024 * 1024;
  if (!AsyncCheck(context.AcquireBuffer(PayloadBytes, lease)))
  {
    return;
  }
  std::memset(lease.GetBytes().GetPtr(), 1, PayloadBytes);
  buffers.PushBack(std::move(lease));
  if (!AsyncCheck(context.Send(server, std::move(buffers), operation)) || !XII_TEST_BOOL(child.Terminate().Succeeded()) || !AsyncWait(context, result))
  {
    return;
  }
  XII_TEST_BOOL(result.m_Operation == operation && result.Failed());
  XII_TEST_BOOL(result.m_uiBytesTransferred < PayloadBytes);
  XII_TEST_BOOL(result.m_Buffers.GetCount() == 1 && result.m_Buffers[0].IsValid());
  XII_TEST_BOOL(!context.TryGetCompletion(result));
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
XII_CREATE_SIMPLE_TEST(NetworkAsync, LocalRejectsAnonymousPipeClient)
{
  const auto               endpoint = xiiNetworkHostTestUtilities::MakeLocalTestEndpoint("local-anon");
  xiiNetworkIOContext      context;
  xiiNetworkIORegistration listener, accepted;
  xiiNetworkIOOperation    operation;
  if (!AsyncCheck(context.Start()) || !AsyncCheck(context.ListenLocal(endpoint, listener)) || !AsyncCheck(context.Accept(listener, operation)))
  {
    return;
  }

  HANDLE hToken = nullptr;
  if (!XII_TEST_BOOL(::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken)))
  {
    return;
  }
  alignas(TOKEN_USER) xiiUInt8 token[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE] = {};
  DWORD                        uiTokenSize                                       = 0;
  const BOOL                   bGotUser                                          = ::GetTokenInformation(hToken, TokenUser, token, sizeof(token), &uiTokenSize);
  ::CloseHandle(hToken);
  LPSTR szSid = nullptr;
  if (!XII_TEST_BOOL(bGotUser && ::ConvertSidToStringSidA(reinterpret_cast<TOKEN_USER*>(token)->User.Sid, &szSid)))
  {
    return;
  }
  xiiStringBuilder address;
  address.SetFormat(R"(\\.\pipe\xii-ipc-{0}-{1})", szSid, endpoint.GetName());
  ::LocalFree(szSid);
  const HANDLE hPipe = ::CreateFileW(xiiStringWChar(address).GetData(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                     OPEN_EXISTING, SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr);
  if (!XII_TEST_BOOL(hPipe != INVALID_HANDLE_VALUE))
  {
    return;
  }
  auto               closePipe = xiiMakeScopeExit([hPipe]() { ::CloseHandle(hPipe); });
  xiiNetworkIOResult result;
  if (!AsyncWait(context, result) || !AsyncCheck(result.m_Error) ||
      !AsyncCheck(context.Register(std::move(result.m_AcceptedLocalSocket), accepted)))
  {
    return;
  }
  xiiNetworkIOBuffer buffer;
  if (!AsyncCheck(context.AcquireBuffer(1, buffer)))
  {
    return;
  }
  xiiNetworkIOBuffers buffers;
  buffers.PushBack(std::move(buffer));
  const xiiUInt8 uiByte    = 1;
  DWORD          uiWritten = 0;
  if (AsyncCheck(context.Receive(accepted, std::move(buffers), operation)) &&
      XII_TEST_BOOL(::WriteFile(hPipe, &uiByte, 1, &uiWritten, nullptr) && uiWritten == 1) &&
      AsyncWait(context, result))
  {
    XII_TEST_BOOL(result.m_Operation == operation && result.m_Error.m_Code == xiiNetworkErrorCode::AuthenticationFailed);
    XII_TEST_INT(result.m_uiBytesTransferred, 0);
  }
}
#endif

#if XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)
XII_CREATE_SIMPLE_TEST(NetworkAsync, LocalPreservesReplacedSocket)
{
  const auto               endpoint = xiiNetworkHostTestUtilities::MakeLocalTestEndpoint("local-replace");
  xiiNetworkIOContext      context;
  xiiNetworkIORegistration listener;
  if (!AsyncCheck(context.Start()) || !AsyncCheck(context.ListenLocal(endpoint, listener)))
  {
    return;
  }
  xiiStringBuilder directory;
  directory.SetFormat("/tmp/xii-ipc-{0}-{1}", static_cast<xiiUInt32>(::geteuid()), endpoint.GetName());
  xiiStringBuilder path = directory;
  path.Append("/s");
  xiiStringBuilder original = directory;
  original.Append("/original");
  struct stat entry = {};
  XII_TEST_BOOL(::lstat(directory.GetData(), &entry) == 0 && (entry.st_mode & (S_IRWXG | S_IRWXO)) == 0);
  if (!XII_TEST_BOOL(::rename(path.GetData(), original.GetData()) == 0))
  {
    return;
  }
  auto      removeEntries = xiiMakeScopeExit([&]() {
    ::unlink(path.GetData());
    ::unlink(original.GetData());
    ::rmdir(directory.GetData());
  });
  const int iSocket       = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (!XII_TEST_BOOL(iSocket >= 0))
  {
    return;
  }
  auto        closeSocket = xiiMakeScopeExit([iSocket]() { ::close(iSocket); });
  sockaddr_un replacement = {};
  replacement.sun_family  = AF_UNIX;
#  if XII_ENABLED(XII_PLATFORM_OSX)
  replacement.sun_len = sizeof(replacement);
#  endif
  std::memcpy(replacement.sun_path, path.GetData(), path.GetElementCount() + 1);
  if (!XII_TEST_BOOL(::bind(iSocket, reinterpret_cast<const sockaddr*>(&replacement), sizeof(replacement)) == 0))
  {
    return;
  }
  context.Shutdown();
  XII_TEST_BOOL(::lstat(path.GetData(), &entry) == 0 && S_ISSOCK(entry.st_mode));
}
#endif
