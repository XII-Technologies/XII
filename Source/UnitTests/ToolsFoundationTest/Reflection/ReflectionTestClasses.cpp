#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiIntegerStruct, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiIntegerStruct>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Int8", GetInt8, SetInt8),
    XII_ACCESSOR_PROPERTY("UInt8", GetUInt8, SetUInt8),
    XII_MEMBER_PROPERTY("Int16", m_iInt16),
    XII_MEMBER_PROPERTY("UInt16", m_iUInt16),
    XII_ACCESSOR_PROPERTY("Int32", GetInt32, SetInt32),
    XII_ACCESSOR_PROPERTY("UInt32", GetUInt32, SetUInt32),
    XII_MEMBER_PROPERTY("Int64", m_iInt64),
    XII_MEMBER_PROPERTY("UInt64", m_iUInt64),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;


XII_BEGIN_STATIC_REFLECTED_TYPE(xiiFloatStruct, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiFloatStruct>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Float", GetFloat, SetFloat),
    XII_ACCESSOR_PROPERTY("Double", GetDouble, SetDouble),
    XII_ACCESSOR_PROPERTY("Time", GetTime, SetTime),
    XII_ACCESSOR_PROPERTY("Angle", GetAngle, SetAngle),
    XII_ACCESSOR_PROPERTY("Angled", GetAngled, SetAngled),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPODClass, 1, xiiRTTIDefaultAllocator<xiiPODClass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Integer", m_IntegerStruct),
    XII_MEMBER_PROPERTY("Float", m_FloatStruct),
    XII_ACCESSOR_PROPERTY("Bool", GetBool, SetBool),
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor),
    XII_MEMBER_PROPERTY("ColorUB", m_Color2),
    XII_ACCESSOR_PROPERTY("String", GetString, SetString),
    XII_ACCESSOR_PROPERTY("Buffer", GetBuffer, SetBuffer),
    XII_ACCESSOR_PROPERTY("VarianceAngle", GetCustom, SetCustom),
    XII_ACCESSOR_PROPERTY("VarianceAngled", GetCustom2, SetCustom2),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMathClass, 1, xiiRTTIDefaultAllocator<xiiMathClass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Vec2I", m_Vec2I),
    XII_MEMBER_PROPERTY("Vec3I", m_Vec3I),
    XII_MEMBER_PROPERTY("Vec4I", m_Vec4I),
    XII_MEMBER_PROPERTY("Vec2U", m_Vec2U),
    XII_MEMBER_PROPERTY("Vec3U", m_Vec3U),
    XII_MEMBER_PROPERTY("Vec4U", m_Vec4U),
    XII_MEMBER_PROPERTY("Vec2I64", m_Vec2I64),
    XII_MEMBER_PROPERTY("Vec3I64", m_Vec3I64),
    XII_MEMBER_PROPERTY("Vec4I64", m_Vec4I64),
    XII_MEMBER_PROPERTY("Vec2U64", m_Vec2U64),
    XII_MEMBER_PROPERTY("Vec3U64", m_Vec3U64),
    XII_MEMBER_PROPERTY("Vec4U64", m_Vec4U64),
    XII_ACCESSOR_PROPERTY("Vec2", GetVec2, SetVec2),
    XII_ACCESSOR_PROPERTY("Vec3", GetVec3, SetVec3),
    XII_ACCESSOR_PROPERTY("Vec4", GetVec4, SetVec4),
    XII_ACCESSOR_PROPERTY("Quat", GetQuat, SetQuat),
    XII_ACCESSOR_PROPERTY("Mat3", GetMat3, SetMat3),
    XII_ACCESSOR_PROPERTY("Mat4", GetMat4, SetMat4),
    XII_ACCESSOR_PROPERTY("Vec2d", GetVec2d, SetVec2d),
    XII_ACCESSOR_PROPERTY("Vec3d", GetVec3d, SetVec3d),
    XII_ACCESSOR_PROPERTY("Vec4d", GetVec4d, SetVec4d),
    XII_ACCESSOR_PROPERTY("Quatd", GetQuatd, SetQuatd),
    XII_ACCESSOR_PROPERTY("Mat3d", GetMat3d, SetMat3d),
    XII_ACCESSOR_PROPERTY("Mat4d", GetMat4d, SetMat4d),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;


XII_BEGIN_STATIC_REFLECTED_ENUM(xiiExampleEnum, 1)
  XII_ENUM_CONSTANTS(xiiExampleEnum::Value1, xiiExampleEnum::Value2)
  XII_ENUM_CONSTANT(xiiExampleEnum::Value3),
XII_END_STATIC_REFLECTED_ENUM;


XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiExampleBitflags, 1)
  XII_BITFLAGS_CONSTANTS(xiiExampleBitflags::Value1, xiiExampleBitflags::Value2)
  XII_BITFLAGS_CONSTANT(xiiExampleBitflags::Value3),
XII_END_STATIC_REFLECTED_BITFLAGS;


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEnumerationsClass, 1, xiiRTTIDefaultAllocator<xiiEnumerationsClass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("Enum", xiiExampleEnum, GetEnum, SetEnum),
    XII_BITFLAGS_ACCESSOR_PROPERTY("Bitflags", xiiExampleBitflags, GetBitflags, SetBitflags),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;


XII_BEGIN_STATIC_REFLECTED_TYPE(InnerStruct, xiiNoBase, 1, xiiRTTIDefaultAllocator<InnerStruct>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("IP1", m_fP1),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(OuterClass, 1, xiiRTTIDefaultAllocator<OuterClass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Inner", m_Inner1),
    XII_MEMBER_PROPERTY("OP1", m_fP1),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(ExtendedOuterClass, 1, xiiRTTIDefaultAllocator<ExtendedOuterClass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MORE", m_more),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiObjectTest, 1, xiiRTTIDefaultAllocator<xiiObjectTest>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MemberClass", m_MemberClass),
    XII_ARRAY_MEMBER_PROPERTY("StandardTypeArray", m_StandardTypeArray),
    XII_ARRAY_MEMBER_PROPERTY("ClassArray", m_ClassArray),
    XII_ARRAY_MEMBER_PROPERTY("ClassPtrArray", m_ClassPtrArray)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_SET_ACCESSOR_PROPERTY("StandardTypeSet", GetStandardTypeSet, StandardTypeSetInsert, StandardTypeSetRemove),
    XII_SET_MEMBER_PROPERTY("SubObjectSet", m_SubObjectSet)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_MAP_MEMBER_PROPERTY("StandardTypeMap", m_StandardTypeMap),
    XII_MAP_MEMBER_PROPERTY("ClassMap", m_ClassMap),
    XII_MAP_MEMBER_PROPERTY("ClassPtrMap", m_ClassPtrMap)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMirrorTest, 1, xiiRTTIDefaultAllocator<xiiMirrorTest>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Math", m_math),
    XII_MEMBER_PROPERTY("Object", m_object),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiArrayPtr<const xiiString> xiiObjectTest::GetStandardTypeSet() const
{
  return m_StandardTypeSet;
}

void xiiObjectTest::StandardTypeSetInsert(const xiiString& value)
{
  if (!m_StandardTypeSet.Contains(value))
    m_StandardTypeSet.PushBack(value);
}

void xiiObjectTest::StandardTypeSetRemove(const xiiString& value)
{
  m_StandardTypeSet.RemoveAndCopy(value);
}
