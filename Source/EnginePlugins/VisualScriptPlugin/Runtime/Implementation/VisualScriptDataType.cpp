#include <Core/CorePCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/World.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVisualScriptDataType, 1)
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Invalid),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Bool),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Byte),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Int),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Int64),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Float),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Double),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Color),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Vector3),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Quaternion),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Transform),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Time),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Angle),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::String),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::HashedString),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::GameObject),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Component),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::TypedPointer),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Variant),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Array),
  XII_ENUM_CONSTANT(xiiVisualScriptDataType::Map),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

namespace
{
  static constexpr xiiVariantType::Enum s_ScriptDataTypeVariantTypes[] = {
    xiiVariantType::Invalid, // Invalid,

    xiiVariantType::Bool,              // Bool,
    xiiVariantType::UInt8,             // Byte,
    xiiVariantType::Int32,             // Int,
    xiiVariantType::Int64,             // Int64,
    xiiVariantType::Float,             // Float,
    xiiVariantType::Double,            // Double,
    xiiVariantType::Color,             // Color,
    xiiVariantType::Vector3,           // Vector3,
    xiiVariantType::Quaternion,        // Quaternion,
    xiiVariantType::Transform,         // Transform,
    xiiVariantType::Time,              // Time,
    xiiVariantType::Angle,             // Angle,
    xiiVariantType::String,            // String,
    xiiVariantType::HashedString,      // HashedString,
    xiiVariantType::TypedObject,       // GameObject,
    xiiVariantType::TypedObject,       // Component,
    xiiVariantType::TypedPointer,      // TypedPointer,
    xiiVariantType::Invalid,           // Variant,
    xiiVariantType::VariantArray,      // Array,
    xiiVariantType::VariantDictionary, // Map,
    xiiVariantType::TypedObject,       // Coroutine,
  };
  static_assert(XII_ARRAY_SIZE(s_ScriptDataTypeVariantTypes) == (size_t)xiiVisualScriptDataType::Count);

  static constexpr xiiUInt32 s_ScriptDataTypeSizes[] = {
    0, // Invalid,

    sizeof(bool),                            // Bool,
    sizeof(xiiUInt8),                        // Byte,
    sizeof(xiiInt32),                        // Int,
    sizeof(xiiInt64),                        // Int64,
    sizeof(float),                           // Float,
    sizeof(double),                          // Double,
    sizeof(xiiColor),                        // Color,
    sizeof(xiiVec3),                         // Vector3,
    sizeof(xiiQuat),                         // Quaternion,
    sizeof(xiiTransform),                    // Transform,
    sizeof(xiiTime),                         // Time,
    sizeof(xiiAngle),                        // Angle,
    sizeof(xiiString),                       // String,
    sizeof(xiiHashedString),                 // HashedString,
    sizeof(xiiVisualScriptGameObjectHandle), // GameObject,
    sizeof(xiiVisualScriptComponentHandle),  // Component,
    sizeof(xiiTypedPointer),                 // TypedPointer,
    sizeof(xiiVariant),                      // Variant,
    sizeof(xiiVariantArray),                 // Array,
    sizeof(xiiVariantDictionary),            // Map,
    sizeof(xiiScriptCoroutineHandle),        // Coroutine,
  };
  static_assert(XII_ARRAY_SIZE(s_ScriptDataTypeSizes) == (size_t)xiiVisualScriptDataType::Count);

  static constexpr xiiUInt32 s_ScriptDataTypeAlignments[] = {
    0, // Invalid,

    alignof(bool),                            // Bool,
    alignof(xiiUInt8),                        // Byte,
    alignof(xiiInt32),                        // Int,
    alignof(xiiInt64),                        // Int64,
    alignof(float),                           // Float,
    alignof(double),                          // Double,
    alignof(xiiColor),                        // Color,
    alignof(xiiVec3),                         // Vector3,
    alignof(xiiQuat),                         // Quaternion,
    alignof(xiiTransform),                    // Transform,
    alignof(xiiTime),                         // Time,
    alignof(xiiAngle),                        // Angle,
    alignof(xiiString),                       // String,
    alignof(xiiHashedString),                 // HashedString,
    alignof(xiiVisualScriptGameObjectHandle), // GameObject,
    alignof(xiiVisualScriptComponentHandle),  // Component,
    alignof(xiiTypedPointer),                 // TypedPointer,
    alignof(xiiVariant),                      // Variant,
    alignof(xiiVariantArray),                 // Array,
    alignof(xiiVariantDictionary),            // Map,
    alignof(xiiScriptCoroutineHandle),        // Coroutine,
  };
  static_assert(XII_ARRAY_SIZE(s_ScriptDataTypeAlignments) == (size_t)xiiVisualScriptDataType::Count);

