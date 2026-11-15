/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Messaging/Implementation/NetworkReflectionEndpointInternal.h>
#include <Foundation/Profiling/Profiling.h>

using namespace xiiNetworkMessagingInternal;
using namespace xiiNetworkReflectionInternal;

namespace
{
  bool Remember(Peer& peer, xiiUInt64 id)
  {
    if (id == 0 || (peer.m_uiHighestRequest >= peer.m_Replay.GetCount() && id <= peer.m_uiHighestRequest - peer.m_Replay.GetCount()))
      return false;
    auto& slot = peer.m_Replay[static_cast<xiiUInt32>(id % peer.m_Replay.GetCount())];
    if (slot == id)
      return false;
    slot                    = id;
    peer.m_uiHighestRequest = xiiMath::Max(peer.m_uiHighestRequest, id);
    return true;
  }
} // namespace
xiiNetworkDataError xiiNetworkReflectionEndpointState::Process(xiiNetworkEvent&& event)
{
  XII_PROFILE_SCOPE("NetworkReflection.Process");
  auto&     registry = xiiNetworkRegistryAccess::Get(m_Registry);
  ReadScope scope(registry);
  if (!scope.m_bAcquired)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  XII_LOCK(m_Mutex);
  if (!m_bRunning)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
  if (event.m_Type == xiiNetworkEventType::Closed)
  {
    for (xiiUInt32 i = m_Pending.GetCount(); i-- > 0;)
      if (m_Pending[i].m_Request.m_Channel.m_Connection == event.m_Connection)
        Complete(i, xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected));
    for (xiiUInt32 i = m_Peers.GetCount(); i-- > 0;)
      if (m_Peers[i]->m_pInfo->m_Connection == event.m_Connection)
      {
        for (const auto& agreement : m_Peers[i]->m_Agreements)
          m_uiSchemaCacheBytes -= agreement.m_Remote.GetCount();
        m_Peers[i]->m_bAlive = false;
        m_Peers.RemoveAtAndCopy(i);
      }
    for (xiiUInt32 i = m_Queue.GetCount(); i-- > 0;)
      if (m_Queue[i].m_Event.m_Connection == event.m_Connection)
      {
        ReleaseRequest(m_Queue[i]);
        m_Statistics.m_uiQueuedBytes -= m_Queue[i].m_Event.m_Payload.GetCapacity();
        m_Queue.RemoveAtAndCopy(i);
      }
    return {};
  }
  if (event.m_Type == xiiNetworkEventType::Connected)
  {
    if (event.m_pSessionInfo == nullptr || (event.m_pSessionInfo->m_ConnectionInfo.m_uiNegotiatedFeatures & xiiNetworkReflectionEndpoint::ProtocolFeature) == 0)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::NotNegotiated);
    if (FindPeer(event.m_pSessionInfo->m_DefaultChannel) != nullptr)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
    if (m_Peers.GetCount() >= m_Config.m_uiMaximumPeers)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure);
    xiiSharedPtr<Peer> peer = XII_DEFAULT_NEW(Peer);
    peer->m_pInfo           = event.m_pSessionInfo;
    peer->m_Replay.SetCount(m_Config.m_uiReplayWindow, 0);
    peer->m_fTokens    = m_Config.m_uiRequestBurst;
    peer->m_LastRefill = xiiTime::Now();
    m_Peers.PushBack(std::move(peer));
    return {};
  }
  if (event.m_Type != xiiNetworkEventType::Message)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownMessage);
  auto peer = FindPeer(event.m_Channel);
  if (peer == nullptr || event.m_pSessionInfo != peer->m_pInfo)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
  const auto bytes = event.m_Payload.GetBytes();
  if (bytes.GetCount() > m_Config.m_Serialization.m_uiMaximumPayloadBytes)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
  xiiNetworkBinaryReader reader(bytes);
  if (reader.U32() != Magic)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
  const auto kind = static_cast<Kind>(reader.U8());
  if (reader.GetError().Failed())
    return reader.GetError();
  if (kind == Kind::Offer || kind == Kind::Agreement)
  {
    auto reliable = Reliable(event.m_Channel);
    if (reliable.Failed())
      return reliable;
    const auto descriptor = reader.Bytes(reader.GetRemaining());
    if (descriptor.GetCount() > m_Config.m_uiMaximumSchemaBytes)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
    xiiNetworkBinaryReader       info(descriptor);
    const auto                   id   = info.U64();
    const xiiRTTI*               type = nullptr;
    xiiSharedPtr<CompiledSchema> schema;
    if (!registry.m_Ids.TryGetValue(id, type) || !registry.m_Schemas.TryGetValue(type, schema))
      return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownType, id);
    const auto incoming = Compatible(descriptor, schema->m_pInfo->m_Descriptor);
    const auto outgoing = Compatible(schema->m_pInfo->m_Descriptor, descriptor);
    auto       error    = kind == Kind::Offer ? incoming : outgoing;
    if (error.Failed())
      return error;
    const bool exact     = descriptor == schema->m_pInfo->m_Descriptor.GetArrayPtr();
    Agreement* agreement = nullptr;
    for (auto& entry : peer->m_Agreements)
      if (entry.m_uiType == id)
        agreement = &entry;
    if (agreement == nullptr)
    {
      if (peer->m_Agreements.GetCount() >= m_Config.m_uiMaximumSchemasPerPeer)
        return xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure);
      agreement = &peer->m_Agreements.ExpandAndGetRef();
    }
    // Changes always reset previous agreement. Peer descriptors are bounded and copied only here.
    if (agreement->m_pLocal == nullptr || agreement->m_pLocal->m_Descriptor != schema->m_pInfo->m_Descriptor || agreement->m_Remote.GetArrayPtr() != descriptor)
    {
      const auto oldSize = agreement->m_Remote.GetCount();
      if (descriptor.GetCount() > m_Config.m_uiMaximumSchemaCacheBytes - (m_uiSchemaCacheBytes - oldSize))
        return xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure);
      m_uiSchemaCacheBytes = m_uiSchemaCacheBytes - oldSize + descriptor.GetCount();
      *agreement           = {};
      agreement->m_uiType  = id;
      agreement->m_pLocal  = schema->m_pInfo;
      agreement->m_Remote  = descriptor;
    }
    agreement->m_bIncoming = incoming.Succeeded();
    agreement->m_bExact    = exact;
    if (kind == Kind::Agreement)
    {
      agreement->m_bOutgoing = true;
      return {};
    }
    xiiDynamicArray<xiiUInt8> response;
    xiiNetworkBinaryWriter    writer(response, m_Config.m_Serialization.m_uiMaximumPayloadBytes);
    Envelope(writer, Kind::Agreement);
    writer.Bytes(schema->m_pInfo->m_Descriptor);
    if (writer.GetError().Failed())
      return writer.GetError();
    return Send(event.m_Channel, response);
  }
  if (kind == Kind::Response)
  {
    const auto service = reader.U64(), method = reader.U64(), request = reader.U64();
    const auto code = reader.U8();
    if (reader.GetError().Failed())
      return reader.GetError();
    if (code > static_cast<xiiUInt8>(xiiNetworkDataErrorCode::DuplicateRequest))
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
    const auto value = reader.Bytes(reader.GetRemaining());
    if (code != 0 && !value.IsEmpty())
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
    for (xiiUInt32 i = 0; i < m_Pending.GetCount(); ++i)
    {
      const auto& call = m_Pending[i];
      if (call.m_Request.m_Channel == event.m_Channel && call.m_Request.m_uiId == request && call.m_uiService == service && call.m_uiMethod == method)
      {
        if (call.m_Deadline <= xiiTime::Now())
        {
          Complete(i, xiiNetworkDataError(xiiNetworkDataErrorCode::TimedOut));
          ++m_Statistics.m_uiTimeouts;
        }
        else
          Complete(i, xiiNetworkDataError(static_cast<xiiNetworkDataErrorCode>(code)), value);
        return {};
      }
    }
    return xiiNetworkDataError(xiiNetworkDataErrorCode::DuplicateRequest);
  }
  if (kind == Kind::Cancel)
  {
    const auto id = reader.U64();
    if (!reader.IsFinished() || id == 0)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
    for (xiiUInt32 i = 0; i < m_Queue.GetCount(); ++i)
      if (m_Queue[i].m_Kind == Kind::Request && m_Queue[i].m_uiRequest == id && m_Queue[i].m_Event.m_Channel == event.m_Channel)
      {
        Reply(m_Queue[i], xiiNetworkDataError(xiiNetworkDataErrorCode::Cancelled)).IgnoreResult();
        ReleaseRequest(m_Queue[i]);
        m_Statistics.m_uiQueuedBytes -= m_Queue[i].m_Event.m_Payload.GetCapacity();
        m_Queue.RemoveAtAndCopy(i);
        return {};
      }
    // With unordered reliable delivery, cancellation may precede its request.
    XII_IGNORE_UNUSED(Remember(*peer, id));
    return {};
  }
  Packet packet;
  packet.m_Kind   = kind;
  packet.m_Queued = xiiTime::Now();
  if (kind == Kind::Message)
  {
    xiiNetworkSerializedHeader header;
    const auto                 error = xiiNetworkSerializer::ReadHeader(reader.Bytes(reader.GetRemaining()), header);
    if (error.Failed())
      return error;
    xiiSharedPtr<Handler> handler;
    if (!m_Handlers.TryGetValue(header.m_uiTypeId, handler))
      return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownMessage, header.m_uiTypeId);
    bool agreed = false;
    for (const auto& agreement : peer->m_Agreements)
      if (agreement.m_uiType == header.m_uiTypeId && agreement.m_bIncoming &&
          (header.m_Encoding == xiiNetworkEncoding::Tagged || (agreement.m_bExact && agreement.m_pLocal->m_uiFingerprint == header.m_uiFingerprint)))
        agreed = true;
    if (!agreed)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::NotNegotiated);
    packet.m_uiService      = header.m_uiTypeId;
    packet.m_uiExecutor     = handler->m_uiExecutor;
    packet.m_uiRegistration = handler->m_uiRegistration;
  }
  else if (kind == Kind::Request)
  {
    auto reliable = Reliable(event.m_Channel);
    if (reliable.Failed())
      return reliable;
    packet.m_uiService       = reader.U64();
    packet.m_uiMethod        = reader.U64();
    packet.m_uiRequest       = reader.U64();
    const auto flags         = reader.U8();
    const auto timeout       = reader.U32();
    packet.m_bFireAndForget  = flags == 1;
    packet.m_Deadline        = packet.m_Queued + xiiTime::MakeFromMilliseconds(timeout);
    const auto signatureSize = reader.U32();
    XII_IGNORE_UNUSED(reader.Bytes(signatureSize));
    if (reader.GetError().Failed())
      return reader.GetError();
    if (flags > 1 || timeout == 0 || timeout > m_Config.m_MaximumCallTimeout.GetMilliseconds() || signatureSize > m_Config.m_uiMaximumSchemaBytes)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
    packet.m_Event = std::move(event);
    if (!Remember(*peer, packet.m_uiRequest))
      return xiiNetworkDataError(xiiNetworkDataErrorCode::DuplicateRequest);
    const auto now     = packet.m_Queued;
    peer->m_fTokens    = xiiMath::Min(static_cast<double>(m_Config.m_uiRequestBurst), peer->m_fTokens + (now - peer->m_LastRefill).GetSeconds() * m_Config.m_uiRequestsPerSecond);
    peer->m_LastRefill = now;
    if (peer->m_fTokens < 1)
    {
      Reply(packet, xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure)).IgnoreResult();
      return xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure);
    }
    peer->m_fTokens -= 1;
    xiiSharedPtr<Service> service;
    if (!m_Services.TryGetValue(packet.m_uiService, service) || service->m_pOwner == nullptr)
    {
      Reply(packet, xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownRpc)).IgnoreResult();
      return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownRpc);
    }
    service->m_fTokens    = xiiMath::Min(static_cast<double>(service->m_Limits.m_uiBurst), service->m_fTokens + (now - service->m_LastRefill).GetSeconds() * service->m_Limits.m_uiRequestsPerSecond);
    service->m_LastRefill = now;
    if (service->m_fTokens < 1 || service->m_uiOutstanding >= service->m_Limits.m_uiMaximumConcurrentRequests)
    {
      Reply(packet, xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure)).IgnoreResult();
      return xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure);
    }
    service->m_fTokens -= 1;
    ++service->m_uiOutstanding;
    packet.m_uiExecutor     = service->m_uiExecutor;
    packet.m_uiRegistration = service->m_uiRegistration;
    ++m_Statistics.m_uiRpcRequests;
  }
  else
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
  if (kind == Kind::Message)
    packet.m_Event = std::move(event);
  const auto size = packet.m_Event.m_Payload.GetCapacity();
  if (m_Queue.GetCount() >= m_Config.m_uiMaximumQueuedPackets || size > m_Config.m_uiMaximumQueuedBytes - m_Statistics.m_uiQueuedBytes)
  {
    ReleaseRequest(packet);
    if (kind == Kind::Request)
      Reply(packet, xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure)).IgnoreResult();
    ++m_Statistics.m_uiRejectedPackets;
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure);
  }
  m_Statistics.m_uiQueuedBytes += size;
  m_Queue.PushBack(std::move(packet));
  return {};
}
xiiNetworkDataError xiiNetworkReflectionEndpointState::Dispatch(xiiUInt32 executor, xiiUInt32 maximum)
{
  XII_PROFILE_SCOPE("NetworkReflection.Dispatch");
  xiiNetworkDataError firstError;
  for (xiiUInt32 i = 0; i < maximum; ++i)
  {
    // The permit covers target ownership, authorization and invocation, including local destructors.
    auto&     registry = xiiNetworkRegistryAccess::Get(m_Registry);
    ReadScope scope(registry);
    if (!scope.m_bAcquired)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
    Packet packet;
    bool   found = false;
    {
      XII_LOCK(m_Mutex);
      if (!m_bRunning)
        return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
      for (xiiUInt32 j = 0; j < m_Queue.GetCount(); ++j)
        if (m_Queue[j].m_uiExecutor == executor)
        {
          packet = std::move(m_Queue[j]);
          m_Queue.RemoveAtAndCopy(j);
          m_Statistics.m_uiQueuedBytes -= packet.m_Event.m_Payload.GetCapacity();
          m_Statistics.m_DispatchDelay += xiiTime::Now() - packet.m_Queued;
          found = true;
          break;
        }
    }
    if (!found)
      break;
    auto error = packet.m_Kind == Kind::Message ? DispatchMessage(packet) : DispatchRpc(packet);
    if (packet.m_Kind == Kind::Request && error.Failed())
      Reply(packet, error).IgnoreResult();
    {
      XII_LOCK(m_Mutex);
      ReleaseRequest(packet);
    }
    if (error.Failed())
    {
      XII_LOCK(m_Mutex);
      ++m_Statistics.m_uiRejectedPackets;
      if (error.m_Code == xiiNetworkDataErrorCode::Unauthorized)
        ++m_Statistics.m_uiUnauthorized;
      if (firstError.Succeeded())
        firstError = error;
    }
  }
  return firstError;
}
xiiNetworkDataError xiiNetworkReflectionEndpointState::DispatchMessage(Packet& packet)
{
  xiiSharedPtr<Handler> handler;
  {
    XII_LOCK(m_Mutex);
    if (FindPeer(packet.m_Event.m_Channel) == nullptr)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
    if (!m_Handlers.TryGetValue(packet.m_uiService, handler) || handler->m_uiRegistration != packet.m_uiRegistration ||
        handler->m_uiGeneration != xiiNetworkRegistryAccess::Get(m_Registry).m_uiGeneration.load())
      return xiiNetworkDataError(xiiNetworkDataErrorCode::HandlerRemoved);
  }
  xiiNetworkDispatchContext context;
  context.m_pSession = packet.m_Event.m_pSessionInfo;
  context.m_Channel  = packet.m_Event.m_Channel;
  if (!handler->m_pOwner->Authorize(context))
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Unauthorized);
  xiiNetworkSerializationContext serialization;
  serialization.m_Limits = m_Config.m_Serialization;
  const auto objectBytes = 2ULL * handler->m_pType->GetTypeSize() + 64;
  if (objectBytes > serialization.m_Limits.m_uiMaximumDecodedBytes)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::AllocationLimit);
  serialization.m_Limits.m_uiMaximumDecodedBytes -= objectBytes;
  auto*      allocator = handler->m_pType->GetAllocator();
  void*      instance  = allocator->Allocate<void>();
  const auto bytes     = packet.m_Event.m_Payload.GetBytes().GetSubArray(5);
  auto       error     = xiiNetworkSerializer::Deserialize(m_Registry, handler->m_pType, instance, bytes, serialization);
  if (error.Succeeded())
  {
    handler->m_pOwner->Handle(context, instance);
    XII_LOCK(m_Mutex);
    ++m_Statistics.m_uiMessagesDispatched;
  }
  allocator->Deallocate(instance);
  return error;
}

void xiiNetworkReflectionEndpointState::ReleaseRequest(const Packet& packet)
{
  if (packet.m_Kind != Kind::Request)
    return;
  xiiSharedPtr<Service> service;
  if (m_Services.TryGetValue(packet.m_uiService, service) && service->m_uiRegistration == packet.m_uiRegistration && service->m_uiOutstanding > 0)
    --service->m_uiOutstanding;
}
