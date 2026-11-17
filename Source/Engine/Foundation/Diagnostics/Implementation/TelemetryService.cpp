/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Diagnostics/TelemetryDiscovery.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Network/Messaging/NetworkMessageHandler.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <atomic>
#include <cmath>

namespace
{
  constexpr xiiUInt32 MaximumSampleBytes = 16 * 1024 * 1024;
  constexpr xiiUInt32 MaximumPacketBytes = MaximumSampleBytes + 128 * 1024;

  xiiNetworkError Configure(const xiiTelemetryConfiguration& configuration, xiiNetworkHostConfiguration& out_network)
  {
    out_network          = configuration.m_Network;
    const auto transport = out_network.m_Transport;
    if ((transport != xiiNetworkTransportType::Local && transport != xiiNetworkTransportType::TlsTcp && transport != xiiNetworkTransportType::SecureDatagram) ||
        (transport != xiiNetworkTransportType::Local && out_network.m_Authentication != xiiNetworkAuthenticationPolicy::Certificate) ||
        out_network.m_bAllowInsecureDevelopment || configuration.m_uiMaximumQueuedSamples == 0 ||
        configuration.m_uiMaximumPublicationsPerUpdate == 0 || configuration.m_uiMaximumPublicationsPerUpdate > 4096 ||
        configuration.m_uiMaximumPublicationBytesPerUpdate < MaximumPacketBytes || configuration.m_uiMaximumPublicationBytesPerUpdate > 256 * 1024 * 1024 ||
        configuration.m_uiMaximumQueuedBytes < MaximumPacketBytes || configuration.m_uiMaximumClientBytes < MaximumPacketBytes ||
        configuration.m_uiMaximumQueuedBytes > 256 * 1024 * 1024 || configuration.m_uiMaximumClientBytes > 256 * 1024 * 1024 ||
        out_network.m_uiMaximumConnections > 64 || (configuration.m_uiPermissions & ~15U) != 0)
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);

    out_network.m_Protocol                  = xiiUuid(0x5849494449414732ULL, 0x54454C454D455452ULL);
    out_network.m_uiMinimumProtocolVersion  = 2;
    out_network.m_uiMaximumProtocolVersion  = 2;
    out_network.m_uiMaximumMessageBytes     = MaximumPacketBytes;
    out_network.m_uiMaximumPeerSendBytes    = configuration.m_uiMaximumClientBytes;
    out_network.m_uiMaximumPeerReceiveBytes = configuration.m_uiMaximumClientBytes;
    out_network.m_uiMaximumReceiveBytes     = xiiMath::Max<xiiUInt64>(configuration.m_uiMaximumClientBytes, 64 * 1024 * 1024);
    out_network.m_uiSendBurstBytes          = MaximumPacketBytes;
    out_network.m_uiPeerSendBurstBytes      = MaximumPacketBytes;
    if (transport == xiiNetworkTransportType::Local)
      out_network.m_Authentication = xiiNetworkAuthenticationPolicy::LocalUser;
    xiiNetworkReflectionEndpoint::EnableProtocol(out_network);
    return {};
  }

  xiiNetworkReflectionConfiguration ReflectionConfiguration(xiiUInt32 uiPeers)
  {
    xiiNetworkReflectionConfiguration config;
    config.m_uiMaximumPeers                          = uiPeers;
    config.m_uiMaximumPendingCalls                   = 32;
    config.m_uiMaximumHandlers                       = 4;
    config.m_uiMaximumSchemasPerPeer                 = 4;
    config.m_uiMaximumQueuedBytes                    = 64 * 1024 * 1024;
    config.m_Serialization.m_uiMaximumPayloadBytes   = MaximumPacketBytes;
    config.m_Serialization.m_uiMaximumDecodedBytes   = 64 * 1024 * 1024;
    config.m_Serialization.m_uiMaximumTemporaryBytes = 128 * 1024 * 1024;
    return config;
  }

  template <typename T>
  class Handler final : public xiiNetworkTypedMessageHandler<T>
  {
  public:
    xiiDelegate<void(const xiiNetworkDispatchContext&, const T&)> m_Callback;

    bool Authorize(const xiiNetworkDispatchContext& context) const override
    {
      return context.m_pSession != nullptr && context.m_Channel == context.m_pSession->m_DefaultChannel &&
        context.m_pSession->m_ConnectionInfo.m_Capabilities.m_bAuthenticated;
    }

    void OnMessage(const xiiNetworkDispatchContext& context, const T& message) override
    {
      m_Callback(context, message);
    }
  };

  template <typename T>
  bool Agreed(xiiNetworkReflectionEndpoint& reflection, xiiNetworkTypeRegistry& registry, xiiNetworkChannel channel)
  {
    bool                                 compact = false;
    xiiSharedPtr<const xiiNetworkSchema> schema;
    return registry.GetSchema(xiiGetStaticRTTI<T>(), schema).Succeeded() &&
      reflection.GetSchemaAgreement(channel, schema->m_uiTypeId, compact).Succeeded();
  }

  xiiResult Offer(xiiNetworkReflectionEndpoint& reflection, xiiNetworkChannel channel)
  {
    XII_SUCCEED_OR_RETURN(reflection.OfferSchema(channel, xiiGetStaticRTTI<xiiTelemetryHello>()).GetResult());
    XII_SUCCEED_OR_RETURN(reflection.OfferSchema(channel, xiiGetStaticRTTI<xiiTelemetrySubscription>()).GetResult());
    XII_SUCCEED_OR_RETURN(reflection.OfferSchema(channel, xiiGetStaticRTTI<xiiTelemetrySample>()).GetResult());
    return reflection.OfferSchema(channel, xiiGetStaticRTTI<xiiTelemetrySubscriptionResult>()).GetResult();
  }

  bool Ready(xiiNetworkReflectionEndpoint& reflection, xiiNetworkTypeRegistry& registry, xiiNetworkChannel channel)
  {
    return Agreed<xiiTelemetryHello>(reflection, registry, channel) &&
      Agreed<xiiTelemetrySubscription>(reflection, registry, channel) &&
      Agreed<xiiTelemetrySample>(reflection, registry, channel) &&
      Agreed<xiiTelemetrySubscriptionResult>(reflection, registry, channel);
  }

  xiiUInt64 FieldBytes(const xiiVariantDictionary& fields)
  {
    xiiUInt64 bytes = 0;
    for (auto it = fields.GetIterator(); it.IsValid(); ++it)
      bytes += it.Key().GetHeapMemoryUsage() + sizeof(xiiVariant) + 64 +
        (it.Value().IsA<xiiString>() ? it.Value().Get<xiiString>().GetHeapMemoryUsage() : 0);
    return bytes;
  }

  xiiUInt64 SampleBytes(const xiiTelemetrySample& sample)
  {
    xiiUInt64 bytes = sizeof(sample) + sample.m_Data.GetCount() + FieldBytes(sample.m_Fields);
    for (const auto& record : sample.m_Records)
      bytes += sizeof(record) + FieldBytes(record.m_Fields);
    return bytes;
  }

  bool ValidFields(const xiiVariantDictionary& fields)
  {
    if (fields.GetCount() > 256)
      return false;
    for (auto it = fields.GetIterator(); it.IsValid(); ++it)
    {
      const auto type = it.Value().GetType();
      if (it.Key().GetElementCount() > 512 || type == xiiVariantType::TypedObject || type == xiiVariantType::TypedPointer ||
          type == xiiVariantType::VariantArray || type == xiiVariantType::VariantDictionary ||
          type == xiiVariantType::DataBuffer || type == xiiVariantType::StringView ||
          (it.Value().IsA<xiiString>() && it.Value().Get<xiiString>().GetElementCount() > 65536))
        return false;
    }
    return true;
  }

  bool Valid(const xiiTelemetrySample& sample)
  {
    if (sample.m_uiSource == 0 || sample.m_uiEncodingVersion == 0 || sample.m_Data.GetCount() > MaximumSampleBytes ||
        sample.m_Records.GetCount() > 4096 || !std::isfinite(sample.m_fTimestampMicroseconds) || !ValidFields(sample.m_Fields))
      return false;
    for (const auto& record : sample.m_Records)
      if (!ValidFields(record.m_Fields))
        return false;
    return true;
  }
} // namespace

