/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Network/Messaging/NetworkMessageHandler.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/ScopeExit.h>
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
        out_network.m_bAllowInsecureDevelopment || configuration.m_uiMaximumQueuedSamples == 0 ||
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

  xiiUInt64 SampleBytes(const xiiTelemetrySample& sample)
  {
    xiiUInt64 bytes = sizeof(sample) + sample.m_Data.GetCount();
    for (auto it = sample.m_Fields.GetIterator(); it.IsValid(); ++it)
      bytes += it.Key().GetHeapMemoryUsage() + sizeof(xiiVariant) + 64 +
        (it.Value().IsA<xiiString>() ? it.Value().Get<xiiString>().GetHeapMemoryUsage() : 0);
    return bytes;
  }

  bool Valid(const xiiTelemetrySample& sample)
  {
    if (sample.m_uiSource == 0 || sample.m_uiEncodingVersion == 0 ||
        sample.m_Data.GetCount() > MaximumSampleBytes || sample.m_Fields.GetCount() > 256 ||
        !std::isfinite(sample.m_fTimestampMicroseconds))
      return false;

    for (auto it = sample.m_Fields.GetIterator(); it.IsValid(); ++it)
    {
      // A diagnostic field is a value. Reflected object construction is excluded from this protocol.
      const auto type = it.Value().GetType();
      if (it.Key().GetElementCount() > 512 || type == xiiVariantType::TypedObject || type == xiiVariantType::TypedPointer ||
          type == xiiVariantType::VariantArray || type == xiiVariantType::VariantDictionary ||
          type == xiiVariantType::DataBuffer || type == xiiVariantType::StringView ||
          (it.Value().IsA<xiiString>() && it.Value().Get<xiiString>().GetElementCount() > 65536))
        return false;
    }
    return true;
  }
} // namespace

class xiiTelemetryServiceState
{
public:
  struct Queued : xiiRefCountingImpl
  {
    xiiTelemetrySample m_Sample;
    xiiNetworkChannel  m_Recipient;
    xiiUInt64          m_uiBytes = 0;
  };
  struct Source
  {
    xiiTelemetrySourceInfo       m_Info;
    xiiTelemetryService::Collect m_Collect;
    xiiTelemetryService::Demand  m_Demand;
    bool                         m_bActive = false;
    xiiTime                      m_LastCollection;
  };
  struct Subscription
  {
    xiiUInt64                             m_uiSource         = 0;
    xiiUInt64                             m_uiGeneration     = 0;
    xiiUInt64                             m_uiSequence       = 0;
    xiiUInt64                             m_uiDropped        = 0;
    xiiUInt64                             m_uiHighestRequest = 0;
    xiiUInt32                             m_uiInterval       = 33;
    xiiTime                               m_LastPublication;
    bool                                  m_bSnapshot = false;
    xiiDynamicArray<xiiSharedPtr<Queued>> m_Pending;
    xiiUInt64                             m_uiPendingBytes = 0;
  };
  struct Peer
  {
    xiiNetworkChannel             m_Channel;
    xiiUInt8                      m_uiPermissions = 0;
    bool                          m_bReady        = false;
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
        if (current->m_uiGeneration != request.m_uiGeneration)
        {
          current->m_uiSequence = 0;
          current->m_uiDropped  = 0;
          current->m_Pending.Clear();
          current->m_uiPendingBytes = 0;
        }
        current->m_uiGeneration     = request.m_uiGeneration;
        current->m_uiHighestRequest = request.m_uiRequestId;
        current->m_uiInterval       = request.m_bEnabled ?
          xiiMath::Max(request.m_uiIntervalMilliseconds, source->m_Info.m_uiMinimumIntervalMilliseconds) :
          0;
        current->m_bSnapshot        = request.m_bSnapshot;
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

