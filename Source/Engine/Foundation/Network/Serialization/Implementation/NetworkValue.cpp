/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Serialization/Implementation/NetworkSerializerInternal.h>
#include <Foundation/Time/Timestamp.h>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>

using namespace xiiNetworkReflectionInternal;
using namespace xiiNetworkSerializationInternal;

namespace
{
  static_assert(sizeof(float) == 4 && sizeof(double) == 8 && std::numeric_limits<float>::is_iec559 && std::numeric_limits<double>::is_iec559);

  template <typename T>
  void WriteNumber(xiiNetworkBinaryWriter& writer, const xiiNetworkFieldSchema& policy, T value)
  {
    if constexpr (std::is_floating_point_v<T>)
    {
      if (!std::isfinite(value))
      {
        writer.Fail(xiiNetworkDataErrorCode::InvalidEncoding);
        return;
      }
      if (policy.m_uiQuantizationBits != 0)
      {
        if (value < policy.m_fMinimum || value > policy.m_fMaximum)
        {
          writer.Fail(xiiNetworkDataErrorCode::InvalidEncoding);
          return;
        }
        const auto   uiMaximum = (xiiUInt64(1) << policy.m_uiQuantizationBits) - 1;
        const double fScaled   = (static_cast<double>(value) - policy.m_fMinimum) / (policy.m_fMaximum - policy.m_fMinimum) * static_cast<double>(uiMaximum);
        writer.VarUInt(static_cast<xiiUInt64>(std::floor(fScaled + 0.5)));
      }
      else if constexpr (sizeof(T) == 4)
      {
        writer.U32(std::bit_cast<xiiUInt32>(value));
      }
      else
      {
        writer.U64(std::bit_cast<xiiUInt64>(value));
      }
    }
    else if constexpr (std::is_signed_v<T>)
    {
      writer.VarInt(value);
    }
    else
    {
      writer.VarUInt(value);
    }
  }
  template <typename T>
  T ReadNumber(xiiNetworkBinaryReader& reader, const xiiNetworkFieldSchema& policy)
  {
    if constexpr (std::is_floating_point_v<T>)
    {
      T value;
      if (policy.m_uiQuantizationBits != 0)
      {
        const auto uiMaximum = (xiiUInt64(1) << policy.m_uiQuantizationBits) - 1;
        const auto uiValue   = reader.VarUInt();
        if (uiValue > uiMaximum)
        {
          reader.Fail(xiiNetworkDataErrorCode::InvalidEncoding);
          return 0;
        }
        value = static_cast<T>(policy.m_fMinimum + static_cast<double>(uiValue) / static_cast<double>(uiMaximum) * (policy.m_fMaximum - policy.m_fMinimum));
      }
      else if constexpr (sizeof(T) == 4)
      {
        value = std::bit_cast<float>(reader.U32());
      }
      else
      {
        value = std::bit_cast<double>(reader.U64());
      }
      if (!std::isfinite(value))
      {
        reader.Fail(xiiNetworkDataErrorCode::InvalidEncoding);
        return 0;
      }
      return value;
    }
    else if constexpr (std::is_signed_v<T>)
    {
      const auto iValue = reader.VarInt();
      if (iValue < std::numeric_limits<T>::min() || iValue > std::numeric_limits<T>::max())
      {
        reader.Fail(xiiNetworkDataErrorCode::InvalidEncoding);
        return 0;
      }
      return static_cast<T>(iValue);
    }
    else
    {
      const auto uiValue = reader.VarUInt();
      if (uiValue > std::numeric_limits<T>::max())
      {
        reader.Fail(xiiNetworkDataErrorCode::InvalidEncoding);
        return 0;
      }
      return static_cast<T>(uiValue);
    }
  }
  template <typename... T>
  void WriteComponents(xiiNetworkBinaryWriter& writer, const xiiNetworkFieldSchema& policy, T... values) { (WriteNumber(writer, policy, values), ...); }
  template <typename... T>
  void ReadComponents(xiiNetworkBinaryReader& reader, const xiiNetworkFieldSchema& policy, T&... values) { ((values = ReadNumber<T>(reader, policy)), ...); }

  bool ValidEnum(const CompiledValue& value, xiiInt64 iNumber)
  {
    return !value.m_bEnumeration || (value.m_bBitflags ? (std::bit_cast<xiiUInt64>(iNumber) & ~value.m_uiEnumMask) == 0 : value.m_EnumValues.Contains(iNumber));
  }
  xiiNetworkDataError WriteNative(const CompiledValue& value, const void* pObject, const xiiNetworkFieldSchema& policy,
                                  xiiNetworkBinaryWriter& writer, xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth);
  xiiNetworkDataError ReadNative(const CompiledValue& value, xiiVariant& ref_object, const xiiNetworkFieldSchema& policy,
                                 xiiNetworkBinaryReader& reader, xiiNetworkEncoding encoding, xiiNetworkDecodeBudget& budget, xiiUInt32 uiDepth);

