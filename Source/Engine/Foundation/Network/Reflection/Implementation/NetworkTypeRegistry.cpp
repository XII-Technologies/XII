/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Reflection/NetworkTypeRegistry.h>

#include <Foundation/Network/Reflection/Implementation/NetworkSchemaInternal.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Time/Timestamp.h>
#include <bit>
#include <cmath>

using namespace xiiNetworkReflectionInternal;

namespace xiiNetworkReflectionInternal
{
  xiiNetworkValueKind GetValueKind(const xiiRTTI* pType)
  {
    if (pType == xiiGetStaticRTTI<bool>())
    {
      return xiiNetworkValueKind::Bool;
    }
    if (pType == xiiGetStaticRTTI<xiiInt8>())
    {
      return xiiNetworkValueKind::Int8;
    }
    if (pType == xiiGetStaticRTTI<xiiInt16>())
    {
      return xiiNetworkValueKind::Int16;
    }
    if (pType == xiiGetStaticRTTI<xiiInt32>())
    {
      return xiiNetworkValueKind::Int32;
    }
    if (pType == xiiGetStaticRTTI<xiiInt64>())
    {
      return xiiNetworkValueKind::Int64;
    }
    if (pType == xiiGetStaticRTTI<xiiUInt8>())
    {
      return xiiNetworkValueKind::UInt8;
    }
    if (pType == xiiGetStaticRTTI<xiiUInt16>())
    {
      return xiiNetworkValueKind::UInt16;
    }
    if (pType == xiiGetStaticRTTI<xiiUInt32>())
    {
      return xiiNetworkValueKind::UInt32;
    }
    if (pType == xiiGetStaticRTTI<xiiUInt64>())
    {
      return xiiNetworkValueKind::UInt64;
    }
    if (pType == xiiGetStaticRTTI<float>())
    {
      return xiiNetworkValueKind::Float;
    }
    if (pType == xiiGetStaticRTTI<double>())
    {
      return xiiNetworkValueKind::Double;
    }
    if (pType == xiiGetStaticRTTI<xiiString>())
    {
      return xiiNetworkValueKind::String;
    }
    if (pType == xiiGetStaticRTTI<xiiUntrackedString>())
    {
      return xiiNetworkValueKind::String;
    }
    if (pType == xiiGetStaticRTTI<xiiDataBuffer>())
    {
      return xiiNetworkValueKind::Bytes;
    }
    if (pType == xiiGetStaticRTTI<xiiTime>())
    {
      return xiiNetworkValueKind::Time;
    }
    if (pType == xiiGetStaticRTTI<xiiTimestamp>())
    {
      return xiiNetworkValueKind::Timestamp;
    }
    if (pType == xiiGetStaticRTTI<xiiUuid>())
    {
      return xiiNetworkValueKind::Uuid;
    }
    if (pType == xiiGetStaticRTTI<xiiVariant>())
    {
      return xiiNetworkValueKind::Variant;
    }
    if (pType == xiiGetStaticRTTI<xiiVariantArray>())
    {
      return xiiNetworkValueKind::VariantArray;
    }
    if (pType == xiiGetStaticRTTI<xiiVariantDictionary>())
    {
      return xiiNetworkValueKind::VariantDictionary;
    }
    if (pType == xiiGetStaticRTTI<xiiColor>())
    {
      return xiiNetworkValueKind::Color;
    }
    if (pType == xiiGetStaticRTTI<xiiColorGammaUB>())
    {
      return xiiNetworkValueKind::ColorGamma;
    }
    if (pType == xiiGetStaticRTTI<xiiAngle>())
    {
      return xiiNetworkValueKind::Angle;
    }
    if (pType == xiiGetStaticRTTI<xiiAngled>())
    {
      return xiiNetworkValueKind::Angled;
    }
    if (pType == xiiGetStaticRTTI<xiiHashedString>())
    {
      return xiiNetworkValueKind::HashedString;
    }
    if (pType == xiiGetStaticRTTI<xiiVec2>())
    {
      return xiiNetworkValueKind::Vector2;
    }
    if (pType == xiiGetStaticRTTI<xiiVec2d>())
    {
      return xiiNetworkValueKind::Vector2d;
    }
    if (pType == xiiGetStaticRTTI<xiiVec2I32>())
    {
      return xiiNetworkValueKind::Vector2I;
    }
    if (pType == xiiGetStaticRTTI<xiiVec2U32>())
    {
      return xiiNetworkValueKind::Vector2U;
    }
    if (pType == xiiGetStaticRTTI<xiiVec2I64>())
    {
      return xiiNetworkValueKind::Vector2I64;
    }
    if (pType == xiiGetStaticRTTI<xiiVec2U64>())
    {
      return xiiNetworkValueKind::Vector2U64;
    }
    if (pType == xiiGetStaticRTTI<xiiVec3>())
    {
      return xiiNetworkValueKind::Vector3;
    }
    if (pType == xiiGetStaticRTTI<xiiVec3d>())
    {
      return xiiNetworkValueKind::Vector3d;
    }
    if (pType == xiiGetStaticRTTI<xiiVec3I32>())
    {
      return xiiNetworkValueKind::Vector3I;
    }
    if (pType == xiiGetStaticRTTI<xiiVec3U32>())
    {
      return xiiNetworkValueKind::Vector3U;
    }
    if (pType == xiiGetStaticRTTI<xiiVec3I64>())
    {
      return xiiNetworkValueKind::Vector3I64;
    }
    if (pType == xiiGetStaticRTTI<xiiVec3U64>())
    {
      return xiiNetworkValueKind::Vector3U64;
    }
    if (pType == xiiGetStaticRTTI<xiiVec4>())
    {
      return xiiNetworkValueKind::Vector4;
    }
    if (pType == xiiGetStaticRTTI<xiiVec4d>())
    {
      return xiiNetworkValueKind::Vector4d;
    }
    if (pType == xiiGetStaticRTTI<xiiVec4I32>())
    {
      return xiiNetworkValueKind::Vector4I;
    }
    if (pType == xiiGetStaticRTTI<xiiVec4U32>())
    {
      return xiiNetworkValueKind::Vector4U;
    }
    if (pType == xiiGetStaticRTTI<xiiVec4I64>())
    {
      return xiiNetworkValueKind::Vector4I64;
    }
    if (pType == xiiGetStaticRTTI<xiiVec4U64>())
    {
      return xiiNetworkValueKind::Vector4U64;
    }
    if (pType == xiiGetStaticRTTI<xiiQuat>())
    {
      return xiiNetworkValueKind::Quaternion;
    }
    if (pType == xiiGetStaticRTTI<xiiQuatd>())
    {
      return xiiNetworkValueKind::Quaterniond;
    }
    if (pType == xiiGetStaticRTTI<xiiMat3>())
    {
      return xiiNetworkValueKind::Matrix3;
    }
    if (pType == xiiGetStaticRTTI<xiiMat4>())
    {
      return xiiNetworkValueKind::Matrix4;
    }
    if (pType == xiiGetStaticRTTI<xiiMat3d>())
    {
      return xiiNetworkValueKind::Matrix3d;
    }
    if (pType == xiiGetStaticRTTI<xiiMat4d>())
    {
      return xiiNetworkValueKind::Matrix4d;
    }
    if (pType == xiiGetStaticRTTI<xiiTransform>())
    {
      return xiiNetworkValueKind::Transform;
    }
    if (pType == xiiGetStaticRTTI<xiiTransformd>())
    {
      return xiiNetworkValueKind::Transformd;
    }
    return xiiNetworkValueKind::Invalid;
  }

