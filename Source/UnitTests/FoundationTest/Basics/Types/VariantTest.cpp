#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/Variant.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

// This file takes ages to compile in a Release build
// since we don't care for runtime performance, just disable all optimizations.
#pragma optimize("", off)

class Blubb : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(Blubb, xiiReflectedClass);

public:
  float u;
  float v;
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(Blubb, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("u", u),
    XII_MEMBER_PROPERTY("v", v),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

template <typename T>
void TestVariant(xiiVariant& v, xiiVariantType::Enum type)
{
  XII_TEST_BOOL(v.IsValid());
  XII_TEST_BOOL(v.GetType() == type);
  XII_TEST_BOOL(v.CanConvertTo<T>());
  XII_TEST_BOOL(v.IsA<T>());
  XII_TEST_BOOL(v.GetReflectedType() == xiiGetStaticRTTI<T>());

  xiiTypedPointer ptr = v.GetWriteAccess();
  XII_TEST_BOOL(ptr.m_pObject == &v.Get<T>());
  XII_TEST_BOOL(ptr.m_pObject == &v.GetWritable<T>());
  XII_TEST_BOOL(ptr.m_pType == xiiGetStaticRTTI<T>());

  XII_TEST_BOOL(ptr.m_pObject == v.GetData());

  xiiVariant      vCopy = v;
  xiiTypedPointer ptr2  = vCopy.GetWriteAccess();
  XII_TEST_BOOL(ptr2.m_pObject == &vCopy.Get<T>());
  XII_TEST_BOOL(ptr2.m_pObject == &vCopy.GetWritable<T>());

  XII_TEST_BOOL(ptr2.m_pObject != ptr.m_pObject);
  XII_TEST_BOOL(ptr2.m_pType == xiiGetStaticRTTI<T>());

  XII_TEST_BOOL(v.Get<T>() == vCopy.Get<T>());

  XII_TEST_BOOL(v.ComputeHash(0) != 0);
}

template <typename T>
inline void TestIntegerVariant(xiiVariant::Type::Enum type)
{
  xiiVariant b((T)23);
  TestVariant<T>(b, type);

  XII_TEST_BOOL(b.Get<T>() == 23);

  XII_TEST_BOOL(b == xiiVariant(23));
  XII_TEST_BOOL(b != xiiVariant(11));
  XII_TEST_BOOL(b == xiiVariant((T)23));
  XII_TEST_BOOL(b != xiiVariant((T)11));

  XII_TEST_BOOL(b == 23);
  XII_TEST_BOOL(b != 24);
  XII_TEST_BOOL(b == (T)23);
  XII_TEST_BOOL(b != (T)24);

  b = (T)17;
  XII_TEST_BOOL(b == (T)17);

  b = xiiVariant((T)19);
  XII_TEST_BOOL(b == (T)19);

  XII_TEST_BOOL(b.IsNumber());
  XII_TEST_BOOL(b.IsFloatingPoint() == false);
  XII_TEST_BOOL(!b.IsString());
}

inline void TestNumberCanConvertTo(const xiiVariant& v)
{
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Invalid) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Bool));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int8));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt8));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int16));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt16));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int32));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt32));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int64));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt64));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Float));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Double));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Color) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2d) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2I) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2I64) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2U) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2U64) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3I) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3I64) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3U) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3U64) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4I) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4I64) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4U) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4U64) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Quaternion) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix3) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix3d) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix4) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix4d) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Transform) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Transformd) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::String));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::StringView) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::HashedString));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TempHashedString));
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::DataBuffer) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Time) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Uuid) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Angle) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Angled) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::ColorGamma) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::VariantArray) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::VariantDictionary) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TypedPointer) == false);
  XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TypedObject) == false);

  xiiResult conversionResult = XII_FAILURE;
  XII_TEST_BOOL(v.ConvertTo<bool>(&conversionResult) == true);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiInt8>(&conversionResult) == 3);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiUInt8>(&conversionResult) == 3);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiInt16>(&conversionResult) == 3);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiUInt16>(&conversionResult) == 3);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiInt32>(&conversionResult) == 3);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiUInt32>(&conversionResult) == 3);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiInt64>(&conversionResult) == 3);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiUInt64>(&conversionResult) == 3);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<float>(&conversionResult) == 3.0f);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<double>(&conversionResult) == 3.0);
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiString>(&conversionResult) == "3");
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiHashedString>(&conversionResult) == xiiMakeHashedString("3"));
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>(&conversionResult) == xiiTempHashedString("3"));
  XII_TEST_BOOL(conversionResult.Succeeded());

  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Bool).Get<bool>() == true);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int8).Get<xiiInt8>() == 3);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt8).Get<xiiUInt8>() == 3);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int16).Get<xiiInt16>() == 3);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt16).Get<xiiUInt16>() == 3);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int32).Get<xiiInt32>() == 3);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt32).Get<xiiUInt32>() == 3);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int64).Get<xiiInt64>() == 3);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt64).Get<xiiUInt64>() == 3);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Float).Get<float>() == 3.0f);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Double).Get<double>() == 3.0);
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "3");
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("3"));
  XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("3"));
}

inline void TestCanOnlyConvertToID(const xiiVariant& v, xiiVariant::Type::Enum type)
{
  for (int iType = xiiVariant::Type::FirstStandardType; iType < xiiVariant::Type::LastExtendedType; ++iType)
  {
    if (iType == xiiVariant::Type::LastStandardType)
      iType = xiiVariant::Type::FirstExtendedType;

    if (iType == type)
    {
      XII_TEST_BOOL(v.CanConvertTo(type));
    }
    else
    {
      XII_TEST_BOOL(v.CanConvertTo((xiiVariant::Type::Enum)iType) == false);
    }
  }
}

inline void TestCanOnlyConvertToStringAndID(const xiiVariant& v, xiiVariant::Type::Enum type, xiiVariant::Type::Enum type2 = xiiVariant::Type::Invalid, xiiVariant::Type::Enum type3 = xiiVariant::Type::Invalid, xiiVariant::Type::Enum type4 = xiiVariant::Type::Invalid, xiiVariant::Type::Enum type5 = xiiVariant::Type::Invalid, xiiVariant::Type::Enum type6 = xiiVariant::Type::Invalid)
{
  if (type2 == xiiVariant::Type::Invalid)
    type2 = type;

  for (int iType = xiiVariant::Type::FirstStandardType; iType < xiiVariant::Type::LastExtendedType; ++iType)
  {
    if (iType == xiiVariant::Type::LastStandardType)
      iType = xiiVariant::Type::FirstExtendedType;

    if (iType == xiiVariant::Type::String || iType == xiiVariant::Type::HashedString || iType == xiiVariant::Type::TempHashedString)
    {
      XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::String));
    }
    else if (iType == type || iType == type2 || iType == type3 || iType == type4 || iType == type5 || iType == type6)
    {
      XII_TEST_BOOL(v.CanConvertTo(type));
    }
    else
    {
      XII_TEST_BOOL(v.CanConvertTo((xiiVariant::Type::Enum)iType) == false);
    }
  }
}

