/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Messaging/Implementation/NetworkReflectionEndpointInternal.h>
#include <Foundation/Profiling/Profiling.h>
#include <cmath>

using namespace xiiNetworkMessagingInternal;
using namespace xiiNetworkReflectionInternal;

xiiNetworkReflectionEndpointState::xiiNetworkReflectionEndpointState(xiiNetworkHost& host, xiiNetworkTypeRegistry& registry) : m_Host(host), m_Registry(registry)
{
  xiiPlugin::Events().AddEventHandler(xiiMakeDelegate(&xiiNetworkReflectionEndpointState::OnPluginEvent, this));
}
xiiNetworkReflectionEndpointState::~xiiNetworkReflectionEndpointState()
{
  xiiPlugin::Events().RemoveEventHandler(xiiMakeDelegate(&xiiNetworkReflectionEndpointState::OnPluginEvent, this));
}
void xiiNetworkReflectionEndpointState::OnPluginEvent(const xiiPluginEvent& event)
{
  if (event.m_EventType != xiiPluginEvent::BeforePluginChanges && event.m_EventType != xiiPluginEvent::BeforeUnloading)
    return;
  // Order-independent barrier: no active callback may retain module code/RTTI past this point.
  m_Registry.Invalidate();
  decltype(m_Handlers) handlers;
  decltype(m_Services) services;
  {
    XII_LOCK(m_Mutex);
    while (!m_Pending.IsEmpty())
      Complete(m_Pending.GetCount() - 1, xiiNetworkDataError(xiiNetworkDataErrorCode::HandlerRemoved));
    m_Queue.Clear();
    m_Statistics.m_uiQueuedBytes = 0;
    m_uiSchemaCacheBytes         = 0;
    for (auto& peer : m_Peers)
      peer.m_Agreements.Clear();
    m_Handlers.Swap(handlers);
    m_Services.Swap(services);
  }
}
void xiiNetworkReflectionEndpointState::Complete(xiiUInt32 index, xiiNetworkDataError error, xiiArrayPtr<const xiiUInt8> value)
{
  const auto& pending = m_Pending[index];
  auto&       result  = m_Results.ExpandAndGetRef();
  result.m_Request    = pending.m_Request;
  result.m_uiService  = pending.m_uiService;
  result.m_uiMethod   = pending.m_uiMethod;
  result.m_Error      = error;
  result.m_Value      = value;
  result.m_Signature  = pending.m_Descriptor;
  m_Pending.RemoveAtAndCopy(index);
  ++m_Statistics.m_uiRpcCompleted;
}
void xiiNetworkReflectionEndpointState::Stop(xiiNetworkDataError error)
{
  decltype(m_Handlers) handlers;
  decltype(m_Services) services;
  {
    XII_LOCK(m_Mutex);
    m_bRunning = false;
    while (!m_Pending.IsEmpty())
      Complete(m_Pending.GetCount() - 1, error);
    m_Queue.Clear();
    m_Statistics.m_uiQueuedBytes = 0;
    m_Peers.Clear();
    m_uiSchemaCacheBytes = 0;
    m_Handlers.Swap(handlers);
    m_Services.Swap(services);
  }
  // Owned callbacks/targets may have application destructors. Never release them under state locks.
}
Peer* xiiNetworkReflectionEndpointState::FindPeer(xiiNetworkChannel channel)
{
  for (auto& peer : m_Peers)
    if (peer.m_pInfo->m_Connection == channel.m_Connection &&
        peer.m_pInfo->m_DefaultChannel.m_uiSessionGeneration == channel.m_uiSessionGeneration)
      return &peer;
  return nullptr;
}
xiiNetworkDataError xiiNetworkReflectionEndpointState::Send(xiiNetworkChannel channel, xiiArrayPtr<const xiiUInt8> bytes)
{
  xiiNetworkMessage message;
  return Transport(m_Host.Send(channel, bytes, message));
}
xiiNetworkDataError xiiNetworkReflectionEndpointState::Reliable(xiiNetworkChannel channel)
{
  xiiNetworkChannelConfiguration configuration;
  const auto                     error = m_Host.GetChannelConfiguration(channel, configuration);
  if (error.Failed())
    return Transport(error);
  if (configuration.m_Delivery != xiiNetworkDeliveryMode::ReliableOrdered && configuration.m_Delivery != xiiNetworkDeliveryMode::ReliableUnordered)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  return {};
}
void xiiNetworkReflectionEndpoint::EnableProtocol(xiiNetworkHostConfiguration& configuration)
{
  configuration.m_uiSupportedFeatures |= ProtocolFeature;
  configuration.m_uiRequiredFeatures |= ProtocolFeature;
}
xiiNetworkReflectionEndpoint::xiiNetworkReflectionEndpoint(xiiNetworkHost& host, xiiNetworkTypeRegistry& registry)
{
  m_pState = XII_DEFAULT_NEW(xiiNetworkReflectionEndpointState, host, registry);
}
xiiNetworkReflectionEndpoint::~xiiNetworkReflectionEndpoint()
{
  Shutdown();
  XII_DEFAULT_DELETE(m_pState);
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::Start(const xiiNetworkReflectionConfiguration& configuration)
{
  const auto& c = configuration;
  if (c.m_uiMaximumPeers == 0 || c.m_uiMaximumPeers > 65536 || c.m_uiMaximumHandlers == 0 || c.m_uiMaximumServices == 0 ||
      c.m_uiMaximumMethodsPerService == 0 || c.m_uiMaximumMethodsPerService > 1024 || c.m_uiMaximumQueuedPackets == 0 ||
      c.m_uiMaximumPendingCalls == 0 || c.m_uiMaximumSchemasPerPeer == 0 || c.m_uiMaximumSchemaBytes < 16 || c.m_uiMaximumSchemaBytes > 262144 ||
      c.m_uiReplayWindow == 0 || c.m_uiReplayWindow > 65536 || c.m_uiRequestBurst == 0 || c.m_uiRequestsPerSecond == 0 ||
      c.m_Serialization.m_uiMaximumPayloadBytes < 128 || c.m_uiMaximumQueuedBytes < c.m_Serialization.m_uiMaximumPayloadBytes ||
      !std::isfinite(c.m_MaximumCallTimeout.GetSeconds()) || c.m_uiMaximumSchemaCacheBytes < c.m_uiMaximumSchemaBytes || c.m_MaximumCallTimeout <= xiiTime::MakeZero() || c.m_MaximumCallTimeout > xiiTime::MakeFromHours(1))
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  XII_LOCK(m_pState->m_Mutex);
  if (m_pState->m_bRunning || !m_pState->m_Results.IsEmpty())
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  m_pState->m_Config     = c;
  m_pState->m_Statistics = {};
  m_pState->m_bRunning   = true;
  return {};
}
void                xiiNetworkReflectionEndpoint::Shutdown() { m_pState->Stop(xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected)); }
xiiNetworkDataError xiiNetworkReflectionEndpoint::RegisterMessageHandler(xiiSharedPtr<xiiNetworkMessageHandler> handler, xiiUInt32 executor)
{
  auto&     registry = xiiNetworkRegistryAccess::Get(m_pState->m_Registry);
  ReadScope scope(registry);
  if (!scope.m_bAcquired)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  if (handler == nullptr || handler->GetType() == nullptr)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  const auto*                  type = handler->GetType();
  xiiSharedPtr<CompiledSchema> schema;
  if (!registry.m_Schemas.TryGetValue(type, schema))
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownType);
  if (!type->GetAllocator()->CanAllocate() || type->GetTypeSize() > m_pState->m_Config.m_Serialization.m_uiMaximumObjectBytes ||
      type->GetTypeSize() > m_pState->m_Config.m_Serialization.m_uiMaximumDecodedBytes)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedNestedType);
  xiiSharedPtr<Handler> entry = XII_DEFAULT_NEW(Handler);
  entry->m_pOwner             = std::move(handler);
  entry->m_pType              = type;
  entry->m_uiExecutor         = executor;
  entry->m_uiGeneration       = registry.m_uiGeneration.load();
  XII_LOCK(m_pState->m_Mutex);
  if (!m_pState->m_bRunning)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
  if (m_pState->m_Handlers.Contains(schema->m_pInfo->m_uiTypeId))
    return xiiNetworkDataError(xiiNetworkDataErrorCode::IdentifierCollision);
  if (m_pState->m_Handlers.GetCount() >= m_pState->m_Config.m_uiMaximumHandlers)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure);
  entry->m_uiRegistration = m_pState->m_uiNextRegistration++;
  m_pState->m_Handlers.Insert(schema->m_pInfo->m_uiTypeId, entry);
  return {};
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::RemoveMessageHandler(xiiUInt64 type)
{
  ReadScope scope(xiiNetworkRegistryAccess::Get(m_pState->m_Registry));
  if (!scope.m_bAcquired)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  xiiSharedPtr<Handler> removed;
  {
    XII_LOCK(m_pState->m_Mutex);
    if (!m_pState->m_Handlers.Remove(type, &removed))
      return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownMessage);
  }
  return {};
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::DeclareRemoteService(xiiUInt64 service, const xiiRTTI* type)
{
  return m_pState->Register(service, type, {}, 0, {});
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::RegisterService(xiiUInt64 service, xiiSharedPtr<xiiNetworkRpcService> owner, xiiUInt32 executor, const xiiNetworkRpcServiceLimits& limits)
{
  if (owner == nullptr || owner->GetObject() == nullptr)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  const auto* type = owner->GetType();
  return m_pState->Register(service, type, std::move(owner), executor, limits);
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::RemoveService(xiiUInt64 service)
{
  ReadScope scope(xiiNetworkRegistryAccess::Get(m_pState->m_Registry));
  if (!scope.m_bAcquired)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  xiiSharedPtr<Service> removed;
  {
    XII_LOCK(m_pState->m_Mutex);
    if (!m_pState->m_Services.Remove(service, &removed))
      return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownRpc);
  }
  return {};
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::OfferSchema(xiiNetworkChannel channel, const xiiRTTI* type)
{
  xiiSharedPtr<const xiiNetworkSchema> schema;
  auto                                 error = m_pState->m_Registry.GetSchema(type, schema);
  if (error.Failed())
    return error;
  XII_LOCK(m_pState->m_Mutex);
  if (!m_pState->m_bRunning || m_pState->FindPeer(channel) == nullptr)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
  error = m_pState->Reliable(channel);
  if (error.Failed())
    return error;
  if (schema->m_Descriptor.GetCount() > m_pState->m_Config.m_uiMaximumSchemaBytes)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
  xiiDynamicArray<xiiUInt8> bytes;
  xiiNetworkBinaryWriter    writer(bytes, m_pState->m_Config.m_Serialization.m_uiMaximumPayloadBytes);
  Envelope(writer, Kind::Offer);
  writer.Bytes(schema->m_Descriptor);
  if (writer.GetError().Failed())
    return writer.GetError();
  return m_pState->Send(channel, bytes);
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::GetSchemaAgreement(xiiNetworkChannel channel, xiiUInt64 type, bool& compact) const
{
  compact = false;
  ReadScope scope(xiiNetworkRegistryAccess::Get(m_pState->m_Registry));
  if (!scope.m_bAcquired)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  xiiSharedPtr<const xiiNetworkSchema> schema;
  auto                                 error = m_pState->m_Registry.GetSchema(type, schema);
  if (error.Failed())
    return error;
  XII_LOCK(m_pState->m_Mutex);
  const auto peer = m_pState->FindPeer(channel);
  if (!m_pState->m_bRunning || peer == nullptr)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
  for (const auto& agreement : peer->m_Agreements)
    if (agreement.m_uiType == type && agreement.m_bOutgoing && agreement.m_pLocal->m_Descriptor == schema->m_Descriptor)
    {
      compact = agreement.m_bExact;
      return {};
    }
  return xiiNetworkDataError(xiiNetworkDataErrorCode::NotNegotiated);
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::SendMessage(xiiNetworkChannel channel, const xiiRTTI* type, const void* message, xiiNetworkEncoding encoding, xiiNetworkMessage* submission, xiiUInt64 userData)
{
  if (submission != nullptr)
    *submission = {};
  auto&     registry = xiiNetworkRegistryAccess::Get(m_pState->m_Registry);
  ReadScope scope(registry);
  if (!scope.m_bAcquired)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  xiiSharedPtr<CompiledSchema> schema;
  if (!registry.m_Schemas.TryGetValue(type, schema))
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownType);
  xiiNetworkSerializationContext context;
  {
    XII_LOCK(m_pState->m_Mutex);
    if (!m_pState->m_bRunning)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
    auto peer = m_pState->FindPeer(channel);
    if (peer == nullptr)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
    bool agreed = false;
    for (const auto& agreement : peer->m_Agreements)
      if (agreement.m_uiType == schema->m_pInfo->m_uiTypeId && agreement.m_bOutgoing && agreement.m_pLocal->m_Descriptor == schema->m_pInfo->m_Descriptor &&
          (encoding == xiiNetworkEncoding::Tagged || agreement.m_bExact))
        agreed = true;
    if (!agreed)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::NotNegotiated);
    context.m_Limits = m_pState->m_Config.m_Serialization;
    context.m_Limits.m_uiMaximumTemporaryBytes /= 2;
  }
  xiiArrayPtr<const xiiUInt8> encoded;
  auto                        error = xiiNetworkSerializer::Serialize(m_pState->m_Registry, type, message, context, encoded, encoding);
  if (error.Failed())
    return error;
  xiiDynamicArray<xiiUInt8> bytes;
  xiiNetworkBinaryWriter    writer(bytes, context.m_Limits.m_uiMaximumPayloadBytes);
  Envelope(writer, Kind::Message);
  writer.Bytes(encoded);
  if (writer.GetError().Failed())
    return writer.GetError();
  XII_LOCK(m_pState->m_Mutex);
  if (!m_pState->m_bRunning || m_pState->FindPeer(channel) == nullptr)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
  xiiNetworkMessage nativeSubmission;
  error = Transport(m_pState->m_Host.Send(channel, bytes, nativeSubmission, userData));
  if (error.Succeeded() && submission != nullptr)
    *submission = nativeSubmission;
  if (error.Succeeded())
    ++m_pState->m_Statistics.m_uiMessagesSent;
  return error;
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::ProcessEvent(xiiNetworkEvent&& event) { return m_pState->Process(std::move(event)); }
xiiNetworkDataError xiiNetworkReflectionEndpoint::Dispatch(xiiUInt32 executor, xiiUInt32 maximum) { return m_pState->Dispatch(executor, maximum); }
void                xiiNetworkReflectionEndpoint::Update()
{
  const auto now = xiiTime::Now();
  XII_LOCK(m_pState->m_Mutex);
  for (xiiUInt32 i = m_pState->m_Pending.GetCount(); i-- > 0;)
    if (m_pState->m_Pending[i].m_Deadline <= now)
    {
      m_pState->Complete(i, xiiNetworkDataError(xiiNetworkDataErrorCode::TimedOut));
      ++m_pState->m_Statistics.m_uiTimeouts;
    }
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::CallRpc(xiiNetworkChannel channel, xiiUInt64 service, xiiUInt64 method,
                                                          xiiArrayPtr<const xiiVariant> arguments, xiiNetworkRpcRequest& request, xiiTime timeout, bool fireAndForget)
{
  return m_pState->Call(channel, service, method, arguments, request, timeout, fireAndForget);
}
xiiNetworkDataError xiiNetworkReflectionEndpoint::CancelRpc(xiiNetworkRpcRequest request)
{
  XII_LOCK(m_pState->m_Mutex);
  for (xiiUInt32 i = 0; i < m_pState->m_Pending.GetCount(); ++i)
    if (m_pState->m_Pending[i].m_Request == request)
    {
      m_pState->Complete(i, xiiNetworkDataError(xiiNetworkDataErrorCode::Cancelled));
      ++m_pState->m_Statistics.m_uiCancellations;
      xiiDynamicArray<xiiUInt8> bytes;
      xiiNetworkBinaryWriter    writer(bytes, 32);
      Envelope(writer, Kind::Cancel);
      writer.U64(request.m_uiId);
      m_pState->Send(request.m_Channel, bytes).IgnoreResult();
      return {};
    }
  return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
}
bool xiiNetworkReflectionEndpoint::TryGetRpcResult(xiiNetworkRpcResult& result)
{
  XII_LOCK(m_pState->m_Mutex);
  if (m_pState->m_Results.IsEmpty())
    return false;
  result = std::move(m_pState->m_Results.PeekFront());
  m_pState->m_Results.PopFront();
  return true;
}
xiiNetworkDataError            xiiNetworkReflectionEndpoint::DecodeRpcResult(const xiiNetworkRpcResult& result, xiiVariant& value) { return m_pState->Decode(result, value); }
xiiNetworkReflectionStatistics xiiNetworkReflectionEndpoint::GetStatistics() const
{
  XII_LOCK(m_pState->m_Mutex);
  auto result              = m_pState->m_Statistics;
  result.m_uiQueuedPackets = m_pState->m_Queue.GetCount();
  result.m_uiPendingCalls  = m_pState->m_Pending.GetCount();
  return result;
}