class xiiTelemetryServiceState
{
public:
  struct Queued : xiiRefCountingImpl
  {
    xiiTelemetrySample                        m_Sample;
    xiiHybridArray<xiiTelemetrySubscriber, 4> m_Recipients;
    xiiUInt64                                 m_uiBytes = 0;
    xiiTime                                   m_CollectedAt;
  };
  struct Source
  {
    xiiTelemetrySourceInfo       m_Info;
    xiiTelemetryService::Collect m_Collect;
    xiiTelemetryService::Demand  m_Demand;
    bool                         m_bActive = false;
    xiiUInt32                    m_uiSlot  = 0;
    xiiTime                      m_LastCollection;
  };
  struct Subscription
  {
    xiiUInt64                             m_uiSource          = 0;
    xiiUInt64                             m_uiGeneration      = 0;
    xiiUInt64                             m_uiSequence        = 0;
    xiiUInt64                             m_uiDropped         = 0;
    xiiUInt64                             m_uiHighestRequest  = 0;
    xiiUInt64                             m_uiProducerDropped = 0;
    xiiTime                               m_ActivatedAt, m_CollectUntil;
    xiiUInt32                             m_uiInterval = 33;
    xiiTime                               m_LastPublication;
    bool                                  m_bSnapshot = false;
    xiiDynamicArray<xiiSharedPtr<Queued>> m_Pending;
    xiiUInt64                             m_uiPendingBytes = 0;
  };
  struct Peer
  {
    xiiNetworkChannel             m_Channel;
    xiiUInt8                      m_uiPermissions     = 0;
    bool                          m_bReady            = false;
    bool                          m_bClosing          = false;
    xiiUInt64                     m_uiCatalogRevision = 0;
    xiiTime                       m_Deadline;
    xiiDynamicArray<Subscription> m_Subscriptions;
  };
  explicit xiiTelemetryServiceState(xiiTelemetryService& owner) : m_Owner(owner), m_Reflection(m_Host, m_Registry) {}

  Source* FindSource(xiiUInt64 id)
  {
    for (auto& source : m_Sources)
      if (source.m_Info.m_uiId == id)
        return &source;
    return nullptr;
  }

  void Subscribe(const xiiNetworkDispatchContext& context, const xiiTelemetrySubscription& request)
  {
    for (auto& peer : m_Peers)
    {
      if (peer.m_Channel != context.m_Channel || !peer.m_bReady)
        continue;
      xiiTelemetrySubscriptionResult response;
      response.m_uiRequestId  = request.m_uiRequestId;
      response.m_uiSource     = request.m_uiSource;
      response.m_uiGeneration = request.m_uiGeneration;
      auto fail               = [&](xiiNetworkDataErrorCode code) {
        response.m_uiError = static_cast<xiiUInt8>(code);
      };
      auto*         source  = FindSource(request.m_uiSource);
      Subscription* current = nullptr;
      for (auto& subscription : peer.m_Subscriptions)
        if (subscription.m_uiSource == request.m_uiSource)
          current = &subscription;
      if (source == nullptr)
        fail(xiiNetworkDataErrorCode::UnknownType);
      else if (request.m_uiRequestId == 0 || request.m_uiGeneration == 0 || request.m_uiIntervalMilliseconds > 60000 ||
               (request.m_bSnapshot && (!source->m_Info.m_bSnapshot || !request.m_bEnabled)))
        fail(xiiNetworkDataErrorCode::InvalidArgument);
      else if ((peer.m_uiPermissions & source->m_Info.m_uiPermission) != source->m_Info.m_uiPermission)
        fail(xiiNetworkDataErrorCode::Unauthorized);
      else if (current != nullptr && (request.m_uiRequestId <= current->m_uiHighestRequest || request.m_uiGeneration < current->m_uiGeneration))
        fail(xiiNetworkDataErrorCode::DuplicateRequest);
      else
      {
        if (current == nullptr)
        {
          current             = &peer.m_Subscriptions.ExpandAndGetRef();
          current->m_uiSource = request.m_uiSource;
        }
        if (request.m_bEnabled && current->m_uiInterval == 0)
        {
          current->m_ActivatedAt       = xiiTime::Now();
          current->m_uiProducerDropped = m_SourceSlots[source->m_uiSlot].m_uiDropped.load();
        }
        if (current->m_uiGeneration != request.m_uiGeneration)
        {
          current->m_ActivatedAt       = xiiTime::Now();
          current->m_uiProducerDropped = m_SourceSlots[source->m_uiSlot].m_uiDropped.load();
          current->m_uiSequence        = 0;
          current->m_uiDropped         = 0;
          current->m_Pending.Clear();
          current->m_uiPendingBytes = 0;
        }
        current->m_uiGeneration     = request.m_uiGeneration;
        current->m_uiHighestRequest = request.m_uiRequestId;
        current->m_uiInterval       = request.m_bEnabled ?
          xiiMath::Max(request.m_uiIntervalMilliseconds, source->m_Info.m_uiMinimumIntervalMilliseconds) :
          0;
        current->m_bSnapshot        = request.m_bSnapshot;
        current->m_CollectUntil     = source->m_Info.m_uiCollectionWindowMilliseconds != 0 && request.m_bEnabled ?
          xiiTime::Now() + xiiTime::MakeFromMilliseconds(source->m_Info.m_uiCollectionWindowMilliseconds) :
          xiiTime::MakeZero();
        if (!request.m_bEnabled)
        {
          current->m_Pending.Clear();
          current->m_uiPendingBytes = 0;
        }
        current->m_LastPublication        = {};
        response.m_uiIntervalMilliseconds = current->m_uiInterval;
      }
      if (m_Reflection.SendMessage(peer.m_Channel, response).Failed())
        m_Host.Disconnect(peer.m_Channel.m_Connection, false).IgnoreResult();
      return;
    }
  }

  class CommandService final : public xiiNetworkRpcService
  {
  public:
    explicit CommandService(xiiTelemetryServiceState& state) : m_State(state) {}
    const xiiRTTI* GetType() const override { return xiiGetStaticRTTI<xiiTelemetryCommands>(); }
    void*          GetObject() override { return &m_State.m_Commands; }
    bool           Authorize(const xiiNetworkDispatchContext& context) const override
    {
      if (!context.m_pSession || !context.m_pSession->m_ConnectionInfo.m_Capabilities.m_bAuthenticated ||
          context.m_uiService != xiiTelemetryCommands::ServiceId || context.m_uiMethod < 1 || context.m_uiMethod > 3)
        return false;
      const auto permission = context.m_uiMethod == 1 ? xiiTelemetryPermission::Configure : xiiTelemetryPermission::Console;
      for (const auto& peer : m_State.m_Peers)
        if (peer.m_Channel == context.m_Channel && peer.m_bReady)
          return (peer.m_uiPermissions & static_cast<xiiUInt8>(permission)) != 0;
      return false;
    }
    xiiTelemetryServiceState& m_State;
  };

