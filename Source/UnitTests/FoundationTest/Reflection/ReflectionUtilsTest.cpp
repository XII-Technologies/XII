/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

template <typename T>
static void SetComponentTest(xiiVec2Template<T> vVector, T value)
{
  xiiVariant var = vVector;
  xiiReflectionUtils::SetComponent(var, 0, value);
  XII_TEST_BOOL(var.Get<xiiVec2Template<T>>().x == value);
  xiiReflectionUtils::SetComponent(var, 1, value);
  XII_TEST_BOOL(var.Get<xiiVec2Template<T>>().y == value);
}

template <typename T>
static void SetComponentTest(xiiVec3Template<T> vVector, T value)
{
  xiiVariant var = vVector;
  xiiReflectionUtils::SetComponent(var, 0, value);
  XII_TEST_BOOL(var.Get<xiiVec3Template<T>>().x == value);
  xiiReflectionUtils::SetComponent(var, 1, value);
  XII_TEST_BOOL(var.Get<xiiVec3Template<T>>().y == value);
  xiiReflectionUtils::SetComponent(var, 2, value);
  XII_TEST_BOOL(var.Get<xiiVec3Template<T>>().z == value);
}

template <typename T>
static void SetComponentTest(xiiVec4Template<T> vVector, T value)
{
  xiiVariant var = vVector;
  xiiReflectionUtils::SetComponent(var, 0, value);
  XII_TEST_BOOL(var.Get<xiiVec4Template<T>>().x == value);
  xiiReflectionUtils::SetComponent(var, 1, value);
  XII_TEST_BOOL(var.Get<xiiVec4Template<T>>().y == value);
  xiiReflectionUtils::SetComponent(var, 2, value);
  XII_TEST_BOOL(var.Get<xiiVec4Template<T>>().z == value);
  xiiReflectionUtils::SetComponent(var, 3, value);
  XII_TEST_BOOL(var.Get<xiiVec4Template<T>>().w == value);
}

template <class T>
static void ClampValueTest(T tooSmall, T tooBig, T min, T max)
{
  xiiClampValueAttribute minClamp(min, {});
  xiiClampValueAttribute maxClamp({}, max);
  xiiClampValueAttribute bothClamp(min, max);

  xiiVariant value = tooSmall;
  XII_TEST_BOOL(xiiReflectionUtils::ClampValue(value, &minClamp).Succeeded());
  XII_TEST_BOOL(value == min);

  value = tooSmall;
  XII_TEST_BOOL(xiiReflectionUtils::ClampValue(value, &bothClamp).Succeeded());
  XII_TEST_BOOL(value == min);

  value = tooBig;
  XII_TEST_BOOL(xiiReflectionUtils::ClampValue(value, &maxClamp).Succeeded());
  XII_TEST_BOOL(value == max);

  value = tooBig;
  XII_TEST_BOOL(xiiReflectionUtils::ClampValue(value, &bothClamp).Succeeded());
  XII_TEST_BOOL(value == max);
}


