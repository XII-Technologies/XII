/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/Messaging/NetworkReflectionEndpoint.h>
#include <Foundation/Threading/Implementation/OSThread.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <FoundationTest/Network/Implementation/NetworkHostTestUtilities.h>
#include <FoundationTest/Network/Implementation/NetworkReflectionTestTypes.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>
#include <atomic>
#include <cstring>

struct NetworkRpcExample
{
  xiiInt32 m_iCalls = 0;
  xiiInt32 Add(xiiInt32 a, xiiInt32 b)
  {
    ++m_iCalls;
    return a + b;
  }
  xiiString Echo(const xiiString& text)
  {
    ++m_iCalls;
    return text;
  }
  NetworkExampleNested Nested(const NetworkExampleNested& value)
  {
    ++m_iCalls;
    return value;
  }
  xiiTimestamp Timestamp(xiiTimestamp time)
  {
    ++m_iCalls;
    return time;
  }
  void Notify() { ++m_iCalls; }
  void Denied() { ++m_iCalls; }
  void Hidden() { ++m_iCalls; }
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkRpcExample);
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkRpcExample, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkRpcExample>)
  {
    XII_BEGIN_FUNCTIONS
    {
      XII_FUNCTION_PROPERTY(Add)->AddAttributes(new xiiNetworkRpcAttribute(1)),
      XII_FUNCTION_PROPERTY(Echo)->AddAttributes(new xiiNetworkRpcAttribute(2)),
      XII_FUNCTION_PROPERTY(Nested)->AddAttributes(new xiiNetworkRpcAttribute(3)),
      XII_FUNCTION_PROPERTY(Timestamp)->AddAttributes(new xiiNetworkRpcAttribute(4)),
      XII_FUNCTION_PROPERTY(Notify)->AddAttributes(new xiiNetworkRpcAttribute(5)),
      XII_FUNCTION_PROPERTY(Denied)->AddAttributes(new xiiNetworkRpcAttribute(6)),
      XII_FUNCTION_PROPERTY(Hidden)} XII_END_FUNCTIONS;
  }
XII_END_STATIC_REFLECTED_TYPE;