  xiiResult RegisterHandler()
  {
    xiiSharedPtr<Handler<xiiTelemetrySubscription>> handler = XII_DEFAULT_NEW(Handler<xiiTelemetrySubscription>);
    handler->m_Callback                                     = xiiMakeDelegate(&xiiTelemetryServiceState::Subscribe, this);
    XII_SUCCEED_OR_RETURN(m_Reflection.RegisterMessageHandler(std::move(handler)).GetResult());
    xiiSharedPtr<CommandService> commands = XII_DEFAULT_NEW(CommandService, *this);
    xiiNetworkRpcServiceLimits   limits;
    limits.m_uiRequestsPerSecond         = 16;
    limits.m_uiBurst                     = 8;
    limits.m_uiMaximumConcurrentRequests = 8;
    limits.m_uiMaximumResponseBytes      = 128 * 1024;
    return m_Reflection.RegisterService(xiiTelemetryCommands::ServiceId, std::move(commands), 0, limits).GetResult();
  }

  void SendHello(Peer& peer)
  {
    xiiTelemetryHello hello;
    hello.m_uiCatalogRevision = m_uiCatalogRevision;
    hello.m_Instance          = m_Instance;
    hello.m_sApplication      = m_Config.m_sApplication;
    hello.m_uiPermissions     = peer.m_uiPermissions;
#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
    hello.m_uiProcessId = static_cast<xiiUInt32>(xiiProcess::GetCurrentProcessID());
#endif
    for (const auto& source : m_Sources)
      if ((peer.m_uiPermissions & source.m_Info.m_uiPermission) == source.m_Info.m_uiPermission)
        hello.m_Sources.PushBack(source.m_Info);
    if (m_Reflection.SendMessage(peer.m_Channel, hello).Failed())
      m_Host.Disconnect(peer.m_Channel.m_Connection, false).IgnoreResult();
    else
    {
      peer.m_bReady            = true;
      peer.m_uiCatalogRevision = m_uiCatalogRevision;
    }
  }

  struct SourceSlot
  {
    std::atomic<xiiUInt64> m_uiId{0};
    std::atomic<xiiUInt64> m_uiDropped{0};
  };
  SourceSlot                            m_SourceSlots[64];
  xiiUInt64                             m_uiCatalogRevision = 1;
  xiiUInt32                             m_uiNextPeer        = 0;
  xiiTelemetryDiscovery                 m_Discovery;
  xiiTelemetryCommands                  m_Commands;
  xiiTelemetryService&                  m_Owner;
  xiiNetworkHost                        m_Host;
  xiiNetworkTypeRegistry                m_Registry;
  xiiNetworkReflectionEndpoint          m_Reflection;
  xiiTelemetryConfiguration             m_Config;
  xiiTelemetryService::Authorize        m_Authorize;
  xiiDynamicArray<Source>               m_Sources;
  xiiDynamicArray<Peer>                 m_Peers;
  xiiDynamicArray<xiiSharedPtr<Queued>> m_Queue;
  xiiDynamicArray<xiiUInt64>            m_CollectingSources;
  xiiTelemetryStatistics                m_Statistics;
  xiiTelemetryService::Snapshot         m_Offline;
  xiiUInt64                             m_uiOfflineSource      = 0;
  xiiUInt64                             m_uiRegistryGeneration = 0;
  xiiUuid                               m_Instance;
  xiiString                             m_sLocalEndpointName;
  mutable xiiMutex                      m_QueueMutex;
  std::atomic<xiiUInt64>                m_uiDropped{0};
  bool                                  m_bUpdating  = false;
  bool                                  m_bAccepting = true;
};

xiiTelemetryService::xiiTelemetryService() : m_pState(XII_DEFAULT_NEW(xiiTelemetryServiceState, *this)) {}
xiiTelemetryService::~xiiTelemetryService()
{
  Shutdown();
  XII_DEFAULT_DELETE(m_pState);
}

xiiNetworkError xiiTelemetryService::Start(const xiiTelemetryConfiguration& configuration, Authorize authorize)
{
  auto& state = *m_pState;
  if (state.m_Host.IsRunning() || configuration.m_sApplication.IsEmpty() || configuration.m_sApplication.GetElementCount() > 256)
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  xiiNetworkHostConfiguration network;
  const auto                  configured = Configure(configuration, network);
  if (configured.Failed())
    return configured;
  state.m_Config = configuration;
  {
    XII_LOCK(state.m_QueueMutex);
    state.m_bAccepting = true;
  }
  state.m_Authorize = std::move(authorize);
  state.m_Instance  = xiiUuid::MakeUuid();
  if (state.m_Reflection.Start(ReflectionConfiguration(network.m_uiMaximumConnections)).Failed() || state.RegisterHandler().Failed())
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  state.m_uiRegistryGeneration = state.m_Registry.GetStatistics().m_uiGeneration;
  return state.m_Host.Start(network);
}

xiiNetworkError xiiTelemetryService::Listen(const xiiNetworkLocalEndpoint& endpoint)
{
  auto& state = *m_pState;
  if (!state.m_sLocalEndpointName.IsEmpty())
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyOpen);
  xiiNetworkListener listener;
  auto               error = state.m_Host.Listen(endpoint, listener);
  if (error.Failed())
    return error;
  xiiTelemetryInstance instance;
  instance.m_Instance     = state.m_Instance;
  instance.m_sApplication = state.m_Config.m_sApplication;
  instance.m_sEndpoint    = endpoint.GetName();
#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
  instance.m_uiProcessId = xiiProcess::GetCurrentProcessID();
#endif
  if (state.m_Discovery.Publish(instance).Failed())
  {
    state.m_Host.CloseListener(listener).IgnoreResult();
    return xiiNetworkError(xiiNetworkErrorCode::PermissionDenied);
  }
  state.m_sLocalEndpointName = endpoint.GetName();
  return {};
}

xiiNetworkError xiiTelemetryService::StartLocal(xiiStringView sApplication, xiiStringView sEndpointName, xiiUInt8 uiPermissions)
{
  xiiTelemetryConfiguration config;
  config.m_uiPermissions                  = uiPermissions;
  config.m_sApplication                   = sApplication;
  config.m_Network.m_Transport            = xiiNetworkTransportType::Local;
  config.m_Network.m_Authentication       = xiiNetworkAuthenticationPolicy::LocalUser;
  config.m_Network.m_uiMaximumConnections = 8;
  xiiStringBuilder name(sEndpointName);
  if (name.IsEmpty())
  {
    xiiUInt64 low, high;
    xiiUuid::MakeUuid().GetValues(low, high);
    name.SetFormat("xii-diag-{}", xiiArgU(low, 16, true, 16));
  }
  xiiNetworkLocalEndpoint endpoint;
  const auto              parsed = xiiNetworkLocalEndpoint::Make(name, endpoint);
  if (parsed.Failed())
    return parsed;
  const auto started = Start(config);
  if (started.Failed())
    return started;
  const auto listening = Listen(endpoint);
  if (listening.Succeeded())
    m_pState->m_sLocalEndpointName = name;
  else
    Shutdown();
  return listening;
}