XII_CREATE_SIMPLE_TEST(Reflection, Utils)
{
  xiiDefaultMemoryStreamStorage StreamStorage;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "WriteObjectToDDL")
  {
    xiiMemoryStreamWriter FileOut(&StreamStorage);

    xiiTestClass2 c2;
    c2.SetCharPtr("Hallo");
    c2.SetString("World");
    c2.SetStringView("!!!");
    c2.m_MyVector.Set(14, 16, 18);
    c2.m_Struct.m_fFloat1 = 128;
    c2.m_Struct.m_UInt8   = 234;
    c2.m_Struct.m_Angle   = xiiAngle::MakeFromDegree(360);
    c2.m_Struct.m_vVec3I  = xiiVec3I32(9, 8, 7);
    c2.m_Struct.m_DataBuffer.Clear();
    c2.m_Color         = xiiColor(0.1f, 0.2f, 0.3f);
    c2.m_Time          = xiiTime::MakeFromSeconds(91.0f);
    c2.m_enumClass     = xiiExampleEnum::Value3;
    c2.m_bitflagsClass = xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2 | xiiExampleBitflags::Value3;
    c2.m_array.PushBack(5.0f);
    c2.m_array.PushBack(10.0f);
    c2.m_Variant = xiiVec3(1.0f, 2.0f, 3.0f);

    xiiReflectionSerializer::WriteObjectToDDL(FileOut, c2.GetDynamicRTTI(), &c2, false, xiiOpenDdlWriter::TypeStringMode::Compliant);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadObjectPropertiesFromDDL")
  {
    xiiMemoryStreamReader FileIn(&StreamStorage);

    xiiTestClass2 c2;

    xiiReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *c2.GetDynamicRTTI(), &c2);

    XII_TEST_STRING(c2.GetCharPtr(), "Hallo");
    XII_TEST_STRING(c2.GetString(), "World");
    XII_TEST_STRING(c2.GetStringView(), "!!!");
    XII_TEST_VEC3(c2.m_MyVector, xiiVec3(3, 4, 5), 0.0f);
    XII_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    XII_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    XII_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    XII_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    XII_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    XII_TEST_INT(c2.m_Struct.m_UInt8, 234);
    XII_TEST_BOOL(c2.m_Struct.m_Angle == xiiAngle::MakeFromDegree(360));
    XII_TEST_BOOL(c2.m_Struct.m_vVec3I == xiiVec3I32(9, 8, 7));
    XII_TEST_BOOL(c2.m_Struct.m_DataBuffer == xiiDataBuffer());
    XII_TEST_BOOL(c2.m_enumClass == xiiExampleEnum::Value3);
    XII_TEST_BOOL(c2.m_bitflagsClass == (xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2 | xiiExampleBitflags::Value3));
    XII_TEST_INT(c2.m_array.GetCount(), 2);
    if (c2.m_array.GetCount() == 2)
    {
      XII_TEST_FLOAT(c2.m_array[0], 5.0f, 0.0f);
      XII_TEST_FLOAT(c2.m_array[1], 10.0f, 0.0f);
    }
    XII_TEST_VEC3(c2.m_Variant.Get<xiiVec3>(), xiiVec3(1.0f, 2.0f, 3.0f), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadObjectPropertiesFromDDL (different type)")
  {
    // here we restore the same properties into a different type of object which has properties that are named the same
    // but may have slightly different types (but which are compatible)

    xiiMemoryStreamReader FileIn(&StreamStorage);

    xiiTestClass2b c2;

    xiiReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *c2.GetDynamicRTTI(), &c2);

    XII_TEST_STRING(c2.GetText(), "Tut"); // not restored, different property name
    XII_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    XII_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    XII_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    XII_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    XII_TEST_INT(c2.m_Struct.m_UInt8, 234);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadObjectFromDDL")
  {
    xiiMemoryStreamReader FileIn(&StreamStorage);

    const xiiRTTI* pRtti;
    void*          pObject = xiiReflectionSerializer::ReadObjectFromDDL(FileIn, pRtti);

    xiiTestClass2& c2 = *((xiiTestClass2*)pObject);

    XII_TEST_STRING(c2.GetCharPtr(), "Hallo");
    XII_TEST_STRING(c2.GetString(), "World");
    XII_TEST_STRING(c2.GetStringView(), "!!!");
    XII_TEST_VEC3(c2.m_MyVector, xiiVec3(3, 4, 5), 0.0f);
    XII_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    XII_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    XII_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    XII_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    XII_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    XII_TEST_INT(c2.m_Struct.m_UInt8, 234);
    XII_TEST_BOOL(c2.m_Struct.m_Angle == xiiAngle::MakeFromDegree(360));
    XII_TEST_BOOL(c2.m_Struct.m_vVec3I == xiiVec3I32(9, 8, 7));
    XII_TEST_BOOL(c2.m_Struct.m_DataBuffer == xiiDataBuffer());
    XII_TEST_BOOL(c2.m_enumClass == xiiExampleEnum::Value3);
    XII_TEST_BOOL(c2.m_bitflagsClass == (xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2 | xiiExampleBitflags::Value3));
    XII_TEST_INT(c2.m_array.GetCount(), 2);
    if (c2.m_array.GetCount() == 2)
    {
      XII_TEST_FLOAT(c2.m_array[0], 5.0f, 0.0f);
      XII_TEST_FLOAT(c2.m_array[1], 10.0f, 0.0f);
    }
    XII_TEST_VEC3(c2.m_Variant.Get<xiiVec3>(), xiiVec3(1.0f, 2.0f, 3.0f), 0.0f);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  xiiFileSystem::ClearAllDataDirectories();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetComponent")
  {
    SetComponentTest(xiiVec2(0.0f, 0.1f), -0.5f);
    SetComponentTest(xiiVec3(0.0f, 0.1f, 0.2f), -0.5f);
    SetComponentTest(xiiVec4(0.0f, 0.1f, 0.2f, 0.3f), -0.5f);
    SetComponentTest(xiiVec2I32(0, 1), -4);
    SetComponentTest(xiiVec3I32(0, 1, 2), -4);
    SetComponentTest(xiiVec4I32(0, 1, 2, 3), -4);
    SetComponentTest(xiiVec2U32(0, 1), 4u);
    SetComponentTest(xiiVec3U32(0, 1, 2), 4u);
    SetComponentTest(xiiVec4U32(0, 1, 2, 3), 4u);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ClampValue")
  {
    ClampValueTest<float>(-1, 1000, 2, 4);
    ClampValueTest<double>(-1, 1000, 2, 4);
    ClampValueTest<xiiInt32>(-1, 1000, 2, 4);
    ClampValueTest<xiiUInt64>(1, 1000, 2, 4);
    ClampValueTest<xiiTime>(xiiTime::MakeFromMilliseconds(1), xiiTime::MakeFromMilliseconds(1000), xiiTime::MakeFromMilliseconds(2), xiiTime::MakeFromMilliseconds(4));
    ClampValueTest<xiiAngle>(xiiAngle::MakeFromDegree(1), xiiAngle::MakeFromDegree(1000), xiiAngle::MakeFromDegree(2), xiiAngle::MakeFromDegree(4));
    ClampValueTest<xiiVec3>(xiiVec3(1), xiiVec3(1000), xiiVec3(2), xiiVec3(4));
    ClampValueTest<xiiVec4I32>(xiiVec4I32(1), xiiVec4I32(1000), xiiVec4I32(2), xiiVec4I32(4));
    ClampValueTest<xiiVec4U32>(xiiVec4U32(1), xiiVec4U32(1000), xiiVec4U32(2), xiiVec4U32(4));

    xiiVarianceTypeFloat vf       = {1.0f, 2.0f};
    xiiVariant           variance = vf;
    XII_TEST_BOOL(xiiReflectionUtils::ClampValue(variance, nullptr).Succeeded());

    xiiVarianceTypeFloat   clamp = {2.0f, 3.0f};
    xiiClampValueAttribute minClamp(clamp, {});
    XII_TEST_BOOL(xiiReflectionUtils::ClampValue(variance, &minClamp).Failed());
  }
}
