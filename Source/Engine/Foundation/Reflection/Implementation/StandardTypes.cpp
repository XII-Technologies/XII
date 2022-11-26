#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/Reflection/Reflection.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiEnumBase, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiBitflagsBase, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectedClass, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// *********************************************
// ***** Standard POD Types for Properties *****

XII_BEGIN_STATIC_REFLECTED_TYPE(bool, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(float, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(double, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiInt8, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUInt8, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiInt16, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUInt16, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiInt32, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUInt32, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiInt64, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUInt64, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiConstCharPtr, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTime, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(Nanoseconds),
    XII_FUNCTION_PROPERTY(Microseconds),
    XII_FUNCTION_PROPERTY(Milliseconds),
    XII_FUNCTION_PROPERTY(Seconds),
    XII_FUNCTION_PROPERTY(Zero),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiColor, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("r", r),
    XII_MEMBER_PROPERTY("g", g),
    XII_MEMBER_PROPERTY("b", b),
    XII_MEMBER_PROPERTY("a", a),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float, float, float),
    XII_CONSTRUCTOR_PROPERTY(float, float, float, float),
    XII_CONSTRUCTOR_PROPERTY(xiiColorLinearUB),
    XII_CONSTRUCTOR_PROPERTY(xiiColorGammaUB),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiColorBaseUB, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("r", r),
    XII_MEMBER_PROPERTY("g", g),
    XII_MEMBER_PROPERTY("b", b),
    XII_MEMBER_PROPERTY("a", a),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8, xiiUInt8),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiColorGammaUB, xiiColorBaseUB, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8, xiiUInt8),
    XII_CONSTRUCTOR_PROPERTY(const xiiColor&),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiColorLinearUB, xiiColorBaseUB, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8, xiiUInt8),
    XII_CONSTRUCTOR_PROPERTY(const xiiColor&),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec2, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float),
    XII_CONSTRUCTOR_PROPERTY(float, float),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec3, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float),
    XII_CONSTRUCTOR_PROPERTY(float, float, float),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec4, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float),
    XII_CONSTRUCTOR_PROPERTY(float, float, float, float),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec2I32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiInt32, xiiInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec3I32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiInt32, xiiInt32, xiiInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec4I32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiInt32, xiiInt32, xiiInt32, xiiInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec2U32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32, xiiUInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec3U32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32, xiiUInt32, xiiUInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec4U32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32, xiiUInt32, xiiUInt32, xiiUInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiQuat, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("v", v),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float, float, float, float),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMat3, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMat4, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTransform, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Position", m_vPosition),
    XII_MEMBER_PROPERTY("Rotation", m_qRotation),
    XII_MEMBER_PROPERTY("Scale", m_vScale),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiVec3, xiiQuat),
    XII_CONSTRUCTOR_PROPERTY(xiiVec3, xiiQuat, xiiVec3),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiBasisAxis, 1)
XII_ENUM_CONSTANT(xiiBasisAxis::PositiveX),
XII_ENUM_CONSTANT(xiiBasisAxis::PositiveY),
XII_ENUM_CONSTANT(xiiBasisAxis::PositiveZ),
XII_ENUM_CONSTANT(xiiBasisAxis::NegativeX),
XII_ENUM_CONSTANT(xiiBasisAxis::NegativeY),
XII_ENUM_CONSTANT(xiiBasisAxis::NegativeZ),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUuid, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVariant, xiiNoBase, 3, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiString, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUntrackedString, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiStringView, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiDataBuffer, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAngle, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(Degree),
    XII_FUNCTION_PROPERTY(Radian),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiFloatInterval, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Start", m_StartValue),
    XII_MEMBER_PROPERTY("End", m_EndValue),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiIntInterval, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Start", m_StartValue),
    XII_MEMBER_PROPERTY("End", m_EndValue),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

// **********************************************************************
// ***** Various RTTI infos that can't be put next to their classes *****

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiTypeFlags, 1)
XII_BITFLAGS_CONSTANTS(xiiTypeFlags::StandardType, xiiTypeFlags::IsEnum, xiiTypeFlags::Bitflags, xiiTypeFlags::Class, xiiTypeFlags::Abstract, xiiTypeFlags::Phantom, xiiTypeFlags::Minimal)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiPropertyFlags, 1)
XII_BITFLAGS_CONSTANTS(xiiPropertyFlags::StandardType, xiiPropertyFlags::IsEnum, xiiPropertyFlags::Bitflags, xiiPropertyFlags::Class)
XII_BITFLAGS_CONSTANTS(xiiPropertyFlags::Const, xiiPropertyFlags::Reference, xiiPropertyFlags::Pointer)
XII_BITFLAGS_CONSTANTS(xiiPropertyFlags::PointerOwner, xiiPropertyFlags::ReadOnly, xiiPropertyFlags::Hidden, xiiPropertyFlags::Phantom)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiFunctionType, 1)
XII_BITFLAGS_CONSTANTS(xiiFunctionType::Member, xiiFunctionType::StaticMember, xiiFunctionType::Constructor)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVariantType, 1)
XII_BITFLAGS_CONSTANTS(xiiVariantType::Invalid, xiiVariantType::Bool, xiiVariantType::Int8, xiiVariantType::UInt8, xiiVariantType::Int16, xiiVariantType::UInt16)
XII_BITFLAGS_CONSTANTS(xiiVariantType::Int32, xiiVariantType::UInt32, xiiVariantType::Int64, xiiVariantType::UInt64, xiiVariantType::Float, xiiVariantType::Double)
XII_BITFLAGS_CONSTANTS(xiiVariantType::Color, xiiVariantType::Vector2, xiiVariantType::Vector3, xiiVariantType::Vector4)
XII_BITFLAGS_CONSTANTS(xiiVariantType::Vector2I, xiiVariantType::Vector3I, xiiVariantType::Vector4I, xiiVariantType::Vector2U, xiiVariantType::Vector3U, xiiVariantType::Vector4U)
XII_BITFLAGS_CONSTANTS(xiiVariantType::Quaternion, xiiVariantType::Matrix3, xiiVariantType::Matrix4, xiiVariantType::Transform)
XII_BITFLAGS_CONSTANTS(xiiVariantType::String, xiiVariantType::StringView, xiiVariantType::DataBuffer, xiiVariantType::Time, xiiVariantType::Uuid, xiiVariantType::Angle, xiiVariantType::ColorGamma)
XII_BITFLAGS_CONSTANTS(xiiVariantType::VariantArray, xiiVariantType::VariantDictionary, xiiVariantType::TypedPointer, xiiVariantType::TypedObject)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiPropertyCategory, 1)
XII_BITFLAGS_CONSTANTS(xiiPropertyCategory::Constant, xiiPropertyCategory::Member, xiiPropertyCategory::Function, xiiPropertyCategory::Array, xiiPropertyCategory::Set, xiiPropertyCategory::Map)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

XII_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_StandardTypes);