xiiNetworkError xiiTelemetryService::StartFromCommandLine(xiiStringView sApplication)
{
  const auto*    command     = xiiCommandLineUtils::GetGlobalInstance();
  const auto     transport   = command->GetStringOption("-DiagnosticsTransport", 0, "local");
  const xiiUInt8 permissions = command->GetBoolOption("-DiagnosticsControl", false) ? 15 : 3;
  if (transport == "local")
    return StartLocal(sApplication, command->GetStringOption("-DiagnosticsEndpoint"), permissions);
  xiiTelemetryConfiguration config;
  config.m_sApplication  = sApplication;
  config.m_uiPermissions = permissions;
  if (transport == "tls")
    config.m_Network.m_Transport = xiiNetworkTransportType::TlsTcp;
  else if (transport == "quic")
    config.m_Network.m_Transport = xiiNetworkTransportType::SecureDatagram;
  else
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  xiiNetworkEndpoint endpoint;
  auto               error = xiiNetworkEndpoint::Parse(command->GetStringOption("-DiagnosticsListen"), endpoint);
  if (error.Failed())
    return error;
  xiiNetworkSecurityConfig security;
  security.m_sCertificateChainFile = command->GetStringOption("-DiagnosticsCert");
  security.m_sPrivateKeyFile       = command->GetStringOption("-DiagnosticsKey");
  security.m_sTrustAnchorsFile     = command->GetStringOption("-DiagnosticsCA");
  security.m_sApplicationProtocol  = "xii-diagnostics/2";
  security.m_bUseSystemTrust       = false;
  config.m_Network.m_pCredentials  = XII_DEFAULT_NEW(xiiNetworkCredentials);
  error                            = config.m_Network.m_pCredentials->Load(security);
  if (error.Failed())
    return error;
  error = Start(config);
  if (error.Failed())
    return error;
  error = Listen(endpoint);
  if (error.Failed())
    Shutdown();
  return error;
}

void xiiTelemetryService::SetConsoleHandler(xiiTelemetryCommands::Console console)
{
  m_pState->m_Commands.m_Console = std::move(console);
}

xiiStringView   xiiTelemetryService::GetLocalEndpointName() const { return m_pState->m_sLocalEndpointName; }
xiiNetworkError xiiTelemetryService::Listen(const xiiNetworkEndpoint& endpoint, xiiNetworkEndpoint* out_endpoint)
{
  xiiNetworkListener listener;
  const auto         error = m_pState->m_Host.Listen(endpoint, listener);
  if (error.Failed() || out_endpoint == nullptr)
    return error;
  return m_pState->m_Host.GetLocalEndpoint(listener, *out_endpoint);
}
void xiiTelemetryService::Shutdown()
{
  auto& state = *m_pState;
  {
    XII_LOCK(state.m_QueueMutex);
    state.m_bAccepting = false;
    state.m_CollectingSources.Clear();
  }
  state.m_Discovery.Reset();
  state.m_Reflection.Shutdown();
  state.m_Host.Shutdown();
  state.m_Peers.Clear();
  for (auto& source : state.m_Sources)
  {
    if (source.m_bActive && source.m_Demand.IsValid())
      source.m_Demand(false);
    source.m_bActive = false;
  }
  XII_LOCK(state.m_QueueMutex);
  state.m_Queue.Clear();
  state.m_Statistics.m_uiQueuedBytes = 0;
}

xiiResult xiiTelemetryService::RegisterSource(const xiiTelemetrySourceInfo& info, Collect collect, Demand demand)
{
  auto& state = *m_pState;
  if (state.m_Sources.GetCount() >= 64 || state.FindSource(info.m_uiId) != nullptr || info.m_uiId == 0 ||
      info.m_sName.IsEmpty() || info.m_sName.GetElementCount() > 256 || info.m_sCategory.GetElementCount() > 256 ||
      info.m_uiEncodingVersion == 0 || info.m_uiMinimumIntervalMilliseconds == 0 || info.m_uiMinimumIntervalMilliseconds > 60000 ||
      info.m_uiPermission == 0 || (info.m_uiPermission & ~15U) != 0 || info.m_uiCollectionWindowMilliseconds > 60000 || state.m_bUpdating)
    return XII_FAILURE;
  auto& source = state.m_Sources.ExpandAndGetRef();
  while (state.m_SourceSlots[source.m_uiSlot].m_uiId.load() != 0)
    ++source.m_uiSlot;
  state.m_SourceSlots[source.m_uiSlot].m_uiDropped.store(0);
  state.m_SourceSlots[source.m_uiSlot].m_uiId.store(info.m_uiId);
  source.m_Info    = info;
  source.m_Collect = std::move(collect);
  source.m_Demand  = std::move(demand);
  ++state.m_uiCatalogRevision;
  return XII_SUCCESS;
}
void xiiTelemetryService::UnregisterSource(xiiUInt64 id)
{
  auto& state = *m_pState;
  XII_ASSERT_DEV(!state.m_bUpdating, "Source mutation must occur outside collection and dispatch.");
  for (xiiUInt32 i = 0; i < state.m_Sources.GetCount(); ++i)
    if (state.m_Sources[i].m_Info.m_uiId == id)
    {
      if (state.m_Sources[i].m_bActive && state.m_Sources[i].m_Demand.IsValid())
        state.m_Sources[i].m_Demand(false);
      state.m_SourceSlots[state.m_Sources[i].m_uiSlot].m_uiId.store(0);
      state.m_Sources.RemoveAtAndCopy(i);
      ++state.m_uiCatalogRevision;
      break;
    }
  for (auto& peer : state.m_Peers)
    for (xiiUInt32 i = peer.m_Subscriptions.GetCount(); i-- > 0;)
      if (peer.m_Subscriptions[i].m_uiSource == id)
        peer.m_Subscriptions.RemoveAtAndCopy(i);
  XII_LOCK(state.m_QueueMutex);
  state.m_CollectingSources.RemoveAndCopy(id);
  for (xiiUInt32 i = state.m_Queue.GetCount(); i-- > 0;)
    if (state.m_Queue[i]->m_Sample.m_uiSource == id)
    {
      state.m_Statistics.m_uiQueuedBytes -= state.m_Queue[i]->m_uiBytes;
      state.m_Queue.RemoveAtAndCopy(i);
    }
}
bool xiiTelemetryService::IsCollecting(xiiUInt64 id) const
{
  const auto& state = *m_pState;
  if (state.m_QueueMutex.TryLock().Failed())
    return false;
  XII_SCOPE_EXIT(state.m_QueueMutex.Unlock());
  return state.m_bAccepting && state.m_CollectingSources.Contains(id);
}
void xiiTelemetryService::RecordDropped(xiiUInt64 source, xiiUInt64 count)
{
  m_pState->m_uiDropped.fetch_add(count);
  if (source == 0)
    return;
  for (auto& slot : m_pState->m_SourceSlots)
    if (slot.m_uiId.load() == source)
    {
      slot.m_uiDropped.fetch_add(count);
      return;
    }
}

xiiResult xiiTelemetryService::Publish(xiiTelemetrySample&& sample, xiiNetworkChannel recipient)
{
  if (!recipient.IsValid())
    return Publish(std::move(sample), xiiArrayPtr<const xiiTelemetrySubscriber>());
  const xiiTelemetrySubscriber subscriber{recipient, sample.m_uiGeneration};
  return Publish(std::move(sample), xiiMakeArrayPtr(&subscriber, 1));
}

xiiResult xiiTelemetryService::Publish(xiiTelemetrySample&& sample, xiiArrayPtr<const xiiTelemetrySubscriber> recipients)
{
  auto&      state = *m_pState;
  const auto bytes = SampleBytes(sample) + recipients.GetCount() * sizeof(xiiTelemetrySubscriber);
  if (recipients.GetCount() > 64 || !Valid(sample) || bytes > MaximumPacketBytes || state.m_QueueMutex.TryLock().Failed())
  {
    if (recipients.IsEmpty())
      RecordDropped(sample.m_uiSource);
    else
      ++state.m_uiDropped;
    return XII_FAILURE;
  }
  XII_SCOPE_EXIT(state.m_QueueMutex.Unlock());
  if (!state.m_bAccepting || !state.m_CollectingSources.Contains(sample.m_uiSource) ||
      state.m_Queue.GetCount() >= state.m_Config.m_uiMaximumQueuedSamples ||
      bytes > state.m_Config.m_uiMaximumQueuedBytes - state.m_Statistics.m_uiQueuedBytes)
  {
    if (recipients.IsEmpty())
      RecordDropped(sample.m_uiSource);
    else
      ++state.m_uiDropped;
    return XII_FAILURE;
  }
  xiiSharedPtr<xiiTelemetryServiceState::Queued> queued = XII_DEFAULT_NEW(xiiTelemetryServiceState::Queued);
  queued->m_uiBytes                                     = bytes;
  queued->m_CollectedAt                                 = xiiTime::Now();
  queued->m_Sample                                      = std::move(sample);
  queued->m_Recipients                                  = recipients;
  state.m_Queue.PushBack(std::move(queued));
  state.m_Statistics.m_uiQueuedBytes += bytes;
  ++state.m_Statistics.m_uiSamplesCollected;
  return XII_SUCCESS;
}

