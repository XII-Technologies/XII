/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/Reflection/NetworkAttributes.h>
#include <Foundation/Network/Serialization/NetworkBinaryStream.h>
#include <limits>

XII_CREATE_SIMPLE_TEST_GROUP(NetworkReflection);

XII_CREATE_SIMPLE_TEST(NetworkReflection, BinaryWireFixtures)
{
  xiiDynamicArray<xiiUInt8> bytes;
  xiiNetworkBinaryWriter    writer(bytes, 128);
  writer.U32(0x01020304U);
  writer.VarUInt(128);
  writer.VarInt(-1);
  writer.Text("XII", 8);
  const xiiUInt8 uiExpected[] = {1, 2, 3, 4, 0x80, 1, 1, 3, 'X', 'I', 'I'};
  XII_TEST_BOOL(writer.GetError().Succeeded());
  XII_TEST_BOOL(bytes.GetArrayPtr() == xiiMakeArrayPtr(uiExpected));
  xiiNetworkBinaryReader reader(bytes);
  XII_TEST_INT(reader.U32(), 0x01020304U);
  XII_TEST_INT(reader.VarUInt(), 128);
  XII_TEST_INT(reader.VarInt(), -1);
  XII_TEST_STRING(reader.Text(8), "XII");
  XII_TEST_BOOL(reader.IsFinished());
  XII_TEST_BOOL(xiiGetStaticRTTI<xiiNetworkAttribute>()->IsDerivedFrom<xiiPropertyAttribute>());
  XII_TEST_BOOL(xiiNetworkStableId("hello") == 0xA430D84680AABD0BULL);
  bytes.Clear();
  xiiNetworkBinaryWriter extremes(bytes, 128);
  extremes.VarInt(std::numeric_limits<xiiInt64>::min());
  extremes.VarInt(std::numeric_limits<xiiInt64>::max());
  extremes.VarUInt(std::numeric_limits<xiiUInt64>::max());
  xiiNetworkBinaryReader bounds(bytes);
  XII_TEST_BOOL(bounds.VarInt() == std::numeric_limits<xiiInt64>::min());
  XII_TEST_BOOL(bounds.VarInt() == std::numeric_limits<xiiInt64>::max());
  XII_TEST_BOOL(bounds.VarUInt() == std::numeric_limits<xiiUInt64>::max());
  XII_TEST_BOOL(bounds.IsFinished());
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, BinaryRejectsMalformedInput)
{
  const xiiUInt8         uiOverlong[] = {0x80, 0};
  xiiNetworkBinaryReader overlong(uiOverlong);
  XII_TEST_INT(overlong.VarUInt(), 0);
  XII_TEST_BOOL(overlong.GetError().m_Code == xiiNetworkDataErrorCode::InvalidEncoding);
  const xiiUInt8         uiOverflow[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 2};
  xiiNetworkBinaryReader overflow(uiOverflow);
  XII_TEST_INT(overflow.VarUInt(), 0);
  XII_TEST_BOOL(overflow.GetError().m_Code == xiiNetworkDataErrorCode::InvalidEncoding);
  const xiiUInt8         uiTruncated[] = {4, 'a'};
  xiiNetworkBinaryReader truncated(uiTruncated);
  XII_TEST_BOOL(truncated.Text(8).IsEmpty());
  XII_TEST_BOOL(truncated.GetError().m_Code == xiiNetworkDataErrorCode::Truncated);
  const xiiUInt8         uiUtf8[] = {2, 0xC0, 0xAF};
  xiiNetworkBinaryReader utf8(uiUtf8);
  XII_TEST_BOOL(utf8.Text(8).IsEmpty());
  XII_TEST_BOOL(utf8.GetError().m_Code == xiiNetworkDataErrorCode::InvalidEncoding);
  xiiDynamicArray<xiiUInt8> bytes;
  xiiNetworkBinaryWriter    limited(bytes, 1);
  limited.U32(1);
  XII_TEST_BOOL(limited.GetError().m_Code == xiiNetworkDataErrorCode::SizeLimit);
  XII_TEST_INT(bytes.GetCount(), 1);
}
