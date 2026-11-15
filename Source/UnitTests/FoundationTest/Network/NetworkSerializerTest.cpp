/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/Serialization/NetworkSerializer.h>
#include <FoundationTest/Network/Implementation/NetworkReflectionTestTypes.h>
#include <cstring>
#include <limits>

namespace
{
  xiiNetworkAttribute* PositionAttribute()
  {
    auto* pAttribute                 = new xiiNetworkAttribute(5);
    pAttribute->m_uiQuantizationBits = 16;
    pAttribute->m_fMinimum           = -100;
    pAttribute->m_fMaximum           = 100;
    return pAttribute;
  }
  xiiNetworkAttribute* FloatAttribute()
  {
    auto* pAttribute                 = new xiiNetworkAttribute(4);
    pAttribute->m_uiQuantizationBits = 12;
    pAttribute->m_fMinimum           = -1;
    pAttribute->m_fMaximum           = 1;
    return pAttribute;
  }
} // namespace

XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkExampleNested, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkExampleNested>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Number", m_iNumber)->AddAttributes(new xiiNetworkAttribute(1)),
      XII_MEMBER_PROPERTY("Local", m_sLocal),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkExampleState, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkExampleState>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Sequence", m_iSequence)->AddAttributes(new xiiNetworkAttribute(1)),
      XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiNetworkAttribute(2)),
      XII_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new xiiNetworkAttribute(3, true)),
      XII_MEMBER_PROPERTY("Quantized", m_fQuantized)->AddAttributes(FloatAttribute()),
      XII_MEMBER_PROPERTY("Position", m_vPosition)->AddAttributes(PositionAttribute()),
      XII_MEMBER_PROPERTY("Time", m_Time)->AddAttributes(new xiiNetworkAttribute(6)),
      XII_MEMBER_PROPERTY("Value", m_Value)->AddAttributes(new xiiNetworkAttribute(7)),
      XII_MEMBER_PROPERTY("Data", m_Data)->AddAttributes(new xiiNetworkAttribute(8)),
      XII_MEMBER_PROPERTY("Nested", m_Nested)->AddAttributes(new xiiNetworkAttribute(9)),
      XII_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddAttributes(new xiiNetworkAttribute(10)),
      XII_MAP_MEMBER_PROPERTY("Map", m_Map)->AddAttributes(new xiiNetworkAttribute(11)),
      XII_ARRAY_MEMBER_PROPERTY("Fixed", m_Fixed)->AddAttributes(new xiiNetworkAttribute(12)),
      XII_ACCESSOR_PROPERTY("Accessor", GetAccessor, SetAccessor)->AddAttributes(new xiiNetworkAttribute(13)),
      XII_MEMBER_PROPERTY("Array", m_Array)->AddAttributes(new xiiNetworkAttribute(14)),
      XII_MEMBER_PROPERTY("Dictionary", m_Dictionary)->AddAttributes(new xiiNetworkAttribute(15)),
      XII_MEMBER_PROPERTY("Local", m_sLocal),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkWireFixture, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkWireFixture>)
  {
    XII_BEGIN_PROPERTIES
    {XII_MEMBER_PROPERTY("Number", m_iNumber)->AddAttributes(new xiiNetworkAttribute(1))} XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {new xiiNetworkIdentityAttribute(0x4D34574952453031ULL)} XII_END_ATTRIBUTES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(NetworkReflection, ReflectedValueRoundTrips)
{
  xiiNetworkTypeRegistry         registry;
  xiiNetworkSerializationContext context;
  NetworkExampleState            source;
  source.m_iSequence  = -77;
  source.m_sText      = "XII \xE2\x98\x83";
  source.m_fQuantized = 0.25f;
  source.m_vPosition  = {1.5f, -4.0f, 99.0f};
  source.SetAccessor(-987.5);
  source.m_Nested.m_iNumber = 7;
  source.m_Nested.m_sLocal  = "private source";
  source.m_Children.SetCount(2);
  source.m_Children[0].m_iNumber = 8;
  source.m_Children[1].m_iNumber = 9;
  source.m_Map["z"].m_iNumber    = 10;
  source.m_Map["a"].m_iNumber    = 11;
  source.m_Fixed.PushBack(5);
  source.m_Fixed.PushBack(-3);
  source.m_Data.SetCount(1024);
  std::memset(source.m_Data.GetData(), 0x3A, source.m_Data.GetCount());
  xiiVariantDictionary values;
  values.Insert("integer", xiiInt64(-123));
  values.Insert("text", xiiString("nested"));
  source.m_Value = values;
  source.m_Array.PushBack(xiiInt32(123));
  source.m_Dictionary = values;
  for (auto encoding : {xiiNetworkEncoding::Tagged, xiiNetworkEncoding::Compact})
  {
    xiiArrayPtr<const xiiUInt8> bytes;
    auto                        encodeError = xiiNetworkSerializer::Serialize(registry, source, context, bytes, encoding);
    if (!XII_TEST_BOOL_MSG(encodeError.Succeeded(), "%s, field %llu", encodeError.GetCodeName(), static_cast<unsigned long long>(encodeError.m_uiField)))
    {
      return;
    }
    NetworkExampleState target;
    target.m_sLocal          = "keep root";
    target.m_Nested.m_sLocal = "keep nested";
    target.m_Children.SetCount(2);
    target.m_Children[0].m_sLocal = "keep child";
    target.m_Map["a"].m_sLocal    = "keep map";
    auto error                    = xiiNetworkSerializer::Deserialize(registry, target, bytes, context);
    if (!XII_TEST_BOOL_MSG(error.Succeeded(), "%s, field %llu", error.GetCodeName(), static_cast<unsigned long long>(error.m_uiField)))
    {
      return;
    }
    XII_TEST_INT(target.m_iSequence, -77);
    XII_TEST_STRING(target.m_sText, source.m_sText);
    XII_TEST_FLOAT(target.m_fQuantized, source.m_fQuantized, 0.00025);
    XII_TEST_BOOL(target.m_vPosition.IsEqual(source.m_vPosition, 0.002f));
    XII_TEST_FLOAT(target.GetAccessor(), source.GetAccessor(), 0.0001);
    XII_TEST_BOOL(target.m_Time.Compare(source.m_Time, xiiTimestamp::CompareMode::Identical));
    XII_TEST_BOOL(target.m_Data == source.m_Data && target.m_Value == source.m_Value);
    XII_TEST_STRING(target.m_sLocal, "keep root");
    XII_TEST_STRING(target.m_Nested.m_sLocal, "keep nested");
    XII_TEST_INT(target.m_Nested.m_iNumber, 7);
    XII_TEST_INT(target.m_Children[1].m_iNumber, 9);
    XII_TEST_STRING(target.m_Children[0].m_sLocal, "keep child");
    XII_TEST_STRING(target.m_Map["a"].m_sLocal, "keep map");
    XII_TEST_BOOL(target.m_Fixed == source.m_Fixed);
    XII_TEST_BOOL(target.m_Array == source.m_Array && target.m_Dictionary == source.m_Dictionary);
  }
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, TransactionalDecoderLimits)
{
  xiiNetworkTypeRegistry         registry;
  xiiNetworkSerializationContext context;
  NetworkExampleState            source;
  source.m_iSequence = 99;
  source.m_sText     = "payload";
  xiiArrayPtr<const xiiUInt8> view;
  if (!XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, source, context, view).Succeeded()))
  {
    return;
  }
  xiiDynamicArray<xiiUInt8> bytes;
  bytes = view;
  for (xiiUInt32 uiLength = 0; uiLength < bytes.GetCount(); ++uiLength)
  {
    NetworkExampleState target;
    target.m_iSequence = -1;
    target.m_sText     = "unchanged";
    const auto error   = xiiNetworkSerializer::Deserialize(registry, target, bytes.GetArrayPtr().GetSubArray(0, uiLength), context);
    XII_TEST_BOOL(error.Failed());
    XII_TEST_INT(target.m_iSequence, -1);
    XII_TEST_STRING(target.m_sText, "unchanged");
  }
  bytes.PushBack(0xFF);
  NetworkExampleState target;
  target.m_iSequence = -1;
  XII_TEST_BOOL(xiiNetworkSerializer::Deserialize(registry, target, bytes, context).Failed());
  XII_TEST_INT(target.m_iSequence, -1);
  bytes.PopBack();
  context.m_Limits.m_uiMaximumTemporaryBytes = 16;
  XII_TEST_BOOL(xiiNetworkSerializer::Deserialize(registry, target, bytes, context).m_Code == xiiNetworkDataErrorCode::AllocationLimit);
  XII_TEST_INT(target.m_iSequence, -1);
  context.m_Limits                        = {};
  context.m_Limits.m_uiMaximumStringBytes = 2;
  XII_TEST_BOOL(xiiNetworkSerializer::Deserialize(registry, target, bytes, context).m_Code == xiiNetworkDataErrorCode::SizeLimit);
  XII_TEST_INT(target.m_iSequence, -1);
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, StableSerializerWireFixture)
{
  xiiNetworkTypeRegistry         registry;
  xiiNetworkSerializationContext context;
  NetworkWireFixture             object;
  xiiArrayPtr<const xiiUInt8>    bytes;
  if (!XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, object, context, bytes).Succeeded()))
  {
    return;
  }
  const xiiUInt8 uiExpected[] = {0x58, 0x4E, 0x53, 0x31, 0x00, 0x4D, 0x34, 0x57, 0x49, 0x52, 0x45, 0x30, 0x31, 0x00, 0x00, 0x00, 0x01, 0x67, 0x20, 0xF7, 0xEC, 0x36, 0xAD, 0x82, 0x98, 0x01, 0x2A, 0x7F, 0x8A, 0x7F, 0x8A, 0x2E, 0x19, 0x49, 0x00, 0x00, 0x00, 0x01, 0x54, 0x00};
  XII_TEST_BOOL(bytes == xiiMakeArrayPtr(uiExpected));
  NetworkWireFixture decoded;
  decoded.m_iNumber = 0;
  XII_TEST_BOOL(xiiNetworkSerializer::Deserialize(registry, decoded, bytes, context).Succeeded());
  XII_TEST_INT(decoded.m_iNumber, 42);
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, DecoderMutationCorpus)
{
  xiiNetworkTypeRegistry         registry;
  xiiNetworkSerializationContext context;
  NetworkExampleState            source;
  source.m_iSequence = 99;
  source.m_sText     = "mutation payload";
  xiiArrayPtr<const xiiUInt8> view;
  if (!XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, source, context, view).Succeeded()))
  {
    return;
  }
  xiiDynamicArray<xiiUInt8> original;
  original                                   = view;
  context.m_Limits.m_uiMaximumArrayElements  = 64;
  context.m_Limits.m_uiMaximumMapEntries     = 64;
  context.m_Limits.m_uiMaximumTemporaryBytes = 65536;
  context.m_Limits.m_uiMaximumDecodedBytes   = 65536;
  xiiUInt32 uiRandom                         = 0x58494934U;
  for (xiiUInt32 uiIteration = 0; uiIteration < 5000; ++uiIteration)
  {
    xiiDynamicArray<xiiUInt8> bytes = original;
    for (xiiUInt32 uiChange = 0; uiChange < 3; ++uiChange)
    {
      uiRandom ^= uiRandom << 13;
      uiRandom ^= uiRandom >> 17;
      uiRandom ^= uiRandom << 5;
      bytes[25 + uiRandom % (bytes.GetCount() - 25)] ^= static_cast<xiiUInt8>(uiRandom >> 24);
    }
    NetworkExampleState target;
    target.m_iSequence        = -9;
    target.m_sText            = "safe";
    target.m_Nested.m_iNumber = -10;
    const auto error          = xiiNetworkSerializer::Deserialize(registry, target, bytes, context);
    if (error.Failed())
    {
      XII_TEST_INT(target.m_iSequence, -9);
      XII_TEST_STRING(target.m_sText, "safe");
      XII_TEST_INT(target.m_Nested.m_iNumber, -10);
      XII_TEST_INT(target.m_Fixed.GetCount(), 0);
      XII_TEST_BOOL(!target.m_Value.IsValid());
    }
    XII_TEST_STRING(target.m_sLocal, "constructor");
  }
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, PrimitiveAndMathematicalVariants)
{
  xiiNetworkTypeRegistry         registry;
  xiiNetworkSerializationContext context;
  xiiVariantArray                samples;
  samples.PushBack(true);
  samples.PushBack(xiiInt8(-128));
  samples.PushBack(xiiInt16(-32000));
  samples.PushBack(xiiInt32(-2000000000));
  samples.PushBack(std::numeric_limits<xiiInt64>::min());
  samples.PushBack(xiiUInt8(255));
  samples.PushBack(xiiUInt16(65535));
  samples.PushBack(std::numeric_limits<xiiUInt32>::max());
  samples.PushBack(std::numeric_limits<xiiUInt64>::max());
  samples.PushBack(1.25f);
  samples.PushBack(-1.5e100);
  samples.PushBack(xiiString("value"));
  samples.PushBack(xiiVec2(1, 2));
  samples.PushBack(xiiVec3(1, 2, 3));
  samples.PushBack(xiiVec4(1, 2, 3, 4));
  samples.PushBack(xiiVec2d(1, 2));
  samples.PushBack(xiiVec3d(1, 2, 3));
  samples.PushBack(xiiVec4d(1, 2, 3, 4));
  samples.PushBack(xiiVec2I32(-1, 2));
  samples.PushBack(xiiVec3U32(1, 2, 3));
  samples.PushBack(xiiVec4I64(-1, 2, 3, 4));
  samples.PushBack(xiiVec3U64(1, 2, std::numeric_limits<xiiUInt64>::max()));
  samples.PushBack(xiiQuat::MakeIdentity());
  samples.PushBack(xiiQuatd::MakeIdentity());
  samples.PushBack(xiiMat3::MakeIdentity());
  samples.PushBack(xiiMat4::MakeIdentity());
  samples.PushBack(xiiMat3d::MakeIdentity());
  samples.PushBack(xiiMat4d::MakeIdentity());
  samples.PushBack(xiiTransform::MakeIdentity());
  samples.PushBack(xiiTransformd::MakeIdentity());
  samples.PushBack(xiiColor(0.25f, 0.5f, 0.75f));
  samples.PushBack(xiiColorGammaUB(1, 2, 3, 4));
  samples.PushBack(xiiAngle::MakeFromRadian(0.25f));
  samples.PushBack(xiiAngled::MakeFromRadian(0.5));
  samples.PushBack(xiiTime::MakeFromSeconds(5));
  samples.PushBack(xiiUuid(1, 2));
  for (const auto& sample : samples)
  {
    NetworkExampleState source;
    source.m_Value = sample;
    xiiArrayPtr<const xiiUInt8> bytes;
    if (!XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, source, context, bytes).Succeeded()))
    {
      return;
    }
    NetworkExampleState target;
    const auto          error = xiiNetworkSerializer::Deserialize(registry, target, bytes, context);
    if (!XII_TEST_BOOL_MSG(error.Succeeded(), "%s", error.GetCodeName()))
    {
      return;
    }
    XII_TEST_BOOL(target.m_Value == source.m_Value);
  }
}