void xiiTelemetryService::GetSubscribers(xiiUInt64 id, xiiDynamicArray<xiiTelemetrySubscriber>& out_subscribers) const
{
  out_subscribers.Clear();
  for (const auto& peer : m_pState->m_Peers)
    for (const auto& subscription : peer.m_Subscriptions)
      if (!peer.m_bClosing && subscription.m_uiSource == id && subscription.m_uiInterval != 0 &&
          (subscription.m_CollectUntil == xiiTime::MakeZero() || xiiTime::Now() < subscription.m_CollectUntil))
        out_subscribers.PushBack({peer.m_Channel, subscription.m_uiGeneration});
}
xiiResult xiiTelemetryService::Capture(xiiUInt64 id, Snapshot snapshot)
{
  auto& state  = *m_pState;
  auto* source = state.FindSource(id);
  if (state.m_bUpdating || state.m_Offline.IsValid() || !snapshot.IsValid() || source == nullptr || !source->m_Collect.IsValid())
    return XII_FAILURE;
  state.m_Offline         = std::move(snapshot);
  state.m_uiOfflineSource = id;
  xiiUInt32 firstNew      = 0;
  bool      wasCollecting, wasAccepting;
  {
    XII_LOCK(state.m_QueueMutex);
    firstNew           = state.m_Queue.GetCount();
    wasCollecting      = state.m_CollectingSources.Contains(id);
    wasAccepting       = state.m_bAccepting;
    state.m_bAccepting = true;
    if (!wasCollecting)
      state.m_CollectingSources.PushBack(id);
  }
  XII_SCOPE_EXIT(
    state.m_Offline = {}; state.m_uiOfflineSource = 0;
    XII_LOCK(state.m_QueueMutex);
    state.m_bAccepting = wasAccepting;
    if (!wasCollecting) state.m_CollectingSources.RemoveAndCopy(id));
  source->m_Collect(*this, id);
  xiiDynamicArray<xiiSharedPtr<xiiTelemetryServiceState::Queued>> captured;
  {
    XII_LOCK(state.m_QueueMutex);
    for (xiiUInt32 i = firstNew; i < state.m_Queue.GetCount();)
    {
      if (state.m_Queue[i]->m_Sample.m_uiSource == id)
      {
        captured.PushBack(state.m_Queue[i]);
        if (wasCollecting)
          ++i;
        else
        {
          state.m_Statistics.m_uiQueuedBytes -= state.m_Queue[i]->m_uiBytes;
          state.m_Queue.RemoveAtAndCopy(i);
        }
      }
      else
        ++i;
    }
  }
  if (captured.IsEmpty())
    return XII_FAILURE;
  for (const auto& queued : captured)
    state.m_Offline(queued->m_Sample);
  return XII_SUCCESS;
}