  xiiResult RegisterHandler()
  {
    xiiSharedPtr<Handler<xiiTelemetrySubscription>> handler = XII_DEFAULT_NEW(Handler<xiiTelemetrySubscription>);
    handler->m_Callback                                     = xiiMakeDelegate(&xiiTelemetryServiceState::Subscribe, this);
    return m_Reflection.RegisterMessageHandler(std::move(handler)).GetResult();
  }

  void SendHello(Peer& peer)
  {
    xiiTelemetryHello hello;
    hello.m_Instance      = m_Instance;
    hello.m_sApplication  = m_Config.m_sApplication;
    hello.m_uiPermissions = peer.m_uiPermissions;
#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
    hello.m_uiProcessId = static_cast<xiiUInt32>(xiiProcess::GetCurrentProcessID());
#endif
    for (const auto& source : m_Sources)
      if ((peer.m_uiPermissions & source.m_Info.m_uiPermission) == source.m_Info.m_uiPermission)
        hello.m_Sources.PushBack(source.m_Info);
    if (m_Reflection.SendMessage(peer.m_Channel, hello).Failed())
      m_Host.Disconnect(peer.m_Channel.m_Connection, false).IgnoreResult();
    else
      peer.m_bReady = true;
  }

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
  if (state.m_Host.IsRunning())
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
  xiiNetworkListener listener;
  return m_pState->m_Host.Listen(endpoint, listener);
}

xiiNetworkError xiiTelemetryService::StartLocal(xiiStringView sApplication, xiiStringView sEndpointName)
{
  xiiTelemetryConfiguration config;
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

xiiStringView   xiiTelemetryService::GetLocalEndpointName() const { return m_pState->m_sLocalEndpointName; }
xiiNetworkError xiiTelemetryService::Listen(const xiiNetworkEndpoint& endpoint)
{
  xiiNetworkListener listener;
  return m_pState->m_Host.Listen(endpoint, listener);
}
void xiiTelemetryService::Shutdown()
{
  auto& state = *m_pState;
  {
    XII_LOCK(state.m_QueueMutex);
    state.m_bAccepting = false;
    state.m_CollectingSources.Clear();
  }
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
      info.m_uiPermission == 0 || (info.m_uiPermission & ~15U) != 0 || state.m_bUpdating)
    return XII_FAILURE;
  auto& source     = state.m_Sources.ExpandAndGetRef();
  source.m_Info    = info;
  source.m_Collect = std::move(collect);
  source.m_Demand  = std::move(demand);
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
      state.m_Sources.RemoveAtAndCopy(i);
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
xiiResult xiiTelemetryService::Publish(xiiTelemetrySample&& sample, xiiNetworkChannel recipient)
{
  auto&      state = *m_pState;
  const auto bytes = SampleBytes(sample);
  if (!Valid(sample) || bytes > MaximumPacketBytes || state.m_QueueMutex.TryLock().Failed())
  {
    ++state.m_uiDropped;
    return XII_FAILURE;
  }
  XII_SCOPE_EXIT(state.m_QueueMutex.Unlock());
  if (!state.m_bAccepting || !state.m_CollectingSources.Contains(sample.m_uiSource) ||
      state.m_Queue.GetCount() >= state.m_Config.m_uiMaximumQueuedSamples ||
      bytes > state.m_Config.m_uiMaximumQueuedBytes - state.m_Statistics.m_uiQueuedBytes)
  {
    ++state.m_uiDropped;
    return XII_FAILURE;
  }
  xiiSharedPtr<xiiTelemetryServiceState::Queued> queued = XII_DEFAULT_NEW(xiiTelemetryServiceState::Queued);
  queued->m_uiBytes                                     = bytes;
  queued->m_Sample                                      = std::move(sample);
  queued->m_Recipient                                   = recipient;
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
      if (subscription.m_uiSource == id && subscription.m_uiInterval != 0)
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
  {
    XII_LOCK(state.m_QueueMutex);
    state.m_CollectingSources.PushBack(id);
  }
  XII_SCOPE_EXIT(
    state.m_Offline = {}; state.m_uiOfflineSource = 0;
    XII_LOCK(state.m_QueueMutex);
    state.m_CollectingSources.RemoveAndCopy(id));
  source->m_Collect(*this, id);
  xiiDynamicArray<xiiSharedPtr<xiiTelemetryServiceState::Queued>> captured;
  {
    XII_LOCK(state.m_QueueMutex);
    for (xiiUInt32 i = 0; i < state.m_Queue.GetCount();)
    {
      if (state.m_Queue[i]->m_Sample.m_uiSource == id)
      {
        state.m_Statistics.m_uiQueuedBytes -= state.m_Queue[i]->m_uiBytes;
        captured.PushBack(std::move(state.m_Queue[i]));
        state.m_Queue.RemoveAtAndCopy(i);
      }
      else
        ++i;
    }
  }
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
      peer.m_bReady = false;
      peer.m_Subscriptions.Clear();
      if (Offer(state.m_Reflection, peer.m_Channel).Failed())
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
      peer.m_Deadline      = xiiTime::Now() + xiiTime::MakeFromSeconds(10);
      if (state.m_Reflection.ProcessEvent(std::move(event)).Failed() || Offer(state.m_Reflection, peer.m_Channel).Failed())
        state.m_Host.Disconnect(connection, false).IgnoreResult();
    }
    else if (type == xiiNetworkEventType::Message)
    {
      if (state.m_Reflection.ProcessEvent(std::move(event)).Failed() || state.m_Reflection.Dispatch(0, 1).Failed())
        state.m_Host.Disconnect(connection, false).IgnoreResult();
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
          if (subscription.m_uiSource == source.m_Info.m_uiId && subscription.m_uiInterval != 0 &&
              !state.m_CollectingSources.Contains(source.m_Info.m_uiId))
            state.m_CollectingSources.PushBack(source.m_Info.m_uiId);
  }
  for (auto& source : state.m_Sources)
  {
    xiiUInt32 interval = 60001;
    for (const auto& peer : state.m_Peers)
      for (const auto& subscription : peer.m_Subscriptions)
        if (subscription.m_uiSource == source.m_Info.m_uiId && subscription.m_uiInterval != 0)
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
  for (auto& peer : state.m_Peers)
  {
    if (!peer.m_bReady)
      continue;
    xiiUInt64 pendingBytes = 0;
    for (const auto& subscription : peer.m_Subscriptions)
      pendingBytes += subscription.m_uiPendingBytes;
    for (auto& subscription : peer.m_Subscriptions)
    {
      if (subscription.m_uiInterval == 0)
        continue;
      const auto* source = state.FindSource(subscription.m_uiSource);
      if (source == nullptr)
        continue;
      for (auto& queued : outgoing)
      {
        if (queued->m_Sample.m_uiSource != subscription.m_uiSource ||
            (queued->m_Sample.m_uiGeneration != 0 && queued->m_Sample.m_uiGeneration != subscription.m_uiGeneration) ||
            (queued->m_Recipient.IsValid() && queued->m_Recipient != peer.m_Channel))
          continue;
        if (source->m_Info.m_bLatestOnly)
        {
          subscription.m_uiDropped += subscription.m_Pending.GetCount();
          pendingBytes -= subscription.m_uiPendingBytes;
          subscription.m_Pending.Clear();
          subscription.m_uiPendingBytes = 0;
        }
        if (subscription.m_Pending.GetCount() >= state.m_Config.m_uiMaximumQueuedSamples ||
            queued->m_uiBytes > state.m_Config.m_uiMaximumClientBytes - pendingBytes)
        {
          // Ordered streams cannot lose dictionary or lifecycle records and remain coherent.
          ++state.m_Statistics.m_uiBackpressureEvents;
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
        auto&      sample             = queued->m_Sample;
        const auto originalGeneration = sample.m_uiGeneration;
        const auto originalSequence   = sample.m_uiSequence;
        sample.m_uiGeneration         = subscription.m_uiGeneration;
        sample.m_uiSequence           = subscription.m_uiSequence + 1;
        const auto dropped            = sample.m_uiDropped;
        sample.m_uiDropped += subscription.m_uiDropped;
        const auto error      = state.m_Reflection.SendMessage(peer.m_Channel, sample);
        sample.m_uiDropped    = dropped;
        sample.m_uiGeneration = originalGeneration;
        sample.m_uiSequence   = originalSequence;
        if (error.Succeeded())
        {
          ++subscription.m_uiSequence;
          subscription.m_uiDropped = 0;
          state.m_Statistics.m_uiPublishedBytes += queued->m_uiBytes;
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
      if (published)
      {
        subscription.m_LastPublication = now;
        if (subscription.m_bSnapshot)
          subscription.m_uiInterval = 0;
      }
    }
  }
  state.m_Statistics.m_PublicationTime += xiiTime::Now() - publicationStart;
  state.m_Reflection.Update();
}
xiiTelemetryStatistics xiiTelemetryService::GetStatistics() const
{
  auto& state = *m_pState;
  XII_LOCK(state.m_QueueMutex);
  auto result               = state.m_Statistics;
  result.m_uiSamplesDropped = state.m_uiDropped.load();
  result.m_uiQueuedSamples  = state.m_Queue.GetCount();
  result.m_uiClients        = state.m_Peers.GetCount();
  for (const auto& peer : state.m_Peers)
    for (const auto& subscription : peer.m_Subscriptions)
      result.m_uiSubscriptions += subscription.m_uiInterval != 0 ? 1 : 0;
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
    m_Hello  = hello;
    m_bReady = true;
  }
  void Sample(const xiiNetworkDispatchContext& context, const xiiTelemetrySample& sample)
  {
    if (context.m_Channel != m_Channel || !m_bReady || !Valid(sample))
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
    if (context.m_Channel == m_Channel && m_Result.IsValid())
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
    m_Deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(10);
    return m_Host.Start(network);
  }
  struct Subscription
  {
    xiiUInt64 m_uiSource     = 0;
    xiiUInt64 m_uiGeneration = 0;
    xiiUInt64 m_uiSequence   = 0;
    bool      m_bEnabled     = false;
  };

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
  Shutdown();
  const auto started = m_pState->Start(configuration);
  if (started.Failed())
    return started;
  return m_pState->m_Host.Connect(endpoint, {}, m_pState->m_Connection);
}
xiiNetworkError xiiTelemetryClient::Connect(const xiiTelemetryConfiguration& configuration, const xiiNetworkEndpoint& endpoint, const xiiNetworkConnectOptions& options)
{
  Shutdown();
  const auto started = m_pState->Start(configuration);
  if (started.Failed())
    return started;
  return m_pState->m_Host.Connect(endpoint, options, m_pState->m_Connection);
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
  auto&           state = *m_pState;
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
void                     xiiTelemetryClient::SetSampleHandler(SampleHandler handler) { m_pState->m_Sample = std::move(handler); }
void                     xiiTelemetryClient::SetSubscriptionHandler(SubscriptionHandler handler) { m_pState->m_Result = std::move(handler); }
xiiNetworkDataError      xiiTelemetryClient::Subscribe(const xiiTelemetrySubscription& subscription)
{
  auto& state = *m_pState;
  if (!state.m_bReady)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::NotNegotiated);
  bool existing = false;
  for (const auto& current : state.m_Subscriptions)
    existing |= current.m_uiSource == subscription.m_uiSource;
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
      current.m_uiGeneration = subscription.m_uiGeneration;
      current.m_bEnabled     = subscription.m_bEnabled;
      return {};
    }
  state.m_Subscriptions.PushBack({subscription.m_uiSource, subscription.m_uiGeneration, 0, subscription.m_bEnabled});
  return {};
}

XII_STATICLINK_FILE(Foundation, Foundation_Diagnostics_Implementation_TelemetryService);