  xiiUInt64 TypeId(const xiiRTTI* pType)
  {
    const auto* pIdentity = pType->GetAttributeByType<xiiNetworkIdentityAttribute>();
    return pIdentity != nullptr && pIdentity->m_uiTypeId != 0 ? pIdentity->m_uiTypeId : xiiNetworkStableId(pType->GetTypeName());
  }
  xiiUInt64 HashBytes(xiiArrayPtr<const xiiUInt8> bytes)
  {
    xiiUInt64 uiHash = 14695981039346656037ULL;
    for (auto uiByte : bytes)
    {
      uiHash = (uiHash ^ uiByte) * 1099511628211ULL;
    }
    return uiHash;
  }
  bool IsQuantizable(xiiNetworkValueKind kind)
  {
    return kind == xiiNetworkValueKind::Float || kind == xiiNetworkValueKind::Double ||
      (kind >= xiiNetworkValueKind::Vector2 && kind <= xiiNetworkValueKind::Vector4d) ||
      kind == xiiNetworkValueKind::Quaternion || kind == xiiNetworkValueKind::Quaterniond;
  }
} // namespace xiiNetworkReflectionInternal

namespace
{
  bool Eligible(const xiiRTTI* pType)
  {
    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (const auto* pProperty : properties)
    {
      if (pProperty->GetAttributeByType<xiiNetworkAttribute>() != nullptr)
      {
        return true;
      }
    }
    for (const auto* pCurrent = pType; pCurrent != nullptr; pCurrent = pCurrent->GetParentType())
    {
      for (const auto* pFunction : pCurrent->GetFunctions())
      {
        if (pFunction->GetAttributeByType<xiiNetworkAttribute>() != nullptr)
        {
          return true;
        }
      }
    }
    return false;
  }
  void ValueDescriptor(const CompiledValue& value, xiiNetworkBinaryWriter& writer)
  {
    writer.U8(static_cast<xiiUInt8>(value.m_Kind));
    writer.U8(value.m_bEnumeration ? (value.m_bBitflags ? 2 : 1) : 0);
    if (value.m_bEnumeration || value.m_pNested != nullptr)
    {
      writer.U64(TypeId(value.m_pType));
    }
    if (value.m_pCodec != nullptr)
    {
      writer.U64(value.m_pCodec->GetWireId());
      writer.U32(value.m_pCodec->GetVersion());
    }
  }
} // namespace