  static constexpr const char* s_ScriptDataTypeNames[] = {
    "Invalid",

    "Bool",
    "Byte",
    "Int",
    "Int64",
    "Float",
    "Double",
    "Color",
    "Vector3",
    "Quaternion",
    "Transform",
    "Time",
    "Angle",
    "String",
    "HashedString",
    "GameObject",
    "Component",
    "TypedPointer",
    "Variant",
    "Array",
    "Map",
    "Coroutine",
    "", // Count,
    "Enum",
    "Bitflag",
    "Resource",
  };
  static_assert(XII_ARRAY_SIZE(s_ScriptDataTypeNames) == (size_t)xiiVisualScriptDataType::ExtendedCount);
} // namespace

// static
xiiVariantType::Enum xiiVisualScriptDataType::GetVariantType(Enum dataType)
{
  XII_ASSERT_DEBUG(dataType >= 0 && dataType < XII_ARRAY_SIZE(s_ScriptDataTypeVariantTypes), "Out of bounds access");
  return s_ScriptDataTypeVariantTypes[dataType];
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptDataType::FromVariantType(xiiVariantType::Enum variantType)
{
  switch (variantType)
  {
    case xiiVariantType::Bool:
      return Bool;
    case xiiVariantType::Int8:
    case xiiVariantType::UInt8:
      return Byte;
    case xiiVariantType::Int16:
    case xiiVariantType::UInt16:
    case xiiVariantType::Int32:
    case xiiVariantType::UInt32:
      return Int;
    case xiiVariantType::Int64:
    case xiiVariantType::UInt64:
      return Int64;
    case xiiVariantType::Float:
      return Float;
    case xiiVariantType::Double:
      return Double;
    case xiiVariantType::Color:
      return Color;
    case xiiVariantType::Vector3:
      return Vector3;
    case xiiVariantType::Quaternion:
      return Quaternion;
    case xiiVariantType::Transform:
      return Transform;
    case xiiVariantType::Time:
      return Time;
    case xiiVariantType::Angle:
      return Angle;
    case xiiVariantType::String:
    case xiiVariantType::StringView:
      return String;
    case xiiVariantType::HashedString:
    case xiiVariantType::TempHashedString:
      return HashedString;
    case xiiVariantType::VariantArray:
      return Array;
    case xiiVariantType::VariantDictionary:
      return Map;
    default:
      return Invalid;
  }
}

xiiProcessingStream::DataType xiiVisualScriptDataType::GetStreamDataType(Enum dataType)
{
  // We treat xiiColor and xiiVec4 as the same in the visual script <=> expression binding
  // so ensure that they have the same size and layout
  static_assert(sizeof(xiiColor) == sizeof(xiiVec4));
  static_assert(offsetof(xiiColor, r) == offsetof(xiiVec4, x));
  static_assert(offsetof(xiiColor, g) == offsetof(xiiVec4, y));
  static_assert(offsetof(xiiColor, b) == offsetof(xiiVec4, z));
  static_assert(offsetof(xiiColor, a) == offsetof(xiiVec4, w));

  switch (dataType)
  {
    case Int:
      return xiiProcessingStream::DataType::Int;
    case Float:
      return xiiProcessingStream::DataType::Float;
    case Vector3:
      return xiiProcessingStream::DataType::Float3;
    case Color:
      return xiiProcessingStream::DataType::Float4;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  return xiiProcessingStream::DataType::Float;
}

// static
const xiiRTTI* xiiVisualScriptDataType::GetRtti(Enum dataType)
{
  // Define table here to prevent issues with static initialization order
  static const xiiRTTI* s_Rttis[] = {
    nullptr, // Invalid,

    xiiGetStaticRTTI<bool>(),                     // Bool,
    xiiGetStaticRTTI<xiiUInt8>(),                 // Byte,
    xiiGetStaticRTTI<xiiInt32>(),                 // Int,
    xiiGetStaticRTTI<xiiInt64>(),                 // Int64,
    xiiGetStaticRTTI<float>(),                    // Float,
    xiiGetStaticRTTI<double>(),                   // Double,
    xiiGetStaticRTTI<xiiColor>(),                 // Color,
    xiiGetStaticRTTI<xiiVec3>(),                  // Vector3,
    xiiGetStaticRTTI<xiiQuat>(),                  // Quaternion,
    xiiGetStaticRTTI<xiiTransform>(),             // Transform,
    xiiGetStaticRTTI<xiiTime>(),                  // Time,
    xiiGetStaticRTTI<xiiAngle>(),                 // Angle,
    xiiGetStaticRTTI<xiiString>(),                // String,
    xiiGetStaticRTTI<xiiHashedString>(),          // HashedString,
    xiiGetStaticRTTI<xiiGameObjectHandle>(),      // GameObject,
    xiiGetStaticRTTI<xiiComponentHandle>(),       // Component,
    nullptr,                                      // TypedPointer,
    xiiGetStaticRTTI<xiiVariant>(),               // Variant,
    xiiGetStaticRTTI<xiiVariantArray>(),          // Array,
    xiiGetStaticRTTI<xiiVariantDictionary>(),     // Map,
    xiiGetStaticRTTI<xiiScriptCoroutineHandle>(), // Coroutine,
    nullptr,                                      // Count,
    nullptr,                                      // EnumValue,
    nullptr,                                      // BitflagValue,
    nullptr,                                      // Resource,
  };
  static_assert(XII_ARRAY_SIZE(s_Rttis) == (size_t)xiiVisualScriptDataType::ExtendedCount);

  XII_ASSERT_DEBUG(dataType >= 0 && dataType < XII_ARRAY_SIZE(s_Rttis), "Out of bounds access");
  return s_Rttis[dataType];
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptDataType::FromRtti(const xiiRTTI* pRtti)
{
  Enum res = FromVariantType(pRtti->GetVariantType());
  if (res != Invalid)
    return res;

  if (pRtti->IsDerivedFrom<xiiGameObject>() || pRtti == xiiGetStaticRTTI<xiiGameObjectHandle>())
    return GameObject;

  if (pRtti->IsDerivedFrom<xiiComponent>() || pRtti == xiiGetStaticRTTI<xiiComponentHandle>())
    return Component;

  if (pRtti == xiiGetStaticRTTI<xiiScriptCoroutineHandle>())
    return Coroutine;

  if (pRtti->GetTypeFlags().IsSet(xiiTypeFlags::Class))
    return TypedPointer;

  if (pRtti->GetTypeFlags().IsSet(xiiTypeFlags::IsEnum))
    return EnumValue;

  if (pRtti->GetTypeFlags().IsSet(xiiTypeFlags::Bitflags))
    return BitflagValue;

  if (pRtti == xiiGetStaticRTTI<xiiVariant>())
    return Variant;

  return Invalid;
}

// static
xiiUInt32 xiiVisualScriptDataType::GetStorageSize(Enum dataType)
{
  XII_ASSERT_DEBUG(dataType >= 0 && dataType < XII_ARRAY_SIZE(s_ScriptDataTypeSizes), "Out of bounds access");
  return s_ScriptDataTypeSizes[dataType];
}

// static
xiiUInt32 xiiVisualScriptDataType::GetStorageAlignment(Enum dataType)
{
  XII_ASSERT_DEBUG(dataType >= 0 && dataType < XII_ARRAY_SIZE(s_ScriptDataTypeAlignments), "Out of bounds access");
  return s_ScriptDataTypeAlignments[dataType];
}

// static
const char* xiiVisualScriptDataType::GetName(Enum dataType)
{
  if (dataType == AnyPointer)
  {
    return "Pointer";
  }
  else if (dataType == Any)
  {
    return "Any";
  }

  XII_ASSERT_DEBUG(dataType >= 0 && dataType < XII_ARRAY_SIZE(s_ScriptDataTypeNames), "Out of bounds access");
  return s_ScriptDataTypeNames[dataType];
}

// static
bool xiiVisualScriptDataType::CanConvertTo(Enum sourceDataType, Enum targetDataType)
{
  if (sourceDataType == targetDataType ||
      sourceDataType == Any ||
      targetDataType == Any ||
      targetDataType == String ||
      targetDataType == HashedString ||
      targetDataType == Variant)
    return true;

  if ((IsNumberOrBool(sourceDataType) || (sourceDataType == EnumValue || sourceDataType == BitflagValue)) &&
      (IsNumberOrBool(targetDataType) || (targetDataType == EnumValue || targetDataType == BitflagValue)))
    return true;

  if ((IsNumberOrBool(sourceDataType) && targetDataType == Vector3) ||
      (sourceDataType == Vector3 && targetDataType == Transform))
    return true;

  if (IsPointer(sourceDataType) &&
      (targetDataType == xiiVisualScriptDataType::AnyPointer || targetDataType == xiiVisualScriptDataType::Bool))
    return true;

  return false;
}

//////////////////////////////////////////////////////////////////////////

xiiGameObject* xiiVisualScriptGameObjectHandle::GetPtr(xiiUInt32 uiExecutionCounter) const
{
  if (m_uiExecutionCounter == uiExecutionCounter || m_Handle.GetInternalID().m_Data == 0)
  {
    return m_Ptr;
  }

  m_Ptr                = nullptr;
  m_uiExecutionCounter = uiExecutionCounter;

  if (xiiWorld* pWorld = xiiWorld::GetWorld(m_Handle))
  {
    bool objectExists = pWorld->TryGetObject(m_Handle, m_Ptr);
    XII_IGNORE_UNUSED(objectExists);
  }

  return m_Ptr;
}

xiiComponent* xiiVisualScriptComponentHandle::GetPtr(xiiUInt32 uiExecutionCounter) const
{
  if (m_uiExecutionCounter == uiExecutionCounter || m_Handle.GetInternalID().m_Data == 0)
  {
    return m_Ptr;
  }

  m_Ptr                = nullptr;
  m_uiExecutionCounter = uiExecutionCounter;

  if (xiiWorld* pWorld = xiiWorld::GetWorld(m_Handle))
  {
    bool componentExists = pWorld->TryGetComponent(m_Handle, m_Ptr);
    XII_IGNORE_UNUSED(componentExists);
  }

  return m_Ptr;
}

XII_STATICLINK_FILE(VisualScriptPlugin, VisualScriptPlugin_Runtime_VisualScriptDataType);
