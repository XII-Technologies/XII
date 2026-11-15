/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Messaging/Implementation/NetworkReflectionEndpointInternal.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Timestamp.h>
#include <cmath>

using namespace xiiNetworkMessagingInternal;
using namespace xiiNetworkReflectionInternal;
using namespace xiiNetworkSerializationInternal;

namespace
{
  xiiNetworkDataError ReadRpcValue(const CompiledValue& value, xiiVariant& output, xiiNetworkBinaryReader& reader,
                                   const xiiNetworkSerializationLimits& limits, xiiNetworkDecodeBudget& budget)
  {
    if (value.m_Kind == xiiNetworkValueKind::Structure)
    {
      if (value.m_pType->GetTypeSize() > limits.m_uiMaximumObjectBytes)
        return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
      const auto error = budget.Reserve(2ULL * value.m_pType->GetTypeSize() + 64);
      if (error.Failed())
        return error;
      void* object = value.m_pType->GetAllocator()->Allocate<void>();
      output.MoveTypedObject(object, value.m_pType);
    }
    return ReadValue(value, output, RpcPolicy(value, limits), reader, xiiNetworkEncoding::Tagged, budget, 0);
  }
} // namespace

xiiNetworkDataError xiiNetworkReflectionEndpointState::Call(xiiNetworkChannel channel, xiiUInt64 serviceId, xiiUInt64 methodId,
                                                            xiiArrayPtr<const xiiVariant> arguments, xiiNetworkRpcRequest& request, xiiTime timeout, bool fireAndForget)
{
  request            = {};
  auto&     registry = xiiNetworkRegistryAccess::Get(m_Registry);
  ReadScope scope(registry);
  if (!scope.m_bAcquired)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  xiiSharedPtr<Service> service;
  {
    XII_LOCK(m_Mutex);
    if (!m_bRunning || FindPeer(channel) == nullptr)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
    if (!m_Services.TryGetValue(serviceId, service) || service->m_uiGeneration != registry.m_uiGeneration.load())
      return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownRpc);
  }
  const auto* method = FindMethod(*service, methodId);
  if (method == nullptr)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownRpc, serviceId, methodId);
  if (arguments.GetCount() != method->m_Arguments.GetCount() || (fireAndForget && !method->m_bFireAndForget))
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  if (timeout == xiiTime::MakeZero())
    timeout = xiiTime::MakeFromMilliseconds(method->m_uiTimeout);
  if (!std::isfinite(timeout.GetSeconds()) || timeout <= xiiTime::MakeZero() || timeout > m_Config.m_MaximumCallTimeout)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  const auto reliable = Reliable(channel);
  if (reliable.Failed())
    return reliable;
  xiiDynamicArray<xiiUInt8> encoded;
  xiiNetworkBinaryWriter    args(encoded, static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(method->m_uiMaximumBytes, xiiMath::Min<xiiUInt64>(m_Config.m_Serialization.m_uiMaximumPayloadBytes, m_Config.m_Serialization.m_uiMaximumTemporaryBytes / 8))));
  for (xiiUInt32 i = 0; i < arguments.GetCount(); ++i)
  {
    auto error = WriteValue(method->m_Arguments[i], arguments[i], RpcPolicy(method->m_Arguments[i], m_Config.m_Serialization), args,
                            xiiNetworkEncoding::Tagged, m_Config.m_Serialization, 0);
    if (error.Failed())
      return error;
  }
  if (args.GetError().Failed())
    return args.GetError();
  XII_LOCK(m_Mutex);
  auto peer = FindPeer(channel);
  if (!m_bRunning || peer == nullptr)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
  if (!fireAndForget && m_Pending.GetCount() + m_Results.GetCount() >= m_Config.m_uiMaximumPendingCalls)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure);
  if (peer->m_uiNextRequest == 0)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
  const auto                id = peer->m_uiNextRequest++;
  xiiDynamicArray<xiiUInt8> bytes;
  xiiNetworkBinaryWriter    writer(bytes, m_Config.m_Serialization.m_uiMaximumPayloadBytes);
  Envelope(writer, Kind::Request);
  writer.U64(serviceId);
  writer.U64(methodId);
  writer.U64(id);
  writer.U8(fireAndForget ? 1 : 0);
  writer.U32(static_cast<xiiUInt32>(xiiMath::Max(1.0, timeout.GetMilliseconds())));
  if (4ULL * (encoded.GetCount() + method->m_Descriptor.GetCount() + 128ULL) > m_Config.m_Serialization.m_uiMaximumTemporaryBytes)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::AllocationLimit);
  writer.U32(method->m_Descriptor.GetCount());
  writer.Bytes(method->m_Descriptor);
  if (2ULL * (writer.GetCount() + encoded.GetCount() + 64ULL) + 2ULL * (encoded.GetCount() + 64ULL) > m_Config.m_Serialization.m_uiMaximumTemporaryBytes)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::AllocationLimit);
  writer.Bytes(encoded);
  if (writer.GetError().Failed())
    return writer.GetError();
  auto error = Send(channel, bytes);
  if (error.Failed())
    return error;
  if (!fireAndForget)
  {
    request.m_Channel    = channel;
    request.m_uiId       = id;
    auto& pending        = m_Pending.ExpandAndGetRef();
    pending.m_Request    = request;
    pending.m_uiService  = serviceId;
    pending.m_uiMethod   = methodId;
    pending.m_Deadline   = xiiTime::Now() + timeout;
    pending.m_Descriptor = method->m_Descriptor;
  }
  return {};
}
xiiNetworkDataError xiiNetworkReflectionEndpointState::Reply(const Packet& packet, xiiNetworkDataError error, xiiArrayPtr<const xiiUInt8> value)
{
  if (packet.m_bFireAndForget)
    return {};
  xiiDynamicArray<xiiUInt8> bytes;
  xiiNetworkBinaryWriter    writer(bytes, m_Config.m_Serialization.m_uiMaximumPayloadBytes);
  Envelope(writer, Kind::Response);
  writer.U64(packet.m_uiService);
  writer.U64(packet.m_uiMethod);
  writer.U64(packet.m_uiRequest);
  writer.U8(static_cast<xiiUInt8>(error.m_Code));
  if (error.Succeeded())
    writer.Bytes(value);
  if (writer.GetError().Failed())
    return writer.GetError();
  return Send(packet.m_Event.m_Channel, bytes);
}
xiiNetworkDataError xiiNetworkReflectionEndpointState::DispatchRpc(Packet& packet)
{
  XII_PROFILE_SCOPE("NetworkReflection.Rpc");
  xiiSharedPtr<Service> service;
  {
    XII_LOCK(m_Mutex);
    if (FindPeer(packet.m_Event.m_Channel) == nullptr)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
    if (!m_Services.TryGetValue(packet.m_uiService, service) || service->m_uiRegistration != packet.m_uiRegistration ||
        service->m_uiGeneration != xiiNetworkRegistryAccess::Get(m_Registry).m_uiGeneration.load() || service->m_pOwner == nullptr)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::HandlerRemoved);
  }
  const auto* method = FindMethod(*service, packet.m_uiMethod);
  if (method == nullptr)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownRpc);
  if (packet.m_Deadline <= xiiTime::Now())
    return xiiNetworkDataError(xiiNetworkDataErrorCode::TimedOut);
  if (packet.m_bFireAndForget && !method->m_bFireAndForget)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Unauthorized);
  xiiNetworkDispatchContext context;
  context.m_pSession  = packet.m_Event.m_pSessionInfo;
  context.m_Channel   = packet.m_Event.m_Channel;
  context.m_uiService = packet.m_uiService;
  context.m_uiMethod  = packet.m_uiMethod;
  context.m_uiRequest = packet.m_uiRequest;
  if (!service->m_pOwner->Authorize(context))
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Unauthorized);
  xiiNetworkBinaryReader reader(packet.m_Event.m_Payload.GetBytes().GetSubArray(34)); // Fixed request header before descriptor size.
  const auto             signatureSize = reader.U32();
  const auto             signature     = reader.Bytes(signatureSize);
  if (reader.GetError().Failed())
    return reader.GetError();
  if (signature != method->m_Descriptor.GetArrayPtr())
    return xiiNetworkDataError(xiiNetworkDataErrorCode::SchemaMismatch);
  if (reader.GetRemaining() > method->m_uiMaximumBytes)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
  xiiNetworkDecodeBudget budget(m_Config.m_Serialization);
  auto                   error = budget.Reserve(static_cast<xiiUInt64>(method->m_Arguments.GetCount()) * (sizeof(xiiVariant) + sizeof(xiiTimestamp)) + 256);
  if (error.Failed())
    return error;
  xiiHybridArray<xiiVariant, 8>   arguments;
  xiiHybridArray<xiiTimestamp, 8> timestamps;
  arguments.SetCount(method->m_Arguments.GetCount());
  timestamps.SetCount(method->m_Arguments.GetCount());
  for (xiiUInt32 i = 0; i < arguments.GetCount(); ++i)
  {
    const auto& value = method->m_Arguments[i];
    error             = ReadRpcValue(value, arguments[i], reader, m_Config.m_Serialization, budget);
    if (error.Failed())
      return error;
    if (value.m_Kind == xiiNetworkValueKind::Timestamp)
    {
      timestamps[i] = xiiTimestamp::MakeFromInt(arguments[i].Get<xiiInt64>(), xiiSIUnitOfTime::Microsecond);
      arguments[i]  = xiiVariant(&timestamps[i], value.m_pType);
    }
  }
  if (!reader.IsFinished())
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
  xiiVariant   result;
  xiiVariant   ownedResult;
  xiiTimestamp timestamp;
  if (method->m_bReturns)
  {
    const auto& value = method->m_Return;
    if (value.m_Kind == xiiNetworkValueKind::Timestamp)
      result = xiiVariant(&timestamp, value.m_pType);
    else if (value.m_Kind == xiiNetworkValueKind::Structure || (value.m_Kind == xiiNetworkValueKind::Custom && value.m_pType->GetVariantType() == xiiVariantType::Invalid))
    {
      if (value.m_pType->GetTypeSize() > m_Config.m_Serialization.m_uiMaximumObjectBytes)
        return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
      error = budget.Reserve(2ULL * value.m_pType->GetTypeSize() + 64);
      if (error.Failed())
        return error;
      void* object = value.m_pType->GetAllocator()->Allocate<void>();
      ownedResult.MoveTypedObject(object, value.m_pType);
      result = xiiVariant(object, value.m_pType);
    }
  }
  // All validation and authorization precede the side effect. No automatic replay after invocation.
  const auto                    started = xiiTime::Now();
  xiiHybridArray<xiiVariant, 8> invocation;
  invocation.Reserve(arguments.GetCount());
  for (xiiUInt32 i = 0; i < arguments.GetCount(); ++i)
  {
    const auto& value = method->m_Arguments[i];
    if (value.m_Kind == xiiNetworkValueKind::Structure ||
        (value.m_Kind == xiiNetworkValueKind::Custom && value.m_pType->GetVariantType() == xiiVariantType::Invalid))
      invocation.PushBack(xiiVariant(const_cast<void*>(arguments[i].GetData()), value.m_pType));
    else
      invocation.PushBack(arguments[i]);
  }
  method->m_pFunction->Execute(service->m_pOwner->GetObject(), invocation, result);
  {
    XII_LOCK(m_Mutex);
    ++m_Statistics.m_uiRpcExecuted;
    m_Statistics.m_RpcExecutionTime += xiiTime::Now() - started;
  }
  xiiDynamicArray<xiiUInt8> encoded;
  xiiNetworkBinaryWriter    writer(encoded, static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(service->m_Limits.m_uiMaximumResponseBytes, xiiMath::Min<xiiUInt64>(m_Config.m_Serialization.m_uiMaximumPayloadBytes - 30, m_Config.m_Serialization.m_uiMaximumTemporaryBytes / 8))));
  if (method->m_bReturns)
  {
    if (method->m_Return.m_Kind == xiiNetworkValueKind::Timestamp)
      result = timestamp.GetInt64(xiiSIUnitOfTime::Microsecond);
    error = WriteValue(method->m_Return, result, RpcPolicy(method->m_Return, m_Config.m_Serialization), writer,
                       xiiNetworkEncoding::Tagged, m_Config.m_Serialization, 0);
    if (error.Failed())
      return error;
  }
  if (writer.GetError().Failed())
    return writer.GetError();
  // A reply transport failure is already a terminal inability to deliver, not another invocation.
  Reply(packet, {}, encoded).IgnoreResult();
  return {};
}
xiiNetworkDataError xiiNetworkReflectionEndpointState::Decode(const xiiNetworkRpcResult& result, xiiVariant& out_value)
{
  if (result.m_Error.Failed())
    return result.m_Error;
  auto&     registry = xiiNetworkRegistryAccess::Get(m_Registry);
  ReadScope scope(registry);
  if (!scope.m_bAcquired)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  xiiSharedPtr<Service> service;
  {
    XII_LOCK(m_Mutex);
    if (!m_Services.TryGetValue(result.m_uiService, service) || service->m_uiGeneration != registry.m_uiGeneration.load())
      return xiiNetworkDataError(xiiNetworkDataErrorCode::HandlerRemoved);
  }
  const auto* method = FindMethod(*service, result.m_uiMethod);
  if (method == nullptr || method->m_Descriptor != result.m_Signature)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::SchemaMismatch);
  if (result.m_Value.GetCount() > m_Config.m_Serialization.m_uiMaximumPayloadBytes)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
  xiiVariant staged;
  if (!method->m_bReturns)
  {
    if (!result.m_Value.IsEmpty())
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
  }
  else
  {
    xiiNetworkBinaryReader reader(result.m_Value);
    xiiNetworkDecodeBudget budget(m_Config.m_Serialization);
    const auto             error = ReadRpcValue(method->m_Return, staged, reader, m_Config.m_Serialization, budget);
    if (error.Failed())
      return error;
    if (!reader.IsFinished())
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
  }
  out_value = std::move(staged);
  return {};
}