bool xiiNetworkTypeRegistryState::Acquire()
{
  if (m_bPaused.load())
  {
    return false;
  }
  m_uiReaders.fetch_add(1);
  if (m_bPaused.load())
  {
    Release();
    return false;
  }
  return true;
}
void xiiNetworkTypeRegistryState::Release()
{
  const auto uiPrevious = m_uiReaders.fetch_sub(1);
  XII_ASSERT_DEV(uiPrevious != 0, "Metadata permits must be balanced.");
  if (uiPrevious == 1 && m_bPaused.load())
  {
    XII_LOCK(m_Drain);
    m_Drain.SignalAll();
  }
}
void xiiNetworkTypeRegistryState::Pause()
{
  m_bPaused.store(true);
  XII_LOCK(m_Drain);
  while (m_uiReaders.load() != 0)
  {
    m_Drain.UnlockWaitForSignalAndLock();
  }
}
void xiiNetworkTypeRegistryState::OnPluginEvent(const xiiPluginEvent& event)
{
  XII_LOCK(m_MutationMutex);
  if (event.m_EventType == xiiPluginEvent::BeforePluginChanges || event.m_EventType == xiiPluginEvent::BeforeUnloading)
  {
    m_bPluginChanging = true;
    Pause();
    m_Schemas.Clear();
    m_Ids.Clear();
    m_Errors.Clear();
    m_uiSchemaCount.store(0);
    ++m_uiGeneration;
    if (event.m_EventType == xiiPluginEvent::BeforeUnloading)
    {
      m_Codecs.Clear();
    }
  }
  if (event.m_EventType == xiiPluginEvent::AfterPluginChanges)
  {
    m_bPluginChanging = false;
    Build().IgnoreResult();
  }
}

