/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Serialization/NetworkSerializer.h>

#include <Foundation/Network/Serialization/Implementation/NetworkSerializerInternal.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Time/Timestamp.h>
#include <cstring>

using namespace xiiNetworkReflectionInternal;
using namespace xiiNetworkSerializationInternal;

namespace
{
  constexpr xiiUInt32 SerializedMagic = 0x584E5331U;

  xiiNetworkDataError Header(xiiNetworkBinaryReader& reader, xiiNetworkSerializedHeader& out_header)
  {
    if (reader.U32() != SerializedMagic)
    {
      return reader.GetError().Failed() ? reader.GetError() : xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
    }
    xiiNetworkSerializedHeader header;
    header.m_Encoding      = static_cast<xiiNetworkEncoding>(reader.U8());
    header.m_uiTypeId      = reader.U64();
    header.m_uiTypeVersion = reader.U32();
    header.m_uiFingerprint = reader.U64();
    if (reader.GetError().Failed())
    {
      return reader.GetError();
    }
    if (header.m_Encoding > xiiNetworkEncoding::Compact || header.m_uiTypeId == 0 || header.m_uiTypeVersion == 0)
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
    }
    out_header = header;
    return {};
  }

  bool NativeClass(const CompiledValue& value)
  {
    return value.m_Kind == xiiNetworkValueKind::Structure || (value.m_Kind == xiiNetworkValueKind::Custom && (value.m_pType->GetVariantType() == xiiVariantType::TypedObject || (value.m_pType->GetVariantType() == xiiVariantType::Invalid && GetValueKind(value.m_pType) == xiiNetworkValueKind::Invalid)));
  }

  void GetNative(const CompiledField& field, const void* pObject, xiiUInt32 uiIndex, xiiStringView sKey, void* pValue)
  {
    if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
    {
      static_cast<const xiiAbstractMemberProperty*>(field.m_pProperty)->GetValuePtr(pObject, pValue);
    }
    else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
    {
      static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty)->GetValue(pObject, uiIndex, pValue);
    }
    else
    {
      static_cast<const xiiAbstractMapProperty*>(field.m_pProperty)->GetValue(pObject, sKey, pValue);
    }
  }
  void SetNative(const CompiledField& field, void* pObject, xiiUInt32 uiIndex, xiiStringView sKey, const void* pValue)
  {
    if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
    {
      static_cast<const xiiAbstractMemberProperty*>(field.m_pProperty)->SetValuePtr(pObject, pValue);
    }
    else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
    {
      static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty)->SetValue(pObject, uiIndex, pValue);
    }
    else
    {
      static_cast<const xiiAbstractMapProperty*>(field.m_pProperty)->Insert(pObject, sKey, pValue);
    }
  }
  xiiNetworkDataError GetValue(const CompiledField& field, const void* pObject, xiiUInt32 uiIndex, xiiStringView sKey,
                               bool bExists, xiiNetworkDecodeBudget* pBudget, xiiVariant& out_value)
  {
    const auto kind = GetValueKind(field.m_Value.m_pType);
    if (kind == xiiNetworkValueKind::VariantArray)
    {
      xiiVariantArray value;
      if (bExists)
      {
        GetNative(field, pObject, uiIndex, sKey, &value);
      }
      out_value = std::move(value);
      return {};
    }
    if (kind == xiiNetworkValueKind::VariantDictionary)
    {
      xiiVariantDictionary value;
      if (bExists)
      {
        GetNative(field, pObject, uiIndex, sKey, &value);
      }
      out_value = std::move(value);
      return {};
    }
    if (field.m_Value.m_Kind == xiiNetworkValueKind::Timestamp)
    {
      xiiTimestamp time;
      if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
      {
        static_cast<const xiiAbstractMemberProperty*>(field.m_pProperty)->GetValuePtr(pObject, &time);
      }
      else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
      {
        static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty)->GetValue(pObject, uiIndex, &time);
      }
      else
      {
        static_cast<const xiiAbstractMapProperty*>(field.m_pProperty)->GetValue(pObject, sKey, &time);
      }
      if (!time.IsValid())
      {
        return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
      }
      out_value = time.GetInt64(xiiSIUnitOfTime::Microsecond);
      return {};
    }
    if (NativeClass(field.m_Value))
    {
      const auto* pType = field.m_Value.m_pType;
      if (pType->GetAllocator() == nullptr || !pType->GetAllocator()->CanAllocate())
      {
        return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedNestedType);
      }
      if (pBudget != nullptr)
      {
        if (pType->GetTypeSize() > pBudget->GetLimits().m_uiMaximumObjectBytes)
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
        }
        auto error = pBudget->Reserve(2ULL * pType->GetTypeSize() + 64);
        if (error.Failed())
        {
          return error;
        }
      }
      void* pValue = pType->GetAllocator()->Allocate<void>();
      if (bExists)
      {
        if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
        {
          static_cast<const xiiAbstractMemberProperty*>(field.m_pProperty)->GetValuePtr(pObject, pValue);
        }
        else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
        {
          static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty)->GetValue(pObject, uiIndex, pValue);
        }
        else
        {
          static_cast<const xiiAbstractMapProperty*>(field.m_pProperty)->GetValue(pObject, sKey, pValue);
        }
      }
      out_value.MoveTypedObject(pValue, pType);
    }
    else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
    {
      out_value = xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(field.m_pProperty), pObject);
    }
    else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
    {
      out_value = xiiReflectionUtils::GetArrayPropertyValue(static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty), pObject, uiIndex);
    }
    else
    {
      out_value = xiiReflectionUtils::GetMapPropertyValue(static_cast<const xiiAbstractMapProperty*>(field.m_pProperty), pObject, sKey);
    }
    return {};
  }

  xiiNetworkDataError WriteElement(const CompiledField& field, const void* pObject, xiiUInt32 uiIndex, xiiStringView sKey,
                                   xiiNetworkBinaryWriter& writer, xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth)
  {
    const void* pValue = nullptr;
    if (!field.m_Value.m_bEnumeration)
    {
      if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
      {
        pValue = static_cast<const xiiAbstractMemberProperty*>(field.m_pProperty)->GetPropertyPointer(pObject);
      }
      else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
      {
        pValue = static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty)->GetValuePointer(const_cast<void*>(pObject), uiIndex);
      }
    }
    if (pValue != nullptr)
    {
      return WriteBorrowedValue(field.m_Value, pValue, field.m_Info, writer, encoding, limits, uiDepth);
    }
    if (field.m_Value.m_pType->GetTypeSize() > limits.m_uiMaximumObjectBytes)
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
    }
    xiiVariant value;
    auto       error = GetValue(field, pObject, uiIndex, sKey, true, nullptr, value);
    return error.Failed() ? error : WriteValue(field.m_Value, value, field.m_Info, writer, encoding, limits, uiDepth);
  }

  xiiNetworkDataError WriteField(const CompiledField& field, const void* pObject, xiiNetworkBinaryWriter& writer,
                                 xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth)
  {
    if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
    {
      return WriteElement(field, pObject, 0, {}, writer, encoding, limits, uiDepth);
    }
    if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
    {
      const auto* pProperty = static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty);
      const auto  uiCount   = pProperty->GetCount(pObject);
      if (uiCount > xiiMath::Min(limits.m_uiMaximumArrayElements, field.m_Info.m_uiMaximumElements))
      {
        return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
      }
      writer.VarUInt(uiCount);
      for (xiiUInt32 uiIndex = 0; uiIndex < uiCount; ++uiIndex)
      {
        auto error = WriteElement(field, pObject, uiIndex, {}, writer, encoding, limits, uiDepth);
        if (error.Failed())
        {
          return error;
        }
      }
      return writer.GetError();
    }
    const auto*                   pProperty = static_cast<const xiiAbstractMapProperty*>(field.m_pProperty);
    xiiHybridArray<xiiString, 16> keys;
    pProperty->GetKeys(pObject, keys);
    if (keys.GetCount() > xiiMath::Min(limits.m_uiMaximumMapEntries, field.m_Info.m_uiMaximumElements))
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
    }
    keys.Sort();
    writer.VarUInt(keys.GetCount());
    for (const auto& key : keys)
    {
      writer.Text(key, xiiMath::Min(limits.m_uiMaximumStringBytes, field.m_Info.m_uiMaximumStringBytes));
      auto error = WriteElement(field, pObject, 0, key, writer, encoding, limits, uiDepth);
      if (error.Failed())
      {
        return error;
      }
    }
    return writer.GetError();
  }

  xiiNetworkDataError ReadElement(const CompiledField& field, void* pObject, xiiUInt32 uiIndex, xiiStringView sKey, bool bExists,
                                  xiiVariant& out_value, xiiNetworkBinaryReader& reader, xiiNetworkEncoding encoding, xiiNetworkDecodeBudget& budget, xiiUInt32 uiDepth)
  {
    if (field.m_Value.m_Kind == xiiNetworkValueKind::Structure)
    {
      auto error = GetValue(field, pObject, uiIndex, sKey, bExists, &budget, out_value);
      if (error.Failed())
      {
        return error;
      }
    }
    return ReadValue(field.m_Value, out_value, field.m_Info, reader, encoding, budget, uiDepth);
  }

  xiiNetworkDataError ReadField(const CompiledField& field, void* pObject, xiiVariant& out_value, xiiNetworkBinaryReader& reader,
                                xiiNetworkEncoding encoding, xiiNetworkDecodeBudget& budget, xiiUInt32 uiDepth)
  {
    const auto& limits = budget.GetLimits();
    if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
    {
      return ReadElement(field, pObject, 0, {}, true, out_value, reader, encoding, budget, uiDepth);
    }
    const auto uiCount = reader.VarUInt();
    if (reader.GetError().Failed())
    {
      return reader.GetError();
    }
    if (uiCount > field.m_Info.m_uiMaximumElements || uiCount > (field.m_Info.m_Category == xiiNetworkFieldCategory::Array ? limits.m_uiMaximumArrayElements : limits.m_uiMaximumMapEntries))
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
    }
    if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
    {
      const auto* pProperty = static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty);
      if (!pProperty->CanSetCount(pObject, static_cast<xiiUInt32>(uiCount)))
      {
        return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
      }
      auto error = budget.Reserve(uiCount * sizeof(xiiVariant) * 4 + 64);
      if (error.Failed())
      {
        return error;
      }
      const auto      uiOriginal = pProperty->GetCount(pObject);
      xiiVariantArray values;
      values.SetCount(static_cast<xiiUInt32>(uiCount));
      for (xiiUInt32 uiIndex = 0; uiIndex < uiCount; ++uiIndex)
      {
        error = ReadElement(field, pObject, uiIndex, {}, uiIndex < uiOriginal, values[uiIndex], reader, encoding, budget, uiDepth);
        if (error.Failed())
        {
          return error;
        }
      }
      out_value = std::move(values);
      return reader.GetError();
    }
    const auto* pProperty = static_cast<const xiiAbstractMapProperty*>(field.m_pProperty);
    auto        error     = budget.Reserve(uiCount * (sizeof(xiiVariant) + sizeof(xiiString) + 32) * 4 + 64);
    if (error.Failed())
    {
      return error;
    }
    xiiVariantDictionary values;
    xiiStringView        previous;
    for (xiiUInt32 uiIndex = 0; uiIndex < uiCount; ++uiIndex)
    {
      const auto key = reader.Text(xiiMath::Min(limits.m_uiMaximumStringBytes, field.m_Info.m_uiMaximumStringBytes));
      if (reader.GetError().Failed())
      {
        return reader.GetError();
      }
      if (uiIndex != 0 && key.Compare(previous) <= 0)
      {
        return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
      }
      previous = key;
      error    = budget.Reserve(4ULL * (key.GetElementCount() + 1ULL));
      if (error.Failed())
      {
        return error;
      }
      xiiVariant value;
      error = ReadElement(field, pObject, 0, key, pProperty->Contains(pObject, key), value, reader, encoding, budget, uiDepth);
      if (error.Failed())
      {
        return error;
      }
      values.Insert(xiiString(key), std::move(value));
    }
    out_value = std::move(values);
    return reader.GetError();
  }

  void ApplyElement(const CompiledField& field, void* pObject, xiiUInt32 uiIndex, xiiStringView sKey, const xiiVariant& value)
  {
    const auto kind = GetValueKind(field.m_Value.m_pType);
    if (kind == xiiNetworkValueKind::VariantArray)
    {
      SetNative(field, pObject, uiIndex, sKey, &value.Get<xiiVariantArray>());
      return;
    }
    if (kind == xiiNetworkValueKind::VariantDictionary)
    {
      SetNative(field, pObject, uiIndex, sKey, &value.Get<xiiVariantDictionary>());
      return;
    }
    if (field.m_Value.m_Kind == xiiNetworkValueKind::Timestamp)
    {
      const auto time = xiiTimestamp::MakeFromInt(value.Get<xiiInt64>(), xiiSIUnitOfTime::Microsecond);
      if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
      {
        static_cast<const xiiAbstractMemberProperty*>(field.m_pProperty)->SetValuePtr(pObject, &time);
      }
      else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
      {
        static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty)->SetValue(pObject, uiIndex, &time);
      }
      else
      {
        static_cast<const xiiAbstractMapProperty*>(field.m_pProperty)->Insert(pObject, sKey, &time);
      }
      return;
    }
    if (NativeClass(field.m_Value))
    {
      if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
      {
        static_cast<const xiiAbstractMemberProperty*>(field.m_pProperty)->SetValuePtr(pObject, value.GetData());
      }
      else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
      {
        static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty)->SetValue(pObject, uiIndex, value.GetData());
      }
      else
      {
        static_cast<const xiiAbstractMapProperty*>(field.m_pProperty)->Insert(pObject, sKey, value.GetData());
      }
    }
    else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
    {
      xiiReflectionUtils::SetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(field.m_pProperty), pObject, value);
    }
    else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
    {
      xiiReflectionUtils::SetArrayPropertyValue(static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty), pObject, uiIndex, value);
    }
    else
    {
      xiiReflectionUtils::SetMapPropertyValue(static_cast<const xiiAbstractMapProperty*>(field.m_pProperty), pObject, sKey, value);
    }
  }
  void ApplyField(const CompiledField& field, void* pObject, const xiiVariant& value)
  {
    if (field.m_Info.m_Category == xiiNetworkFieldCategory::Member)
    {
      ApplyElement(field, pObject, 0, {}, value);
    }
    else if (field.m_Info.m_Category == xiiNetworkFieldCategory::Array)
    {
      const auto& values    = value.Get<xiiVariantArray>();
      const auto* pProperty = static_cast<const xiiAbstractArrayProperty*>(field.m_pProperty);
      if (pProperty->GetCount(pObject) != values.GetCount())
      {
        pProperty->SetCount(pObject, values.GetCount());
      }
      for (xiiUInt32 uiIndex = 0; uiIndex < values.GetCount(); ++uiIndex)
      {
        ApplyElement(field, pObject, uiIndex, {}, values[uiIndex]);
      }
    }
    else
    {
      const auto& values    = value.Get<xiiVariantDictionary>();
      const auto* pProperty = static_cast<const xiiAbstractMapProperty*>(field.m_pProperty);
      pProperty->Clear(pObject);
      for (auto iterator = values.GetIterator(); iterator.IsValid(); ++iterator)
      {
        ApplyElement(field, pObject, 0, iterator.Key(), iterator.Value());
      }
    }
  }
} // namespace