void xiiTelemetryService::Update()
{
  auto& state = *m_pState;
  XII_ASSERT_DEV(!state.m_bUpdating, "Telemetry updates are serialized by the application.");
  state.m_bUpdating = true;
  XII_SCOPE_EXIT(state.m_bUpdating = false);
  const auto generation = state.m_Registry.GetStatistics().m_uiGeneration;
  if (generation != state.m_uiRegistryGeneration && state.RegisterHandler().Succeeded())
  {
    state.m_uiRegistryGeneration = generation;
    for (auto& peer : state.m_Peers)
    {
      peer.m_bReady   = false;
      peer.m_bClosing = true;
      peer.m_Subscriptions.Clear();
      state.m_Host.Disconnect(peer.m_Channel.m_Connection, false).IgnoreResult();
    }
  }

  xiiNetworkEvent event;
  for (xiiUInt32 i = 0; i < 256 && state.m_Host.TryGetEvent(event); ++i)
  {
    const auto type       = event.m_Type;
    const auto connection = event.m_Connection;
    if (type == xiiNetworkEventType::Connected)
    {
      if (event.m_pSessionInfo == nullptr)
        continue;
      auto& peer           = state.m_Peers.ExpandAndGetRef();
      peer.m_Channel       = event.m_pSessionInfo->m_DefaultChannel;
      peer.m_uiPermissions = state.m_Authorize.IsValid() ? state.m_Authorize(*event.m_pSessionInfo) & state.m_Config.m_uiPermissions : state.m_Config.m_uiPermissions;
      if (peer.m_uiPermissions == 0)
      {
        peer.m_bClosing = true;
        state.m_Host.Disconnect(connection, false).IgnoreResult();
        continue;
      }
      peer.m_Deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(10);
      if (state.m_Reflection.ProcessEvent(std::move(event)).Failed() || Offer(state.m_Reflection, peer.m_Channel).Failed())
        state.m_Host.Disconnect(connection, false).IgnoreResult();
    }
    else if (type == xiiNetworkEventType::Message)
    {
      if (state.m_Reflection.ProcessEvent(std::move(event)).Failed())
        state.m_Host.Disconnect(connection, false).IgnoreResult();
      else
        // Dispatch rejects unauthorized/invalid requests and queues their correlated RPC errors.
        // Closing here would cancel the response before transport completion.
        state.m_Reflection.Dispatch(0, 1).IgnoreResult();
    }
    else if (type == xiiNetworkEventType::Closed)
    {
      state.m_Reflection.ProcessEvent(std::move(event)).IgnoreResult();
      for (xiiUInt32 p = 0; p < state.m_Peers.GetCount(); ++p)
        if (state.m_Peers[p].m_Channel.m_Connection == connection)
        {
          state.m_Peers.RemoveAtAndCopy(p);
          break;
        }
    }
  }
  const auto now = xiiTime::Now();
  for (auto& peer : state.m_Peers)
  {
    if (peer.m_bClosing)
      continue;
    if (peer.m_bReady && peer.m_uiCatalogRevision != state.m_uiCatalogRevision)
      state.SendHello(peer);
    if (!peer.m_bReady && Ready(state.m_Reflection, state.m_Registry, peer.m_Channel))
      state.SendHello(peer);
    else if (!peer.m_bReady && now >= peer.m_Deadline)
      state.m_Host.Disconnect(peer.m_Channel.m_Connection, false).IgnoreResult();
  }

  const auto collectionStart = xiiTime::Now();
  {
    XII_LOCK(state.m_QueueMutex);
    state.m_CollectingSources.Clear();
    for (const auto& source : state.m_Sources)
      for (const auto& peer : state.m_Peers)
        for (const auto& subscription : peer.m_Subscriptions)
          if (!peer.m_bClosing && subscription.m_uiSource == source.m_Info.m_uiId && subscription.m_uiInterval != 0 &&
              (subscription.m_CollectUntil == xiiTime::MakeZero() || now < subscription.m_CollectUntil) &&
              !state.m_CollectingSources.Contains(source.m_Info.m_uiId))
            state.m_CollectingSources.PushBack(source.m_Info.m_uiId);
  }
  for (auto& source : state.m_Sources)
  {
    xiiUInt32 interval = 60001;
    for (const auto& peer : state.m_Peers)
      for (const auto& subscription : peer.m_Subscriptions)
        if (!peer.m_bClosing && subscription.m_uiSource == source.m_Info.m_uiId && subscription.m_uiInterval != 0 &&
            (subscription.m_CollectUntil == xiiTime::MakeZero() || now < subscription.m_CollectUntil))
          interval = xiiMath::Min(interval, subscription.m_uiInterval);
    const bool active = interval <= 60000;
    if (active != source.m_bActive)
    {
      source.m_bActive = active;
      if (source.m_Demand.IsValid())
        source.m_Demand(active);
    }
    if (interval <= 60000 && now - source.m_LastCollection >= xiiTime::MakeFromMilliseconds(interval) && source.m_Collect.IsValid())
    {
      source.m_LastCollection = now;
      source.m_Collect(*this, source.m_Info.m_uiId);
    }
  }
  state.m_Statistics.m_CollectionTime += xiiTime::Now() - collectionStart;
  xiiDynamicArray<xiiSharedPtr<xiiTelemetryServiceState::Queued>> outgoing;
  {
    XII_LOCK(state.m_QueueMutex);
    outgoing.Swap(state.m_Queue);
    state.m_Statistics.m_uiQueuedBytes = 0;
  }
  const auto publicationStart = xiiTime::Now();
  xiiUInt32  publications     = 0;
  xiiUInt64  publicationBytes = 0;
  for (xiiUInt32 p = 0; p < state.m_Peers.GetCount(); ++p)
  {
    auto& peer = state.m_Peers[(p + state.m_uiNextPeer) % state.m_Peers.GetCount()];
    if (!peer.m_bReady)
      continue;
    xiiUInt64 pendingBytes = 0;
    for (const auto& subscription : peer.m_Subscriptions)
      pendingBytes += subscription.m_uiPendingBytes;
    for (auto& subscription : peer.m_Subscriptions)
    {
      if (peer.m_bClosing)
        break;
      if (subscription.m_uiInterval == 0)
        continue;
      const auto* source = state.FindSource(subscription.m_uiSource);
      if (source == nullptr)
        continue;
      for (auto& queued : outgoing)
      {
        if (queued->m_CollectedAt < subscription.m_ActivatedAt ||
            (subscription.m_CollectUntil != xiiTime::MakeZero() && queued->m_CollectedAt >= subscription.m_CollectUntil) ||
            queued->m_Sample.m_uiSource != subscription.m_uiSource ||
            (queued->m_Sample.m_uiGeneration != 0 && queued->m_Sample.m_uiGeneration != subscription.m_uiGeneration))
          continue;
        bool recipient = queued->m_Recipients.IsEmpty();
        for (const auto& target : queued->m_Recipients)
          recipient |= target.m_Channel == peer.m_Channel && (target.m_uiGeneration == 0 || target.m_uiGeneration == subscription.m_uiGeneration);
        if (!recipient)
          continue;
        if (source->m_Info.m_bLatestOnly)
        {
          subscription.m_uiDropped += subscription.m_Pending.GetCount();
          state.m_Statistics.m_uiSamplesDropped += subscription.m_Pending.GetCount();
          pendingBytes -= subscription.m_uiPendingBytes;
          subscription.m_Pending.Clear();
          subscription.m_uiPendingBytes = 0;
        }
        if (subscription.m_Pending.GetCount() >= state.m_Config.m_uiMaximumQueuedSamples ||
            queued->m_uiBytes > state.m_Config.m_uiMaximumClientBytes - pendingBytes)
        {
          // Ordered streams cannot lose dictionary or lifecycle records and remain coherent.
          ++state.m_Statistics.m_uiBackpressureEvents;
          peer.m_bClosing = true;
          peer.m_bReady   = false;
          state.m_Host.Disconnect(peer.m_Channel.m_Connection, false).IgnoreResult();
          pendingBytes -= subscription.m_uiPendingBytes;
          subscription.m_Pending.Clear();
          subscription.m_uiPendingBytes = 0;
          subscription.m_uiInterval     = 0;
          break;
        }
        subscription.m_Pending.PushBack(queued);
        subscription.m_uiPendingBytes += queued->m_uiBytes;
        pendingBytes += queued->m_uiBytes;
      }
      if (subscription.m_uiInterval == 0 || now - subscription.m_LastPublication < xiiTime::MakeFromMilliseconds(subscription.m_uiInterval))
        continue;
      bool      published = false;
      xiiUInt32 uiSent    = 0;
      for (auto& queued : subscription.m_Pending)
      {
        if (publications >= state.m_Config.m_uiMaximumPublicationsPerUpdate || queued->m_uiBytes > state.m_Config.m_uiMaximumPublicationBytesPerUpdate - publicationBytes)
          break;
        auto&      sample             = queued->m_Sample;
        const auto originalGeneration = sample.m_uiGeneration;
        const auto originalSequence   = sample.m_uiSequence;
        sample.m_uiGeneration         = subscription.m_uiGeneration;
        sample.m_uiSequence           = subscription.m_uiSequence + 1;
        const auto dropped            = sample.m_uiDropped;
        const auto producerDropped    = state.m_SourceSlots[source->m_uiSlot].m_uiDropped.load();
        sample.m_uiDropped += producerDropped - subscription.m_uiProducerDropped;
        sample.m_uiDropped += subscription.m_uiDropped;
        const auto encodedBefore      = state.m_Registry.GetStatistics().m_uiBytesEncoded;
        const auto serializationStart = xiiTime::Now();
        ++publications;
        publicationBytes += queued->m_uiBytes;
        const auto error = state.m_Reflection.SendMessage(peer.m_Channel, sample);
        state.m_Statistics.m_SerializationTime += xiiTime::Now() - serializationStart;
        sample.m_uiDropped    = dropped;
        sample.m_uiGeneration = originalGeneration;
        sample.m_uiSequence   = originalSequence;
        if (error.Succeeded())
        {
          ++subscription.m_uiSequence;
          subscription.m_uiDropped         = 0;
          subscription.m_uiProducerDropped = producerDropped;
          state.m_Statistics.m_uiPublishedBytes += state.m_Registry.GetStatistics().m_uiBytesEncoded - encodedBefore;
          ++state.m_Statistics.m_uiSamplesPublished;
          if (sample.m_fTimestampMicroseconds > 0)
            state.m_Statistics.m_MaximumPublicationDelay = xiiMath::Max(state.m_Statistics.m_MaximumPublicationDelay,
                                                                        xiiTime::Now() - xiiTime::MakeFromMicroseconds(sample.m_fTimestampMicroseconds));
          subscription.m_uiPendingBytes -= queued->m_uiBytes;
          pendingBytes -= queued->m_uiBytes;
          ++uiSent;
          published = true;
        }
        else
        {
          ++state.m_Statistics.m_uiBackpressureEvents;
          break;
        }
      }
      subscription.m_Pending.RemoveAtAndCopy(0, uiSent);
      if (subscription.m_CollectUntil != xiiTime::MakeZero() && now >= subscription.m_CollectUntil && subscription.m_Pending.IsEmpty())
        subscription.m_uiInterval = 0;
      if (published)
      {
        subscription.m_LastPublication = now;
        if (subscription.m_bSnapshot)
          subscription.m_uiInterval = 0;
      }
    }
  }
  if (!state.m_Peers.IsEmpty())
    state.m_uiNextPeer = (state.m_uiNextPeer + 1) % state.m_Peers.GetCount();
  state.m_Statistics.m_PublicationTime += xiiTime::Now() - publicationStart;
  state.m_Reflection.Update();
}
xiiTelemetryStatistics xiiTelemetryService::GetStatistics() const
{
  auto& state = *m_pState;
  XII_LOCK(state.m_QueueMutex);
  auto result = state.m_Statistics;
  result.m_uiSamplesDropped += state.m_uiDropped.load();
  result.m_uiQueuedSamples = state.m_Queue.GetCount();
  result.m_uiClients       = state.m_Peers.GetCount();
  for (const auto& peer : state.m_Peers)
    for (const auto& subscription : peer.m_Subscriptions)
    {
      result.m_uiSubscriptions += subscription.m_uiInterval != 0 ? 1 : 0;
      result.m_uiQueuedSamples += subscription.m_Pending.GetCount();
      result.m_uiQueuedBytes += subscription.m_uiPendingBytes;
    }
  return result;
}
xiiNetworkHostStatistics xiiTelemetryService::GetNetworkStatistics() const { return m_pState->m_Host.GetStatistics(); }
xiiUuid                  xiiTelemetryService::GetInstanceId() const { return m_pState->m_Instance; }