xiiNetworkDataError xiiNetworkTypeRegistryState::CompileValue(const xiiRTTI* pType, xiiBitflags<xiiPropertyFlags> flags, xiiUInt64 uiCodec, CompiledValue& out_value)
{
  if (pType == nullptr || flags.IsAnySet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner | xiiPropertyFlags::Phantom))
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedProperty);
  }
  CompiledValue value;
  value.m_pType = pType;
  if (uiCodec != 0)
  {
    if (!m_Codecs.TryGetValue(uiCodec, value.m_pCodec) || value.m_pCodec->GetType() != pType)
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::CustomCodecFailure);
    }
    value.m_Kind = xiiNetworkValueKind::Custom;
  }
  else if (flags.IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags) || pType->GetTypeFlags().IsAnySet(xiiTypeFlags::IsEnum | xiiTypeFlags::Bitflags))
  {
    value.m_Kind         = xiiNetworkValueKind::Int64;
    value.m_bEnumeration = true;
    value.m_bBitflags    = flags.IsSet(xiiPropertyFlags::Bitflags) || pType->GetTypeFlags().IsSet(xiiTypeFlags::Bitflags);
    for (const auto* pProperty : pType->GetProperties())
    {
      if (pProperty->GetCategory() == xiiPropertyCategory::Constant)
      {
        const auto iValue = static_cast<const xiiAbstractConstantProperty*>(pProperty)->GetConstant().ConvertTo<xiiInt64>();
        value.m_EnumValues.PushBack(iValue);
        value.m_uiEnumMask |= std::bit_cast<xiiUInt64>(iValue);
      }
    }
  }
  else
  {
    value.m_Kind = GetValueKind(pType);
    if (value.m_Kind == xiiNetworkValueKind::Invalid)
    {
      // Registered custom codecs may explicitly provide the default encoding of a value type.
      for (auto iterator = m_Codecs.GetIterator(); iterator.IsValid(); ++iterator)
      {
        if (iterator.Value()->GetType() == pType)
        {
          value.m_pCodec = iterator.Value();
          value.m_Kind   = xiiNetworkValueKind::Custom;
          break;
        }
      }
      if (value.m_Kind == xiiNetworkValueKind::Invalid)
      {
        auto error = BuildSchema(pType, value.m_pNested);
        if (error.Failed())
        {
          return error;
        }
        if (pType->GetAllocator() == nullptr || !pType->GetAllocator()->CanAllocate())
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedNestedType);
        }
        value.m_Kind = xiiNetworkValueKind::Structure;
      }
    }
  }
  out_value = std::move(value);
  return {};
}