  xiiNetworkDataError WriteDynamic(const xiiVariant& object, const xiiNetworkFieldSchema& policy, xiiNetworkBinaryWriter& writer,
                                   xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth)
  {
    if (!object.IsValid())
    {
      writer.U8(0);
      return writer.GetError();
    }
    if (object.GetType() == xiiVariantType::TypedPointer || object.GetType() == xiiVariantType::TypedObject)
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedProperty);
    }
    if (object.GetType() == xiiVariantType::StringView)
    {
      writer.U8(static_cast<xiiUInt8>(xiiNetworkValueKind::String));
      writer.Text(object.Get<xiiStringView>(), xiiMath::Min(limits.m_uiMaximumStringBytes, policy.m_uiMaximumStringBytes));
      return writer.GetError();
    }
    CompiledValue value;
    value.m_pType = object.GetType() == xiiVariantType::VariantArray ? xiiGetStaticRTTI<xiiVariantArray>() :
                                                                       (object.GetType() == xiiVariantType::VariantDictionary ? xiiGetStaticRTTI<xiiVariantDictionary>() : object.GetReflectedType());
    value.m_Kind  = GetValueKind(value.m_pType);
    if (value.m_Kind == xiiNetworkValueKind::Invalid)
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedProperty);
    }
    writer.U8(static_cast<xiiUInt8>(value.m_Kind));
    return WriteNative(value, object.GetData(), policy, writer, encoding, limits, uiDepth + 1);
  }
  xiiNetworkDataError ReadDynamic(xiiVariant& out_object, const xiiNetworkFieldSchema& policy, xiiNetworkBinaryReader& reader,
                                  xiiNetworkEncoding encoding, xiiNetworkDecodeBudget& budget, xiiUInt32 uiDepth)
  {
    const auto kind = static_cast<xiiNetworkValueKind>(reader.U8());
    if (reader.GetError().Failed())
    {
      return reader.GetError();
    }
    if (kind == xiiNetworkValueKind::Invalid)
    {
      out_object = {};
      return {};
    }
    if (kind == xiiNetworkValueKind::Structure || kind == xiiNetworkValueKind::Custom || kind == xiiNetworkValueKind::Timestamp || kind == xiiNetworkValueKind::Variant)
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
    }
    CompiledValue value;
    value.m_Kind = kind;
    return ReadNative(value, out_object, policy, reader, encoding, budget, uiDepth + 1);
  }

  xiiNetworkDataError WriteNative(const CompiledValue& value, const void* pObject, const xiiNetworkFieldSchema& policy,
                                  xiiNetworkBinaryWriter& writer, xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth)
  {
    if (uiDepth > limits.m_uiMaximumNestingDepth)
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
    }
    switch (value.m_Kind)
    {
      case xiiNetworkValueKind::Bool:
        writer.U8(*static_cast<const bool*>(pObject) ? 1 : 0);
        break;
      case xiiNetworkValueKind::Int8:
        WriteNumber(writer, policy, *static_cast<const xiiInt8*>(pObject));
        break;
      case xiiNetworkValueKind::Int16:
        WriteNumber(writer, policy, *static_cast<const xiiInt16*>(pObject));
        break;
      case xiiNetworkValueKind::Int32:
        WriteNumber(writer, policy, *static_cast<const xiiInt32*>(pObject));
        break;
      case xiiNetworkValueKind::Int64:
        WriteNumber(writer, policy, *static_cast<const xiiInt64*>(pObject));
        break;
      case xiiNetworkValueKind::UInt8:
        WriteNumber(writer, policy, *static_cast<const xiiUInt8*>(pObject));
        break;
      case xiiNetworkValueKind::UInt16:
        WriteNumber(writer, policy, *static_cast<const xiiUInt16*>(pObject));
        break;
      case xiiNetworkValueKind::UInt32:
        WriteNumber(writer, policy, *static_cast<const xiiUInt32*>(pObject));
        break;
      case xiiNetworkValueKind::UInt64:
        WriteNumber(writer, policy, *static_cast<const xiiUInt64*>(pObject));
        break;
      case xiiNetworkValueKind::Float:
        WriteNumber(writer, policy, *static_cast<const float*>(pObject));
        break;
      case xiiNetworkValueKind::Double:
        WriteNumber(writer, policy, *static_cast<const double*>(pObject));
        break;
      case xiiNetworkValueKind::Vector2:
      {
        const auto& object = *static_cast<const xiiVec2*>(pObject);
        WriteComponents(writer, policy, object.x, object.y);
        break;
      }
      case xiiNetworkValueKind::Vector2d:
      {
        const auto& object = *static_cast<const xiiVec2d*>(pObject);
        WriteComponents(writer, policy, object.x, object.y);
        break;
      }
      case xiiNetworkValueKind::Vector2I:
      {
        const auto& object = *static_cast<const xiiVec2I32*>(pObject);
        WriteComponents(writer, policy, object.x, object.y);
        break;
      }
      case xiiNetworkValueKind::Vector2U:
      {
        const auto& object = *static_cast<const xiiVec2U32*>(pObject);
        WriteComponents(writer, policy, object.x, object.y);
        break;
      }
      case xiiNetworkValueKind::Vector2I64:
      {
        const auto& object = *static_cast<const xiiVec2I64*>(pObject);
        WriteComponents(writer, policy, object.x, object.y);
        break;
      }
      case xiiNetworkValueKind::Vector2U64:
      {
        const auto& object = *static_cast<const xiiVec2U64*>(pObject);
        WriteComponents(writer, policy, object.x, object.y);
        break;
      }
      case xiiNetworkValueKind::Vector3:
      {
        const auto& object = *static_cast<const xiiVec3*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z);
        break;
      }
      case xiiNetworkValueKind::Vector3d:
      {
        const auto& object = *static_cast<const xiiVec3d*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z);
        break;
      }
      case xiiNetworkValueKind::Vector3I:
      {
        const auto& object = *static_cast<const xiiVec3I32*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z);
        break;
      }
      case xiiNetworkValueKind::Vector3U:
      {
        const auto& object = *static_cast<const xiiVec3U32*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z);
        break;
      }
      case xiiNetworkValueKind::Vector3I64:
      {
        const auto& object = *static_cast<const xiiVec3I64*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z);
        break;
      }
      case xiiNetworkValueKind::Vector3U64:
      {
        const auto& object = *static_cast<const xiiVec3U64*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z);
        break;
      }
      case xiiNetworkValueKind::Vector4:
      {
        const auto& object = *static_cast<const xiiVec4*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z, object.w);
        break;
      }
      case xiiNetworkValueKind::Vector4d:
      {
        const auto& object = *static_cast<const xiiVec4d*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z, object.w);
        break;
      }
      case xiiNetworkValueKind::Vector4I:
      {
        const auto& object = *static_cast<const xiiVec4I32*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z, object.w);
        break;
      }
      case xiiNetworkValueKind::Vector4U:
      {
        const auto& object = *static_cast<const xiiVec4U32*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z, object.w);
        break;
      }
      case xiiNetworkValueKind::Vector4I64:
      {
        const auto& object = *static_cast<const xiiVec4I64*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z, object.w);
        break;
      }
      case xiiNetworkValueKind::Vector4U64:
      {
        const auto& object = *static_cast<const xiiVec4U64*>(pObject);
        WriteComponents(writer, policy, object.x, object.y, object.z, object.w);
        break;
      }
      case xiiNetworkValueKind::Quaternion:
      {
        const auto& object = *static_cast<const xiiQuat*>(pObject);
        if (!object.IsValid(static_cast<decltype(object.x)>(0.0001)))
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
        WriteComponents(writer, policy, object.x, object.y, object.z, object.w);
        break;
      }
      case xiiNetworkValueKind::Quaterniond:
      {
        const auto& object = *static_cast<const xiiQuatd*>(pObject);
        if (!object.IsValid(static_cast<decltype(object.x)>(0.0001)))
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
        WriteComponents(writer, policy, object.x, object.y, object.z, object.w);
        break;
      }
      case xiiNetworkValueKind::Color:
      {
        const auto& object = *static_cast<const xiiColor*>(pObject);
        WriteComponents(writer, policy, object.r, object.g, object.b, object.a);
        break;
      }
      case xiiNetworkValueKind::Transform:
      {
        const auto& object = *static_cast<const xiiTransform*>(pObject);
        WriteComponents(writer, policy, object.m_vPosition.x, object.m_vPosition.y, object.m_vPosition.z, object.m_qRotation.x, object.m_qRotation.y, object.m_qRotation.z, object.m_qRotation.w, object.m_vScale.x, object.m_vScale.y, object.m_vScale.z);
        break;
      }
      case xiiNetworkValueKind::Transformd:
      {
        const auto& object = *static_cast<const xiiTransformd*>(pObject);
        WriteComponents(writer, policy, object.m_vPosition.x, object.m_vPosition.y, object.m_vPosition.z, object.m_qRotation.x, object.m_qRotation.y, object.m_qRotation.z, object.m_qRotation.w, object.m_vScale.x, object.m_vScale.y, object.m_vScale.z);
        break;
      }
      case xiiNetworkValueKind::Matrix3:
      {
        const auto& object = *static_cast<const xiiMat3*>(pObject);
        for (xiiInt32 iColumn = 0; iColumn < 3; ++iColumn)
        {
          for (xiiInt32 iRow = 0; iRow < 3; ++iRow)
          {
            WriteNumber(writer, policy, object.Element(iColumn, iRow));
          }
        }
        break;
      }
      case xiiNetworkValueKind::Matrix3d:
      {
        const auto& object = *static_cast<const xiiMat3d*>(pObject);
        for (xiiInt32 iColumn = 0; iColumn < 3; ++iColumn)
        {
          for (xiiInt32 iRow = 0; iRow < 3; ++iRow)
          {
            WriteNumber(writer, policy, object.Element(iColumn, iRow));
          }
        }
        break;
      }
      case xiiNetworkValueKind::Matrix4:
      {
        const auto& object = *static_cast<const xiiMat4*>(pObject);
        for (xiiInt32 iColumn = 0; iColumn < 4; ++iColumn)
        {
          for (xiiInt32 iRow = 0; iRow < 4; ++iRow)
          {
            WriteNumber(writer, policy, object.Element(iColumn, iRow));
          }
        }
        break;
      }
      case xiiNetworkValueKind::Matrix4d:
      {
        const auto& object = *static_cast<const xiiMat4d*>(pObject);
        for (xiiInt32 iColumn = 0; iColumn < 4; ++iColumn)
        {
          for (xiiInt32 iRow = 0; iRow < 4; ++iRow)
          {
            WriteNumber(writer, policy, object.Element(iColumn, iRow));
          }
        }
        break;
      }
      case xiiNetworkValueKind::String:
        writer.Text(value.m_pType == xiiGetStaticRTTI<xiiUntrackedString>() ? static_cast<const xiiUntrackedString*>(pObject)->GetView() : static_cast<const xiiString*>(pObject)->GetView(),
                    xiiMath::Min(limits.m_uiMaximumStringBytes, policy.m_uiMaximumStringBytes));
        break;
      case xiiNetworkValueKind::Bytes:
      {
        const auto& bytes = *static_cast<const xiiDataBuffer*>(pObject);
        if (bytes.GetCount() > limits.m_uiMaximumPayloadBytes)
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
        }
        writer.VarUInt(bytes.GetCount());
        writer.Bytes(bytes);
        break;
      }
      case xiiNetworkValueKind::ColorGamma:
      {
        const auto& object = *static_cast<const xiiColorGammaUB*>(pObject);
        writer.U8(object.r);
        writer.U8(object.g);
        writer.U8(object.b);
        writer.U8(object.a);
        break;
      }
      case xiiNetworkValueKind::Angle:
        WriteNumber(writer, policy, static_cast<const xiiAngle*>(pObject)->GetRadian());
        break;
      case xiiNetworkValueKind::Angled:
        WriteNumber(writer, policy, static_cast<const xiiAngled*>(pObject)->GetRadian());
        break;
      case xiiNetworkValueKind::Time:
        WriteNumber(writer, policy, static_cast<const xiiTime*>(pObject)->GetSeconds());
        break;
      case xiiNetworkValueKind::Timestamp:
      {
        const auto& time = *static_cast<const xiiTimestamp*>(pObject);
        if (!time.IsValid())
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
        }
        writer.VarInt(time.GetInt64(xiiSIUnitOfTime::Microsecond));
        break;
      }
      case xiiNetworkValueKind::Uuid:
      {
        xiiUInt64 uiLow = 0, uiHigh = 0;
        static_cast<const xiiUuid*>(pObject)->GetValues(uiLow, uiHigh);
        writer.U64(uiHigh);
        writer.U64(uiLow);
        break;
      }
      case xiiNetworkValueKind::Variant:
        return WriteDynamic(*static_cast<const xiiVariant*>(pObject), policy, writer, encoding, limits, uiDepth + 1);
      case xiiNetworkValueKind::VariantArray:
      {
        const auto& values = *static_cast<const xiiVariantArray*>(pObject);
        if (values.GetCount() > xiiMath::Min(limits.m_uiMaximumArrayElements, policy.m_uiMaximumElements))
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
        }
        writer.VarUInt(values.GetCount());
        for (const auto& item : values)
        {
          auto error = WriteDynamic(item, policy, writer, encoding, limits, uiDepth + 1);
          if (error.Failed())
          {
            return error;
          }
        }
        break;
      }
      case xiiNetworkValueKind::VariantDictionary:
      {
        const auto& values = *static_cast<const xiiVariantDictionary*>(pObject);
        if (values.GetCount() > xiiMath::Min(limits.m_uiMaximumMapEntries, policy.m_uiMaximumElements))
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
        }
        xiiHybridArray<xiiStringView, 16> keys;
        for (auto iterator = values.GetIterator(); iterator.IsValid(); ++iterator)
        {
          keys.PushBack(iterator.Key());
        }
        keys.Sort();
        writer.VarUInt(keys.GetCount());
        for (const auto key : keys)
        {
          writer.Text(key, xiiMath::Min(limits.m_uiMaximumStringBytes, policy.m_uiMaximumStringBytes));
          const auto* pItem = values.GetValue(key);
          auto        error = WriteDynamic(*pItem, policy, writer, encoding, limits, uiDepth + 1);
          if (error.Failed())
          {
            return error;
          }
        }
        break;
      }
      case xiiNetworkValueKind::Structure:
        return WriteObject(*value.m_pNested, pObject, writer, encoding, limits, uiDepth + 1);
      case xiiNetworkValueKind::Custom:
      {
        const auto       uiBlock = writer.BeginBlock();
        const xiiVariant object(const_cast<void*>(pObject), value.m_pType);
        auto             error = value.m_pCodec->Encode(object, writer);
        writer.EndBlock(uiBlock);
        return writer.GetError().Failed() ? writer.GetError() : error;
      }
      default:
        return xiiNetworkDataError(xiiNetworkDataErrorCode::UnsupportedProperty);
    }
    return writer.GetError();
  }

  xiiNetworkDataError ReadNative(const CompiledValue& value, xiiVariant& ref_object, const xiiNetworkFieldSchema& policy,
                                 xiiNetworkBinaryReader& reader, xiiNetworkEncoding encoding, xiiNetworkDecodeBudget& budget, xiiUInt32 uiDepth)
  {
    const auto& limits = budget.GetLimits();
    if (uiDepth > limits.m_uiMaximumNestingDepth)
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
    }
    auto representationBudget = budget.Reserve(256);
    if (representationBudget.Failed())
    {
      return representationBudget;
    }
    switch (value.m_Kind)
    {
      case xiiNetworkValueKind::Bool:
      {
        const auto uiValue = reader.U8();
        if (uiValue > 1)
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
        }
        ref_object = uiValue != 0;
        break;
      }
      case xiiNetworkValueKind::Int8:
        ref_object = ReadNumber<xiiInt8>(reader, policy);
        break;
      case xiiNetworkValueKind::Int16:
        ref_object = ReadNumber<xiiInt16>(reader, policy);
        break;
      case xiiNetworkValueKind::Int32:
        ref_object = ReadNumber<xiiInt32>(reader, policy);
        break;
      case xiiNetworkValueKind::Int64:
        ref_object = ReadNumber<xiiInt64>(reader, policy);
        break;
      case xiiNetworkValueKind::UInt8:
        ref_object = ReadNumber<xiiUInt8>(reader, policy);
        break;
      case xiiNetworkValueKind::UInt16:
        ref_object = ReadNumber<xiiUInt16>(reader, policy);
        break;
      case xiiNetworkValueKind::UInt32:
        ref_object = ReadNumber<xiiUInt32>(reader, policy);
        break;
      case xiiNetworkValueKind::UInt64:
        ref_object = ReadNumber<xiiUInt64>(reader, policy);
        break;
      case xiiNetworkValueKind::Float:
        ref_object = ReadNumber<float>(reader, policy);
        break;
      case xiiNetworkValueKind::Double:
        ref_object = ReadNumber<double>(reader, policy);
        break;
      case xiiNetworkValueKind::Vector2:
      {
        xiiVec2 object;
        ReadComponents(reader, policy, object.x, object.y);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector2d:
      {
        xiiVec2d object;
        ReadComponents(reader, policy, object.x, object.y);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector2I:
      {
        xiiVec2I32 object;
        ReadComponents(reader, policy, object.x, object.y);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector2U:
      {
        xiiVec2U32 object;
        ReadComponents(reader, policy, object.x, object.y);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector2I64:
      {
        xiiVec2I64 object;
        ReadComponents(reader, policy, object.x, object.y);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector2U64:
      {
        xiiVec2U64 object;
        ReadComponents(reader, policy, object.x, object.y);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector3:
      {
        xiiVec3 object;
        ReadComponents(reader, policy, object.x, object.y, object.z);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector3d:
      {
        xiiVec3d object;
        ReadComponents(reader, policy, object.x, object.y, object.z);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector3I:
      {
        xiiVec3I32 object;
        ReadComponents(reader, policy, object.x, object.y, object.z);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector3U:
      {
        xiiVec3U32 object;
        ReadComponents(reader, policy, object.x, object.y, object.z);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector3I64:
      {
        xiiVec3I64 object;
        ReadComponents(reader, policy, object.x, object.y, object.z);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector3U64:
      {
        xiiVec3U64 object;
        ReadComponents(reader, policy, object.x, object.y, object.z);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector4:
      {
        xiiVec4 object;
        ReadComponents(reader, policy, object.x, object.y, object.z, object.w);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector4d:
      {
        xiiVec4d object;
        ReadComponents(reader, policy, object.x, object.y, object.z, object.w);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector4I:
      {
        xiiVec4I32 object;
        ReadComponents(reader, policy, object.x, object.y, object.z, object.w);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector4U:
      {
        xiiVec4U32 object;
        ReadComponents(reader, policy, object.x, object.y, object.z, object.w);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector4I64:
      {
        xiiVec4I64 object;
        ReadComponents(reader, policy, object.x, object.y, object.z, object.w);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Vector4U64:
      {
        xiiVec4U64 object;
        ReadComponents(reader, policy, object.x, object.y, object.z, object.w);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Quaternion:
      {
        xiiQuat object;
        ReadComponents(reader, policy, object.x, object.y, object.z, object.w);
        const double error = policy.m_uiQuantizationBits == 0 ? 0.0001 :
                                                                4.0 / static_cast<double>((xiiUInt64(1) << policy.m_uiQuantizationBits) - 1) + 0.0001;
        if (!object.IsValid(static_cast<decltype(object.x)>(error)))
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
        if (policy.m_uiQuantizationBits != 0)
          object.Normalize();
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Quaterniond:
      {
        xiiQuatd object;
        ReadComponents(reader, policy, object.x, object.y, object.z, object.w);
        const double error = policy.m_uiQuantizationBits == 0 ? 0.0001 :
                                                                4.0 / static_cast<double>((xiiUInt64(1) << policy.m_uiQuantizationBits) - 1) + 0.0001;
        if (!object.IsValid(static_cast<decltype(object.x)>(error)))
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
        if (policy.m_uiQuantizationBits != 0)
          object.Normalize();
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Color:
      {
        xiiColor object;
        ReadComponents(reader, policy, object.r, object.g, object.b, object.a);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Transform:
      {
        xiiTransform object;
        ReadComponents(reader, policy, object.m_vPosition.x, object.m_vPosition.y, object.m_vPosition.z, object.m_qRotation.x, object.m_qRotation.y, object.m_qRotation.z, object.m_qRotation.w, object.m_vScale.x, object.m_vScale.y, object.m_vScale.z);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Transformd:
      {
        xiiTransformd object;
        ReadComponents(reader, policy, object.m_vPosition.x, object.m_vPosition.y, object.m_vPosition.z, object.m_qRotation.x, object.m_qRotation.y, object.m_qRotation.z, object.m_qRotation.w, object.m_vScale.x, object.m_vScale.y, object.m_vScale.z);
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Matrix3:
      {
        xiiMat3 object;
        for (xiiInt32 iColumn = 0; iColumn < 3; ++iColumn)
        {
          for (xiiInt32 iRow = 0; iRow < 3; ++iRow)
          {
            object.Element(iColumn, iRow) = ReadNumber<float>(reader, policy);
          }
        }
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Matrix3d:
      {
        xiiMat3d object;
        for (xiiInt32 iColumn = 0; iColumn < 3; ++iColumn)
        {
          for (xiiInt32 iRow = 0; iRow < 3; ++iRow)
          {
            object.Element(iColumn, iRow) = ReadNumber<double>(reader, policy);
          }
        }
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Matrix4:
      {
        xiiMat4 object;
        for (xiiInt32 iColumn = 0; iColumn < 4; ++iColumn)
        {
          for (xiiInt32 iRow = 0; iRow < 4; ++iRow)
          {
            object.Element(iColumn, iRow) = ReadNumber<float>(reader, policy);
          }
        }
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Matrix4d:
      {
        xiiMat4d object;
        for (xiiInt32 iColumn = 0; iColumn < 4; ++iColumn)
        {
          for (xiiInt32 iRow = 0; iRow < 4; ++iRow)
          {
            object.Element(iColumn, iRow) = ReadNumber<double>(reader, policy);
          }
        }
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::String:
      {
        const auto text = reader.Text(xiiMath::Min(limits.m_uiMaximumStringBytes, policy.m_uiMaximumStringBytes));
        if (reader.GetError().Failed())
        {
          return reader.GetError();
        }
        auto error = budget.Reserve(2ULL * (text.GetElementCount() + 1ULL) + 64);
        if (error.Failed())
        {
          return error;
        }
        ref_object = xiiString(text);
        break;
      }
      case xiiNetworkValueKind::Bytes:
      {
        const auto uiCount = reader.VarUInt();
        if (uiCount > limits.m_uiMaximumPayloadBytes)
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
        }
        const auto bytes = reader.Bytes(static_cast<xiiUInt32>(uiCount));
        if (reader.GetError().Failed())
        {
          return reader.GetError();
        }
        auto error = budget.Reserve(2 * uiCount + 64);
        if (error.Failed())
        {
          return error;
        }
        xiiDataBuffer object;
        object     = bytes;
        ref_object = std::move(object);
        break;
      }
      case xiiNetworkValueKind::ColorGamma:
      {
        xiiColorGammaUB object;
        object.r   = reader.U8();
        object.g   = reader.U8();
        object.b   = reader.U8();
        object.a   = reader.U8();
        ref_object = object;
        break;
      }
      case xiiNetworkValueKind::Angle:
        ref_object = xiiAngle::MakeFromRadian(ReadNumber<float>(reader, policy));
        break;
      case xiiNetworkValueKind::Angled:
        ref_object = xiiAngled::MakeFromRadian(ReadNumber<double>(reader, policy));
        break;
      case xiiNetworkValueKind::Time:
        ref_object = xiiTime::MakeFromSeconds(ReadNumber<double>(reader, policy));
        break;
      case xiiNetworkValueKind::Timestamp:
      {
        const auto iValue = reader.VarInt();
        if (iValue == std::numeric_limits<xiiInt64>::min())
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
        }
        ref_object = iValue;
        break;
      }
      case xiiNetworkValueKind::Uuid:
      {
        const auto uiHigh = reader.U64();
        const auto uiLow  = reader.U64();
        ref_object        = xiiUuid(uiLow, uiHigh);
        break;
      }
      case xiiNetworkValueKind::Variant:
        return ReadDynamic(ref_object, policy, reader, encoding, budget, uiDepth + 1);
      case xiiNetworkValueKind::VariantArray:
      {
        const auto uiCount = reader.VarUInt();
        if (uiCount > xiiMath::Min(limits.m_uiMaximumArrayElements, policy.m_uiMaximumElements))
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
        }
        auto error = budget.Reserve(uiCount * sizeof(xiiVariant) * 2 + 64);
        if (error.Failed())
        {
          return error;
        }
        xiiVariantArray values;
        values.SetCount(static_cast<xiiUInt32>(uiCount));
        for (auto& item : values)
        {
          error = ReadDynamic(item, policy, reader, encoding, budget, uiDepth + 1);
          if (error.Failed())
          {
            return error;
          }
        }
        ref_object = std::move(values);
        break;
      }
      case xiiNetworkValueKind::VariantDictionary:
      {
        const auto uiCount = reader.VarUInt();
        if (uiCount > xiiMath::Min(limits.m_uiMaximumMapEntries, policy.m_uiMaximumElements))
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
        }
        auto error = budget.Reserve(uiCount * (sizeof(xiiVariant) + sizeof(xiiString) + 32) * 4 + 64);
        if (error.Failed())
        {
          return error;
        }
        xiiVariantDictionary values;
        xiiStringView        previous;
        for (xiiUInt64 uiIndex = 0; uiIndex < uiCount; ++uiIndex)
        {
          const auto key = reader.Text(xiiMath::Min(limits.m_uiMaximumStringBytes, policy.m_uiMaximumStringBytes));
          if (reader.GetError().Failed())
          {
            return reader.GetError();
          }
          if (uiIndex != 0 && key.Compare(previous) <= 0)
          {
            return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
          }
          previous = key;
          error    = budget.Reserve(2ULL * (key.GetElementCount() + 1ULL));
          if (error.Failed())
          {
            return error;
          }
          xiiVariant item;
          error = ReadDynamic(item, policy, reader, encoding, budget, uiDepth + 1);
          if (error.Failed())
          {
            return error;
          }
          values.Insert(xiiString(key), std::move(item));
        }
        ref_object = std::move(values);
        break;
      }
      case xiiNetworkValueKind::Structure:
      {
        if (!ref_object.IsValid() || ref_object.GetReflectedType() != value.m_pType || ref_object.GetType() != xiiVariantType::TypedObject)
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidArgument);
        }
        return ReadObject(*value.m_pNested, ref_object.GetWriteAccess().m_pObject, reader, encoding, budget, uiDepth + 1);
      }
      case xiiNetworkValueKind::Custom:
      {
        const auto uiBytes = reader.U32();
        const auto bytes   = reader.Bytes(uiBytes);
        if (reader.GetError().Failed())
        {
          return reader.GetError();
        }
        xiiNetworkBinaryReader bounded(bytes);
        xiiVariant             result;
        auto                   error = value.m_pCodec->Decode(bounded, budget, result);
        if (error.Failed())
        {
          return error;
        }
        if (!bounded.IsFinished() || ValueType(result) != value.m_pType || result.GetType() == xiiVariantType::TypedPointer)
        {
          return xiiNetworkDataError(xiiNetworkDataErrorCode::CustomCodecFailure);
        }
        ref_object = std::move(result);
        break;
      }
      default:
        return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
    }
    if (reader.GetError().Failed())
    {
      return reader.GetError();
    }
    if (value.m_bEnumeration && !ValidEnum(value, ref_object.Get<xiiInt64>()))
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
    }
    return {};
  }
} // namespace

namespace xiiNetworkSerializationInternal
{
  const xiiRTTI* ValueType(const xiiVariant& value)
  {
    if (value.GetType() == xiiVariantType::VariantArray)
    {
      return xiiGetStaticRTTI<xiiVariantArray>();
    }
    if (value.GetType() == xiiVariantType::VariantDictionary)
    {
      return xiiGetStaticRTTI<xiiVariantDictionary>();
    }
    return value.GetReflectedType();
  }

  xiiNetworkDataError WriteValue(const CompiledValue& value, const xiiVariant& object, const xiiNetworkFieldSchema& policy,
                                 xiiNetworkBinaryWriter& writer, xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth)
  {
    if (value.m_Kind == xiiNetworkValueKind::Timestamp && object.IsA<xiiInt64>())
    {
      writer.VarInt(object.Get<xiiInt64>());
      return writer.GetError();
    }
    if (value.m_Kind == xiiNetworkValueKind::Variant)
    {
      return WriteDynamic(object, policy, writer, encoding, limits, uiDepth + 1);
    }
    if (value.m_bEnumeration)
    {
      if (!object.IsA<xiiInt64>() || !ValidEnum(value, object.Get<xiiInt64>()))
      {
        return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
      }
    }
    else if (value.m_Kind == xiiNetworkValueKind::String && object.IsA<xiiString>())
    {
      writer.Text(object.Get<xiiString>(), xiiMath::Min(limits.m_uiMaximumStringBytes, policy.m_uiMaximumStringBytes));
      return writer.GetError();
    }
    else if ((ValueType(object) != value.m_pType &&
              !(value.m_Kind == xiiNetworkValueKind::VariantArray && object.IsA<xiiVariantArray>()) &&
              !(value.m_Kind == xiiNetworkValueKind::VariantDictionary && object.IsA<xiiVariantDictionary>())) ||
             !object.IsValid())
    {
      return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidFieldType);
    }
    return WriteNative(value, object.GetData(), policy, writer, encoding, limits, uiDepth);
  }
  xiiNetworkDataError ReadValue(const CompiledValue& value, xiiVariant& ref_object, const xiiNetworkFieldSchema& policy,
                                xiiNetworkBinaryReader& reader, xiiNetworkEncoding encoding, xiiNetworkDecodeBudget& budget, xiiUInt32 uiDepth)
  {
    return ReadNative(value, ref_object, policy, reader, encoding, budget, uiDepth);
  }

  xiiNetworkDataError WriteBorrowedValue(const CompiledValue& value, const void* pObject, const xiiNetworkFieldSchema& policy,
                                         xiiNetworkBinaryWriter& writer, xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth)
  {
    return WriteNative(value, pObject, policy, writer, encoding, limits, uiDepth);
  }
} // namespace xiiNetworkSerializationInternal
