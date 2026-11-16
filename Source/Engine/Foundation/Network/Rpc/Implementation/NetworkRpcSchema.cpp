/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Messaging/Implementation/NetworkReflectionEndpointInternal.h>
#include <bit>

using namespace xiiNetworkMessagingInternal;
using namespace xiiNetworkReflectionInternal;

namespace
{
  xiiNetworkDataError Compile(xiiNetworkTypeRegistryState& registry, const xiiRTTI* type, xiiBitflags<xiiPropertyFlags> flags, CompiledValue& value)
  {
    if (type == nullptr || flags.IsAnySet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner | xiiPropertyFlags::VarOut | xiiPropertyFlags::VarInOut | xiiPropertyFlags::Phantom) ||
        (flags.IsSet(xiiPropertyFlags::Reference) && !flags.IsSet(xiiPropertyFlags::Const)))
      return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedSignature);
    value.m_pType = type;
    value.m_Kind  = GetValueKind(type);
    if (type->GetTypeFlags().IsAnySet(xiiTypeFlags::IsEnum | xiiTypeFlags::Bitflags))
    {
      value.m_Kind         = xiiNetworkValueKind::Int64;
      value.m_bEnumeration = true;
      value.m_bBitflags    = type->GetTypeFlags().IsSet(xiiTypeFlags::Bitflags);
      for (const auto* property : type->GetProperties())
        if (property->GetCategory() == xiiPropertyCategory::Constant)
        {
          const auto number = static_cast<const xiiAbstractConstantProperty*>(property)->GetConstant().ConvertTo<xiiInt64>();
          value.m_EnumValues.PushBack(number);
          value.m_uiEnumMask |= std::bit_cast<xiiUInt64>(number);
          if (value.m_EnumValues.GetCount() > 512)
            return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
        }
      value.m_EnumValues.Sort();
    }
    if (value.m_Kind == xiiNetworkValueKind::Invalid)
    {
      for (auto codec = registry.m_Codecs.GetIterator(); codec.IsValid(); ++codec)
        if (codec.Value()->GetType() == type)
        {
          value.m_pCodec = codec.Value();
          value.m_Kind   = xiiNetworkValueKind::Custom;
          break;
        }
      if (value.m_Kind == xiiNetworkValueKind::Invalid)
      {
        xiiSharedPtr<CompiledSchema> schema;
        if (!registry.m_Schemas.TryGetValue(type, schema))
          return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedSignature);
        value.m_pNested = schema.Borrow();
        value.m_Kind    = xiiNetworkValueKind::Structure;
      }
      if (type->GetAllocator() == nullptr || !type->GetAllocator()->CanAllocate())
        return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedSignature);
    }
    return {};
  }
  void Describe(const CompiledValue& value, xiiNetworkBinaryWriter& writer)
  {
    writer.U8(static_cast<xiiUInt8>(value.m_Kind));
    writer.U64(TypeId(value.m_pType));
    writer.U8(value.m_bEnumeration ? (value.m_bBitflags ? 2 : 1) : 0);
    writer.U32(value.m_EnumValues.GetCount());
    for (auto number : value.m_EnumValues)
      writer.VarInt(number);
    writer.U64(value.m_pCodec != nullptr ? value.m_pCodec->GetWireId() : 0);
    writer.U32(value.m_pCodec != nullptr ? value.m_pCodec->GetVersion() : 0);
    writer.U32(value.m_pNested != nullptr ? value.m_pNested->m_pInfo->m_Descriptor.GetCount() : 0);
    if (value.m_pNested != nullptr)
      writer.Bytes(value.m_pNested->m_pInfo->m_Descriptor);
  }
} // namespace
namespace xiiNetworkMessagingInternal
{
  xiiNetworkDataError CompileService(xiiNetworkTypeRegistryState& registry, Service& service, const xiiNetworkReflectionConfiguration& config)
  {
    for (const auto* type = service.m_pType; type != nullptr; type = type->GetParentType())
      for (const auto* function : type->GetFunctions())
      {
        const auto* attribute = function->GetAttributeByType<xiiNetworkRpcAttribute>();
        if (attribute == nullptr)
          continue;
        if (function->GetFunctionType() != xiiFunctionType::Member || function->GetArgumentCount() > 32 ||
            function->GetReturnFlags().IsAnySet(xiiPropertyFlags::Pointer | xiiPropertyFlags::Reference) || attribute->m_uiTimeoutMilliseconds == 0 ||
            attribute->m_uiMaximumArgumentBytes == 0 || attribute->m_uiMaximumArgumentBytes > config.m_Serialization.m_uiMaximumPayloadBytes)
          return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedSignature);
        if (service.m_Methods.GetCount() >= config.m_uiMaximumMethodsPerService)
          return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
        Method method;
        method.m_pFunction      = function;
        method.m_uiId           = attribute->m_uiMethodId != 0 ? attribute->m_uiMethodId : xiiNetworkStableId(function->GetPropertyName());
        method.m_uiMaximumBytes = attribute->m_uiMaximumArgumentBytes;
        method.m_uiTimeout      = attribute->m_uiTimeoutMilliseconds;
        method.m_bFireAndForget = attribute->m_bAllowFireAndForget;
        for (xiiUInt32 i = 0; i < function->GetArgumentCount(); ++i)
        {
          CompiledValue value;
          const auto    error = Compile(registry, function->GetArgumentType(i), function->GetArgumentFlags(i), value);
          if (error.Failed())
            return xiiNetworkDataError(error.m_Code, TypeId(type), method.m_uiId);
          method.m_Arguments.PushBack(std::move(value));
        }
        method.m_bReturns = function->GetReturnType() != xiiGetStaticRTTI<void>();
        if (method.m_bReturns)
        {
          const auto error = Compile(registry, function->GetReturnType(), function->GetReturnFlags(), method.m_Return);
          if (error.Failed())
            return xiiNetworkDataError(error.m_Code, TypeId(type), method.m_uiId);
        }
        xiiNetworkBinaryWriter writer(method.m_Descriptor, config.m_uiMaximumSchemaBytes);
        writer.U64(TypeId(service.m_pType));
        writer.U64(method.m_uiId);
        writer.U32(method.m_Arguments.GetCount());
        for (const auto& value : method.m_Arguments)
          Describe(value, writer);
        writer.U8(method.m_bReturns ? 1 : 0);
        if (method.m_bReturns)
          Describe(method.m_Return, writer);
        if (writer.GetError().Failed())
          return writer.GetError();
        service.m_Methods.PushBack(std::move(method));
      }
    service.m_Methods.Sort([](const Method& a, const Method& b) { return a.m_uiId < b.m_uiId; });
    xiiUInt64 previous = 0;
    for (const auto& method : service.m_Methods)
    {
      if (method.m_uiId == previous)
        return xiiNetworkDataError(xiiNetworkDataErrorCode::IdentifierCollision);
      previous = method.m_uiId;
    }
    return service.m_Methods.IsEmpty() ? xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownRpc) : xiiNetworkDataError();
  }
  const Method* FindMethod(const Service& service, xiiUInt64 id)
  {
    xiiUInt32 begin = 0, end = service.m_Methods.GetCount();
    while (begin < end)
    {
      const auto mid = begin + (end - begin) / 2;
      if (service.m_Methods[mid].m_uiId < id)
        begin = mid + 1;
      else
        end = mid;
    }
    return begin < service.m_Methods.GetCount() && service.m_Methods[begin].m_uiId == id ? &service.m_Methods[begin] : nullptr;
  }
  xiiNetworkFieldSchema RpcPolicy(const CompiledValue& value, const xiiNetworkSerializationLimits& limits)
  {
    xiiNetworkFieldSchema policy;
    policy.m_Kind                 = value.m_Kind;
    policy.m_uiMaximumElements    = xiiMath::Min(limits.m_uiMaximumArrayElements, limits.m_uiMaximumMapEntries);
    policy.m_uiMaximumStringBytes = limits.m_uiMaximumStringBytes;
    return policy;
  }
} // namespace xiiNetworkMessagingInternal
xiiNetworkDataError xiiNetworkReflectionEndpointState::Register(xiiUInt64 id, const xiiRTTI* type, xiiSharedPtr<xiiNetworkRpcService> owner, xiiUInt32 executor, const xiiNetworkRpcServiceLimits& limits)
{
  auto&     registry = xiiNetworkRegistryAccess::Get(m_Registry);
  ReadScope scope(registry);
  if (!scope.m_bAcquired)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  if (id == 0 || type == nullptr || limits.m_uiRequestsPerSecond == 0 || limits.m_uiBurst == 0 || limits.m_uiMaximumConcurrentRequests == 0 || limits.m_uiMaximumResponseBytes == 0)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  xiiSharedPtr<Service> service = XII_DEFAULT_NEW(Service);
  service->m_pType              = type;
  service->m_pOwner             = std::move(owner);
  service->m_uiId               = id;
  service->m_uiExecutor         = executor;
  service->m_uiGeneration       = registry.m_uiGeneration.load();
  service->m_Limits             = limits;
  service->m_fTokens            = limits.m_uiBurst;
  service->m_LastRefill         = xiiTime::Now();
  auto error                    = CompileService(registry, *service, m_Config);
  if (error.Failed())
    return error;
  XII_LOCK(m_Mutex);
  if (!m_bRunning)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Disconnected);
  if (m_Services.Contains(id))
    return xiiNetworkDataError(xiiNetworkDataErrorCode::IdentifierCollision);
  if (m_Services.GetCount() >= m_Config.m_uiMaximumServices)
    return xiiNetworkDataError(xiiNetworkDataErrorCode::Backpressure);
  service->m_uiRegistration = m_uiNextRegistration++;
  m_Services.Insert(id, service);
  return {};
}