xiiNetworkDataError xiiNetworkTypeRegistryState::BuildSchema(const xiiRTTI* pType, CompiledSchema*& out_pSchema)
{
  xiiNetworkDataError error;
  if (m_Errors.TryGetValue(pType, error))
  {
    return error;
  }
  xiiSharedPtr<CompiledSchema> pFound;
  if (m_Schemas.TryGetValue(pType, pFound))
  {
    out_pSchema = pFound.Borrow();
    return {};
  }
  if (m_BuildStack.Contains(pType) || m_BuildStack.GetCount() >= 32 || !Eligible(pType))
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedNestedType, TypeId(pType));
  }
  m_BuildStack.PushBack(pType);
  xiiSharedPtr<CompiledSchema> pSchema = XII_DEFAULT_NEW(CompiledSchema);
  pSchema->m_pInfo                     = XII_DEFAULT_NEW(xiiNetworkSchema);
  auto& info                           = *pSchema->m_pInfo;
  info.m_sTypeName                     = pType->GetTypeName();
  info.m_uiTypeId                      = TypeId(pType);
  info.m_uiTypeVersion                 = pType->GetTypeVersion();
  auto fail                            = [&](xiiNetworkDataErrorCode code, xiiUInt64 uiField = 0) {
    m_BuildStack.PopBack();
    const xiiNetworkDataError failure(code, info.m_uiTypeId, uiField);
    m_Errors.Insert(pType, failure);
    return failure;
  };
  if (info.m_uiTypeId == 0 || info.m_uiTypeVersion == 0)
  {
    return fail(xiiNetworkDataErrorCode::InvalidAttribute);
  }
  for (const auto* pCurrent = pType; pCurrent != nullptr; pCurrent = pCurrent->GetParentType())
  {
    for (const auto* pFunction : pCurrent->GetFunctions())
    {
      if (pFunction->GetAttributeByType<xiiNetworkAttribute>() != nullptr)
      {
        return fail(xiiNetworkDataErrorCode::UnsupportedProperty);
      }
    }
  }
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);
  for (const auto* pProperty : properties)
  {
    const auto* pAttribute = pProperty->GetAttributeByType<xiiNetworkAttribute>();
    if (pAttribute == nullptr)
    {
      continue;
    }
    CompiledField field;
    field.m_pProperty                = pProperty;
    auto& fieldInfo                  = field.m_Info;
    fieldInfo.m_sName                = pProperty->GetPropertyName();
    fieldInfo.m_uiId                 = pAttribute->m_uiFieldId != 0 ? pAttribute->m_uiFieldId : xiiNetworkStableId(fieldInfo.m_sName);
    fieldInfo.m_bOptional            = pAttribute->m_bOptional;
    fieldInfo.m_uiMaximumElements    = pAttribute->m_uiMaximumElements;
    fieldInfo.m_uiMaximumStringBytes = pAttribute->m_uiMaximumStringBytes;
    fieldInfo.m_uiQuantizationBits   = pAttribute->m_uiQuantizationBits;
    fieldInfo.m_fMinimum             = pAttribute->m_fMinimum;
    fieldInfo.m_fMaximum             = pAttribute->m_fMaximum;
    if (fieldInfo.m_uiId == 0 || fieldInfo.m_uiMaximumElements == 0 || fieldInfo.m_uiMaximumStringBytes == 0 ||
        fieldInfo.m_uiQuantizationBits > 24 || (fieldInfo.m_uiQuantizationBits != 0 && (!std::isfinite(fieldInfo.m_fMinimum) || !std::isfinite(fieldInfo.m_fMaximum) || fieldInfo.m_fMinimum >= fieldInfo.m_fMaximum)))
    {
      return fail(xiiNetworkDataErrorCode::InvalidAttribute, fieldInfo.m_uiId);
    }
    const auto flags = pProperty->GetFlags();
    if (flags.IsAnySet(xiiPropertyFlags::ReadOnly | xiiPropertyFlags::Const | xiiPropertyFlags::Reference) ||
        (pProperty->GetCategory() != xiiPropertyCategory::Member && pProperty->GetCategory() != xiiPropertyCategory::Array && pProperty->GetCategory() != xiiPropertyCategory::Map))
    {
      return fail(xiiNetworkDataErrorCode::UnsupportedProperty, fieldInfo.m_uiId);
    }
    fieldInfo.m_Category = pProperty->GetCategory() == xiiPropertyCategory::Array ? xiiNetworkFieldCategory::Array :
                                                                                    (pProperty->GetCategory() == xiiPropertyCategory::Map ? xiiNetworkFieldCategory::Map : xiiNetworkFieldCategory::Member);
    error                = CompileValue(pProperty->GetSpecificType(), flags, pAttribute->m_uiCodecId, field.m_Value);
    if (error.Failed())
    {
      return fail(error.m_Code, fieldInfo.m_uiId);
    }
    fieldInfo.m_Kind = field.m_Value.m_Kind;
    if (fieldInfo.m_uiQuantizationBits != 0 && !IsQuantizable(fieldInfo.m_Kind))
    {
      return fail(xiiNetworkDataErrorCode::InvalidAttribute, fieldInfo.m_uiId);
    }
    if (field.m_Value.m_pNested != nullptr)
    {
      fieldInfo.m_uiNestedTypeId = field.m_Value.m_pNested->m_pInfo->m_uiTypeId;
    }
    if (field.m_Value.m_pCodec != nullptr)
    {
      fieldInfo.m_uiCodecId      = field.m_Value.m_pCodec->GetWireId();
      fieldInfo.m_uiCodecVersion = field.m_Value.m_pCodec->GetVersion();
    }
    xiiDynamicArray<xiiUInt8> signature;
    xiiNetworkBinaryWriter    signatureWriter(signature, 256);
    signatureWriter.U8(static_cast<xiiUInt8>(fieldInfo.m_Category));
    ValueDescriptor(field.m_Value, signatureWriter);
    signatureWriter.U8(fieldInfo.m_uiQuantizationBits);
    if (fieldInfo.m_uiQuantizationBits != 0)
    {
      signatureWriter.U64(std::bit_cast<xiiUInt64>(fieldInfo.m_fMinimum));
      signatureWriter.U64(std::bit_cast<xiiUInt64>(fieldInfo.m_fMaximum));
    }
    if (signatureWriter.GetError().Failed())
    {
      return fail(signatureWriter.GetError().m_Code, fieldInfo.m_uiId);
    }
    fieldInfo.m_uiWireSignature = HashBytes(signature);
    pSchema->m_Fields.PushBack(std::move(field));
    if (pSchema->m_Fields.GetCount() > 512)
    {
      return fail(xiiNetworkDataErrorCode::SizeLimit);
    }
  }
  pSchema->m_Fields.Sort([](const CompiledField& a, const CompiledField& b) { return a.m_Info.m_uiId < b.m_Info.m_uiId; });
  xiiNetworkBinaryWriter descriptor(info.m_Descriptor, 262144);
  descriptor.U64(info.m_uiTypeId);
  descriptor.U32(info.m_uiTypeVersion);
  descriptor.U32(pSchema->m_Fields.GetCount());
  xiiUInt64 uiPrevious = 0;
  for (const auto& field : pSchema->m_Fields)
  {
    if (field.m_Info.m_uiId == uiPrevious)
    {
      return fail(xiiNetworkDataErrorCode::IdentifierCollision, uiPrevious);
    }
    uiPrevious = field.m_Info.m_uiId;
    info.m_Fields.PushBack(field.m_Info);
    descriptor.U64(field.m_Info.m_uiId);
    descriptor.U64(field.m_Info.m_uiWireSignature);
    descriptor.U8(field.m_Info.m_bOptional ? 1 : 0);
    descriptor.U32(field.m_Info.m_uiMaximumElements);
    descriptor.U32(field.m_Info.m_uiMaximumStringBytes);
    descriptor.U64(field.m_Value.m_pNested != nullptr ? field.m_Value.m_pNested->m_pInfo->m_uiFingerprint : 0);
  }
  if (descriptor.GetError().Failed())
  {
    return fail(descriptor.GetError().m_Code);
  }
  info.m_uiFingerprint = HashBytes(info.m_Descriptor);
  for (auto iterator = m_Schemas.GetIterator(); iterator.IsValid(); ++iterator)
  {
    const auto& other = *iterator.Value()->m_pInfo;
    if (info.m_uiFingerprint == other.m_uiFingerprint && info.m_Descriptor != other.m_Descriptor)
    {
      m_Errors.Insert(iterator.Key(), xiiNetworkDataError(xiiNetworkDataErrorCode::IdentifierCollision, other.m_uiTypeId));
      return fail(xiiNetworkDataErrorCode::IdentifierCollision);
    }
  }
  m_BuildStack.PopBack();
  out_pSchema = pSchema.Borrow();
  m_Schemas.Insert(pType, std::move(pSchema));
  return {};
}