namespace
{
  class ExampleHandler final : public xiiNetworkTypedMessageHandler<NetworkExampleState>
  {
  public:
    bool Authorize(const xiiNetworkDispatchContext& context) const override { return m_bAllow && context.m_pSession != nullptr; }
    void OnMessage(const xiiNetworkDispatchContext& context, const NetworkExampleState& state) override
    {
      XII_TEST_STRING(state.m_sLocal, "constructor");
      m_pSession = context.m_pSession;
      m_iLast    = state.m_iSequence;
      ++m_iCalls;
    }
    bool                                      m_bAllow = true;
    xiiInt32                                  m_iCalls = 0, m_iLast = 0;
    xiiSharedPtr<const xiiNetworkSessionInfo> m_pSession;
  };
  class ExampleService final : public xiiNetworkRpcService
  {
  public:
    const xiiRTTI* GetType() const override { return xiiGetStaticRTTI<NetworkRpcExample>(); }
    void*          GetObject() override { return &m_Target; }
    bool           Authorize(const xiiNetworkDispatchContext& context) const override
    {
      return context.m_pSession != nullptr && context.m_uiService == 7 && context.m_uiMethod != 6 && m_bAllow;
    }
    NetworkRpcExample m_Target;
    bool              m_bAllow = true;
  };
  struct Pair
  {
    xiiNetworkTypeRegistry                    m_Registry;
    xiiNetworkHost                            m_Server, m_Client;
    xiiNetworkReflectionEndpoint              m_Receiver{m_Server, m_Registry}, m_Sender{m_Client, m_Registry};
    xiiNetworkChannel                         m_ServerChannel, m_ClientChannel;
    xiiSharedPtr<const xiiNetworkSessionInfo> m_ServerInfo, m_ClientInfo;
    NetworkSecurityTestCertificates           m_Files;
    ~Pair()
    {
      m_Sender.Shutdown();
      m_Receiver.Shutdown();
      m_Client.Shutdown();
      m_Server.Shutdown();
    }
    bool Start(xiiNetworkTransportType transport = xiiNetworkTransportType::Tcp)
    {
      auto server = xiiNetworkHostTestUtilities::HostTestConfiguration(transport);
      auto client = server;
      xiiNetworkReflectionEndpoint::EnableProtocol(server);
      xiiNetworkReflectionEndpoint::EnableProtocol(client);
      if (transport != xiiNetworkTransportType::Tcp)
      {
        if (!XII_TEST_BOOL(m_Files.Initialize().Succeeded()))
          return false;
        server.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
        client.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
        if (!XII_TEST_BOOL(server.m_pCredentials->Load(m_Files.Configuration()).Succeeded()) ||
            !XII_TEST_BOOL(client.m_pCredentials->Load(m_Files.Configuration("client")).Succeeded()))
          return false;
      }
      if (!XII_TEST_BOOL(m_Server.Start(server).Succeeded()) || !XII_TEST_BOOL(m_Client.Start(client).Succeeded()) ||
          !XII_TEST_BOOL(m_Receiver.Start().Succeeded()) || !XII_TEST_BOOL(m_Sender.Start().Succeeded()))
        return false;
      xiiNetworkListener listener;
      xiiNetworkEndpoint endpoint;
      if (!XII_TEST_BOOL(m_Server.Listen({xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 0}, listener).Succeeded()) ||
          !XII_TEST_BOOL(m_Server.GetLocalEndpoint(listener, endpoint).Succeeded()))
        return false;
      xiiNetworkConnection     connection;
      xiiNetworkConnectOptions options;
      options.m_sServerName = "localhost";
      if (!XII_TEST_BOOL(m_Client.Connect(endpoint, options, connection).Succeeded()))
        return false;
      xiiNetworkEvent event;
      if (!xiiNetworkHostTestUtilities::HostTestEvent(m_Server, xiiNetworkEventType::Connected, event))
        return false;
      m_ServerChannel = event.m_pSessionInfo->m_DefaultChannel;
      m_ServerInfo    = event.m_pSessionInfo;
      if (!XII_TEST_BOOL(m_Receiver.ProcessEvent(std::move(event)).Succeeded()))
        return false;
      if (!xiiNetworkHostTestUtilities::HostTestEvent(m_Client, xiiNetworkEventType::Connected, event))
        return false;
      m_ClientChannel = event.m_pSessionInfo->m_DefaultChannel;
      m_ClientInfo    = event.m_pSessionInfo;
      return XII_TEST_BOOL(m_Sender.ProcessEvent(std::move(event)).Succeeded());
    }
    bool Receive(xiiNetworkHost& host, xiiNetworkReflectionEndpoint& endpoint)
    {
      xiiNetworkEvent event;
      if (!xiiNetworkHostTestUtilities::HostTestEvent(host, xiiNetworkEventType::Message, event))
        return false;
      const auto error = endpoint.ProcessEvent(std::move(event));
      return XII_TEST_BOOL_MSG(error.Succeeded(), "%s", error.GetCodeName());
    }
    bool Agree()
    {
      return XII_TEST_BOOL(m_Sender.OfferSchema(m_ClientChannel, xiiGetStaticRTTI<NetworkExampleState>()).Succeeded()) &&
        Receive(m_Server, m_Receiver) && Receive(m_Client, m_Sender);
    }
    bool Rpc(xiiUInt64 method, xiiArrayPtr<const xiiVariant> arguments, xiiNetworkRpcResult& result, xiiNetworkDataErrorCode expected = xiiNetworkDataErrorCode::Success)
    {
      xiiNetworkRpcRequest request;
      if (!XII_TEST_BOOL(m_Sender.CallRpc(m_ClientChannel, 7, method, arguments, request).Succeeded()) || !Receive(m_Server, m_Receiver))
        return false;
      const auto dispatched = m_Receiver.Dispatch(12);
      if (!XII_TEST_BOOL(dispatched.m_Code == expected) || !Receive(m_Client, m_Sender) || !XII_TEST_BOOL(m_Sender.TryGetRpcResult(result)))
        return false;
      XII_TEST_BOOL(result.m_Request == request);
      return XII_TEST_BOOL(result.m_Error.m_Code == expected);
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(NetworkReflection, TypedMessagingAndRpc)
{
  for (auto transport : {xiiNetworkTransportType::Tcp, xiiNetworkTransportType::TlsTcp, xiiNetworkTransportType::SecureDatagram})
  {
    Pair pair;
    if (!pair.Start(transport))
      return;
    xiiSharedPtr<ExampleHandler> handler = XII_DEFAULT_NEW(ExampleHandler);
    xiiSharedPtr<ExampleService> service = XII_DEFAULT_NEW(ExampleService);
    XII_TEST_BOOL(pair.m_Receiver.RegisterMessageHandler(handler, 11).Succeeded());
    const auto registration = pair.m_Receiver.RegisterService(7, service, 12);
    XII_TEST_BOOL_MSG(registration.Succeeded(), "%s method %llu", registration.GetCodeName(), static_cast<unsigned long long>(registration.m_uiField));
    XII_TEST_BOOL(pair.m_Sender.DeclareRemoteService(7, xiiGetStaticRTTI<NetworkRpcExample>()).Succeeded());
    NetworkExampleState message;
    message.m_iSequence = 123;
    message.m_sLocal    = "never transmitted";
    XII_TEST_BOOL(pair.m_Sender.SendMessage(pair.m_ClientChannel, message).m_Code == xiiNetworkDataErrorCode::NotNegotiated);
    if (!pair.Agree())
      return;
    for (auto encoding : {xiiNetworkEncoding::Tagged, xiiNetworkEncoding::Compact})
    {
      if (!XII_TEST_BOOL(pair.m_Sender.SendMessage(pair.m_ClientChannel, message, encoding).Succeeded()) || !pair.Receive(pair.m_Server, pair.m_Receiver))
        return;
      XII_TEST_BOOL(pair.m_Receiver.Dispatch(10).Succeeded());
      XII_TEST_INT(handler->m_iCalls, encoding == xiiNetworkEncoding::Tagged ? 0 : 1);
      XII_TEST_BOOL(pair.m_Receiver.Dispatch(11).Succeeded());
      XII_TEST_INT(handler->m_iLast, 123);
    }
    xiiNetworkRpcResult result;
    const xiiVariant    args[] = {xiiInt32(19), xiiInt32(23)};
    if (!pair.Rpc(1, args, result))
      return;
    xiiVariant value;
    XII_TEST_BOOL(pair.m_Sender.DecodeRpcResult(result, value).Succeeded());
    XII_TEST_INT(value.Get<xiiInt32>(), 42);
    const xiiVariant text[] = {xiiString("portable utf8: \xCF\x80")};
    if (!pair.Rpc(2, text, result))
      return;
    XII_TEST_BOOL(pair.m_Sender.DecodeRpcResult(result, value).Succeeded());
    XII_TEST_STRING(value.Get<xiiString>(), text[0].Get<xiiString>());
    NetworkExampleNested nested;
    nested.m_iNumber             = 91;
    nested.m_sLocal              = "sender only";
    const xiiVariant structure[] = {xiiVariant(&nested, xiiGetStaticRTTI<NetworkExampleNested>())};
    if (!pair.Rpc(3, structure, result))
      return;
    XII_TEST_BOOL(pair.m_Sender.DecodeRpcResult(result, value).Succeeded());
    XII_TEST_INT(static_cast<const NetworkExampleNested*>(value.GetData())->m_iNumber, 91);
    XII_TEST_STRING(static_cast<const NetworkExampleNested*>(value.GetData())->m_sLocal, "constructor");
    const xiiVariant timestamp[] = {xiiInt64(123456789)};
    if (!pair.Rpc(4, timestamp, result))
      return;
    XII_TEST_BOOL(pair.m_Sender.DecodeRpcResult(result, value).Succeeded());
    XII_TEST_INT(value.Get<xiiInt64>(), 123456789);
    if (!pair.Rpc(6, {}, result, xiiNetworkDataErrorCode::Unauthorized))
      return;
    XII_TEST_INT(service->m_Target.m_iCalls, 4);
    xiiNetworkRpcRequest request;
    XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, xiiNetworkStableId("Hidden"), {}, request).m_Code == xiiNetworkDataErrorCode::UnknownRpc);
    XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 5, {}, request, xiiTime::MakeZero(), true).Succeeded());
    XII_TEST_BOOL(!request.IsValid());
    if (!pair.Receive(pair.m_Server, pair.m_Receiver))
      return;
    XII_TEST_BOOL(pair.m_Receiver.Dispatch(12).Succeeded());
    XII_TEST_INT(service->m_Target.m_iCalls, 5);
    XII_TEST_BOOL(!pair.m_Sender.TryGetRpcResult(result));
    XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 5, {}, request).Succeeded());
    if (!pair.Receive(pair.m_Server, pair.m_Receiver))
      return;
    XII_TEST_BOOL(pair.m_Sender.CancelRpc(request).Succeeded());
    XII_TEST_BOOL(pair.m_Sender.TryGetRpcResult(result));
    XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkDataErrorCode::Cancelled);
    if (!pair.Receive(pair.m_Server, pair.m_Receiver))
      return;
    XII_TEST_BOOL(pair.m_Receiver.Dispatch(12).Succeeded());
    XII_TEST_INT(service->m_Target.m_iCalls, 5);
    // Timeout is local and remains terminal even if a late response arrives.
    XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 5, {}, request, xiiTime::MakeFromMilliseconds(1)).Succeeded());
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(5));
    pair.m_Sender.Update();
    XII_TEST_BOOL(pair.m_Sender.TryGetRpcResult(result));
    XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkDataErrorCode::TimedOut);
    // Removal suppresses queued delivery; it cannot destroy an active callback target.
    message.m_iSequence = 456;
    XII_TEST_BOOL(pair.m_Sender.SendMessage(pair.m_ClientChannel, message).Succeeded());
    // Drain any preceding RPC/cancellation response without assuming completion timing.
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    while (pair.m_Receiver.GetStatistics().m_uiQueuedPackets < 2 && xiiTime::Now() < deadline)
      if (!pair.Receive(pair.m_Server, pair.m_Receiver))
        return;
    xiiSharedPtr<const xiiNetworkSchema> schema;
    XII_TEST_BOOL(pair.m_Registry.GetSchema(xiiGetStaticRTTI<NetworkExampleState>(), schema).Succeeded());
    XII_TEST_BOOL(pair.m_Receiver.RemoveMessageHandler(schema->m_uiTypeId).Succeeded());
    XII_TEST_BOOL(pair.m_Receiver.Dispatch(11).m_Code == xiiNetworkDataErrorCode::HandlerRemoved);
    XII_TEST_INT(handler->m_iCalls, 2);
    XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 5, {}, request).Succeeded());
    pair.m_Sender.Shutdown();
    XII_TEST_BOOL(pair.m_Sender.TryGetRpcResult(result));
    XII_TEST_BOOL(result.m_Error.m_Code == xiiNetworkDataErrorCode::Disconnected);
    XII_TEST_BOOL(!pair.m_Sender.TryGetRpcResult(result));
  }
}