XII_CREATE_SIMPLE_TEST(Basics, Variant)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid")
  {
    xiiVariant b;
    XII_TEST_BOOL(b.GetType() == xiiVariant::Type::Invalid);
    XII_TEST_BOOL(b == xiiVariant());
    XII_TEST_BOOL(b != xiiVariant(0));
    XII_TEST_BOOL(!b.IsValid());
    XII_TEST_BOOL(!b[0].IsValid());
    XII_TEST_BOOL(!b["x"].IsValid());
    XII_TEST_BOOL(b.GetReflectedType() == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "bool")
  {
    xiiVariant b(true);
    TestVariant<bool>(b, xiiVariantType::Bool);

    XII_TEST_BOOL(b.Get<bool>() == true);

    XII_TEST_BOOL(b == xiiVariant(true));
    XII_TEST_BOOL(b != xiiVariant(false));

    XII_TEST_BOOL(b == true);
    XII_TEST_BOOL(b != false);

    b = false;
    XII_TEST_BOOL(b == false);

    b = xiiVariant(true);
    XII_TEST_BOOL(b == true);
    XII_TEST_BOOL(!b[0].IsValid());

    XII_TEST_BOOL(b.IsNumber());
    XII_TEST_BOOL(!b.IsString());
    XII_TEST_BOOL(b.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiInt8")
  {
    TestIntegerVariant<xiiInt8>(xiiVariant::Type::Int8);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiUInt8")
  {
    TestIntegerVariant<xiiUInt8>(xiiVariant::Type::UInt8);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiInt16")
  {
    TestIntegerVariant<xiiInt16>(xiiVariant::Type::Int16);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiUInt16")
  {
    TestIntegerVariant<xiiUInt16>(xiiVariant::Type::UInt16);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiInt32")
  {
    TestIntegerVariant<xiiInt32>(xiiVariant::Type::Int32);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiUInt32")
  {
    TestIntegerVariant<xiiUInt32>(xiiVariant::Type::UInt32);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiInt64")
  {
    TestIntegerVariant<xiiInt64>(xiiVariant::Type::Int64);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiUInt64")
  {
    TestIntegerVariant<xiiUInt64>(xiiVariant::Type::UInt64);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "float")
  {
    xiiVariant b(42.0f);
    TestVariant<float>(b, xiiVariantType::Float);

    XII_TEST_BOOL(b.Get<float>() == 42.0f);

    XII_TEST_BOOL(b == xiiVariant(42));
    XII_TEST_BOOL(b != xiiVariant(11));
    XII_TEST_BOOL(b == xiiVariant(42.0));
    XII_TEST_BOOL(b != xiiVariant(11.0));
    XII_TEST_BOOL(b == xiiVariant(42.0f));
    XII_TEST_BOOL(b != xiiVariant(11.0f));

    XII_TEST_BOOL(b == 42);
    XII_TEST_BOOL(b != 41);
    XII_TEST_BOOL(b == 42.0);
    XII_TEST_BOOL(b != 41.0);
    XII_TEST_BOOL(b == 42.0f);
    XII_TEST_BOOL(b != 41.0f);

    b = 17.0f;
    XII_TEST_BOOL(b == 17.0f);

    b = xiiVariant(19.0f);
    XII_TEST_BOOL(b == 19.0f);

    XII_TEST_BOOL(b.IsNumber());
    XII_TEST_BOOL(!b.IsString());
    XII_TEST_BOOL(b.IsFloatingPoint());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "double")
  {
    xiiVariant b(42.0);
    TestVariant<double>(b, xiiVariantType::Double);
    XII_TEST_BOOL(b.Get<double>() == 42.0);

    XII_TEST_BOOL(b == xiiVariant(42));
    XII_TEST_BOOL(b != xiiVariant(11));
    XII_TEST_BOOL(b == xiiVariant(42.0));
    XII_TEST_BOOL(b != xiiVariant(11.0));
    XII_TEST_BOOL(b == xiiVariant(42.0f));
    XII_TEST_BOOL(b != xiiVariant(11.0f));

    XII_TEST_BOOL(b == 42);
    XII_TEST_BOOL(b != 41);
    XII_TEST_BOOL(b == 42.0);
    XII_TEST_BOOL(b != 41.0);
    XII_TEST_BOOL(b == 42.0f);
    XII_TEST_BOOL(b != 41.0f);

    b = 17.0;
    XII_TEST_BOOL(b == 17.0);

    b = xiiVariant(19.0);
    XII_TEST_BOOL(b == 19.0);

    XII_TEST_BOOL(b.IsNumber());
    XII_TEST_BOOL(!b.IsString());
    XII_TEST_BOOL(b.IsFloatingPoint());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiColor")
  {
    xiiVariant v(xiiColor(1, 2, 3, 1));
    TestVariant<xiiColor>(v, xiiVariantType::Color);

    XII_TEST_BOOL(v.CanConvertTo<xiiColorGammaUB>());
    XII_TEST_BOOL(v.ConvertTo<xiiColorGammaUB>() == static_cast<xiiColorGammaUB>(xiiColor(1, 2, 3, 1)));
    XII_TEST_BOOL(v.Get<xiiColor>() == xiiColor(1, 2, 3, 1));

    XII_TEST_BOOL(v == xiiVariant(xiiColor(1, 2, 3)));
    XII_TEST_BOOL(v != xiiVariant(xiiColor(1, 1, 1)));

    XII_TEST_BOOL(v == xiiColor(1, 2, 3));
    XII_TEST_BOOL(v != xiiColor(1, 4, 3));

    v = xiiColor(5, 8, 9);
    XII_TEST_BOOL(v == xiiColor(5, 8, 9));

    v = xiiVariant(xiiColor(7, 9, 4));
    XII_TEST_BOOL(v == xiiColor(7, 9, 4));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 4);
    XII_TEST_BOOL(v[3] == 1);
    XII_TEST_BOOL(v[4] == xiiVariant());
    XII_TEST_BOOL(!v[4].IsValid());
    XII_TEST_BOOL(v["r"] == 7);
    XII_TEST_BOOL(v["g"] == 9);
    XII_TEST_BOOL(v["b"] == 4);
    XII_TEST_BOOL(v["a"] == 1);
    XII_TEST_BOOL(v["x"] == xiiVariant());
    XII_TEST_BOOL(!v["x"].IsValid());

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiColorGammaUB")
  {
    xiiVariant v(xiiColorGammaUB(64, 128, 255, 255));
    TestVariant<xiiColorGammaUB>(v, xiiVariantType::ColorGamma);

    XII_TEST_BOOL(v.CanConvertTo<xiiColor>());
    XII_TEST_BOOL(v.Get<xiiColorGammaUB>() == xiiColorGammaUB(64, 128, 255, 255));

    XII_TEST_BOOL(v == xiiVariant(xiiColorGammaUB(64, 128, 255, 255)));
    XII_TEST_BOOL(v != xiiVariant(xiiColorGammaUB(255, 128, 255, 255)));

    XII_TEST_BOOL(v == xiiColorGammaUB(64, 128, 255, 255));
    XII_TEST_BOOL(v != xiiColorGammaUB(64, 42, 255, 255));

    v = xiiColorGammaUB(10, 50, 200);
    XII_TEST_BOOL(v == xiiColorGammaUB(10, 50, 200));

    v = xiiVariant(xiiColorGammaUB(17, 120, 200));
    XII_TEST_BOOL(v == xiiColorGammaUB(17, 120, 200));
    XII_TEST_BOOL(v[0] == 17);
    XII_TEST_BOOL(v[1] == 120);
    XII_TEST_BOOL(v[2] == 200);
    XII_TEST_BOOL(v[3] == 255);
    XII_TEST_BOOL(v[4] == xiiVariant());
    XII_TEST_BOOL(!v[4].IsValid());
    XII_TEST_BOOL(v["r"] == 17);
    XII_TEST_BOOL(v["g"] == 120);
    XII_TEST_BOOL(v["b"] == 200);
    XII_TEST_BOOL(v["a"] == 255);
    XII_TEST_BOOL(v["x"] == xiiVariant());
    XII_TEST_BOOL(!v["x"].IsValid());

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec2")
  {
    xiiVariant v(xiiVec2(1, 2));
    TestVariant<xiiVec2>(v, xiiVariantType::Vector2);

    XII_TEST_BOOL(v.Get<xiiVec2>() == xiiVec2(1, 2));

    XII_TEST_BOOL(v == xiiVariant(xiiVec2(1, 2)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec2(1, 1)));

    XII_TEST_BOOL(v == xiiVec2(1, 2));
    XII_TEST_BOOL(v != xiiVec2(1, 4));

    v = xiiVec2(5, 8);
    XII_TEST_BOOL(v == xiiVec2(5, 8));

    v = xiiVariant(xiiVec2(7, 9));
    XII_TEST_BOOL(v == xiiVec2(7, 9));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec2d")
  {
    xiiVariant v(xiiVec2d(1, 2));
    TestVariant<xiiVec2d>(v, xiiVariantType::Vector2d);

    XII_TEST_BOOL(v.Get<xiiVec2d>() == xiiVec2d(1, 2));

    XII_TEST_BOOL(v == xiiVariant(xiiVec2d(1, 2)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec2d(1, 1)));

    XII_TEST_BOOL(v == xiiVec2d(1, 2));
    XII_TEST_BOOL(v != xiiVec2d(1, 4));

    v = xiiVec2d(5, 8);
    XII_TEST_BOOL(v == xiiVec2d(5, 8));

    v = xiiVariant(xiiVec2d(7, 9));
    XII_TEST_BOOL(v == xiiVec2d(7, 9));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec3")
  {
    xiiVariant v(xiiVec3(1, 2, 3));
    TestVariant<xiiVec3>(v, xiiVariantType::Vector3);

    XII_TEST_BOOL(v.Get<xiiVec3>() == xiiVec3(1, 2, 3));

    XII_TEST_BOOL(v == xiiVariant(xiiVec3(1, 2, 3)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec3(1, 1, 3)));

    XII_TEST_BOOL(v == xiiVec3(1, 2, 3));
    XII_TEST_BOOL(v != xiiVec3(1, 4, 3));

    v = xiiVec3(5, 8, 9);
    XII_TEST_BOOL(v == xiiVec3(5, 8, 9));

    v = xiiVariant(xiiVec3(7, 9, 8));
    XII_TEST_BOOL(v == xiiVec3(7, 9, 8));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec3d")
  {
    xiiVariant v(xiiVec3d(1, 2, 3));
    TestVariant<xiiVec3d>(v, xiiVariantType::Vector3d);

    XII_TEST_BOOL(v.Get<xiiVec3d>() == xiiVec3d(1, 2, 3));

    XII_TEST_BOOL(v == xiiVariant(xiiVec3d(1, 2, 3)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec3d(1, 1, 3)));

    XII_TEST_BOOL(v == xiiVec3d(1, 2, 3));
    XII_TEST_BOOL(v != xiiVec3d(1, 4, 3));

    v = xiiVec3d(5, 8, 9);
    XII_TEST_BOOL(v == xiiVec3d(5, 8, 9));

    v = xiiVariant(xiiVec3d(7, 9, 8));
    XII_TEST_BOOL(v == xiiVec3d(7, 9, 8));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec4")
  {
    xiiVariant v(xiiVec4(1, 2, 3, 4));
    TestVariant<xiiVec4>(v, xiiVariantType::Vector4);

    XII_TEST_BOOL(v.Get<xiiVec4>() == xiiVec4(1, 2, 3, 4));

    XII_TEST_BOOL(v == xiiVariant(xiiVec4(1, 2, 3, 4)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec4(1, 1, 3, 4)));

    XII_TEST_BOOL(v == xiiVec4(1, 2, 3, 4));
    XII_TEST_BOOL(v != xiiVec4(1, 4, 3, 4));

    v = xiiVec4(5, 8, 9, 3);
    XII_TEST_BOOL(v == xiiVec4(5, 8, 9, 3));

    v = xiiVariant(xiiVec4(7, 9, 8, 4));
    XII_TEST_BOOL(v == xiiVec4(7, 9, 8, 4));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v[3] == 4);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);
    XII_TEST_BOOL(v["w"] == 4);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec4d")
  {
    xiiVariant v(xiiVec4d(1, 2, 3, 4));
    TestVariant<xiiVec4d>(v, xiiVariantType::Vector4d);

    XII_TEST_BOOL(v.Get<xiiVec4d>() == xiiVec4d(1, 2, 3, 4));

    XII_TEST_BOOL(v == xiiVariant(xiiVec4d(1, 2, 3, 4)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec4d(1, 1, 3, 4)));

    XII_TEST_BOOL(v == xiiVec4d(1, 2, 3, 4));
    XII_TEST_BOOL(v != xiiVec4d(1, 4, 3, 4));

    v = xiiVec4d(5, 8, 9, 3);
    XII_TEST_BOOL(v == xiiVec4d(5, 8, 9, 3));

    v = xiiVariant(xiiVec4d(7, 9, 8, 4));
    XII_TEST_BOOL(v == xiiVec4d(7, 9, 8, 4));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v[3] == 4);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);
    XII_TEST_BOOL(v["w"] == 4);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec2I32")
  {
    xiiVariant v(xiiVec2I32(1, 2));
    TestVariant<xiiVec2I32>(v, xiiVariantType::Vector2I);

    XII_TEST_BOOL(v.Get<xiiVec2I32>() == xiiVec2I32(1, 2));

    XII_TEST_BOOL(v == xiiVariant(xiiVec2I32(1, 2)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec2I32(1, 1)));

    XII_TEST_BOOL(v == xiiVec2I32(1, 2));
    XII_TEST_BOOL(v != xiiVec2I32(1, 4));

    v = xiiVec2I32(5, 8);
    XII_TEST_BOOL(v == xiiVec2I32(5, 8));

    v = xiiVariant(xiiVec2I32(7, 9));
    XII_TEST_BOOL(v == xiiVec2I32(7, 9));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec2I64")
  {
    xiiVariant v(xiiVec2I64(1, 2));
    TestVariant<xiiVec2I64>(v, xiiVariantType::Vector2I64);

    XII_TEST_BOOL(v.Get<xiiVec2I64>() == xiiVec2I64(1, 2));

    XII_TEST_BOOL(v == xiiVariant(xiiVec2I64(1, 2)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec2I64(1, 1)));

    XII_TEST_BOOL(v == xiiVec2I64(1, 2));
    XII_TEST_BOOL(v != xiiVec2I64(1, 4));

    v = xiiVec2I64(5, 8);
    XII_TEST_BOOL(v == xiiVec2I64(5, 8));

    v = xiiVariant(xiiVec2I64(7, 9));
    XII_TEST_BOOL(v == xiiVec2I64(7, 9));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec2U32")
  {
    xiiVariant v(xiiVec2U32(1, 2));
    TestVariant<xiiVec2U32>(v, xiiVariantType::Vector2U);

    XII_TEST_BOOL(v.Get<xiiVec2U32>() == xiiVec2U32(1, 2));

    XII_TEST_BOOL(v == xiiVariant(xiiVec2U32(1, 2)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec2U32(1, 1)));

    XII_TEST_BOOL(v == xiiVec2U32(1, 2));
    XII_TEST_BOOL(v != xiiVec2U32(1, 4));

    v = xiiVec2U32(5, 8);
    XII_TEST_BOOL(v == xiiVec2U32(5, 8));

    v = xiiVariant(xiiVec2U32(7, 9));
    XII_TEST_BOOL(v == xiiVec2U32(7, 9));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec2U64")
  {
    xiiVariant v(xiiVec2U64(1, 2));
    TestVariant<xiiVec2U64>(v, xiiVariantType::Vector2U64);

    XII_TEST_BOOL(v.Get<xiiVec2U64>() == xiiVec2U64(1, 2));

    XII_TEST_BOOL(v == xiiVariant(xiiVec2U64(1, 2)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec2U64(1, 1)));

    XII_TEST_BOOL(v == xiiVec2U64(1, 2));
    XII_TEST_BOOL(v != xiiVec2U64(1, 4));

    v = xiiVec2U64(5, 8);
    XII_TEST_BOOL(v == xiiVec2U64(5, 8));

    v = xiiVariant(xiiVec2U64(7, 9));
    XII_TEST_BOOL(v == xiiVec2U64(7, 9));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec3I32")
  {
    xiiVariant v(xiiVec3I32(1, 2, 3));
    TestVariant<xiiVec3I32>(v, xiiVariantType::Vector3I);

    XII_TEST_BOOL(v.Get<xiiVec3I32>() == xiiVec3I32(1, 2, 3));

    XII_TEST_BOOL(v == xiiVariant(xiiVec3I32(1, 2, 3)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec3I32(1, 1, 3)));

    XII_TEST_BOOL(v == xiiVec3I32(1, 2, 3));
    XII_TEST_BOOL(v != xiiVec3I32(1, 4, 3));

    v = xiiVec3I32(5, 8, 9);
    XII_TEST_BOOL(v == xiiVec3I32(5, 8, 9));

    v = xiiVariant(xiiVec3I32(7, 9, 8));
    XII_TEST_BOOL(v == xiiVec3I32(7, 9, 8));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec3I64")
  {
    xiiVariant v(xiiVec3I64(1, 2, 3));
    TestVariant<xiiVec3I64>(v, xiiVariantType::Vector3I64);

    XII_TEST_BOOL(v.Get<xiiVec3I64>() == xiiVec3I64(1, 2, 3));

    XII_TEST_BOOL(v == xiiVariant(xiiVec3I64(1, 2, 3)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec3I64(1, 1, 3)));

    XII_TEST_BOOL(v == xiiVec3I64(1, 2, 3));
    XII_TEST_BOOL(v != xiiVec3I64(1, 4, 3));

    v = xiiVec3I64(5, 8, 9);
    XII_TEST_BOOL(v == xiiVec3I64(5, 8, 9));

    v = xiiVariant(xiiVec3I64(7, 9, 8));
    XII_TEST_BOOL(v == xiiVec3I64(7, 9, 8));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec3U32")
  {
    xiiVariant v(xiiVec3U32(1, 2, 3));
    TestVariant<xiiVec3U32>(v, xiiVariantType::Vector3U);

    XII_TEST_BOOL(v.Get<xiiVec3U32>() == xiiVec3U32(1, 2, 3));

    XII_TEST_BOOL(v == xiiVariant(xiiVec3U32(1, 2, 3)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec3U32(1, 1, 3)));

    XII_TEST_BOOL(v == xiiVec3U32(1, 2, 3));
    XII_TEST_BOOL(v != xiiVec3U32(1, 4, 3));

    v = xiiVec3U32(5, 8, 9);
    XII_TEST_BOOL(v == xiiVec3U32(5, 8, 9));

    v = xiiVariant(xiiVec3U32(7, 9, 8));
    XII_TEST_BOOL(v == xiiVec3U32(7, 9, 8));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec3U64")
  {
    xiiVariant v(xiiVec3U64(1, 2, 3));
    TestVariant<xiiVec3U64>(v, xiiVariantType::Vector3U64);

    XII_TEST_BOOL(v.Get<xiiVec3U64>() == xiiVec3U64(1, 2, 3));

    XII_TEST_BOOL(v == xiiVariant(xiiVec3U64(1, 2, 3)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec3U64(1, 1, 3)));

    XII_TEST_BOOL(v == xiiVec3U64(1, 2, 3));
    XII_TEST_BOOL(v != xiiVec3U64(1, 4, 3));

    v = xiiVec3U64(5, 8, 9);
    XII_TEST_BOOL(v == xiiVec3U64(5, 8, 9));

    v = xiiVariant(xiiVec3U64(7, 9, 8));
    XII_TEST_BOOL(v == xiiVec3U64(7, 9, 8));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec4I32")
  {
    xiiVariant v(xiiVec4I32(1, 2, 3, 4));
    TestVariant<xiiVec4I32>(v, xiiVariantType::Vector4I);

    XII_TEST_BOOL(v.Get<xiiVec4I32>() == xiiVec4I32(1, 2, 3, 4));

    XII_TEST_BOOL(v == xiiVariant(xiiVec4I32(1, 2, 3, 4)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec4I32(1, 1, 3, 4)));

    XII_TEST_BOOL(v == xiiVec4I32(1, 2, 3, 4));
    XII_TEST_BOOL(v != xiiVec4I32(1, 4, 3, 4));

    v = xiiVec4I32(5, 8, 9, 3);
    XII_TEST_BOOL(v == xiiVec4I32(5, 8, 9, 3));

    v = xiiVariant(xiiVec4I32(7, 9, 8, 4));
    XII_TEST_BOOL(v == xiiVec4I32(7, 9, 8, 4));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v[3] == 4);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);
    XII_TEST_BOOL(v["w"] == 4);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec4I64")
  {
    xiiVariant v(xiiVec4I64(1, 2, 3, 4));
    TestVariant<xiiVec4I64>(v, xiiVariantType::Vector4I64);

    XII_TEST_BOOL(v.Get<xiiVec4I64>() == xiiVec4I64(1, 2, 3, 4));

    XII_TEST_BOOL(v == xiiVariant(xiiVec4I64(1, 2, 3, 4)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec4I64(1, 1, 3, 4)));

    XII_TEST_BOOL(v == xiiVec4I64(1, 2, 3, 4));
    XII_TEST_BOOL(v != xiiVec4I64(1, 4, 3, 4));

    v = xiiVec4I64(5, 8, 9, 3);
    XII_TEST_BOOL(v == xiiVec4I64(5, 8, 9, 3));

    v = xiiVariant(xiiVec4I64(7, 9, 8, 4));
    XII_TEST_BOOL(v == xiiVec4I64(7, 9, 8, 4));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v[3] == 4);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);
    XII_TEST_BOOL(v["w"] == 4);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec4U32")
  {
    xiiVariant v(xiiVec4U32(1, 2, 3, 4));
    TestVariant<xiiVec4U32>(v, xiiVariantType::Vector4U);

    XII_TEST_BOOL(v.Get<xiiVec4U32>() == xiiVec4U32(1, 2, 3, 4));

    XII_TEST_BOOL(v == xiiVariant(xiiVec4U32(1, 2, 3, 4)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec4U32(1, 1, 3, 4)));

    XII_TEST_BOOL(v == xiiVec4U32(1, 2, 3, 4));
    XII_TEST_BOOL(v != xiiVec4U32(1, 4, 3, 4));

    v = xiiVec4U32(5, 8, 9, 3);
    XII_TEST_BOOL(v == xiiVec4U32(5, 8, 9, 3));

    v = xiiVariant(xiiVec4U32(7, 9, 8, 4));
    XII_TEST_BOOL(v == xiiVec4U32(7, 9, 8, 4));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v[3] == 4);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);
    XII_TEST_BOOL(v["w"] == 4);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVec4U64")
  {
    xiiVariant v(xiiVec4U64(1, 2, 3, 4));
    TestVariant<xiiVec4U64>(v, xiiVariantType::Vector4U64);

    XII_TEST_BOOL(v.Get<xiiVec4U64>() == xiiVec4U64(1, 2, 3, 4));

    XII_TEST_BOOL(v == xiiVariant(xiiVec4U64(1, 2, 3, 4)));
    XII_TEST_BOOL(v != xiiVariant(xiiVec4U64(1, 1, 3, 4)));

    XII_TEST_BOOL(v == xiiVec4U64(1, 2, 3, 4));
    XII_TEST_BOOL(v != xiiVec4U64(1, 4, 3, 4));

    v = xiiVec4U64(5, 8, 9, 3);
    XII_TEST_BOOL(v == xiiVec4U64(5, 8, 9, 3));

    v = xiiVariant(xiiVec4U64(7, 9, 8, 4));
    XII_TEST_BOOL(v == xiiVec4U64(7, 9, 8, 4));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v[3] == 4);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);
    XII_TEST_BOOL(v["w"] == 4);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiQuat")
  {
    xiiVariant v(xiiQuat(1, 2, 3, 4));
    TestVariant<xiiQuat>(v, xiiVariantType::Quaternion);

    XII_TEST_BOOL(v.Get<xiiQuat>() == xiiQuat(1, 2, 3, 4));

    XII_TEST_BOOL(v == xiiQuat(1, 2, 3, 4));
    XII_TEST_BOOL(v != xiiQuat(1, 2, 3, 5));

    XII_TEST_BOOL(v == xiiQuat(1, 2, 3, 4));
    XII_TEST_BOOL(v != xiiQuat(1, 4, 3, 4));

    v = xiiQuat(5, 8, 9, 3);
    XII_TEST_BOOL(v == xiiQuat(5, 8, 9, 3));

    v = xiiVariant(xiiQuat(7, 9, 8, 4));
    XII_TEST_BOOL(v == xiiQuat(7, 9, 8, 4));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v[3] == 4);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);
    XII_TEST_BOOL(v["w"] == 4);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);

    xiiTypedPointer ptr = v.GetWriteAccess();
    XII_TEST_BOOL(ptr.m_pObject == &v.Get<xiiQuat>());
    XII_TEST_BOOL(ptr.m_pObject == &v.GetWritable<xiiQuat>());
    XII_TEST_BOOL(ptr.m_pType == xiiGetStaticRTTI<xiiQuat>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiQuatd")
  {
    xiiVariant v(xiiQuatd(1, 2, 3, 4));
    TestVariant<xiiQuatd>(v, xiiVariantType::Quaterniond);

    XII_TEST_BOOL(v.Get<xiiQuatd>() == xiiQuatd(1, 2, 3, 4));

    XII_TEST_BOOL(v == xiiQuatd(1, 2, 3, 4));
    XII_TEST_BOOL(v != xiiQuatd(1, 2, 3, 5));

    XII_TEST_BOOL(v == xiiQuatd(1, 2, 3, 4));
    XII_TEST_BOOL(v != xiiQuatd(1, 4, 3, 4));

    v = xiiQuatd(5, 8, 9, 3);
    XII_TEST_BOOL(v == xiiQuatd(5, 8, 9, 3));

    v = xiiVariant(xiiQuatd(7, 9, 8, 4));
    XII_TEST_BOOL(v == xiiQuatd(7, 9, 8, 4));
    XII_TEST_BOOL(v[0] == 7);
    XII_TEST_BOOL(v[1] == 9);
    XII_TEST_BOOL(v[2] == 8);
    XII_TEST_BOOL(v[3] == 4);
    XII_TEST_BOOL(v["x"] == 7);
    XII_TEST_BOOL(v["y"] == 9);
    XII_TEST_BOOL(v["z"] == 8);
    XII_TEST_BOOL(v["w"] == 4);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);

    xiiTypedPointer ptr = v.GetWriteAccess();
    XII_TEST_BOOL(ptr.m_pObject == &v.Get<xiiQuatd>());
    XII_TEST_BOOL(ptr.m_pObject == &v.GetWritable<xiiQuatd>());
    XII_TEST_BOOL(ptr.m_pType == xiiGetStaticRTTI<xiiQuatd>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiMat3")
  {
    xiiVariant v(xiiMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9));
    TestVariant<xiiMat3>(v, xiiVariantType::Matrix3);

    XII_TEST_BOOL(v.Get<xiiMat3>() == xiiMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9));

    XII_TEST_BOOL(v == xiiVariant(xiiMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9)));
    XII_TEST_BOOL(v != xiiVariant(xiiMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 8)));

    XII_TEST_BOOL(v == xiiMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9));
    XII_TEST_BOOL(v != xiiMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 8));

    v = xiiMat3::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5);
    XII_TEST_BOOL(v == xiiMat3::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5));

    v = xiiVariant(xiiMat3::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 4));
    XII_TEST_BOOL(v == xiiMat3::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 4));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiMat3d")
  {
    xiiVariant v(xiiMat3d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9));
    TestVariant<xiiMat3d>(v, xiiVariantType::Matrix3d);

    XII_TEST_BOOL(v.Get<xiiMat3d>() == xiiMat3d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9));

    XII_TEST_BOOL(v == xiiVariant(xiiMat3d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9)));
    XII_TEST_BOOL(v != xiiVariant(xiiMat3d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 8)));

    XII_TEST_BOOL(v == xiiMat3d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9));
    XII_TEST_BOOL(v != xiiMat3d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 8));

    v = xiiMat3d::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5);
    XII_TEST_BOOL(v == xiiMat3d::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5));

    v = xiiVariant(xiiMat3d::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 4));
    XII_TEST_BOOL(v == xiiMat3d::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 4));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiMat4")
  {
    xiiVariant v(xiiMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    TestVariant<xiiMat4>(v, xiiVariantType::Matrix4);

    XII_TEST_BOOL(v.Get<xiiMat4>() == xiiMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    XII_TEST_BOOL(v == xiiVariant(xiiMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
    XII_TEST_BOOL(v != xiiVariant(xiiMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 15)));

    XII_TEST_BOOL(v == xiiMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    XII_TEST_BOOL(v != xiiMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 2, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    v = xiiMat4::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8);
    XII_TEST_BOOL(v == xiiMat4::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8));

    v = xiiVariant(xiiMat4::MakeFromValues(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));
    XII_TEST_BOOL(v == xiiMat4::MakeFromValues(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiMat4d")
  {
    xiiVariant v(xiiMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    TestVariant<xiiMat4d>(v, xiiVariantType::Matrix4d);

    XII_TEST_BOOL(v.Get<xiiMat4d>() == xiiMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    XII_TEST_BOOL(v == xiiVariant(xiiMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
    XII_TEST_BOOL(v != xiiVariant(xiiMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 15)));

    XII_TEST_BOOL(v == xiiMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    XII_TEST_BOOL(v != xiiMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 2, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    v = xiiMat4d::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8);
    XII_TEST_BOOL(v == xiiMat4d::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8));

    v = xiiVariant(xiiMat4d::MakeFromValues(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));
    XII_TEST_BOOL(v == xiiMat4d::MakeFromValues(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTransform")
  {
    xiiVariant v(xiiTransform(xiiVec3(1, 2, 3), xiiQuat(4, 5, 6, 7), xiiVec3(8, 9, 10)));
    TestVariant<xiiTransform>(v, xiiVariantType::Transform);

    XII_TEST_BOOL(v.Get<xiiTransform>() == xiiTransform(xiiVec3(1, 2, 3), xiiQuat(4, 5, 6, 7), xiiVec3(8, 9, 10)));

    XII_TEST_BOOL(v == xiiTransform(xiiVec3(1, 2, 3), xiiQuat(4, 5, 6, 7), xiiVec3(8, 9, 10)));
    XII_TEST_BOOL(v != xiiTransform(xiiVec3(1, 2, 3), xiiQuat(4, 5, 6, 7), xiiVec3(8, 9, 11)));

    v = xiiTransform(xiiVec3(5, 8, 9), xiiQuat(3, 1, 2, 3), xiiVec3(4, 5, 3));
    XII_TEST_BOOL(v == xiiTransform(xiiVec3(5, 8, 9), xiiQuat(3, 1, 2, 3), xiiVec3(4, 5, 3)));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTransformd")
  {
    xiiVariant v(xiiTransformd(xiiVec3d(1, 2, 3), xiiQuatd(4, 5, 6, 7), xiiVec3d(8, 9, 10)));
    TestVariant<xiiTransformd>(v, xiiVariantType::Transformd);

    XII_TEST_BOOL(v.Get<xiiTransformd>() == xiiTransformd(xiiVec3d(1, 2, 3), xiiQuatd(4, 5, 6, 7), xiiVec3d(8, 9, 10)));

    XII_TEST_BOOL(v == xiiTransformd(xiiVec3d(1, 2, 3), xiiQuatd(4, 5, 6, 7), xiiVec3d(8, 9, 10)));
    XII_TEST_BOOL(v != xiiTransformd(xiiVec3d(1, 2, 3), xiiQuatd(4, 5, 6, 7), xiiVec3d(8, 9, 11)));

    v = xiiTransformd(xiiVec3d(5, 8, 9), xiiQuatd(3, 1, 2, 3), xiiVec3d(4, 5, 3));
    XII_TEST_BOOL(v == xiiTransformd(xiiVec3d(5, 8, 9), xiiQuatd(3, 1, 2, 3), xiiVec3d(4, 5, 3)));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "const char*")
  {
    xiiVariant v("This is a const char array");
    TestVariant<xiiString>(v, xiiVariantType::String);

    XII_TEST_BOOL(v.IsA<const char*>());
    XII_TEST_BOOL(v.IsA<char*>());
    XII_TEST_BOOL(v.Get<xiiString>() == xiiString("This is a const char array"));

    XII_TEST_BOOL(v == xiiVariant("This is a const char array"));
    XII_TEST_BOOL(v != xiiVariant("This is something else"));

    XII_TEST_BOOL(v == xiiString("This is a const char array"));
    XII_TEST_BOOL(v != xiiString("This is another string"));

    XII_TEST_BOOL(v == "This is a const char array");
    XII_TEST_BOOL(v != "This is another string");

    XII_TEST_BOOL(v == (const char*)"This is a const char array");
    XII_TEST_BOOL(v != (const char*)"This is another string");

    v = "blurg!";
    XII_TEST_BOOL(v == xiiString("blurg!"));

    v = xiiVariant("blärg!");
    XII_TEST_BOOL(v == xiiString("blärg!"));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(v.IsString());
    XII_TEST_BOOL(v.CanConvertTo<xiiStringView>());

    xiiStringView view = v.ConvertTo<xiiStringView>();
    XII_TEST_BOOL(view == v.Get<xiiString>());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiString")
  {
    xiiVariant v(xiiString("This is a xiiString"));
    TestVariant<xiiString>(v, xiiVariantType::String);

    XII_TEST_BOOL(v.Get<xiiString>() == xiiString("This is a xiiString"));

    XII_TEST_BOOL(v == xiiVariant(xiiString("This is a xiiString")));
    XII_TEST_BOOL(v == xiiVariant(xiiStringView("This is a xiiString"), false));
    XII_TEST_BOOL(v != xiiVariant(xiiString("This is something else")));

    XII_TEST_BOOL(v == xiiString("This is a xiiString"));
    XII_TEST_BOOL(v != xiiString("This is another xiiString"));

    v = xiiString("blurg!");
    XII_TEST_BOOL(v == xiiString("blurg!"));

    v = xiiVariant(xiiString("blärg!"));
    XII_TEST_BOOL(v == xiiString("blärg!"));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(v.IsString());
    XII_TEST_BOOL(v.CanConvertTo<xiiStringView>());

    xiiStringView view = v.ConvertTo<xiiStringView>();
    XII_TEST_BOOL(view == v.Get<xiiString>());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiStringView")
  {
    const char*   szTemp = "This is a xiiStringView";
    xiiStringView bla(szTemp);
    xiiVariant    v(bla, false);
    TestVariant<xiiStringView>(v, xiiVariantType::StringView);

    const xiiString sCopy = szTemp;
    XII_TEST_BOOL(v.Get<xiiStringView>() == sCopy);

    XII_TEST_BOOL(v == xiiVariant(xiiStringView(sCopy.GetData()), false));
    XII_TEST_BOOL(v == xiiVariant(xiiString("This is a xiiStringView")));
    XII_TEST_BOOL(v != xiiVariant(xiiStringView("This is something else"), false));

    XII_TEST_BOOL(v == xiiStringView(sCopy.GetData()));
    XII_TEST_BOOL(v != xiiStringView("This is something else"));

    v = xiiVariant(xiiStringView("blurg!"), false);
    XII_TEST_BOOL(v == xiiStringView("blurg!"));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(v.IsString());
    XII_TEST_BOOL(v.CanConvertTo<xiiString>());

    xiiString sString = v.ConvertTo<xiiString>();
    XII_TEST_BOOL(sString == v.Get<xiiStringView>());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiHashedString")
  {
    xiiVariant v(xiiMakeHashedString("ABCDE"));
    TestVariant<xiiHashedString>(v, xiiVariantType::HashedString);

    XII_TEST_BOOL(v.Get<xiiHashedString>() == xiiMakeHashedString("ABCDE"));

    XII_TEST_BOOL(v == xiiVariant(xiiMakeHashedString("ABCDE")));
    XII_TEST_BOOL(v != xiiVariant(xiiMakeHashedString("ABCDK")));
    XII_TEST_BOOL(v == xiiVariant(xiiTempHashedString("ABCDE")));
    XII_TEST_BOOL(v != xiiVariant(xiiTempHashedString("ABCDK")));

    XII_TEST_BOOL(v == xiiMakeHashedString("ABCDE"));
    XII_TEST_BOOL(v != xiiMakeHashedString("ABCDK"));
    XII_TEST_BOOL(v == xiiTempHashedString("ABCDE"));
    XII_TEST_BOOL(v != xiiTempHashedString("ABCDK"));

    v = xiiMakeHashedString("HHH");
    XII_TEST_BOOL(v == xiiMakeHashedString("HHH"));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(v.IsString() == false);
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTempHashedString")
  {
    xiiVariant v(xiiTempHashedString("ABCDE"));
    TestVariant<xiiTempHashedString>(v, xiiVariantType::TempHashedString);

    XII_TEST_BOOL(v.Get<xiiTempHashedString>() == xiiTempHashedString("ABCDE"));

    XII_TEST_BOOL(v == xiiVariant(xiiTempHashedString("ABCDE")));
    XII_TEST_BOOL(v != xiiVariant(xiiTempHashedString("ABCDK")));
    XII_TEST_BOOL(v == xiiVariant(xiiMakeHashedString("ABCDE")));
    XII_TEST_BOOL(v != xiiVariant(xiiMakeHashedString("ABCDK")));

    XII_TEST_BOOL(v == xiiTempHashedString("ABCDE"));
    XII_TEST_BOOL(v != xiiTempHashedString("ABCDK"));
    XII_TEST_BOOL(v == xiiMakeHashedString("ABCDE"));
    XII_TEST_BOOL(v != xiiMakeHashedString("ABCDK"));

    v = xiiTempHashedString("HHH");
    XII_TEST_BOOL(v == xiiTempHashedString("HHH"));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(v.IsString() == false);
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiDataBuffer")
  {
    xiiDataBuffer a, a2;
    a.PushBack(xiiUInt8(1));
    a.PushBack(xiiUInt8(2));
    a.PushBack(xiiUInt8(255));

    xiiVariant va(a);
    TestVariant<xiiDataBuffer>(va, xiiVariantType::DataBuffer);

    const xiiDataBuffer&        b  = va.Get<xiiDataBuffer>();
    xiiArrayPtr<const xiiUInt8> b2 = va.Get<xiiDataBuffer>();

    XII_TEST_BOOL(a == b);
    XII_TEST_BOOL(a == b2);

    XII_TEST_BOOL(a != a2);

    XII_TEST_BOOL(va == a);
    XII_TEST_BOOL(va != a2);

    XII_TEST_BOOL(va.IsNumber() == false);
    XII_TEST_BOOL(!va.IsString());
    XII_TEST_BOOL(va.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTime")
  {
    xiiVariant v(xiiTime::MakeFromSeconds(1337));
    TestVariant<xiiTime>(v, xiiVariantType::Time);

    XII_TEST_BOOL(v.Get<xiiTime>() == xiiTime::MakeFromSeconds(1337));

    XII_TEST_BOOL(v == xiiVariant(xiiTime::MakeFromSeconds(1337)));
    XII_TEST_BOOL(v != xiiVariant(xiiTime::MakeFromSeconds(1336)));

    XII_TEST_BOOL(v == xiiTime::MakeFromSeconds(1337));
    XII_TEST_BOOL(v != xiiTime::MakeFromSeconds(1338));

    v = xiiTime::MakeFromSeconds(8472);
    XII_TEST_BOOL(v == xiiTime::MakeFromSeconds(8472));

    v = xiiVariant(xiiTime::MakeFromSeconds(13));
    XII_TEST_BOOL(v == xiiTime::MakeFromSeconds(13));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiUuid")
  {
    xiiUuid    id;
    xiiVariant v(id);
    TestVariant<xiiUuid>(v, xiiVariantType::Uuid);

    XII_TEST_BOOL(v.Get<xiiUuid>() == xiiUuid());

    const xiiUuid uuid = xiiUuid::MakeUuid();
    XII_TEST_BOOL(v != xiiVariant(uuid));
    XII_TEST_BOOL(xiiVariant(uuid).Get<xiiUuid>() == uuid);

    const xiiUuid uuid2 = xiiUuid::MakeUuid();
    XII_TEST_BOOL(xiiVariant(uuid) != xiiVariant(uuid2));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiAngle")
  {
    xiiVariant v(xiiAngle::MakeFromDegree(1337));
    TestVariant<xiiAngle>(v, xiiVariantType::Angle);

    XII_TEST_BOOL(v.Get<xiiAngle>() == xiiAngle::MakeFromDegree(1337));

    XII_TEST_BOOL(v == xiiVariant(xiiAngle::MakeFromDegree(1337)));
    XII_TEST_BOOL(v != xiiVariant(xiiAngle::MakeFromDegree(1336)));

    XII_TEST_BOOL(v == xiiAngle::MakeFromDegree(1337));
    XII_TEST_BOOL(v != xiiAngle::MakeFromDegree(1338));

    v = xiiAngle::MakeFromDegree(8472);
    XII_TEST_BOOL(v == xiiAngle::MakeFromDegree(8472));

    v = xiiVariant(xiiAngle::MakeFromDegree(13));
    XII_TEST_BOOL(v == xiiAngle::MakeFromDegree(13));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiAngled")
  {
    xiiVariant v(xiiAngled::MakeFromDegree(1337));
    TestVariant<xiiAngled>(v, xiiVariantType::Angled);

    XII_TEST_BOOL(v.Get<xiiAngled>() == xiiAngled::MakeFromDegree(1337));

    XII_TEST_BOOL(v == xiiVariant(xiiAngled::MakeFromDegree(1337)));
    XII_TEST_BOOL(v != xiiVariant(xiiAngled::MakeFromDegree(1336)));

    XII_TEST_BOOL(v == xiiAngled::MakeFromDegree(1337));
    XII_TEST_BOOL(v != xiiAngled::MakeFromDegree(1338));

    v = xiiAngled::MakeFromDegree(8472);
    XII_TEST_BOOL(v == xiiAngled::MakeFromDegree(8472));

    v = xiiVariant(xiiAngled::MakeFromDegree(13));
    XII_TEST_BOOL(v == xiiAngled::MakeFromDegree(13));

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVariantArray")
  {
    xiiVariantArray a, a2;
    a.PushBack("This");
    a.PushBack("is a");
    a.PushBack("test");

    xiiVariant va(a);
    XII_TEST_BOOL(va.IsValid());
    XII_TEST_BOOL(va.GetType() == xiiVariant::Type::VariantArray);
    XII_TEST_BOOL(va.IsA<xiiVariantArray>());
    XII_TEST_BOOL(va.GetReflectedType() == nullptr);

    const xiiArrayPtr<const xiiVariant>& b  = va.Get<xiiVariantArray>();
    xiiArrayPtr<const xiiVariant>        b2 = va.Get<xiiVariantArray>();

    XII_TEST_BOOL(a == b);
    XII_TEST_BOOL(a == b2);

    XII_TEST_BOOL(a != a2);

    XII_TEST_BOOL(va == a);
    XII_TEST_BOOL(va != a2);

    XII_TEST_BOOL(va[0] == xiiString("This"));
    XII_TEST_BOOL(va[1] == xiiString("is a"));
    XII_TEST_BOOL(va[2] == xiiString("test"));
    XII_TEST_BOOL(va[4] == xiiVariant());
    XII_TEST_BOOL(!va[4].IsValid());

    XII_TEST_BOOL(va.IsNumber() == false);
    XII_TEST_BOOL(!va.IsString());
    XII_TEST_BOOL(va.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiVariantDictionary")
  {
    xiiVariantDictionary a, a2;
    a["my"]  = true;
    a["luv"] = 4;
    a["pon"] = "ies";

    xiiVariant va(a);
    XII_TEST_BOOL(va.IsValid());
    XII_TEST_BOOL(va.GetType() == xiiVariant::Type::VariantDictionary);
    XII_TEST_BOOL(va.IsA<xiiVariantDictionary>());
    XII_TEST_BOOL(va.GetReflectedType() == nullptr);

    const xiiVariantDictionary& d1 = va.Get<xiiVariantDictionary>();
    xiiVariantDictionary        d2 = va.Get<xiiVariantDictionary>();

    XII_TEST_BOOL(a == d1);
    XII_TEST_BOOL(a == d2);
    XII_TEST_BOOL(d1 == d2);

    XII_TEST_BOOL(va == a);
    XII_TEST_BOOL(va != a2);

    XII_TEST_BOOL(va["my"] == true);
    XII_TEST_BOOL(va["luv"] == 4);
    XII_TEST_BOOL(va["pon"] == xiiString("ies"));
    XII_TEST_BOOL(va["x"] == xiiVariant());
    XII_TEST_BOOL(!va["x"].IsValid());

    XII_TEST_BOOL(va.IsNumber() == false);
    XII_TEST_BOOL(!va.IsString());
    XII_TEST_BOOL(va.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTypedPointer")
  {
    Blubb blubb;
    blubb.u = 1.0f;
    blubb.v = 2.0f;

    Blubb blubb2;

    xiiVariant v(&blubb);

    XII_TEST_BOOL(v.IsValid());
    XII_TEST_BOOL(v.GetType() == xiiVariant::Type::TypedPointer);
    XII_TEST_BOOL(v.IsA<Blubb*>());
    XII_TEST_BOOL(v.Get<Blubb*>() == &blubb);
    XII_TEST_BOOL(v.IsA<xiiReflectedClass*>());
    XII_TEST_BOOL(v.Get<xiiReflectedClass*>() == &blubb);
    XII_TEST_BOOL(v.Get<xiiReflectedClass*>() != &blubb2);
    XII_TEST_BOOL(xiiDynamicCast<Blubb*>(v) == &blubb);
    XII_TEST_BOOL(xiiDynamicCast<xiiVec3*>(v) == nullptr);
    XII_TEST_BOOL(v.IsA<void*>());
    XII_TEST_BOOL(v.Get<void*>() == &blubb);
    XII_TEST_BOOL(v.IsA<const void*>());
    XII_TEST_BOOL(v.Get<const void*>() == &blubb);
    XII_TEST_BOOL(v.GetData() == &blubb);
    XII_TEST_BOOL(v.IsA<xiiTypedPointer>());
    XII_TEST_BOOL(v.GetReflectedType() == xiiGetStaticRTTI<Blubb>());
    XII_TEST_BOOL(!v.IsA<xiiVec3*>());

    xiiTypedPointer ptr = v.Get<xiiTypedPointer>();
    XII_TEST_BOOL(ptr.m_pObject == &blubb);
    XII_TEST_BOOL(ptr.m_pType == xiiGetStaticRTTI<Blubb>());

    xiiTypedPointer ptr2 = v.GetWriteAccess();
    XII_TEST_BOOL(ptr2.m_pObject == &blubb);
    XII_TEST_BOOL(ptr2.m_pType == xiiGetStaticRTTI<Blubb>());

    XII_TEST_BOOL(v[0] == 1.0f);
    XII_TEST_BOOL(v[1] == 2.0f);
    XII_TEST_BOOL(v["u"] == 1.0f);
    XII_TEST_BOOL(v["v"] == 2.0f);
    xiiVariant v2 = &blubb;
    XII_TEST_BOOL(v == v2);
    xiiVariant v3 = ptr;
    XII_TEST_BOOL(v == v3);

    XII_TEST_BOOL(v.IsNumber() == false);
    XII_TEST_BOOL(!v.IsString());
    XII_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTypedPointer nullptr")
  {
    xiiTypedPointer ptr = {nullptr, xiiGetStaticRTTI<Blubb>()};
    xiiVariant      v   = ptr;
    XII_TEST_BOOL(v.IsValid());
    XII_TEST_BOOL(v.GetType() == xiiVariant::Type::TypedPointer);
    XII_TEST_BOOL(v.IsA<Blubb*>());
    XII_TEST_BOOL(v.Get<Blubb*>() == nullptr);
    XII_TEST_BOOL(v.IsA<xiiReflectedClass*>());
    XII_TEST_BOOL(v.Get<xiiReflectedClass*>() == nullptr);
    XII_TEST_BOOL(xiiDynamicCast<Blubb*>(v) == nullptr);
    XII_TEST_BOOL(xiiDynamicCast<xiiVec3*>(v) == nullptr);
    XII_TEST_BOOL(v.IsA<void*>());
    XII_TEST_BOOL(v.Get<void*>() == nullptr);
    XII_TEST_BOOL(v.IsA<const void*>());
    XII_TEST_BOOL(v.Get<const void*>() == nullptr);
    XII_TEST_BOOL(v.IsA<xiiTypedPointer>());
    XII_TEST_BOOL(v.GetReflectedType() == xiiGetStaticRTTI<Blubb>());
    XII_TEST_BOOL(!v.IsA<xiiVec3*>());

    xiiTypedPointer ptr2 = v.Get<xiiTypedPointer>();
    XII_TEST_BOOL(ptr2.m_pObject == nullptr);
    XII_TEST_BOOL(ptr2.m_pType == xiiGetStaticRTTI<Blubb>());

    XII_TEST_BOOL(!v[0].IsValid());
    XII_TEST_BOOL(!v["u"].IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTypedObject inline")
  {
    // xiiVarianceTypeAngle
    {
      // xiiAngle::Degree(90.0f) was replaced with radian as release builds generate a different float then debug.
      xiiVarianceTypeAngle value  = {0.1f, xiiAngle::MakeFromRadian(1.57079637f)};
      xiiVarianceTypeAngle value2 = {0.2f, xiiAngle::MakeFromRadian(1.57079637f)};

      xiiVariant v(value);
      TestVariant<xiiVarianceTypeAngle>(v, xiiVariantType::TypedObject);

      XII_TEST_BOOL(v.IsA<xiiTypedObject>());
      XII_TEST_BOOL(!v.IsA<void*>());
      XII_TEST_BOOL(!v.IsA<const void*>());
      XII_TEST_BOOL(!v.IsA<xiiVec3*>());
      XII_TEST_BOOL(xiiDynamicCast<xiiVec3*>(v) == nullptr);

      const xiiVarianceTypeAngle& valueGet = v.Get<xiiVarianceTypeAngle>();
      XII_TEST_BOOL(value == valueGet);

      xiiVariant va = value;
      XII_TEST_BOOL(v == va);

      xiiVariant v2 = value2;
      XII_TEST_BOOL(v != v2);

      xiiUInt64 uiHash = v.ComputeHash(0);
      XII_TEST_INT(uiHash, 8527525522777555267UL);

      xiiVarianceTypeAngle* pTypedAngle = XII_DEFAULT_NEW(xiiVarianceTypeAngle, {0.1f, xiiAngle::MakeFromRadian(1.57079637f)});
      xiiVariant            copy;
      copy.CopyTypedObject(pTypedAngle, xiiGetStaticRTTI<xiiVarianceTypeAngle>());
      xiiVariant move;
      move.MoveTypedObject(pTypedAngle, xiiGetStaticRTTI<xiiVarianceTypeAngle>());
      XII_TEST_BOOL(v == copy);
      XII_TEST_BOOL(v == move);
    }

    // xiiVarianceTypeAngled
    {
      xiiVarianceTypeAngled value  = {0.1, xiiAngled::MakeFromRadian(1.57079637)};
      xiiVarianceTypeAngled value2 = {0.2, xiiAngled::MakeFromRadian(1.57079637)};

      xiiVariant v(value);
      TestVariant<xiiVarianceTypeAngled>(v, xiiVariantType::TypedObject);

      XII_TEST_BOOL(v.IsA<xiiTypedObject>());
      XII_TEST_BOOL(!v.IsA<void*>());
      XII_TEST_BOOL(!v.IsA<const void*>());
      XII_TEST_BOOL(!v.IsA<xiiVec3d*>());
      XII_TEST_BOOL(xiiDynamicCast<xiiVec3d*>(v) == nullptr);

      const xiiVarianceTypeAngled& valueGet = v.Get<xiiVarianceTypeAngled>();
      XII_TEST_BOOL(value == valueGet);

      xiiVariant va = value;
      XII_TEST_BOOL(v == va);

      xiiVariant v2 = value2;
      XII_TEST_BOOL(v != v2);

      xiiUInt64 uiHash = v.ComputeHash(0);
      XII_TEST_INT(uiHash, 5230335272281280496UL);

      xiiVarianceTypeAngled* pTypedAngle = XII_DEFAULT_NEW(xiiVarianceTypeAngled, {0.1, xiiAngled::MakeFromRadian(1.57079637)});
      xiiVariant             copy;
      copy.CopyTypedObject(pTypedAngle, xiiGetStaticRTTI<xiiVarianceTypeAngled>());
      xiiVariant move;
      move.MoveTypedObject(pTypedAngle, xiiGetStaticRTTI<xiiVarianceTypeAngled>());
      XII_TEST_BOOL(v == copy);
      XII_TEST_BOOL(v == move);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTypedObject shared")
  {
    xiiTypedObjectStruct data;
    xiiVariant           v = data;
    XII_TEST_BOOL(v.IsValid());
    XII_TEST_BOOL(v.GetType() == xiiVariant::Type::TypedObject);
    XII_TEST_BOOL(v.IsA<xiiTypedObject>());
    XII_TEST_BOOL(v.IsA<xiiTypedObjectStruct>());
    XII_TEST_BOOL(!v.IsA<void*>());
    XII_TEST_BOOL(!v.IsA<const void*>());
    XII_TEST_BOOL(!v.IsA<xiiVec3*>());
    XII_TEST_BOOL(xiiDynamicCast<xiiVec3*>(v) == nullptr);
    XII_TEST_BOOL(v.GetReflectedType() == xiiGetStaticRTTI<xiiTypedObjectStruct>());

    xiiVariant v2 = v;

    xiiTypedPointer ptr = v.GetWriteAccess();
    XII_TEST_BOOL(ptr.m_pObject == &v.Get<xiiTypedObjectStruct>());
    XII_TEST_BOOL(ptr.m_pObject == &v.GetWritable<xiiTypedObjectStruct>());
    XII_TEST_BOOL(ptr.m_pObject != &v2.Get<xiiTypedObjectStruct>());
    XII_TEST_BOOL(ptr.m_pType == xiiGetStaticRTTI<xiiTypedObjectStruct>());

    XII_TEST_BOOL(xiiReflectionUtils::IsEqual(ptr.m_pObject, &v2.Get<xiiTypedObjectStruct>(), xiiGetStaticRTTI<xiiTypedObjectStruct>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (bool)")
  {
    xiiVariant v(true);

    XII_TEST_BOOL(v.CanConvertTo<bool>());
    XII_TEST_BOOL(v.CanConvertTo<xiiInt32>());

    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Invalid) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Bool));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int8));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt8));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int16));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt16));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int32));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt32));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int64));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt64));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Float));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Double));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Color) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2I) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3I) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4I) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Quaternion) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix3) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix4) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Transform) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Transformd) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::String));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::StringView) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::DataBuffer) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Time) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Angle) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Angled) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::VariantArray) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::VariantDictionary) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TypedPointer) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TypedObject) == false);

    XII_TEST_BOOL(v.ConvertTo<bool>() == true);
    XII_TEST_BOOL(v.ConvertTo<xiiInt8>() == 1);
    XII_TEST_BOOL(v.ConvertTo<xiiUInt8>() == 1);
    XII_TEST_BOOL(v.ConvertTo<xiiInt16>() == 1);
    XII_TEST_BOOL(v.ConvertTo<xiiUInt16>() == 1);
    XII_TEST_BOOL(v.ConvertTo<xiiInt32>() == 1);
    XII_TEST_BOOL(v.ConvertTo<xiiUInt32>() == 1);
    XII_TEST_BOOL(v.ConvertTo<xiiInt64>() == 1);
    XII_TEST_BOOL(v.ConvertTo<xiiUInt64>() == 1);
    XII_TEST_BOOL(v.ConvertTo<float>() == 1.0f);
    XII_TEST_BOOL(v.ConvertTo<double>() == 1.0);
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "true");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("true"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("true"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Bool).Get<bool>() == true);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int8).Get<xiiInt8>() == 1);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt8).Get<xiiUInt8>() == 1);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int16).Get<xiiInt16>() == 1);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt16).Get<xiiUInt16>() == 1);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int32).Get<xiiInt32>() == 1);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt32).Get<xiiUInt32>() == 1);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int64).Get<xiiInt64>() == 1);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt64).Get<xiiUInt64>() == 1);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Float).Get<float>() == 1.0f);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Double).Get<double>() == 1.0);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "true");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("true"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("true"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiInt8)")
  {
    xiiVariant v((xiiInt8)3);
    TestNumberCanConvertTo(v);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiUInt8)")
  {
    xiiVariant v((xiiUInt8)3);
    TestNumberCanConvertTo(v);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiInt16)")
  {
    xiiVariant v((xiiInt16)3);
    TestNumberCanConvertTo(v);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiUInt16)")
  {
    xiiVariant v((xiiUInt16)3);
    TestNumberCanConvertTo(v);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiInt32)")
  {
    xiiVariant v((xiiInt32)3);
    TestNumberCanConvertTo(v);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiUInt32)")
  {
    xiiVariant v((xiiUInt32)3);
    TestNumberCanConvertTo(v);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiInt64)")
  {
    xiiVariant v((xiiInt64)3);
    TestNumberCanConvertTo(v);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiUInt64)")
  {
    xiiVariant v((xiiUInt64)3);
    TestNumberCanConvertTo(v);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (float)")
  {
    xiiVariant v((float)3.0f);
    TestNumberCanConvertTo(v);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (double)")
  {
    xiiVariant v((double)3.0f);
    TestNumberCanConvertTo(v);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (Color)")
  {
    xiiColor   c(3, 3, 4, 0);
    xiiVariant v(c);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Color, xiiVariant::Type::ColorGamma);

    xiiResult conversionResult = XII_FAILURE;
    XII_TEST_BOOL(v.ConvertTo<xiiColor>(&conversionResult) == c);
    XII_TEST_BOOL(conversionResult.Succeeded());

    XII_TEST_BOOL(v.ConvertTo<xiiString>(&conversionResult) == "{ r=3, g=3, b=4, a=0 }");
    XII_TEST_BOOL(conversionResult.Succeeded());

    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ r=3, g=3, b=4, a=0 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ r=3, g=3, b=4, a=0 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Color).Get<xiiColor>() == c);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ r=3, g=3, b=4, a=0 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ r=3, g=3, b=4, a=0 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ r=3, g=3, b=4, a=0 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (ColorGamma)")
  {
    xiiColorGammaUB c(0, 128, 64, 255);
    xiiVariant      v(c);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::ColorGamma, xiiVariant::Type::Color);

    xiiResult conversionResult = XII_FAILURE;
    XII_TEST_BOOL(v.ConvertTo<xiiColorGammaUB>(&conversionResult) == c);
    XII_TEST_BOOL(conversionResult.Succeeded());

    xiiString val = v.ConvertTo<xiiString>(&conversionResult);
    XII_TEST_BOOL(val == "{ r=0, g=128, b=64, a=255 }");
    XII_TEST_BOOL(conversionResult.Succeeded());

    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ r=0, g=128, b=64, a=255 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ r=0, g=128, b=64, a=255 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::ColorGamma).Get<xiiColorGammaUB>() == c);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ r=0, g=128, b=64, a=255 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ r=0, g=128, b=64, a=255 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ r=0, g=128, b=64, a=255 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec2)")
  {
    xiiVec2    vec(3.0f, 4.0f);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector2, xiiVariant::Type::Vector2I, xiiVariant::Type::Vector2U, xiiVariant::Type::Vector2d, xiiVariant::Type::Vector2I64, xiiVariant::Type::Vector2U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec2>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec2d>() == xiiVec2d(3.0, 4.0));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2I32>() == xiiVec2I32(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2U32>() == xiiVec2U32(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2I64>() == xiiVec2I64(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2U64>() == xiiVec2U64(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2).Get<xiiVec2>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2I).Get<xiiVec2I32>() == xiiVec2I32(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2U).Get<xiiVec2U32>() == xiiVec2U32(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2I64).Get<xiiVec2I64>() == xiiVec2I64(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2U64).Get<xiiVec2U64>() == xiiVec2U64(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec2d)")
  {
    xiiVec2d   vec(3.0, 4.0);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector2d, xiiVariant::Type::Vector2I, xiiVariant::Type::Vector2U, xiiVariant::Type::Vector2, xiiVariant::Type::Vector2I64, xiiVariant::Type::Vector2U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec2d>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec2>() == xiiVec2(3.0f, 4.0f));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2I32>() == xiiVec2I32(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2U32>() == xiiVec2U32(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2I64>() == xiiVec2I64(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2U64>() == xiiVec2U64(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2d).Get<xiiVec2d>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2).Get<xiiVec2>() == xiiVec2(3.0f, 4.0f));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2I).Get<xiiVec2I32>() == xiiVec2I32(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2U).Get<xiiVec2U32>() == xiiVec2U32(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2I64).Get<xiiVec2I64>() == xiiVec2I64(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2U64).Get<xiiVec2U64>() == xiiVec2U64(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec3)")
  {
    xiiVec3    vec(3.0f, 4.0f, 6.0f);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector3, xiiVariant::Type::Vector3I, xiiVariant::Type::Vector3U, xiiVariant::Type::Vector3d, xiiVariant::Type::Vector3I64, xiiVariant::Type::Vector3U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec3>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec3d>() == xiiVec3d(3.0, 4.0, 6.0));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3I32>() == xiiVec3I32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3U32>() == xiiVec3U32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3I64>() == xiiVec3I64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3U64>() == xiiVec3U64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=6 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=6 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=6 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3).Get<xiiVec3>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3d).Get<xiiVec3d>() == xiiVec3d(3.0, 4.0, 6.0));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3I).Get<xiiVec3I32>() == xiiVec3I32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3U).Get<xiiVec3U32>() == xiiVec3U32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3I64).Get<xiiVec3I64>() == xiiVec3I64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3U64).Get<xiiVec3U64>() == xiiVec3U64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=6 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=6 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=6 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec3d)")
  {
    xiiVec3d   vec(3.0, 4.0, 6.0);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector3d, xiiVariant::Type::Vector3I, xiiVariant::Type::Vector3U, xiiVariant::Type::Vector3, xiiVariant::Type::Vector3I64, xiiVariant::Type::Vector3U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec3d>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec3>() == xiiVec3(3.0f, 4.0f, 6.0f));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3I32>() == xiiVec3I32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3U32>() == xiiVec3U32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3I64>() == xiiVec3I64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3U64>() == xiiVec3U64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=6 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=6 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=6 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3d).Get<xiiVec3d>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3).Get<xiiVec3>() == xiiVec3(3.0f, 4.0f, 6.0f));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3I).Get<xiiVec3I32>() == xiiVec3I32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3U).Get<xiiVec3U32>() == xiiVec3U32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3I64).Get<xiiVec3I64>() == xiiVec3I64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3U64).Get<xiiVec3U64>() == xiiVec3U64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=6 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=6 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=6 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec4)")
  {
    xiiVec4    vec(3.0f, 4.0f, 3.0f, 56.0f);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector4, xiiVariant::Type::Vector4I, xiiVariant::Type::Vector4U, xiiVariant::Type::Vector4d, xiiVariant::Type::Vector4I64, xiiVariant::Type::Vector4U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec4>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec4d>() == xiiVec4d(3.0, 4.0, 3.0, 56.0));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4I32>() == xiiVec4I32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4U32>() == xiiVec4U32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4I64>() == xiiVec4I64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4U64>() == xiiVec4U64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4).Get<xiiVec4>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4d).Get<xiiVec4d>() == xiiVec4d(3.0, 4.0, 3.0, 56.0));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4I).Get<xiiVec4I32>() == xiiVec4I32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4U).Get<xiiVec4U32>() == xiiVec4U32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4I64).Get<xiiVec4I64>() == xiiVec4I64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4U64).Get<xiiVec4U64>() == xiiVec4U64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec4d)")
  {
    xiiVec4d   vec(3.0, 4.0, 3.0, 56.0);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector4d, xiiVariant::Type::Vector4I, xiiVariant::Type::Vector4U, xiiVariant::Type::Vector4, xiiVariant::Type::Vector4I64, xiiVariant::Type::Vector4U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec4d>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec4>() == xiiVec4(3.0f, 4.0f, 3.0f, 56.0f));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4I32>() == xiiVec4I32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4U32>() == xiiVec4U32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4I64>() == xiiVec4I64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4U64>() == xiiVec4U64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4d).Get<xiiVec4d>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4).Get<xiiVec4>() == xiiVec4(3.0f, 4.0f, 3.0f, 56.0f));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4I).Get<xiiVec4I32>() == xiiVec4I32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4U).Get<xiiVec4U32>() == xiiVec4U32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4I64).Get<xiiVec4I64>() == xiiVec4I64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4U64).Get<xiiVec4U64>() == xiiVec4U64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec2I32)")
  {
    xiiVec2I32 vec(3, 4);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector2I, xiiVariant::Type::Vector2U, xiiVariant::Type::Vector2, xiiVariant::Type::Vector2d, xiiVariant::Type::Vector2I64, xiiVariant::Type::Vector2U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec2I32>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec2>() == xiiVec2(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2U32>() == xiiVec2U32(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2I64>() == xiiVec2I64(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2U64>() == xiiVec2U64(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2I).Get<xiiVec2I32>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2).Get<xiiVec2>() == xiiVec2(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2d).Get<xiiVec2d>() == xiiVec2d(3.0, 4.0));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2U).Get<xiiVec2U32>() == xiiVec2U32(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2I64).Get<xiiVec2I64>() == xiiVec2I64(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2U64).Get<xiiVec2U64>() == xiiVec2U64(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec3I32)")
  {
    xiiVec3I32 vec(3, 4, 6);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector3I, xiiVariant::Type::Vector3U, xiiVariant::Type::Vector3, xiiVariant::Type::Vector3d, xiiVariant::Type::Vector3I64, xiiVariant::Type::Vector3U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec3I32>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec3>() == xiiVec3(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3d>() == xiiVec3d(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3U32>() == xiiVec3U32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3I64>() == xiiVec3I64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3U64>() == xiiVec3U64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=6 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=6 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=6 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3I).Get<xiiVec3I32>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3).Get<xiiVec3>() == xiiVec3(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3d).Get<xiiVec3d>() == xiiVec3d(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3U).Get<xiiVec3U32>() == xiiVec3U32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3I64).Get<xiiVec3I64>() == xiiVec3I64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3U64).Get<xiiVec3U64>() == xiiVec3U64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=6 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=6 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=6 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec4I32)")
  {
    xiiVec4I32 vec(3, 4, 3, 56);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector4I, xiiVariant::Type::Vector4U, xiiVariant::Type::Vector4, xiiVariant::Type::Vector4d, xiiVariant::Type::Vector4I64, xiiVariant::Type::Vector4U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec4I32>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec4>() == xiiVec4(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4d>() == xiiVec4d(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4U32>() == xiiVec4U32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4I64>() == xiiVec4I64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4U64>() == xiiVec4U64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4I).Get<xiiVec4I32>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4).Get<xiiVec4>() == xiiVec4(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4d).Get<xiiVec4d>() == xiiVec4d(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4U).Get<xiiVec4U32>() == xiiVec4U32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4I64).Get<xiiVec4I64>() == xiiVec4I64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4U64).Get<xiiVec4U64>() == xiiVec4U64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec2I64)")
  {
    xiiVec2I64 vec(3, 4);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector2I64, xiiVariant::Type::Vector2U, xiiVariant::Type::Vector2, xiiVariant::Type::Vector2d, xiiVariant::Type::Vector2I, xiiVariant::Type::Vector2U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec2I64>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec2>() == xiiVec2(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2I32>() == xiiVec2I32(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2U32>() == xiiVec2U32(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2U64>() == xiiVec2U64(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2I64).Get<xiiVec2I64>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2).Get<xiiVec2>() == xiiVec2(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2d).Get<xiiVec2d>() == xiiVec2d(3.0, 4.0));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2I).Get<xiiVec2I32>() == xiiVec2I32(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2U).Get<xiiVec2U32>() == xiiVec2U32(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2U64).Get<xiiVec2U64>() == xiiVec2U64(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec3I64)")
  {
    xiiVec3I64 vec(3, 4, 6);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector3I64, xiiVariant::Type::Vector3U, xiiVariant::Type::Vector3, xiiVariant::Type::Vector3d, xiiVariant::Type::Vector3I, xiiVariant::Type::Vector3U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec3I64>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec3>() == xiiVec3(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3d>() == xiiVec3d(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3I32>() == xiiVec3I32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3U32>() == xiiVec3U32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3U64>() == xiiVec3U64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=6 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=6 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=6 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3I64).Get<xiiVec3I64>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3).Get<xiiVec3>() == xiiVec3(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3d).Get<xiiVec3d>() == xiiVec3d(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3I).Get<xiiVec3I32>() == xiiVec3I32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3U).Get<xiiVec3U32>() == xiiVec3U32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3U64).Get<xiiVec3U64>() == xiiVec3U64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=6 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=6 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=6 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec4I64)")
  {
    xiiVec4I64 vec(3, 4, 3, 56);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector4I64, xiiVariant::Type::Vector4U, xiiVariant::Type::Vector4, xiiVariant::Type::Vector4d, xiiVariant::Type::Vector4I, xiiVariant::Type::Vector4U64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec4I64>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec4>() == xiiVec4(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4d>() == xiiVec4d(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4I32>() == xiiVec4I32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4U32>() == xiiVec4U32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4U64>() == xiiVec4U64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4I64).Get<xiiVec4I64>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4).Get<xiiVec4>() == xiiVec4(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4d).Get<xiiVec4d>() == xiiVec4d(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4I).Get<xiiVec4I32>() == xiiVec4I32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4U).Get<xiiVec4U32>() == xiiVec4U32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4U64).Get<xiiVec4U64>() == xiiVec4U64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec2U64)")
  {
    xiiVec2U64 vec(3, 4);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector2U64, xiiVariant::Type::Vector2U, xiiVariant::Type::Vector2, xiiVariant::Type::Vector2d, xiiVariant::Type::Vector2I, xiiVariant::Type::Vector2I64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec2U64>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec2>() == xiiVec2(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2I32>() == xiiVec2I32(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2U32>() == xiiVec2U32(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiVec2I64>() == xiiVec2I64(3, 4));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2U64).Get<xiiVec2U64>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2).Get<xiiVec2>() == xiiVec2(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2d).Get<xiiVec2d>() == xiiVec2d(3.0, 4.0));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2I).Get<xiiVec2I32>() == xiiVec2I32(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2U).Get<xiiVec2U32>() == xiiVec2U32(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector2I64).Get<xiiVec2I64>() == xiiVec2I64(3, 4));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec3U64)")
  {
    xiiVec3U64 vec(3, 4, 6);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector3U64, xiiVariant::Type::Vector3U, xiiVariant::Type::Vector3, xiiVariant::Type::Vector3d, xiiVariant::Type::Vector3I, xiiVariant::Type::Vector3I64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec3U64>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec3>() == xiiVec3(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3d>() == xiiVec3d(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3I32>() == xiiVec3I32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3U32>() == xiiVec3U32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiVec3I64>() == xiiVec3I64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=6 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=6 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=6 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3U64).Get<xiiVec3U64>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3).Get<xiiVec3>() == xiiVec3(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3d).Get<xiiVec3d>() == xiiVec3d(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3I).Get<xiiVec3I32>() == xiiVec3I32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3U).Get<xiiVec3U32>() == xiiVec3U32(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector3I64).Get<xiiVec3I64>() == xiiVec3I64(3, 4, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=6 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=6 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=6 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVec4U64)")
  {
    xiiVec4U64 vec(3, 4, 3, 56);
    xiiVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Vector4U64, xiiVariant::Type::Vector4U, xiiVariant::Type::Vector4, xiiVariant::Type::Vector4d, xiiVariant::Type::Vector4I, xiiVariant::Type::Vector4I64);

    XII_TEST_BOOL(v.ConvertTo<xiiVec4U64>() == vec);
    XII_TEST_BOOL(v.ConvertTo<xiiVec4>() == xiiVec4(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4d>() == xiiVec4d(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4I32>() == xiiVec4I32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4U32>() == xiiVec4U32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiVec4I64>() == xiiVec4I64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4U64).Get<xiiVec4U64>() == vec);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4).Get<xiiVec4>() == xiiVec4(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4d).Get<xiiVec4d>() == xiiVec4d(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4I).Get<xiiVec4I32>() == xiiVec4I32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4U).Get<xiiVec4U32>() == xiiVec4U32(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Vector4I64).Get<xiiVec4I64>() == xiiVec4I64(3, 4, 3, 56));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiQuat)")
  {
    xiiQuat    q(3.0f, 4.0f, 3.0f, 56.0f);
    xiiVariant v(q);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Quaternion, xiiVariant::Type::Quaterniond);

    XII_TEST_BOOL(v.ConvertTo<xiiQuat>() == q);
    XII_TEST_BOOL(v.ConvertTo<xiiQuatd>() == xiiQuatd(3.0, 4.0, 3.0, 56.0));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Quaternion).Get<xiiQuat>() == q);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Quaterniond).Get<xiiQuatd>() == xiiQuatd(3.0, 4.0, 3.0, 56.0));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiQuatd)")
  {
    xiiQuatd   q(3.0, 4.0, 3.0, 56.0);
    xiiVariant v(q);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Quaterniond, xiiVariant::Type::Quaternion);

    XII_TEST_BOOL(v.ConvertTo<xiiQuatd>() == q);
    XII_TEST_BOOL(v.ConvertTo<xiiQuat>() == xiiQuat(3.0f, 4.0f, 3.0f, 56.0f));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Quaterniond).Get<xiiQuatd>() == q);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Quaternion).Get<xiiQuat>() == xiiQuat(3.0f, 4.0f, 3.0f, 56.0f));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ x=3, y=4, z=3, w=56 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiMat3)")
  {
    xiiMat3    m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    xiiVariant v(m);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Matrix3, xiiVariant::Type::Matrix3d);

    XII_TEST_BOOL(v.ConvertTo<xiiMat3>() == m);
    XII_TEST_BOOL(v.ConvertTo<xiiMat3d>() == xiiMat3d(1, 2, 3, 4, 5, 6, 7, 8, 9));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Matrix3).Get<xiiMat3>() == m);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Matrix3d).Get<xiiMat3d>() == xiiMat3d(1, 2, 3, 4, 5, 6, 7, 8, 9));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiMat3d)")
  {
    xiiMat3d   m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    xiiVariant v(m);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Matrix3d, xiiVariant::Type::Matrix3);

    XII_TEST_BOOL(v.ConvertTo<xiiMat3d>() == m);
    XII_TEST_BOOL(v.ConvertTo<xiiMat3>() == xiiMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Matrix3d).Get<xiiMat3d>() == m);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Matrix3).Get<xiiMat3>() == xiiMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiMat4)")
  {
    xiiMat4    m(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6);
    xiiVariant v(m);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Matrix4, xiiVariant::Type::Matrix4d);

    XII_TEST_BOOL(v.ConvertTo<xiiMat4>() == m);
    XII_TEST_BOOL(v.ConvertTo<xiiMat4d>() == xiiMat4d(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Matrix4).Get<xiiMat4>() == m);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Matrix4d).Get<xiiMat4d>() == xiiMat4d(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiMat4d)")
  {
    xiiMat4d   m(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6);
    xiiVariant v(m);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Matrix4d, xiiVariant::Type::Matrix4);

    XII_TEST_BOOL(v.ConvertTo<xiiMat4d>() == m);
    XII_TEST_BOOL(v.ConvertTo<xiiMat4>() == xiiMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Matrix4d).Get<xiiMat4d>() == m);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Matrix4).Get<xiiMat4>() == xiiMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiTransform)")
  {
    xiiTransform t(xiiVec3(1, 2, 3), xiiQuat(4, 5, 6, 7), xiiVec3(8, 9, 10));
    xiiVariant   v(t);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Transform, xiiVariant::Type::Transformd);

    XII_TEST_BOOL(v.ConvertTo<xiiTransform>() == t);
    XII_TEST_BOOL(v.ConvertTo<xiiTransformd>() == xiiTransformd(xiiVec3d(1, 2, 3), xiiQuatd(4, 5, 6, 7), xiiVec3d(8, 9, 10)));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ position={ x=1, y=2, z=3 }, rotation={ x=4, y=5, z=6, w=7 }, scale={ x=8, y=9, z=10 } }");

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Transform).Get<xiiTransform>() == t);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Transformd).Get<xiiTransformd>() == xiiTransformd(xiiVec3d(1, 2, 3), xiiQuatd(4, 5, 6, 7), xiiVec3d(8, 9, 10)));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ position={ x=1, y=2, z=3 }, rotation={ x=4, y=5, z=6, w=7 }, scale={ x=8, y=9, z=10 } }");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiTransformd)")
  {
    xiiTransformd t(xiiVec3d(1, 2, 3), xiiQuatd(4, 5, 6, 7), xiiVec3d(8, 9, 10));
    xiiVariant    v(t);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Transformd, xiiVariant::Type::Transform);

    XII_TEST_BOOL(v.ConvertTo<xiiTransformd>() == t);
    XII_TEST_BOOL(v.ConvertTo<xiiTransform>() == xiiTransform(xiiVec3(1, 2, 3), xiiQuat(4, 5, 6, 7), xiiVec3(8, 9, 10)));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "{ position={ x=1, y=2, z=3 }, rotation={ x=4, y=5, z=6, w=7 }, scale={ x=8, y=9, z=10 } }");

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Transformd).Get<xiiTransformd>() == t);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Transform).Get<xiiTransform>() == xiiTransform(xiiVec3(1, 2, 3), xiiQuat(4, 5, 6, 7), xiiVec3(8, 9, 10)));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "{ position={ x=1, y=2, z=3 }, rotation={ x=4, y=5, z=6, w=7 }, scale={ x=8, y=9, z=10 } }");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiString)")
  {
    xiiVariant v("ich hab keine Lust mehr");

    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Invalid) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Bool));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int8));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt8));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int16));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt16));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int32));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt32));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int64));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt64));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Float));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Double));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Color) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2d) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3d) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4d) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2I) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2I64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3I) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3I64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4I) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4I64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2U) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2U64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3U) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3U64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4U) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4U64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Quaternion) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Quaterniond) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix3) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix3d) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix4) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix4d) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Transform) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Transformd) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::String));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::StringView));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::HashedString));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TempHashedString));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::DataBuffer) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Time) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Angle) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Angled) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::ColorGamma) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::VariantArray) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::VariantDictionary) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TypedPointer) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TypedObject) == false);

    {
      xiiResult ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == false);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<xiiInt8>(&ConversionStatus) == 0);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<xiiUInt8>(&ConversionStatus) == 0);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<xiiInt16>(&ConversionStatus) == 0);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<xiiUInt16>(&ConversionStatus) == 0);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<xiiInt32>(&ConversionStatus) == 0);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<xiiUInt32>(&ConversionStatus) == 0);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<xiiInt64>(&ConversionStatus) == 0);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<xiiUInt64>(&ConversionStatus) == 0);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 0.0f);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 0.0);
      XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<xiiHashedString>(&ConversionStatus) == xiiMakeHashedString("ich hab keine Lust mehr"));
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_SUCCESS;
      XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>(&ConversionStatus) == xiiTempHashedString("ich hab keine Lust mehr"));
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "true";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == true);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Bool, &ConversionStatus).Get<bool>() == true);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "-128";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<xiiInt8>(&ConversionStatus) == -128);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int8, &ConversionStatus).Get<xiiInt8>() == -128);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "255";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<xiiUInt8>(&ConversionStatus) == 255);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt8, &ConversionStatus).Get<xiiUInt8>() == 255);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "-5643";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<xiiInt16>(&ConversionStatus) == -5643);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int16, &ConversionStatus).Get<xiiInt16>() == -5643);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "9001";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<xiiUInt16>(&ConversionStatus) == 9001);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt16, &ConversionStatus).Get<xiiUInt16>() == 9001);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "46";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<xiiInt32>(&ConversionStatus) == 46);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int32, &ConversionStatus).Get<xiiInt32>() == 46);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "356";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<xiiUInt32>(&ConversionStatus) == 356);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt32, &ConversionStatus).Get<xiiUInt32>() == 356);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "64";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<xiiInt64>(&ConversionStatus) == 64);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Int64, &ConversionStatus).Get<xiiInt64>() == 64);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "6464";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<xiiUInt64>(&ConversionStatus) == 6464);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::UInt64, &ConversionStatus).Get<xiiUInt64>() == 6464);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "0.07564f";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 0.07564f);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Float, &ConversionStatus).Get<float>() == 0.07564f);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }

    {
      v                          = "0.4453";
      xiiResult ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 0.4453);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

      ConversionStatus = XII_FAILURE;
      XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Double, &ConversionStatus).Get<double>() == 0.4453);
      XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiStringView)")
  {
    xiiStringView va0("Test String");
    xiiVariant    v(va0, false);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::StringView);

    XII_TEST_BOOL(v.ConvertTo<xiiStringView>() == va0);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::StringView).Get<xiiStringView>() == va0);

    {
      xiiVariant va, va2;

      va = "Bla";
      XII_TEST_BOOL(va.IsA<xiiString>());
      XII_TEST_BOOL(va.CanConvertTo<xiiString>());
      XII_TEST_BOOL(va.CanConvertTo<xiiStringView>());

      va = xiiVariant("Bla"_xiisv, false);
      XII_TEST_BOOL(va.IsA<xiiStringView>());
      XII_TEST_BOOL(va.CanConvertTo<xiiString>());
      XII_TEST_BOOL(va.CanConvertTo<xiiStringView>());

      va2 = va;
      XII_TEST_BOOL(va2.IsA<xiiStringView>());
      XII_TEST_BOOL(va2.CanConvertTo<xiiString>());
      XII_TEST_BOOL(va2.CanConvertTo<xiiStringView>());
      XII_TEST_BOOL(va2.ConvertTo<xiiStringView>() == "Bla");
      XII_TEST_BOOL(va2.ConvertTo<xiiString>() == "Bla");

      xiiVariant va3 = va2.ConvertTo(xiiVariantType::StringView);
      XII_TEST_BOOL(va3.IsA<xiiStringView>());
      XII_TEST_BOOL(va3.ConvertTo<xiiString>() == "Bla");

      va = "Blub";
      XII_TEST_BOOL(va.IsA<xiiString>());

      xiiVariant va4 = va.ConvertTo(xiiVariantType::StringView);
      XII_TEST_BOOL(va4.IsA<xiiStringView>());
      XII_TEST_BOOL(va4.ConvertTo<xiiString>() == "Blub");
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiHashedString)")
  {
    xiiVariant v(xiiMakeHashedString("78"));

    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Invalid) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Bool));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int8));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt8));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int16));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt16));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int32));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt32));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Int64));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::UInt64));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Float));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Double));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Color) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2I) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3I) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4I) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2I64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3I64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4I64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2U) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3U) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4U) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2U64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3U64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4U64) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector2d) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector3d) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Vector4d) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Quaternion) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Quaterniond) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix3) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix3d) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix4) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Matrix4d) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Transform) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Transformd) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::String));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::StringView));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::HashedString));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TempHashedString));
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::DataBuffer) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Time) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Angle) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::Angled) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::ColorGamma) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::VariantArray) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::VariantDictionary) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TypedPointer) == false);
    XII_TEST_BOOL(v.CanConvertTo(xiiVariant::Type::TypedObject) == false);

    xiiResult ConversionStatus = XII_SUCCESS;
    XII_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == false);
    XII_TEST_BOOL(ConversionStatus == XII_FAILURE);

    ConversionStatus = XII_FAILURE;
    XII_TEST_INT(v.ConvertTo<xiiInt8>(&ConversionStatus), 78);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_INT(v.ConvertTo<xiiUInt8>(&ConversionStatus), 78);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_INT(v.ConvertTo<xiiInt16>(&ConversionStatus), 78);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_INT(v.ConvertTo<xiiUInt16>(&ConversionStatus), 78);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_INT(v.ConvertTo<xiiInt32>(&ConversionStatus), 78);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_INT(v.ConvertTo<xiiUInt32>(&ConversionStatus), 78);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_INT(v.ConvertTo<xiiInt64>(&ConversionStatus), 78);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_INT(v.ConvertTo<xiiUInt64>(&ConversionStatus), 78);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 78.0f);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 78.0);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_STRING(v.ConvertTo<xiiString>(&ConversionStatus), "78");
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_BOOL(v.ConvertTo<xiiStringView>(&ConversionStatus) == "78"_xiisv);
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);

    ConversionStatus = XII_FAILURE;
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>(&ConversionStatus) == xiiTempHashedString("78"));
    XII_TEST_BOOL(ConversionStatus == XII_SUCCESS);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiTempHashedString)")
  {
    xiiTempHashedString s("VVVV");
    xiiVariant          v(s);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::TempHashedString);

    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("VVVV"));
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "0x69d489c8b7fa5f47");

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("VVVV"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "0x69d489c8b7fa5f47");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiDataBuffer)")
  {
    xiiDataBuffer va;
    va.PushBack(255);
    va.PushBack(4);
    xiiVariant v(va);

    TestCanOnlyConvertToID(v, xiiVariant::Type::DataBuffer);

    XII_TEST_BOOL(v.ConvertTo<xiiDataBuffer>() == va);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::DataBuffer).Get<xiiDataBuffer>() == va);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiTime)")
  {
    xiiTime    t = xiiTime::MakeFromSeconds(123.0);
    xiiVariant v(t);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Time);

    XII_TEST_BOOL(v.ConvertTo<xiiTime>() == t);
    // XII_TEST_BOOL(v.ConvertTo<xiiString>() == "");

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Time).Get<xiiTime>() == t);
    // XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiUuid)")
  {
    xiiUuid uuid;
    uuid.CreateNewUuid();
    xiiVariant v(uuid);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Uuid);

    XII_TEST_BOOL(v.ConvertTo<xiiUuid>() == uuid);
    // XII_TEST_BOOL(v.ConvertTo<xiiString>() == "");

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Uuid).Get<xiiUuid>() == uuid);
    // XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiAngle)")
  {
    xiiAngle   t = xiiAngle::Degree(123.0);
    xiiVariant v(t);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Angle);

    XII_TEST_BOOL(v.ConvertTo<xiiAngle>() == t);
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "123.0°");
    // XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("123.0°")); // For some reason the compiler stumbles upon the degree sign, encoding weirdness most likely
    // XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("123.0°"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Angle).Get<xiiAngle>() == t);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "123.0°");
    // XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("123.0°"));
    // XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("123.0°"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiAngled)")
  {
    xiiAngled  t = xiiAngled::Degree(123.0);
    xiiVariant v(t);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::Angled);

    XII_TEST_BOOL(v.ConvertTo<xiiAngled>() == t);
    XII_TEST_BOOL(v.ConvertTo<xiiString>() == "123.0°");
    // XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("123.0°")); // For some reason the compiler stumbles upon the degree sign, encoding weirdness most likely
    // XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("123.0°"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::Angled).Get<xiiAngled>() == t);
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>() == "123.0°");
    // XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("123.0°"));
    // XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("123.0°"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (VariantArray)")
  {
    xiiVariantArray va;
    va.PushBack(2.5);
    va.PushBack("ABC");
    va.PushBack(xiiVariant());

    xiiVariant v(va);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::VariantArray);

    XII_TEST_BOOL(v.ConvertTo<xiiVariantArray>() == va);
    XII_TEST_STRING(v.ConvertTo<xiiString>(), "[2.5, ABC, <Invalid>]");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("[2.5, ABC, <Invalid>]"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("[2.5, ABC, <Invalid>]"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::VariantArray).Get<xiiVariantArray>() == va);
    XII_TEST_STRING(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>(), "[2.5, ABC, <Invalid>]");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("[2.5, ABC, <Invalid>]"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("[2.5, ABC, <Invalid>]"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "(Can)ConvertTo (xiiVariantDictionary)")
  {
    xiiVariantDictionary va;
    va.Insert("A", 2.5);
    va.Insert("B", "ABC");
    va.Insert("C", xiiVariant());

    xiiVariant v(va);

    TestCanOnlyConvertToStringAndID(v, xiiVariant::Type::VariantDictionary);

    XII_TEST_BOOL(v.ConvertTo<xiiVariantDictionary>() == va);
    XII_TEST_STRING(v.ConvertTo<xiiString>(), "{A=2.5, C=<Invalid>, B=ABC}");
    XII_TEST_BOOL(v.ConvertTo<xiiHashedString>() == xiiMakeHashedString("{A=2.5, C=<Invalid>, B=ABC}"));
    XII_TEST_BOOL(v.ConvertTo<xiiTempHashedString>() == xiiTempHashedString("{A=2.5, C=<Invalid>, B=ABC}"));

    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::VariantDictionary).Get<xiiVariantDictionary>() == va);
    XII_TEST_STRING(v.ConvertTo(xiiVariant::Type::String).Get<xiiString>(), "{A=2.5, C=<Invalid>, B=ABC}");
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::HashedString).Get<xiiHashedString>() == xiiMakeHashedString("{A=2.5, C=<Invalid>, B=ABC}"));
    XII_TEST_BOOL(v.ConvertTo(xiiVariant::Type::TempHashedString).Get<xiiTempHashedString>() == xiiTempHashedString("{A=2.5, C=<Invalid>, B=ABC}"));
  }
}

#pragma optimize("", on)