xiiNetworkDataError xiiNetworkTypeRegistryState::Build()
{
  const auto start = xiiTime::Now();
  m_Schemas.Clear();
  m_Ids.Clear();
  m_Errors.Clear();
  m_BuildStack.Clear();
  xiiHybridArray<const xiiRTTI*, 128> types;
  xiiRTTI::ForEachType([&](const xiiRTTI* pType) { if (Eligible(pType)) { types.PushBack(pType); } });
  if (types.GetCount() > 4096)
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
  }
  for (const auto* pType : types)
  {
    const auto     uiId   = TypeId(pType);
    const xiiRTTI* pOther = nullptr;
    if (m_Ids.TryGetValue(uiId, pOther) && pOther != pType)
    {
      m_Errors.Insert(pOther, xiiNetworkDataError(xiiNetworkDataErrorCode::IdentifierCollision, uiId));
      m_Errors.Insert(pType, xiiNetworkDataError(xiiNetworkDataErrorCode::IdentifierCollision, uiId));
    }
    else
    {
      m_Ids.Insert(uiId, pType);
    }
  }
  for (const auto* pType : types)
  {
    CompiledSchema* pSchema = nullptr;
    BuildSchema(pType, pSchema).IgnoreResult();
  }
  // Propagate rejected nested metadata without deleting schemas still referenced by cached parents.
  bool bChanged = true;
  while (bChanged)
  {
    bChanged = false;
    for (auto iterator = m_Schemas.GetIterator(); iterator.IsValid(); ++iterator)
    {
      if (m_Errors.Contains(iterator.Key()))
      {
        continue;
      }
      for (const auto& field : iterator.Value()->m_Fields)
      {
        if (field.m_Value.m_pNested != nullptr && m_Errors.Contains(field.m_Value.m_pType))
        {
          m_Errors.Insert(iterator.Key(), xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedNestedType, TypeId(iterator.Key()), field.m_Info.m_uiId));
          bChanged = true;
          break;
        }
      }
    }
  }
  xiiUInt32 uiValid = 0;
  for (auto iterator = m_Schemas.GetIterator(); iterator.IsValid(); ++iterator)
  {
    uiValid += m_Errors.Contains(iterator.Key()) ? 0 : 1;
  }
  m_uiSchemaCount.store(uiValid);
  m_uiFailureCount.store(m_Errors.GetCount());
  m_iConstructionMicroseconds.store(static_cast<xiiInt64>((xiiTime::Now() - start).GetMicroseconds()));
  ++m_uiGeneration;
  m_bPaused.store(false);
  return {};
}