class xiiTelemetryClientState
{
public:
  xiiTelemetryClientState() : m_Reflection(m_Host, m_Registry) {}

  void Hello(const xiiNetworkDispatchContext& context, const xiiTelemetryHello& hello)
  {
    if (context.m_Channel != m_Channel || !hello.m_Instance.IsValid() || hello.m_Sources.GetCount() > 64)
      return;
    if (hello.m_sApplication.GetElementCount() > 256 || (hello.m_uiPermissions & ~15U) != 0 ||
        (m_Hello.m_Instance.IsValid() && m_Hello.m_Instance != hello.m_Instance))
    {
      m_Host.Disconnect(m_Connection, false).IgnoreResult();
      return;
    }
    xiiHybridArray<xiiUInt64, 64> ids;
    for (const auto& source : hello.m_Sources)
    {
      if (source.m_uiId == 0 || ids.Contains(source.m_uiId) || source.m_sName.GetElementCount() > 256 || source.m_sCategory.GetElementCount() > 256 ||
          source.m_uiEncodingVersion == 0 || source.m_uiMinimumIntervalMilliseconds == 0 || source.m_uiMinimumIntervalMilliseconds > 60000 || source.m_uiCollectionWindowMilliseconds > 60000)
      {
        m_Host.Disconnect(m_Connection, false).IgnoreResult();
        return;
      }
      ids.PushBack(source.m_uiId);
    }
    m_Hello  = hello;
    m_bReady = true;
  }
  void Sample(const xiiNetworkDispatchContext& context, const xiiTelemetrySample& sample)
  {
    if (context.m_Channel != m_Channel || !m_bReady || !Valid(sample))
      return;
    bool supported = false;
    for (const auto& source : m_Hello.m_Sources)
      supported |= source.m_uiId == sample.m_uiSource && source.m_uiEncodingVersion == sample.m_uiEncodingVersion;
    if (!supported)
      return;
    for (auto& subscription : m_Subscriptions)
    {
      if (subscription.m_uiSource != sample.m_uiSource || subscription.m_uiGeneration != sample.m_uiGeneration ||
          sample.m_uiSequence <= subscription.m_uiSequence || !subscription.m_bEnabled)
        continue;
      subscription.m_uiSequence = sample.m_uiSequence;
      if (m_Sample.IsValid())
        m_Sample(sample);
      break;
    }
  }
  void Result(const xiiNetworkDispatchContext& context, const xiiTelemetrySubscriptionResult& result)
  {
    if (context.m_Channel != m_Channel)
      return;
    for (auto& subscription : m_Subscriptions)
      if (subscription.m_uiSource == result.m_uiSource && subscription.m_uiGeneration == result.m_uiGeneration &&
          subscription.m_uiHighestRequest == result.m_uiRequestId && result.m_uiError != 0)
        subscription.m_bEnabled = false;
    if (m_Result.IsValid())
      m_Result(result);
  }
  xiiNetworkError Start(const xiiTelemetryConfiguration& configuration)
  {
    xiiNetworkHostConfiguration network;
    const auto                  configured = Configure(configuration, network);
    if (configured.Failed())
      return configured;
    network.m_uiMaximumConnections = 1;
    if (m_Reflection.Start(ReflectionConfiguration(1)).Failed())
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    xiiSharedPtr<Handler<xiiTelemetryHello>> hello               = XII_DEFAULT_NEW(Handler<xiiTelemetryHello>);
    hello->m_Callback                                            = xiiMakeDelegate(&xiiTelemetryClientState::Hello, this);
    xiiSharedPtr<Handler<xiiTelemetrySample>> sample             = XII_DEFAULT_NEW(Handler<xiiTelemetrySample>);
    sample->m_Callback                                           = xiiMakeDelegate(&xiiTelemetryClientState::Sample, this);
    xiiSharedPtr<Handler<xiiTelemetrySubscriptionResult>> result = XII_DEFAULT_NEW(Handler<xiiTelemetrySubscriptionResult>);
    result->m_Callback                                           = xiiMakeDelegate(&xiiTelemetryClientState::Result, this);
    if (m_Reflection.RegisterMessageHandler(std::move(hello)).Failed() || m_Reflection.RegisterMessageHandler(std::move(sample)).Failed() ||
        m_Reflection.RegisterMessageHandler(std::move(result)).Failed())
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    if (m_Reflection.DeclareRemoteService(xiiTelemetryCommands::ServiceId, xiiGetStaticRTTI<xiiTelemetryCommands>()).Failed())
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    m_uiRegistryGeneration = m_Registry.GetStatistics().m_uiGeneration;
    m_Deadline             = xiiTime::Now() + xiiTime::MakeFromSeconds(10);
    return m_Host.Start(network);
  }
  struct Subscription
  {
    xiiUInt64 m_uiSource         = 0;
    xiiUInt64 m_uiGeneration     = 0;
    xiiUInt64 m_uiSequence       = 0;
    bool      m_bEnabled         = false;
    xiiUInt64 m_uiHighestRequest = 0;
  };

  xiiUInt64                               m_uiRegistryGeneration = 0;
  xiiNetworkHost                          m_Host;
  xiiNetworkTypeRegistry                  m_Registry;
  xiiNetworkReflectionEndpoint            m_Reflection;
  xiiNetworkConnection                    m_Connection;
  xiiNetworkChannel                       m_Channel;
  xiiTelemetryHello                       m_Hello;
  xiiDynamicArray<Subscription>           m_Subscriptions;
  xiiTelemetryClient::SampleHandler       m_Sample;
  xiiTelemetryClient::SubscriptionHandler m_Result;
  xiiNetworkError                         m_Error;
  xiiTime                                 m_Deadline;
  bool                                    m_bReady = false;
};