namespace
{
  class ReflectionWorker
  {
  public:
    explicit ReflectionWorker(xiiDelegate<void()> function) : m_Function(std::move(function)), m_Thread(Entry, this, "Reflection worker", 1024 * 1024) { m_Thread.Start(); }
    ~ReflectionWorker() { m_Thread.Join(); }

  private:
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    static xiiMinWindows::DWORD __stdcall Entry(void* data)
#else
    static void* Entry(void* data)
#endif
    {
      static_cast<ReflectionWorker*>(data)->m_Function();
      return {};
    }
    xiiDelegate<void()> m_Function;
    xiiOSThread         m_Thread;
  };
  class BlockingHandler final : public xiiNetworkTypedMessageHandler<NetworkExampleState>
  {
  public:
    explicit BlockingHandler(std::atomic<bool>& destroyed) : m_Destroyed(destroyed) {}
    ~BlockingHandler() override { m_Destroyed = true; }
    bool Authorize(const xiiNetworkDispatchContext& context) const override { return context.m_pSession != nullptr; }
    void OnMessage(const xiiNetworkDispatchContext&, const NetworkExampleState&) override
    {
      // Queries and reentrant submissions must not run under an endpoint mutex.
      XII_TEST_BOOL(m_pEndpoint->GetStatistics().m_uiQueuedPackets == 0);
      m_Entered           = true;
      const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
      while (!m_Release && xiiTime::Now() < deadline)
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
      XII_TEST_BOOL(m_Release.load());
    }
    xiiNetworkReflectionEndpoint* m_pEndpoint = nullptr;
    std::atomic<bool>             m_Entered{false}, m_Release{false};
    std::atomic<bool>&            m_Destroyed;
  };
  xiiNetworkEvent Synthetic(Pair& pair, xiiArrayPtr<const xiiUInt8> bytes)
  {
    xiiNetworkEvent event;
    event.m_Type         = xiiNetworkEventType::Message;
    event.m_Connection   = pair.m_ServerChannel.m_Connection;
    event.m_Channel      = pair.m_ServerChannel;
    event.m_pSessionInfo = pair.m_ServerInfo;
    if (XII_TEST_BOOL(pair.m_Server.AcquireBuffer(bytes.GetCount(), event.m_Payload).Succeeded()))
      if (!bytes.IsEmpty())
        std::memcpy(event.m_Payload.GetBytes().GetPtr(), bytes.GetPtr(), bytes.GetCount());
    return event;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(NetworkReflection, DispatchLifetimeAndMetadataBarrier)
{
  Pair pair;
  if (!pair.Start() || !pair.Agree())
    return;
  std::atomic<bool>             destroyed{false}, invalidated{false};
  xiiSharedPtr<BlockingHandler> handler = XII_DEFAULT_NEW(BlockingHandler, destroyed);
  handler->m_pEndpoint                  = &pair.m_Receiver;
  auto* active                          = handler.Borrow();
  XII_TEST_BOOL(pair.m_Receiver.RegisterMessageHandler(handler, 3).Succeeded());
  NetworkExampleState message;
  XII_TEST_BOOL(pair.m_Sender.SendMessage(pair.m_ClientChannel, message).Succeeded());
  if (!pair.Receive(pair.m_Server, pair.m_Receiver))
    return;
  {
    ReflectionWorker dispatch([&]() { XII_TEST_BOOL(pair.m_Receiver.Dispatch(3, 1).Succeeded()); });
    const auto       deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    while (!active->m_Entered && xiiTime::Now() < deadline)
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
    if (!XII_TEST_BOOL(active->m_Entered.load()))
    {
      active->m_Release = true;
      return;
    }
    xiiSharedPtr<const xiiNetworkSchema> schema;
    XII_TEST_BOOL(pair.m_Registry.GetSchema(xiiGetStaticRTTI<NetworkExampleState>(), schema).Succeeded());
    XII_TEST_BOOL(pair.m_Receiver.RemoveMessageHandler(schema->m_uiTypeId).Succeeded());
    handler.Clear();
    XII_TEST_BOOL(!destroyed.load());
    ReflectionWorker invalidate([&]() { pair.m_Registry.Invalidate(); invalidated = true; });
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    XII_TEST_BOOL(!invalidated.load());
    active->m_Release = true;
  }
  XII_TEST_BOOL(invalidated.load());
  XII_TEST_BOOL(destroyed.load());
  XII_TEST_BOOL(pair.m_Registry.Refresh().Succeeded());
  // Real plugin-change notifications invalidate callbacks and agreements, retaining live sessions.
  xiiSharedPtr<ExampleHandler> replacement = XII_DEFAULT_NEW(ExampleHandler);
  XII_TEST_BOOL(pair.m_Receiver.RegisterMessageHandler(replacement).Succeeded());
  xiiPlugin::BeginPluginChanges();
  xiiPlugin::EndPluginChanges();
  XII_TEST_BOOL(pair.m_Sender.SendMessage(pair.m_ClientChannel, message).m_Code == xiiNetworkDataErrorCode::NotNegotiated);
  XII_TEST_BOOL(pair.m_Receiver.RegisterMessageHandler(replacement).Succeeded());
  if (!pair.Agree())
    return;
  XII_TEST_BOOL(pair.m_Sender.SendMessage(pair.m_ClientChannel, message).Succeeded());
  if (!pair.Receive(pair.m_Server, pair.m_Receiver))
    return;
  XII_TEST_BOOL(pair.m_Receiver.Dispatch(0).Succeeded());
  XII_TEST_INT(replacement->m_iCalls, 1);
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, RpcCorrelationReplayAndLimits)
{
  Pair pair;
  if (!pair.Start())
    return;
  xiiSharedPtr<ExampleService> service = XII_DEFAULT_NEW(ExampleService);
  xiiNetworkRpcServiceLimits   limits;
  limits.m_uiMaximumConcurrentRequests = 2;
  XII_TEST_BOOL(pair.m_Receiver.RegisterService(7, service, 12, limits).Succeeded());
  XII_TEST_BOOL(pair.m_Sender.DeclareRemoteService(7, xiiGetStaticRTTI<NetworkRpcExample>()).Succeeded());
  const xiiVariant     args[] = {xiiInt32(19), xiiInt32(23)};
  xiiNetworkRpcRequest first, second, third;
  XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 1, args, first).Succeeded());
  xiiNetworkEvent event;
  if (!xiiNetworkHostTestUtilities::HostTestEvent(pair.m_Server, xiiNetworkEventType::Message, event))
    return;
  xiiDynamicArray<xiiUInt8> saved(event.m_Payload.GetBytes());
  XII_TEST_BOOL(pair.m_Receiver.ProcessEvent(std::move(event)).Succeeded());
  XII_TEST_BOOL(pair.m_Receiver.ProcessEvent(Synthetic(pair, saved)).m_Code == xiiNetworkDataErrorCode::DuplicateRequest);
  XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 1, args, second).Succeeded());
  if (!pair.Receive(pair.m_Server, pair.m_Receiver))
    return;
  XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 1, args, third).Succeeded());
  if (!xiiNetworkHostTestUtilities::HostTestEvent(pair.m_Server, xiiNetworkEventType::Message, event))
    return;
  XII_TEST_BOOL(pair.m_Receiver.ProcessEvent(std::move(event)).m_Code == xiiNetworkDataErrorCode::Backpressure);
  if (!pair.Receive(pair.m_Client, pair.m_Sender))
    return;
  xiiNetworkRpcResult result;
  XII_TEST_BOOL(pair.m_Sender.TryGetRpcResult(result));
  XII_TEST_BOOL(result.m_Request == third && result.m_Error.m_Code == xiiNetworkDataErrorCode::Backpressure);
  XII_TEST_BOOL(pair.m_Receiver.Dispatch(12).Succeeded());
  for (auto expected : {first, second})
  {
    if (!pair.Receive(pair.m_Client, pair.m_Sender))
      return;
    XII_TEST_BOOL(pair.m_Sender.TryGetRpcResult(result));
    XII_TEST_BOOL(result.m_Request == expected);
    xiiInt32 sum = 0;
    XII_TEST_BOOL(pair.m_Sender.DecodeRpcResult(result, sum).Succeeded());
    XII_TEST_INT(sum, 42);
  }
  XII_TEST_INT(service->m_Target.m_iCalls, 2);
  // Corrupt a complete method descriptor while preserving a fresh request ID.
  saved[28] = 100;
  saved[38] ^= 1;
  XII_TEST_BOOL(pair.m_Receiver.ProcessEvent(Synthetic(pair, saved)).Succeeded());
  XII_TEST_BOOL(pair.m_Receiver.Dispatch(12).m_Code == xiiNetworkDataErrorCode::SchemaMismatch);
  XII_TEST_INT(service->m_Target.m_iCalls, 2);
  // Queue a call, remove its service, and confirm no target invocation.
  XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 1, args, first).Succeeded());
  if (!pair.Receive(pair.m_Server, pair.m_Receiver))
    return;
  XII_TEST_BOOL(pair.m_Receiver.RemoveService(7).Succeeded());
  XII_TEST_BOOL(pair.m_Receiver.Dispatch(12).m_Code == xiiNetworkDataErrorCode::HandlerRemoved);
  XII_TEST_INT(service->m_Target.m_iCalls, 2);
  // Disconnect completes every remaining local call once, independently of late responses.
  xiiNetworkEvent closed;
  closed.m_Type       = xiiNetworkEventType::Closed;
  closed.m_Connection = pair.m_ClientChannel.m_Connection;
  pair.m_Registry.Invalidate();
  XII_TEST_BOOL(pair.m_Sender.ProcessEvent(std::move(closed)).Succeeded());
  XII_TEST_BOOL(pair.m_Sender.TryGetRpcResult(result));
  XII_TEST_BOOL(result.m_Request == first && result.m_Error.m_Code == xiiNetworkDataErrorCode::Disconnected);
  XII_TEST_BOOL(!pair.m_Sender.TryGetRpcResult(result));
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, TypedParserMutationCorpus)
{
  Pair pair;
  if (!pair.Start())
    return;
  xiiUInt32 random = 0x12345678;
  for (xiiUInt32 i = 0; i < 3000; ++i)
  {
    xiiUInt8 bytes[128];
    for (auto& byte : bytes)
    {
      random = random * 1664525U + 1013904223U;
      byte   = static_cast<xiiUInt8>(random >> 24);
    }
    if (i % 2 == 0)
    {
      bytes[0] = 0x58;
      bytes[1] = 0x4E;
      bytes[2] = 0x52;
      bytes[3] = 0x31;
      bytes[4] = static_cast<xiiUInt8>(1 + i % 6);
    }
    const auto count = i % 129;
    pair.m_Receiver.ProcessEvent(Synthetic(pair, {bytes, count})).IgnoreResult();
  }
  XII_TEST_BOOL(pair.m_Receiver.GetStatistics().m_uiQueuedPackets == 0);
  XII_TEST_BOOL(pair.m_Receiver.Dispatch(0).Succeeded());
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, TypedDispatchAndRpcBenchmark)
{
  if (!xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-networkReflectionBenchmark"))
    return;
  Pair pair;
  if (!pair.Start() || !pair.Agree())
    return;
  xiiSharedPtr<ExampleHandler> handler = XII_DEFAULT_NEW(ExampleHandler);
  xiiSharedPtr<ExampleService> service = XII_DEFAULT_NEW(ExampleService);
  XII_TEST_BOOL(pair.m_Receiver.RegisterMessageHandler(handler, 11).Succeeded());
  XII_TEST_BOOL(pair.m_Receiver.RegisterService(7, service, 12).Succeeded());
  XII_TEST_BOOL(pair.m_Sender.DeclareRemoteService(7, xiiGetStaticRTTI<NetworkRpcExample>()).Succeeded());
  NetworkExampleState message;
  for (xiiUInt32 i = 0; i < 32; ++i)
  {
    XII_TEST_BOOL(pair.m_Sender.SendMessage(pair.m_ClientChannel, message, xiiNetworkEncoding::Compact).Succeeded());
    if (!pair.Receive(pair.m_Server, pair.m_Receiver))
      return;
  }
  auto started = xiiTime::Now();
  XII_TEST_BOOL(pair.m_Receiver.Dispatch(11, 32).Succeeded());
  xiiTestFramework::Output(xiiTestOutput::Details, "Typed cached decode+dispatch: %.1f ns/message", (xiiTime::Now() - started).GetNanoseconds() / 32);
  const xiiVariant    args[] = {xiiInt32(19), xiiInt32(23)};
  xiiNetworkRpcResult result;
  started = xiiTime::Now();
  for (xiiUInt32 i = 0; i < 100; ++i)
    if (!pair.Rpc(1, args, result))
      return;
  xiiTestFramework::Output(xiiTestOutput::Details, "TCP loopback sequential RPC: %.1f us/roundtrip, includes application polling", (xiiTime::Now() - started).GetMicroseconds() / 100);
  for (xiiUInt32 i = 0; i < 16; ++i)
  {
    xiiNetworkRpcRequest request;
    XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 1, args, request).Succeeded());
    if (!pair.Receive(pair.m_Server, pair.m_Receiver))
      return;
  }
  started = xiiTime::Now();
  XII_TEST_BOOL(pair.m_Receiver.Dispatch(12, 16).Succeeded());
  xiiTestFramework::Output(xiiTestOutput::Details, "Cached RPC decode+invoke+response admission: %.1f ns/call", (xiiTime::Now() - started).GetNanoseconds() / 16);
  XII_TEST_INT(service->m_Target.m_iCalls, 116);
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, RpcPortableWireFixture)
{
  Pair pair;
  if (!pair.Start())
    return;
  xiiSharedPtr<ExampleService> service = XII_DEFAULT_NEW(ExampleService);
  XII_TEST_BOOL(pair.m_Receiver.RegisterService(7, service, 12).Succeeded());
  XII_TEST_BOOL(pair.m_Sender.DeclareRemoteService(7, xiiGetStaticRTTI<NetworkRpcExample>()).Succeeded());
  xiiNetworkRpcRequest request;
  XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 5, {}, request, xiiTime::MakeZero(), true).Succeeded());
  xiiNetworkEvent event;
  if (!xiiNetworkHostTestUtilities::HostTestEvent(pair.m_Server, xiiNetworkEventType::Message, event))
    return;
  constexpr xiiUInt8 expected[] = {
    0x58, 0x4E, 0x52, 0x31, 0x04,
    0, 0, 0, 0, 0, 0, 0, 7,
    0, 0, 0, 0, 0, 0, 0, 5,
    0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0x13, 0x88, 0, 0, 0, 21,
    0x04, 0x13, 0xAB, 0x3A, 0xD5, 0xB1, 0x09, 0x2A,
    0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0};
  XII_TEST_BOOL(event.m_Payload.GetBytes() == xiiMakeArrayPtr(expected));
  XII_TEST_BOOL(pair.m_Receiver.ProcessEvent(std::move(event)).Succeeded());
  XII_TEST_BOOL(pair.m_Receiver.Dispatch(12).Succeeded());
  XII_TEST_INT(service->m_Target.m_iCalls, 1);
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, ChannelCloseCompletesOnlyItsCalls)
{
  Pair pair;
  if (!pair.Start())
    return;
  xiiSharedPtr<ExampleService> service = XII_DEFAULT_NEW(ExampleService);
  XII_TEST_BOOL(pair.m_Receiver.RegisterService(7, service, 12).Succeeded());
  XII_TEST_BOOL(pair.m_Sender.DeclareRemoteService(7, xiiGetStaticRTTI<NetworkRpcExample>()).Succeeded());
  xiiNetworkChannel channel;
  if (!XII_TEST_BOOL(pair.m_Client.OpenChannel(pair.m_ClientChannel.m_Connection, {}, channel).Succeeded()))
    return;
  using xiiNetworkHostTestUtilities::HostTestEvent;
  xiiNetworkEvent event;
  if (!HostTestEvent(pair.m_Client, xiiNetworkEventType::ChannelOpened, event) ||
      !HostTestEvent(pair.m_Server, xiiNetworkEventType::ChannelOpened, event))
    return;
  xiiNetworkRpcRequest secondary, primary;
  if (!XII_TEST_BOOL(pair.m_Sender.CallRpc(channel, 7, 5, {}, secondary).Succeeded()) ||
      !XII_TEST_BOOL(pair.m_Sender.CallRpc(pair.m_ClientChannel, 7, 5, {}, primary).Succeeded()) ||
      !pair.Receive(pair.m_Server, pair.m_Receiver) || !pair.Receive(pair.m_Server, pair.m_Receiver))
    return;
  if (!XII_TEST_BOOL(pair.m_Client.CloseChannel(channel).Succeeded()) ||
      !HostTestEvent(pair.m_Client, xiiNetworkEventType::ChannelClosed, event))
    return;
  XII_TEST_BOOL(pair.m_Sender.ProcessEvent(std::move(event)).Succeeded());
  xiiNetworkRpcResult result;
  XII_TEST_BOOL(pair.m_Sender.TryGetRpcResult(result));
  XII_TEST_BOOL(result.m_Request == secondary && result.m_Error.m_Code == xiiNetworkDataErrorCode::Disconnected);
  XII_TEST_BOOL(!pair.m_Sender.TryGetRpcResult(result));
  if (!HostTestEvent(pair.m_Server, xiiNetworkEventType::ChannelClosed, event))
    return;
  XII_TEST_BOOL(pair.m_Receiver.ProcessEvent(std::move(event)).Succeeded());
  XII_TEST_BOOL(pair.m_Receiver.Dispatch(12).Succeeded());
  if (!pair.Receive(pair.m_Client, pair.m_Sender))
    return;
  XII_TEST_BOOL(pair.m_Sender.TryGetRpcResult(result));
  XII_TEST_BOOL(result.m_Request == primary && result.m_Error.Succeeded());
  XII_TEST_INT(service->m_Target.m_iCalls, 1);
}
