/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Reflection.h>

#include <initializer_list>
#include <utility>

namespace
{
  bool IsExposedMember(const xiiAbstractProperty& property)
  {
    return property.GetCategory() == xiiPropertyCategory::Member && !property.GetFlags().IsAnySet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner) &&
      property.GetAttributeByType<xiiHiddenAttribute>() == nullptr && property.GetAttributeByType<xiiTemporaryAttribute>() == nullptr;
  }

  const char* GetJsonType(xiiVariantType::Enum type)
  {
    switch (type)
    {
      case xiiVariantType::Bool:
        return "boolean";
      case xiiVariantType::Int8:
      case xiiVariantType::UInt8:
      case xiiVariantType::Int16:
      case xiiVariantType::UInt16:
      case xiiVariantType::Int32:
      case xiiVariantType::UInt32:
      case xiiVariantType::Int64:
      case xiiVariantType::UInt64:
        return "integer";
      case xiiVariantType::Float:
      case xiiVariantType::Double:
      case xiiVariantType::Time:
      case xiiVariantType::Angle:
      case xiiVariantType::Angled:
        return "number";
      case xiiVariantType::String:
      case xiiVariantType::StringView:
      case xiiVariantType::Uuid:
      case xiiVariantType::HashedString:
        return "string";
      case xiiVariantType::Vector2:
      case xiiVariantType::Vector3:
      case xiiVariantType::Vector4:
      case xiiVariantType::Quaternion:
      case xiiVariantType::Color:
        return "array";
      default:
        return nullptr;
    }
  }

  bool IsRuntimeEditable(const xiiAbstractProperty& property)
  {
    if (!IsExposedMember(property) || property.GetFlags().IsAnySet(xiiPropertyFlags::ReadOnly | xiiPropertyFlags::Bitflags) ||
        property.GetAttributeByType<xiiReadOnlyAttribute>() != nullptr || property.GetAttributeByType<xiiRuntimeEditableAttribute>() == nullptr)
    {
      return false;
    }

    if (property.GetFlags().IsSet(xiiPropertyFlags::IsEnum))
    {
      return true;
    }

    const auto type = property.GetSpecificType()->GetVariantType();
    if (!xiiReflectionUtils::IsValueType(&property) || GetJsonType(type) == nullptr)
    {
      return false;
    }

    return type != xiiVariantType::StringView && type != xiiVariantType::Uuid && type != xiiVariantType::HashedString &&
      type != xiiVariantType::Time && type != xiiVariantType::Angle && type != xiiVariantType::Angled;
  }

  xiiVariantArray Components(std::initializer_list<double> values)
  {
    xiiVariantArray result;
    for (double fValue : values)
    {
      result.PushBack(xiiMath::IsFinite(fValue) ? xiiVariant(fValue) : xiiVariant());
    }
    return result;
  }

  xiiVariantDictionary DescribeType(const xiiRTTI& type, xiiUInt32 uiDepth, bool bRoot)
  {
    xiiVariantDictionary result;
    result["type"]  = xiiString("object");
    result["title"] = xiiString(type.GetTypeName());
    xiiVariantDictionary members;
    if (uiDepth != 0)
    {
      xiiHybridArray<const xiiAbstractProperty*, 32> properties;
      type.GetAllProperties(properties);
      for (const auto* pProperty : properties)
      {
        if (!IsExposedMember(*pProperty) || members.GetCount() >= 64)
        {
          continue;
        }
        const auto*          pType = pProperty->GetSpecificType();
        const auto           flags = pProperty->GetFlags();
        xiiVariantDictionary schema;
        if (flags.IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
        {
          schema["type"] = xiiString("string");
          xiiDynamicArray<xiiReflectionUtils::EnumKeyValuePair> keys;
          xiiReflectionUtils::GetEnumKeysAndValues(pType, keys);
          xiiVariantArray names;
          for (const auto& entry : keys)
          {
            names.PushBack(xiiString(entry.m_sKey));
          }
          schema[flags.IsSet(xiiPropertyFlags::IsEnum) ? "enum" : "x-xii-flags"] = std::move(names);
        }
        else if (const char* szJsonType = GetJsonType(pType->GetVariantType()))
        {
          schema["type"] = xiiString(szJsonType);
        }
        else if (flags.IsSet(xiiPropertyFlags::Class))
        {
          schema = DescribeType(*pType, uiDepth - 1, false);
        }
        else
        {
          continue;
        }
        if (bRoot && IsRuntimeEditable(*pProperty))
        {
          schema["x-xii-runtime-editable"] = true;
          if (const auto* pClamp = pProperty->GetAttributeByType<xiiClampValueAttribute>())
          {
            if (pClamp->GetMinValue().IsNumber())
            {
              schema["minimum"] = pClamp->GetMinValue();
            }
            if (pClamp->GetMaxValue().IsNumber())
            {
              schema["maximum"] = pClamp->GetMaxValue();
            }
          }
        }
        schema["readOnly"]                    = flags.IsSet(xiiPropertyFlags::ReadOnly);
        members[pProperty->GetPropertyName()] = std::move(schema);
      }
    }
    result["properties"] = std::move(members);
    return result;
  }

  xiiVariantDictionary ReadObject(const xiiRTTI& type, const void* pObject, xiiUInt32 uiDepth)
  {
    xiiVariantDictionary result;
    if (uiDepth == 0)
    {
      return result;
    }

    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    type.GetAllProperties(properties);
    for (const auto* pProperty : properties)
    {
      if (!IsExposedMember(*pProperty) || result.GetCount() >= 64)
      {
        continue;
      }
      const auto* pMember = static_cast<const xiiAbstractMemberProperty*>(pProperty);
      const auto  flags   = pProperty->GetFlags();
      xiiVariant  propertyValue;
      if (flags.IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
      {
        const auto       value = xiiReflectionUtils::GetMemberPropertyValue(pMember, pObject);
        xiiStringBuilder sValue;
        if (xiiReflectionUtils::EnumerationToString(pProperty->GetSpecificType(), value.ConvertTo<xiiInt64>(), sValue))
        {
          result[pProperty->GetPropertyName()] = xiiString(sValue);
        }
      }
      else if (GetJsonType(pProperty->GetSpecificType()->GetVariantType()) != nullptr && xiiReflectionUtils::IsValueType(pProperty))
      {
        if (xiiMCPReflection::ConvertValue(xiiReflectionUtils::GetMemberPropertyValue(pMember, pObject), propertyValue))
        {
          result[pProperty->GetPropertyName()] = std::move(propertyValue);
        }
      }
      else if (flags.IsSet(xiiPropertyFlags::Class))
      {
        if (const void* pNested = pMember->GetPropertyPointer(pObject))
        {
          result[pProperty->GetPropertyName()] = ReadObject(*pProperty->GetSpecificType(), pNested, uiDepth - 1);
        }
      }
    }
    return result;
  }

  template <typename ValueType>
  bool ConvertInteger(const xiiVariant& json, xiiVariant& out_value)
  {
    if (json.IsA<xiiInt64>())
    {
      const auto iValue = json.Get<xiiInt64>();
      if (!std::in_range<ValueType>(iValue))
      {
        return false;
      }
      out_value = static_cast<ValueType>(iValue);
      return true;
    }

    xiiUInt64 uiValue = 0;
    if (!xiiMCPJson::ReadUInt(json, uiValue) || !std::in_range<ValueType>(uiValue))
    {
      return false;
    }
    out_value = static_cast<ValueType>(uiValue);
    return true;
  }
} // namespace

xiiVariantDictionary xiiMCPReflection::Describe(const xiiRTTI& type)
{
  return DescribeType(type, 3, true);
}

xiiVariantDictionary xiiMCPReflection::ReadMembers(const xiiRTTI& type, const void* pObject)
{
  return ReadObject(type, pObject, 3);
}

bool xiiMCPReflection::ConvertValue(const xiiVariant& value, xiiVariant& out_json)
{
  switch (value.GetType())
  {
    case xiiVariantType::Vector2:
    {
      const auto& v = value.Get<xiiVec2>();
      out_json      = Components({v.x, v.y});
      return true;
    }
    case xiiVariantType::Vector3:
    {
      const auto& v = value.Get<xiiVec3>();
      out_json      = Components({v.x, v.y, v.z});
      return true;
    }
    case xiiVariantType::Vector4:
    {
      const auto& v = value.Get<xiiVec4>();
      out_json      = Components({v.x, v.y, v.z, v.w});
      return true;
    }
    case xiiVariantType::Quaternion:
    {
      const auto& q = value.Get<xiiQuat>();
      out_json      = Components({q.x, q.y, q.z, q.w});
      return true;
    }
    case xiiVariantType::Color:
    {
      const auto& color = value.Get<xiiColor>();
      out_json          = Components({color.r, color.g, color.b, color.a});
      return true;
    }
    case xiiVariantType::Uuid:
    {
      xiiStringBuilder sGuid;
      xiiConversionUtils::ToString(value.Get<xiiUuid>(), sGuid);
      out_json = xiiString(sGuid);
      return true;
    }
    case xiiVariantType::Time:
      out_json = value.Get<xiiTime>().GetSeconds();
      return true;
    case xiiVariantType::Angle:
      out_json = value.Get<xiiAngle>().GetDegree();
      return true;
    case xiiVariantType::Angled:
      out_json = value.Get<xiiAngled>().GetDegree();
      return true;
    case xiiVariantType::HashedString:
    case xiiVariantType::StringView:
      out_json = xiiString(value.ConvertTo<xiiStringView>());
      return true;
    default:
      if (GetJsonType(value.GetType()) == nullptr || (value.IsNumber() && !xiiMath::IsFinite(value.ConvertTo<double>())))
      {
        return false;
      }
      out_json = value;
      return true;
  }
}

bool xiiMCPReflection::ReadFloatArray(const xiiVariant& json, xiiArrayPtr<float> out_values)
{
  if (!json.IsA<xiiVariantArray>() || json.Get<xiiVariantArray>().GetCount() != out_values.GetCount())
  {
    return false;
  }

  const auto& elements = json.Get<xiiVariantArray>();
  for (xiiUInt32 uiIndex = 0; uiIndex < elements.GetCount(); ++uiIndex)
  {
    const auto& value = elements[uiIndex];
    if (!value.IsNumber() || value.IsA<bool>() || !xiiMath::IsInRange(value.ConvertTo<double>(), -1.0e8, 1.0e8))
    {
      return false;
    }
    out_values[uiIndex] = value.ConvertTo<float>();
  }
  return true;
}

bool xiiMCPReflection::ConvertArgument(const xiiAbstractProperty& property, const xiiVariant& json, xiiVariant& out_value)
{
  if (!IsExposedMember(property) || property.GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
  {
    return false;
  }

  if (property.GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
  {
    xiiInt64 iValue = 0;
    if (!json.IsA<xiiString>() || !xiiReflectionUtils::StringToEnumeration(property.GetSpecificType(), json.Get<xiiString>(), iValue))
    {
      return false;
    }
    out_value = iValue;
    return true;
  }

  const auto type = property.GetSpecificType()->GetVariantType();
  switch (type)
  {
    case xiiVariantType::Int8:
      return ConvertInteger<xiiInt8>(json, out_value);
    case xiiVariantType::UInt8:
      return ConvertInteger<xiiUInt8>(json, out_value);
    case xiiVariantType::Int16:
      return ConvertInteger<xiiInt16>(json, out_value);
    case xiiVariantType::UInt16:
      return ConvertInteger<xiiUInt16>(json, out_value);
    case xiiVariantType::Int32:
      return ConvertInteger<xiiInt32>(json, out_value);
    case xiiVariantType::UInt32:
      return ConvertInteger<xiiUInt32>(json, out_value);
    case xiiVariantType::Int64:
      return ConvertInteger<xiiInt64>(json, out_value);
    case xiiVariantType::UInt64:
      return ConvertInteger<xiiUInt64>(json, out_value);
    case xiiVariantType::Float:
    case xiiVariantType::Double:
      if (!json.IsNumber() || json.IsA<bool>() || !xiiMath::IsFinite(json.ConvertTo<double>()) ||
          (type == xiiVariantType::Float && xiiMath::Abs(json.ConvertTo<double>()) > xiiMath::MaxValue<float>()))
      {
        return false;
      }
      out_value = json.ConvertTo(type);
      return true;
    case xiiVariantType::Bool:
    case xiiVariantType::String:
      if (json.GetType() != type)
      {
        return false;
      }
      out_value = json;
      return true;
    case xiiVariantType::Vector2:
    case xiiVariantType::Vector3:
    case xiiVariantType::Vector4:
    case xiiVariantType::Color:
    case xiiVariantType::Quaternion:
    {
      float           fValues[4] = {};
      const xiiUInt32 uiCount    = type == xiiVariantType::Vector2 ? 2 : type == xiiVariantType::Vector3 ? 3 :
                                                                                                           4;
      if (!ReadFloatArray(json, xiiArrayPtr<float>(fValues, uiCount)))
      {
        return false;
      }
      switch (type)
      {
        case xiiVariantType::Vector2:
          out_value = xiiVec2(fValues[0], fValues[1]);
          return true;
        case xiiVariantType::Vector3:
          out_value = xiiVec3(fValues[0], fValues[1], fValues[2]);
          return true;
        case xiiVariantType::Vector4:
          out_value = xiiVec4(fValues[0], fValues[1], fValues[2], fValues[3]);
          return true;
        case xiiVariantType::Color:
          out_value = xiiColor(fValues[0], fValues[1], fValues[2], fValues[3]);
          return true;
        default:
        {
          const auto qRotation = xiiQuat::MakeFromElements(fValues[0], fValues[1], fValues[2], fValues[3]);
          if (!qRotation.IsValid())
          {
            return false;
          }
          out_value = qRotation;
          return true;
        }
      }
    }
    default:
      return false;
  }
}

xiiStatus xiiMCPReflection::SetRuntimeMember(const xiiRTTI& type, void* pObject, xiiStringView sProperty, const xiiVariant& json)
{
  const auto* pProperty = type.FindPropertyByName(sProperty);
  if (pProperty == nullptr || !IsRuntimeEditable(*pProperty))
  {
    return xiiStatus("Use a property marked x-xii-runtime-editable in the component schema.");
  }

  xiiVariant value;
  if (!ConvertArgument(*pProperty, json, value))
  {
    return xiiStatus("The value does not match the reflected property type.");
  }

  if (const auto* pClamp = pProperty->GetAttributeByType<xiiClampValueAttribute>())
  {
    xiiVariant clamped = value;
    if (xiiReflectionUtils::ClampValue(clamped, pClamp).Failed() || clamped != value)
    {
      return xiiStatus("The value is outside the reflected property bounds.");
    }
  }

  xiiReflectionUtils::SetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pProperty), pObject, value);
  return XII_SUCCESS;
}
