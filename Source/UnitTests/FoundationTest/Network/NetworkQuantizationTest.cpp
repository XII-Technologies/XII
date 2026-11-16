/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/Serialization/NetworkSerializer.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

struct NetworkRotationValue
{
  xiiQuat                         m_Rotation = xiiQuat::MakeIdentity();
  xiiQuatd                        m_Double   = xiiQuatd::MakeIdentity();
  xiiEnum<xiiExampleEnum>         m_Enum;
  xiiBitflags<xiiExampleBitflags> m_Flags;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkRotationValue);
namespace
{
  xiiNetworkAttribute* RotationAttribute(xiiUInt64 id)
  {
    auto* attribute                 = new xiiNetworkAttribute(id);
    attribute->m_uiQuantizationBits = 12;
    attribute->m_fMinimum           = -1;
    attribute->m_fMaximum           = 1;
    return attribute;
  }
} // namespace
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkRotationValue, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkRotationValue>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Rotation", m_Rotation)->AddAttributes(RotationAttribute(1)),
      XII_MEMBER_PROPERTY("Double", m_Double)->AddAttributes(RotationAttribute(2)),
      XII_ENUM_MEMBER_PROPERTY("Enum", xiiExampleEnum, m_Enum)->AddAttributes(new xiiNetworkAttribute(3)),
      XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiExampleBitflags, m_Flags)->AddAttributes(new xiiNetworkAttribute(4)),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(NetworkReflection, RotationQuantizationAndEnumValidation)
{
  xiiNetworkTypeRegistry         registry;
  xiiNetworkSerializationContext context;
  NetworkRotationValue           source, target;
  source.m_Rotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(91));
  source.m_Double   = xiiQuatd::MakeFromAxisAndAngle(xiiVec3d(1, 0, 0), xiiAngled::MakeFromDegree(123));
  source.m_Enum     = xiiExampleEnum::Value2;
  source.m_Flags    = xiiExampleBitflags::Value1 | xiiExampleBitflags::Value3;
  for (auto encoding : {xiiNetworkEncoding::Tagged, xiiNetworkEncoding::Compact})
  {
    xiiArrayPtr<const xiiUInt8> bytes;
    XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, source, context, bytes, encoding).Succeeded());
    XII_TEST_BOOL(xiiNetworkSerializer::Deserialize(registry, target, bytes, context).Succeeded());
    XII_TEST_BOOL(target.m_Rotation.IsValid());
    XII_TEST_BOOL(target.m_Double.IsValid());
    XII_TEST_BOOL(target.m_Rotation.IsEqualRotation(source.m_Rotation, 0.002f));
    XII_TEST_BOOL(target.m_Double.IsEqualRotation(source.m_Double, 0.002));
    XII_TEST_BOOL(target.m_Enum == source.m_Enum);
    XII_TEST_BOOL(target.m_Flags == source.m_Flags);
  }
  xiiArrayPtr<const xiiUInt8> bytes;
  source.m_Enum = static_cast<xiiExampleEnum::Enum>(99);
  XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, source, context, bytes).Failed());
  source.m_Enum = xiiExampleEnum::Value1;
  source.m_Flags.SetValue(8);
  XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, source, context, bytes).Failed());
  source.m_Flags.Clear();
  source.m_Rotation.x = 0;
  source.m_Rotation.y = 0;
  source.m_Rotation.z = 0;
  source.m_Rotation.w = 0;
  XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, source, context, bytes).Failed());
}
