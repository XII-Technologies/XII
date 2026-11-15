/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/Messaging/NetworkReflectionEndpoint.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <FoundationTest/Network/Implementation/NetworkHostTestUtilities.h>
#include <FoundationTest/Network/Implementation/NetworkReflectionTestTypes.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>

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
  class ExampleHandler final : public xiiNetworkMessageHandler
  {
  public:
    const xiiRTTI* GetType() const override { return xiiGetStaticRTTI<NetworkExampleState>(); }
    bool           Authorize(const xiiNetworkDispatchContext& context) const override { return m_bAllow && context.m_pSession != nullptr; }
    void           Handle(const xiiNetworkDispatchContext& context, const void* message) override
    {
      const auto& state = *static_cast<const NetworkExampleState*>(message);
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
    xiiNetworkTypeRegistry          m_Registry;
    xiiNetworkHost                  m_Server, m_Client;
    xiiNetworkReflectionEndpoint    m_Receiver{m_Server, m_Registry}, m_Sender{m_Client, m_Registry};
    xiiNetworkChannel               m_ServerChannel, m_ClientChannel;
    NetworkSecurityTestCertificates m_Files;
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
      if (!XII_TEST_BOOL(m_Receiver.ProcessEvent(std::move(event)).Succeeded()))
        return false;
      if (!xiiNetworkHostTestUtilities::HostTestEvent(m_Client, xiiNetworkEventType::Connected, event))
        return false;
      m_ClientChannel = event.m_pSessionInfo->m_DefaultChannel;
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