namespace xiiNetworkSerializationInternal
{
  xiiNetworkDataError WriteObject(const CompiledSchema& schema, const void* pObject, xiiNetworkBinaryWriter& writer,
                                  xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth)
  {
    if (uiDepth > limits.m_uiMaximumNestingDepth || schema.m_Fields.GetCount() > limits.m_uiMaximumFields)
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
    }
    for (const auto& field : schema.m_Fields)
    {
      xiiUInt32 uiBlock = 0;
      if (encoding == xiiNetworkEncoding::Tagged)
      {
        writer.VarUInt(field.m_Info.m_uiId);
        writer.U64(field.m_Info.m_uiWireSignature);
        uiBlock = writer.BeginBlock();
      }
      auto error = WriteField(field, pObject, writer, encoding, limits, uiDepth + 1);
      if (encoding == xiiNetworkEncoding::Tagged)
      {
        writer.EndBlock(uiBlock);
      }
      if (error.Failed())
      {
        error.m_uiType  = schema.m_pInfo->m_uiTypeId;
        error.m_uiField = field.m_Info.m_uiId;
        return error;
      }
    }
    if (encoding == xiiNetworkEncoding::Tagged)
    {
      writer.VarUInt(0);
    }
    return writer.GetError();
  }

  xiiNetworkDataError ReadObject(const CompiledSchema& schema, void* pObject, xiiNetworkBinaryReader& reader,
                                 xiiNetworkEncoding encoding, xiiNetworkDecodeBudget& budget, xiiUInt32 uiDepth, bool bRequireFinished)
  {
    const auto& limits   = budget.GetLimits();
    const auto  uiFields = schema.m_Fields.GetCount();
    if (uiDepth > limits.m_uiMaximumNestingDepth || uiFields > limits.m_uiMaximumFields)
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
    }
    auto error = budget.Reserve(static_cast<xiiUInt64>(uiFields) * (sizeof(xiiVariant) + 1) * 2 + 64);
    if (error.Failed())
    {
      return error;
    }
    xiiHybridArray<xiiVariant, 16> values;
    values.SetCount(uiFields);
    xiiHybridArray<bool, 16> present;
    present.SetCount(uiFields, false);
    if (encoding == xiiNetworkEncoding::Compact)
    {
      for (xiiUInt32 uiIndex = 0; uiIndex < uiFields; ++uiIndex)
      {
        const auto& field = schema.m_Fields[uiIndex];
        error             = ReadField(field, pObject, values[uiIndex], reader, encoding, budget, uiDepth + 1);
        if (error.Failed())
        {
          error.m_uiType  = schema.m_pInfo->m_uiTypeId;
          error.m_uiField = field.m_Info.m_uiId;
          return error;
        }
        present[uiIndex] = true;
      }
    }
    else
    {
      xiiUInt64 uiPrevious = 0;
      xiiUInt32 uiSeen = 0, uiIndex = 0;
      while (true)
      {
        const auto uiId = reader.VarUInt();
        if (reader.GetError().Failed())
        {
          return reader.GetError();
        }
        if (uiId == 0)
        {
          break;
        }
        if (uiId <= uiPrevious || ++uiSeen > limits.m_uiMaximumFields)
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
        }
        uiPrevious             = uiId;
        const auto uiSignature = reader.U64();
        const auto uiSize      = reader.U32();
        const auto uiOffset    = reader.GetPosition();
        const auto bytes       = reader.Bytes(uiSize);
        if (reader.GetError().Failed())
        {
          return reader.GetError();
        }
        while (uiIndex < uiFields && schema.m_Fields[uiIndex].m_Info.m_uiId < uiId)
        {
          ++uiIndex;
        }
        if (uiIndex == uiFields || schema.m_Fields[uiIndex].m_Info.m_uiId != uiId)
        {
          continue;
        }
        const auto& field = schema.m_Fields[uiIndex];
        if (uiSignature != field.m_Info.m_uiWireSignature)
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidFieldType, schema.m_pInfo->m_uiTypeId, uiId, uiOffset);
        }
        xiiNetworkBinaryReader bounded(bytes);
        error = ReadField(field, pObject, values[uiIndex], bounded, encoding, budget, uiDepth + 1);
        if (error.Succeeded() && !bounded.IsFinished())
        {
          error = xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding, 0, 0, bounded.GetPosition());
        }
        if (error.Failed())
        {
          error.m_uiType  = schema.m_pInfo->m_uiTypeId;
          error.m_uiField = uiId;
          error.m_uiOffset += uiOffset;
          return error;
        }
        present[uiIndex] = true;
      }
    }
    if (reader.GetError().Failed())
    {
      return reader.GetError();
    }
    if (bRequireFinished && !reader.IsFinished())
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding, schema.m_pInfo->m_uiTypeId, 0, reader.GetPosition());
    }
    for (xiiUInt32 uiIndex = 0; uiIndex < uiFields; ++uiIndex)
    {
      if (!present[uiIndex] && !schema.m_Fields[uiIndex].m_Info.m_bOptional)
      {
        return xiiNetworkDataError(xiiNetworkDataErrorCode::MissingRequiredField, schema.m_pInfo->m_uiTypeId, schema.m_Fields[uiIndex].m_Info.m_uiId);
      }
    }
    for (xiiUInt32 uiIndex = 0; uiIndex < uiFields; ++uiIndex)
    {
      if (present[uiIndex])
      {
        ApplyField(schema.m_Fields[uiIndex], pObject, values[uiIndex]);
      }
    }
    return {};
  }
} // namespace xiiNetworkSerializationInternal