xiiTelemetryClient::xiiTelemetryClient() : m_pState(XII_DEFAULT_NEW(xiiTelemetryClientState)) {}
xiiTelemetryClient::~xiiTelemetryClient()
{
  Shutdown();
  XII_DEFAULT_DELETE(m_pState);
}
xiiNetworkError xiiTelemetryClient::Connect(const xiiTelemetryConfiguration& configuration, const xiiNetworkLocalEndpoint& endpoint)
{
  const auto started = PrepareConnection(configuration);
  if (started.Failed())
    return started;
  return m_pState->m_Host.Connect(endpoint, {}, m_pState->m_Connection);
}
xiiNetworkError xiiTelemetryClient::Connect(const xiiTelemetryConfiguration& configuration, const xiiNetworkEndpoint& endpoint, const xiiNetworkConnectOptions& options)
{
  const auto started = PrepareConnection(configuration);
  if (started.Failed())
    return started;
  return m_pState->m_Host.Connect(endpoint, options, m_pState->m_Connection);
}
xiiNetworkError xiiTelemetryClient::PrepareConnection(const xiiTelemetryConfiguration& configuration)
{
  Shutdown();
  auto sample = std::move(m_pState->m_Sample);
  auto result = std::move(m_pState->m_Result);
  // Network hosts and I/O contexts are single-use. A fresh host also gives reconnections an
  // independent handle namespace, so late completions cannot refer to the new session.
  XII_DEFAULT_DELETE(m_pState);
  m_pState           = XII_DEFAULT_NEW(xiiTelemetryClientState);
  m_pState->m_Sample = std::move(sample);
  m_pState->m_Result = std::move(result);
  return m_pState->Start(configuration);
}
void xiiTelemetryClient::Shutdown()
{
  auto& state = *m_pState;
  state.m_Reflection.Shutdown();
  state.m_Host.Shutdown();
  xiiNetworkEvent event;
  while (state.m_Host.TryGetEvent(event))
  {
  }
  state.m_Connection = {};
  state.m_Channel    = {};
  state.m_Hello      = {};
  state.m_Subscriptions.Clear();
  state.m_bReady = false;
  state.m_Error  = {};
}
void xiiTelemetryClient::Update()
{
  auto&      state      = *m_pState;
  const auto generation = state.m_Registry.GetStatistics().m_uiGeneration;
  if (generation != state.m_uiRegistryGeneration)
  {
    state.m_uiRegistryGeneration = generation;
    state.m_bReady               = false;
    state.m_Subscriptions.Clear();
    state.m_Host.Disconnect(state.m_Connection, false).IgnoreResult();
  }
  xiiNetworkEvent event;
  for (xiiUInt32 i = 0; i < 256 && state.m_Host.TryGetEvent(event); ++i)
  {
    if (event.m_Connection != state.m_Connection)
      continue;
    const auto type = event.m_Type;
    if (type == xiiNetworkEventType::Connected)
    {
      state.m_Channel = event.m_pSessionInfo->m_DefaultChannel;
      if (state.m_Reflection.ProcessEvent(std::move(event)).Failed() || Offer(state.m_Reflection, state.m_Channel).Failed())
        state.m_Host.Disconnect(state.m_Connection, false).IgnoreResult();
    }
    else if (type == xiiNetworkEventType::Message)
    {
      if (state.m_Reflection.ProcessEvent(std::move(event)).Failed() || state.m_Reflection.Dispatch(0, 1).Failed())
        state.m_Host.Disconnect(state.m_Connection, false).IgnoreResult();
    }
    else if (type == xiiNetworkEventType::Closed)
    {
      state.m_Error = event.m_Error;
      state.m_Reflection.ProcessEvent(std::move(event)).IgnoreResult();
      state.m_bReady     = false;
      state.m_Channel    = {};
      state.m_Connection = {};
      state.m_Subscriptions.Clear();
    }
  }
  if (state.m_Connection.IsValid() && !state.m_bReady && xiiTime::Now() >= state.m_Deadline)
  {
    state.m_Host.Disconnect(state.m_Connection, false).IgnoreResult();
    state.m_Error = xiiNetworkError(xiiNetworkErrorCode::TimedOut);
  }
  state.m_Reflection.Update();
}
bool                     xiiTelemetryClient::IsReady() const { return m_pState->m_bReady; }
const xiiTelemetryHello& xiiTelemetryClient::GetServerInfo() const { return m_pState->m_Hello; }
xiiNetworkError          xiiTelemetryClient::GetConnectionError() const { return m_pState->m_Error; }
bool                     xiiTelemetryClient::IsSourceAvailable(xiiUInt64 source) const
{
  if (!IsReady())
    return false;
  for (const auto& info : m_pState->m_Hello.m_Sources)
    if (info.m_uiId == source)
      return true;
  return false;
}
xiiNetworkError xiiTelemetryClient::GetConnectionInfo(xiiNetworkConnectionInfo& out_info) const
{
  return m_pState->m_Host.GetConnectionInfo(m_pState->m_Connection, out_info);
}
void                xiiTelemetryClient::SetSampleHandler(SampleHandler handler) { m_pState->m_Sample = std::move(handler); }
void                xiiTelemetryClient::SetSubscriptionHandler(SubscriptionHandler handler) { m_pState->m_Result = std::move(handler); }
xiiNetworkDataError xiiTelemetryClient::Subscribe(const xiiTelemetrySubscription& subscription)
{
  auto& state = *m_pState;
  if (!state.m_bReady)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::NotNegotiated);
  if (subscription.m_uiRequestId == 0 || subscription.m_uiGeneration == 0 || subscription.m_uiSource == 0 || subscription.m_uiIntervalMilliseconds > 60000)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  bool existing = false;
  for (const auto& current : state.m_Subscriptions)
  {
    if (current.m_uiSource != subscription.m_uiSource)
      continue;
    existing = true;
    if (subscription.m_uiRequestId <= current.m_uiHighestRequest || subscription.m_uiGeneration < current.m_uiGeneration)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::DuplicateRequest);
  }
  if (!existing && state.m_Subscriptions.GetCount() >= 64)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
  const auto result = state.m_Reflection.SendMessage(state.m_Channel, subscription);
  if (result.Failed())
    return result;
  for (auto& current : state.m_Subscriptions)
    if (current.m_uiSource == subscription.m_uiSource)
    {
      if (current.m_uiGeneration != subscription.m_uiGeneration)
        current.m_uiSequence = 0;
      current.m_uiGeneration     = subscription.m_uiGeneration;
      current.m_bEnabled         = subscription.m_bEnabled;
      current.m_uiHighestRequest = subscription.m_uiRequestId;
      return {};
    }
  state.m_Subscriptions.PushBack({subscription.m_uiSource, subscription.m_uiGeneration, 0, subscription.m_bEnabled, subscription.m_uiRequestId});
  return {};
}

xiiNetworkDataError xiiTelemetryClient::SetCVar(xiiStringView sName, const xiiVariant& value, xiiNetworkRpcRequest& out_request)
{
  if (!IsReady())
    return xiiNetworkDataError(xiiNetworkDataErrorCode::NotNegotiated);
  const xiiVariant arguments[] = {xiiString(sName), value};
  return m_pState->m_Reflection.CallRpc(m_pState->m_Channel, xiiTelemetryCommands::ServiceId, 1, xiiMakeArrayPtr(arguments), out_request, xiiTime::MakeFromSeconds(5));
}

xiiNetworkDataError xiiTelemetryClient::Console(xiiStringView sCommand, bool bComplete, xiiNetworkRpcRequest& out_request)
{
  if (!IsReady())
    return xiiNetworkDataError(xiiNetworkDataErrorCode::NotNegotiated);
  const xiiVariant arguments[] = {xiiString(sCommand)};
  return m_pState->m_Reflection.CallRpc(m_pState->m_Channel, xiiTelemetryCommands::ServiceId, bComplete ? 3 : 2, xiiMakeArrayPtr(arguments), out_request, xiiTime::MakeFromSeconds(5));
}

xiiNetworkDataError xiiTelemetryClient::CancelCommand(xiiNetworkRpcRequest request)
{
  return m_pState->m_Reflection.CancelRpc(request);
}

bool xiiTelemetryClient::TryGetCommandResult(xiiNetworkRpcResult& out_result, xiiTelemetryCommandResponse& out_response)
{
  if (!m_pState->m_Reflection.TryGetRpcResult(out_result))
    return false;
  out_response = {};
  if (out_result.m_Error.Succeeded())
    out_result.m_Error = m_pState->m_Reflection.DecodeRpcResult(out_result, out_response);
  return true;
}

XII_STATICLINK_FILE(Foundation, Foundation_Diagnostics_Implementation_TelemetryService);