xiiNetworkTypeRegistry::xiiNetworkTypeRegistry() : m_pState(XII_DEFAULT_NEW(xiiNetworkTypeRegistryState))
{
  xiiPlugin::Events().AddEventHandler(xiiMakeDelegate(&xiiNetworkTypeRegistryState::OnPluginEvent, m_pState));
  Refresh().IgnoreResult();
}
xiiNetworkTypeRegistry::~xiiNetworkTypeRegistry()
{
  xiiPlugin::Events().RemoveEventHandler(xiiMakeDelegate(&xiiNetworkTypeRegistryState::OnPluginEvent, m_pState));
  m_pState->Pause();
  XII_DEFAULT_DELETE(m_pState);
}
xiiNetworkDataError xiiNetworkTypeRegistry::Refresh()
{
  XII_LOCK(m_pState->m_MutationMutex);
  if (m_pState->m_bPluginChanging)
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  }
  m_pState->Pause();
  return m_pState->Build();
}
void xiiNetworkTypeRegistry::Invalidate()
{
  XII_LOCK(m_pState->m_MutationMutex);
  m_pState->Pause();
  m_pState->m_Schemas.Clear();
  m_pState->m_Ids.Clear();
  m_pState->m_Errors.Clear();
  m_pState->m_uiSchemaCount.store(0);
  ++m_pState->m_uiGeneration;
}
xiiNetworkDataError xiiNetworkTypeRegistry::GetSchema(const xiiRTTI* pType, xiiSharedPtr<const xiiNetworkSchema>& out_schema) const
{
  ReadScope scope(*m_pState);
  if (!scope.m_bAcquired)
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  }
  xiiNetworkDataError error;
  if (m_pState->m_Errors.TryGetValue(pType, error))
  {
    return error;
  }
  xiiSharedPtr<CompiledSchema> pSchema;
  if (!m_pState->m_Schemas.TryGetValue(pType, pSchema))
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownType);
  }
  out_schema = pSchema->m_pInfo;
  return {};
}
xiiNetworkDataError xiiNetworkTypeRegistry::GetSchema(xiiUInt64 uiTypeId, xiiSharedPtr<const xiiNetworkSchema>& out_schema) const
{
  ReadScope scope(*m_pState);
  if (!scope.m_bAcquired)
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  }
  const xiiRTTI*      pType = nullptr;
  xiiNetworkDataError error;
  if (!m_pState->m_Ids.TryGetValue(uiTypeId, pType))
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownType, uiTypeId);
  }
  if (m_pState->m_Errors.TryGetValue(pType, error))
  {
    return error;
  }
  xiiSharedPtr<CompiledSchema> pSchema;
  if (!m_pState->m_Schemas.TryGetValue(pType, pSchema))
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownType, uiTypeId);
  }
  out_schema = pSchema->m_pInfo;
  return {};
}
xiiNetworkDataError xiiNetworkTypeRegistry::RegisterCodec(xiiSharedPtr<xiiNetworkCustomCodec> pCodec)
{
  if (pCodec == nullptr || pCodec->GetType() == nullptr || pCodec->GetWireId() == 0 || pCodec->GetVersion() == 0)
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  }
  XII_LOCK(m_pState->m_MutationMutex);
  const bool bWasPaused = m_pState->m_bPaused.load();
  m_pState->Pause();
  for (auto iterator = m_pState->m_Codecs.GetIterator(); iterator.IsValid(); ++iterator)
  {
    if (iterator.Key() == pCodec->GetWireId() || iterator.Value()->GetType() == pCodec->GetType())
    {
      m_pState->m_bPaused.store(bWasPaused);
      return xiiNetworkDataError(xiiNetworkDataErrorCode::IdentifierCollision);
    }
  }
  m_pState->m_Codecs.Insert(pCodec->GetWireId(), std::move(pCodec));
  return bWasPaused ? xiiNetworkDataError() : m_pState->Build();
}
xiiNetworkDataError xiiNetworkTypeRegistry::RemoveCodec(xiiUInt64 uiCodecId)
{
  XII_LOCK(m_pState->m_MutationMutex);
  const bool bWasPaused = m_pState->m_bPaused.load();
  m_pState->Pause();
  if (!m_pState->m_Codecs.Remove(uiCodecId))
  {
    m_pState->m_bPaused.store(bWasPaused);
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  }
  return bWasPaused ? xiiNetworkDataError() : m_pState->Build();
}
xiiNetworkSchemaStatistics xiiNetworkTypeRegistry::GetStatistics() const
{
  xiiNetworkSchemaStatistics result;
  result.m_uiSchemas                   = m_pState->m_uiSchemaCount.load();
  result.m_uiValidationFailures        = m_pState->m_uiFailureCount.load();
  result.m_uiGeneration                = m_pState->m_uiGeneration.load();
  result.m_ConstructionTime            = xiiTime::MakeFromMicroseconds(static_cast<double>(m_pState->m_iConstructionMicroseconds.load()));
  result.m_uiSerializationOperations   = m_pState->m_uiEncoded.load();
  result.m_uiDeserializationOperations = m_pState->m_uiDecoded.load();
  result.m_uiSerializationFailures     = m_pState->m_uiEncodeFailures.load();
  result.m_uiDeserializationFailures   = m_pState->m_uiDecodeFailures.load();
  result.m_uiBytesEncoded              = m_pState->m_uiEncodedBytes.load();
  result.m_uiBytesDecoded              = m_pState->m_uiDecodedBytes.load();
  result.m_uiPeakEncodedBytes          = m_pState->m_uiPeakEncoded.load();
  return result;
}