xiiNetworkDataError xiiNetworkSerializer::ReadHeader(xiiArrayPtr<const xiiUInt8> bytes, xiiNetworkSerializedHeader& out_header)
{
  xiiNetworkBinaryReader reader(bytes);
  return Header(reader, out_header);
}

xiiNetworkDataError xiiNetworkSerializer::Serialize(xiiNetworkTypeRegistry& registry, const xiiRTTI* pType, const void* pObject,
                                                    xiiNetworkSerializationContext& context, xiiArrayPtr<const xiiUInt8>& out_bytes, xiiNetworkEncoding encoding)
{
  XII_PROFILE_SCOPE("Network Serialize");
  out_bytes = {};
  if (pType == nullptr || pObject == nullptr || encoding > xiiNetworkEncoding::Compact)
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  }
  auto&     state = xiiNetworkRegistryAccess::Get(registry);
  ReadScope scope(state);
  if (!scope.m_bAcquired)
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  }
  ++state.m_uiEncoded;
  auto fail = [&](xiiNetworkDataError error) {
    ++state.m_uiEncodeFailures;
    return error;
  };
  xiiNetworkDataError error;
  if (state.m_Errors.TryGetValue(pType, error))
  {
    return fail(error);
  }
  xiiSharedPtr<CompiledSchema> pSchema;
  if (!state.m_Schemas.TryGetValue(pType, pSchema))
  {
    return fail(xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownType));
  }
  auto& scratch = xiiNetworkSerializationAccess::Scratch(context);
  scratch.Clear();
  if (scratch.GetHeapMemoryUsage() > context.m_Limits.m_uiMaximumTemporaryBytes)
  {
    scratch.Compact();
  }
  if (context.m_Limits.m_uiMaximumTemporaryBytes <= 64)
  {
    return fail(xiiNetworkDataError(xiiNetworkDataErrorCode::AllocationLimit));
  }
  // Bound DynamicArray growth and alignment before allowing another allocation.
  const auto uiWriterLimit = static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(context.m_Limits.m_uiMaximumPayloadBytes,
                                                                            (context.m_Limits.m_uiMaximumTemporaryBytes - 64) / 2));

  const auto&            info = *pSchema->m_pInfo;
  xiiNetworkBinaryWriter writer(scratch, uiWriterLimit);
  writer.U32(SerializedMagic);
  writer.U8(static_cast<xiiUInt8>(encoding));
  writer.U64(info.m_uiTypeId);
  writer.U32(info.m_uiTypeVersion);
  writer.U64(info.m_uiFingerprint);
  error = WriteObject(*pSchema, pObject, writer, encoding, context.m_Limits, 0);
  if (writer.GetError().m_Code == xiiNetworkDataErrorCode::SizeLimit && uiWriterLimit < context.m_Limits.m_uiMaximumPayloadBytes)
  {
    error = xiiNetworkDataError(xiiNetworkDataErrorCode::AllocationLimit);
  }

  if (error.Failed())
  {
    return fail(error);
  }
  out_bytes = scratch;
  state.m_uiEncodedBytes.fetch_add(scratch.GetCount());
  auto uiPeak = state.m_uiPeakEncoded.load();
  while (uiPeak < scratch.GetCount() && !state.m_uiPeakEncoded.compare_exchange_weak(uiPeak, scratch.GetCount()))
  {
  }
  return {};
}
xiiNetworkDataError xiiNetworkSerializer::Deserialize(xiiNetworkTypeRegistry& registry, const xiiRTTI* pType, void* pObject,
                                                      xiiArrayPtr<const xiiUInt8> bytes, xiiNetworkSerializationContext& context)
{
  XII_PROFILE_SCOPE("Network Deserialize");
  if (pType == nullptr || pObject == nullptr)
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
  }
  auto&     state = xiiNetworkRegistryAccess::Get(registry);
  ReadScope scope(state);
  if (!scope.m_bAcquired)
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::RegistryUnavailable);
  }
  ++state.m_uiDecoded;
  auto fail = [&](xiiNetworkDataError error) {
    ++state.m_uiDecodeFailures;
    return error;
  };
  if (bytes.GetCount() > context.m_Limits.m_uiMaximumPayloadBytes)
  {
    return fail(xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit));
  }
  xiiNetworkDataError error;
  if (state.m_Errors.TryGetValue(pType, error))
  {
    return fail(error);
  }
  xiiSharedPtr<CompiledSchema> pSchema;
  if (!state.m_Schemas.TryGetValue(pType, pSchema))
  {
    return fail(xiiNetworkDataError(xiiNetworkDataErrorCode::UnknownType));
  }
  if (pType->GetTypeSize() > context.m_Limits.m_uiMaximumObjectBytes)
  {
    return fail(xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit));
  }
  xiiNetworkBinaryReader     reader(bytes);
  xiiNetworkSerializedHeader header;
  error = Header(reader, header);
  if (error.Failed())
  {
    return fail(error);
  }
  if (header.m_uiTypeId != pSchema->m_pInfo->m_uiTypeId || (header.m_Encoding == xiiNetworkEncoding::Compact && header.m_uiFingerprint != pSchema->m_pInfo->m_uiFingerprint))
  {
    return fail(xiiNetworkDataError(xiiNetworkDataErrorCode::SchemaMismatch, header.m_uiTypeId));
  }
  xiiNetworkDecodeBudget budget(context.m_Limits);
  error = ReadObject(*pSchema, pObject, reader, header.m_Encoding, budget, 0, true);
  xiiNetworkSerializationAccess::TrackBudget(context, budget.GetUsedBytes());
  if (error.Failed())
  {
    return fail(error);
  }
  state.m_uiDecodedBytes.fetch_add(bytes.GetCount());
  return {};
}
