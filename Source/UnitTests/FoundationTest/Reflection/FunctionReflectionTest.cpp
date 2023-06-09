#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

struct FunctionTest
{
  int StandardTypeFunction(int v, const xiiVec2 vCv, xiiVec3& ref_vRv, const xiiVec4& vCrv, xiiVec2U32* pPv, const xiiVec3U32* pCpv)
  {
    XII_TEST_BOOL(m_values[0] == v);
    XII_TEST_BOOL(m_values[1] == vCv);
    XII_TEST_BOOL(m_values[2] == ref_vRv);
    XII_TEST_BOOL(m_values[3] == vCrv);
    if (m_bPtrAreNull)
    {
      XII_TEST_BOOL(!pPv);
      XII_TEST_BOOL(!pCpv);
    }
    else
    {
      XII_TEST_BOOL(m_values[4] == *pPv);
      XII_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_vRv.Set(1, 2, 3);
    if (pPv)
    {
      pPv->Set(1, 2);
    }
    return 5;
  }

  xiiVarianceTypeAngle CustomTypeFunction(xiiVarianceTypeAngle v, const xiiVarianceTypeAngle cv, xiiVarianceTypeAngle& ref_rv, const xiiVarianceTypeAngle& crv, xiiVarianceTypeAngle* pPv, const xiiVarianceTypeAngle* pCpv)
  {
    XII_TEST_BOOL(m_values[0] == v);
    XII_TEST_BOOL(m_values[1] == cv);
    XII_TEST_BOOL(m_values[2] == ref_rv);
    XII_TEST_BOOL(m_values[3] == crv);
    if (m_bPtrAreNull)
    {
      XII_TEST_BOOL(!pPv);
      XII_TEST_BOOL(!pCpv);
    }
    else
    {
      XII_TEST_BOOL(m_values[4] == *pPv);
      XII_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_rv = {2.0f, xiiAngle::Degree(200.0f)};
    if (pPv)
    {
      *pPv = {4.0f, xiiAngle::Degree(400.0f)};
    }
    return {0.6f, xiiAngle::Degree(60.0f)};
  }

  xiiVarianceTypeAngle CustomTypeFunction2(xiiVarianceTypeAngle v, const xiiVarianceTypeAngle cv, xiiVarianceTypeAngle& ref_rv, const xiiVarianceTypeAngle& crv, xiiVarianceTypeAngle* pPv, const xiiVarianceTypeAngle* pCpv)
  {
    XII_TEST_BOOL(*m_values[0].Get<xiiVarianceTypeAngle*>() == v);
    XII_TEST_BOOL(*m_values[1].Get<xiiVarianceTypeAngle*>() == cv);
    XII_TEST_BOOL(*m_values[2].Get<xiiVarianceTypeAngle*>() == ref_rv);
    XII_TEST_BOOL(*m_values[3].Get<xiiVarianceTypeAngle*>() == crv);
    if (m_bPtrAreNull)
    {
      XII_TEST_BOOL(!pPv);
      XII_TEST_BOOL(!pCpv);
    }
    else
    {
      XII_TEST_BOOL(*m_values[4].Get<xiiVarianceTypeAngle*>() == *pPv);
      XII_TEST_BOOL(*m_values[5].Get<xiiVarianceTypeAngle*>() == *pCpv);
    }
    ref_rv = {2.0f, xiiAngle::Degree(200.0f)};
    if (pPv)
    {
      *pPv = {4.0f, xiiAngle::Degree(400.0f)};
    }
    return {0.6f, xiiAngle::Degree(60.0f)};
  }

  const char* StringTypeFunction(const char* szString, xiiString& ref_sString, xiiStringView sView)
  {
    if (m_bPtrAreNull)
    {
      XII_TEST_BOOL(!szString);
    }
    else
    {
      XII_TEST_BOOL(m_values[0] == szString);
    }
    XII_TEST_BOOL(m_values[1] == ref_sString);
    XII_TEST_BOOL(m_values[2] == sView);
    return "StringRet";
  }

  xiiEnum<xiiExampleEnum> EnumFunction(
    xiiEnum<xiiExampleEnum>        e,
    xiiEnum<xiiExampleEnum>&       ref_re,
    const xiiEnum<xiiExampleEnum>& cre,
    xiiEnum<xiiExampleEnum>*       pPe,
    const xiiEnum<xiiExampleEnum>* pCpe)
  {
    XII_TEST_BOOL(m_values[0].Get<xiiInt64>() == e.GetValue());
    XII_TEST_BOOL(m_values[1].Get<xiiInt64>() == ref_re.GetValue());
    XII_TEST_BOOL(m_values[2].Get<xiiInt64>() == cre.GetValue());
    if (m_bPtrAreNull)
    {
      XII_TEST_BOOL(!pPe);
      XII_TEST_BOOL(!pCpe);
    }
    else
    {
      XII_TEST_BOOL(m_values[3].Get<xiiInt64>() == pPe->GetValue());
      XII_TEST_BOOL(m_values[4].Get<xiiInt64>() == pCpe->GetValue());
    }
    return xiiExampleEnum::Value1;
  }

  xiiBitflags<xiiExampleBitflags> BitflagsFunction(xiiBitflags<xiiExampleBitflags> e, xiiBitflags<xiiExampleBitflags>& ref_re, const xiiBitflags<xiiExampleBitflags>& cre, xiiBitflags<xiiExampleBitflags>* pPe, const xiiBitflags<xiiExampleBitflags>* pCpe)
  {
    XII_TEST_BOOL(e == m_values[0].Get<xiiInt64>());
    XII_TEST_BOOL(ref_re == m_values[1].Get<xiiInt64>());
    XII_TEST_BOOL(cre == m_values[2].Get<xiiInt64>());
    if (m_bPtrAreNull)
    {
      XII_TEST_BOOL(!pPe);
      XII_TEST_BOOL(!pCpe);
    }
    else
    {
      XII_TEST_BOOL(*pPe == m_values[3].Get<xiiInt64>());
      XII_TEST_BOOL(*pCpe == m_values[4].Get<xiiInt64>());
    }
    return xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2;
  }

  xiiTestStruct3 StructFunction(
    xiiTestStruct3        s,
    const xiiTestStruct3  cs,
    xiiTestStruct3&       ref_rs,
    const xiiTestStruct3& crs,
    xiiTestStruct3*       pPs,
    const xiiTestStruct3* pCps)
  {
    XII_TEST_BOOL(*static_cast<xiiTestStruct3*>(m_values[0].Get<void*>()) == s);
    XII_TEST_BOOL(*static_cast<xiiTestStruct3*>(m_values[1].Get<void*>()) == cs);
    XII_TEST_BOOL(*static_cast<xiiTestStruct3*>(m_values[2].Get<void*>()) == ref_rs);
    XII_TEST_BOOL(*static_cast<xiiTestStruct3*>(m_values[3].Get<void*>()) == crs);
    if (m_bPtrAreNull)
    {
      XII_TEST_BOOL(!pPs);
      XII_TEST_BOOL(!pCps);
    }
    else
    {
      XII_TEST_BOOL(*static_cast<xiiTestStruct3*>(m_values[4].Get<void*>()) == *pPs);
      XII_TEST_BOOL(*static_cast<xiiTestStruct3*>(m_values[5].Get<void*>()) == *pCps);
    }
    ref_rs.m_fFloat1 = 999.0f;
    ref_rs.m_UInt8   = 666;
    if (pPs)
    {
      pPs->m_fFloat1 = 666.0f;
      pPs->m_UInt8   = 999;
    }
    xiiTestStruct3 retS;
    retS.m_fFloat1 = 42;
    retS.m_UInt8   = 42;
    return retS;
  }

  xiiTestClass1 ReflectedClassFunction(
    xiiTestClass1        s,
    const xiiTestClass1  cs,
    xiiTestClass1&       ref_rs,
    const xiiTestClass1& crs,
    xiiTestClass1*       pPs,
    const xiiTestClass1* pCps)
  {
    XII_TEST_BOOL(*static_cast<xiiTestClass1*>(m_values[0].ConvertTo<void*>()) == s);
    XII_TEST_BOOL(*static_cast<xiiTestClass1*>(m_values[1].ConvertTo<void*>()) == cs);
    XII_TEST_BOOL(*static_cast<xiiTestClass1*>(m_values[2].ConvertTo<void*>()) == ref_rs);
    XII_TEST_BOOL(*static_cast<xiiTestClass1*>(m_values[3].ConvertTo<void*>()) == crs);
    if (m_bPtrAreNull)
    {
      XII_TEST_BOOL(!pPs);
      XII_TEST_BOOL(!pCps);
    }
    else
    {
      XII_TEST_BOOL(*static_cast<xiiTestClass1*>(m_values[4].ConvertTo<void*>()) == *pPs);
      XII_TEST_BOOL(*static_cast<xiiTestClass1*>(m_values[5].ConvertTo<void*>()) == *pCps);
    }
    ref_rs.m_Color.SetRGB(1, 2, 3);
    ref_rs.m_MyVector.Set(1, 2, 3);
    if (pPs)
    {
      pPs->m_Color.SetRGB(1, 2, 3);
      pPs->m_MyVector.Set(1, 2, 3);
    }
    xiiTestClass1 retS;
    retS.m_Color.SetRGB(42, 42, 42);
    retS.m_MyVector.Set(42, 42, 42);
    return retS;
  }

  xiiVariant VariantFunction(xiiVariant v, const xiiVariant cv, xiiVariant& ref_rv, const xiiVariant& crv, xiiVariant* pPv, const xiiVariant* pCpv)
  {
    XII_TEST_BOOL(m_values[0] == v);
    XII_TEST_BOOL(m_values[1] == cv);
    XII_TEST_BOOL(m_values[2] == ref_rv);
    XII_TEST_BOOL(m_values[3] == crv);
    if (m_bPtrAreNull)
    {
      // Can't have variant as nullptr as it must exist in the array and there is no further
      // way of distinguishing a between a xiiVariant* and a xiiVariant that is invalid.
      XII_TEST_BOOL(!pPv->IsValid());
      XII_TEST_BOOL(!pCpv->IsValid());
    }
    else
    {
      XII_TEST_BOOL(m_values[4] == *pPv);
      XII_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_rv = xiiVec3(1, 2, 3);
    if (pPv)
    {
      *pPv = xiiVec2U32(1, 2);
    }
    return 5;
  }

  static void StaticFunction(bool b, xiiVariant v)
  {
    XII_TEST_BOOL(b == true);
    XII_TEST_BOOL(v == 4.0f);
  }

  static int StaticFunction2() { return 42; }

  bool                        m_bPtrAreNull = false;
  xiiDynamicArray<xiiVariant> m_values;
};

using ParamSig = std::tuple<const xiiRTTI*, xiiBitflags<xiiPropertyFlags>>;

void VerifyFunctionSignature(const xiiAbstractFunctionProperty* pFunc, xiiArrayPtr<ParamSig> params, ParamSig ret)
{
  XII_TEST_INT(params.GetCount(), pFunc->GetArgumentCount());
  for (xiiUInt32 i = 0; i < xiiMath::Min(params.GetCount(), pFunc->GetArgumentCount()); i++)
  {
    XII_TEST_BOOL(pFunc->GetArgumentType(i) == std::get<0>(params[i]));
    XII_TEST_BOOL(pFunc->GetArgumentFlags(i) == std::get<1>(params[i]));
  }
  XII_TEST_BOOL(pFunc->GetReturnType() == std::get<0>(ret));
  XII_TEST_BOOL(pFunc->GetReturnFlags() == std::get<1>(ret));
}

XII_CREATE_SIMPLE_TEST(Reflection, Functions)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Member Functions - StandardTypes")
  {
    xiiFunctionProperty<decltype(&FunctionTest::StandardTypeFunction)> funccall("", &FunctionTest::StandardTypeFunction);
    ParamSig                                                           testSet[] = {
      ParamSig(xiiGetStaticRTTI<int>(), xiiPropertyFlags::StandardType),
      ParamSig(xiiGetStaticRTTI<xiiVec2>(), xiiPropertyFlags::StandardType),
      ParamSig(xiiGetStaticRTTI<xiiVec3>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiVec4>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Const | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiVec2U32>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Pointer),
      ParamSig(xiiGetStaticRTTI<xiiVec3U32>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Const | xiiPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<int>(), xiiPropertyFlags::StandardType));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(1);
    test.m_values.PushBack(xiiVec2(2));
    test.m_values.PushBack(xiiVec3(3));
    test.m_values.PushBack(xiiVec4(4));
    test.m_values.PushBack(xiiVec2U32(5));
    test.m_values.PushBack(xiiVec3U32(6));

    xiiVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Int32);
    XII_TEST_BOOL(ret == 5);
    XII_TEST_BOOL(test.m_values[2] == xiiVec3(1, 2, 3));
    XII_TEST_BOOL(test.m_values[4] == xiiVec2U32(1, 2));

    test.m_bPtrAreNull = true;
    test.m_values[4]   = xiiVariant();
    test.m_values[5]   = xiiVariant();
    ret                = xiiVariant();
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Int32);
    XII_TEST_BOOL(ret == 5);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Member Functions - CustomType")
  {
    xiiFunctionProperty<decltype(&FunctionTest::CustomTypeFunction)> funccall("", &FunctionTest::CustomTypeFunction);
    ParamSig                                                         testSet[] = {
      ParamSig(xiiGetStaticRTTI<xiiVarianceTypeAngle>(), xiiPropertyFlags::Class),
      ParamSig(xiiGetStaticRTTI<xiiVarianceTypeAngle>(), xiiPropertyFlags::Class),
      ParamSig(xiiGetStaticRTTI<xiiVarianceTypeAngle>(), xiiPropertyFlags::Class | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiVarianceTypeAngle>(), xiiPropertyFlags::Class | xiiPropertyFlags::Const | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiVarianceTypeAngle>(), xiiPropertyFlags::Class | xiiPropertyFlags::Pointer),
      ParamSig(xiiGetStaticRTTI<xiiVarianceTypeAngle>(), xiiPropertyFlags::Class | xiiPropertyFlags::Const | xiiPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<xiiVarianceTypeAngle>(), xiiPropertyFlags::Class));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Member);

    {
      FunctionTest test;
      test.m_values.PushBack(xiiVarianceTypeAngle{0.0f, xiiAngle::Degree(0.0f)});
      test.m_values.PushBack(xiiVarianceTypeAngle{0.1f, xiiAngle::Degree(10.0f)});
      test.m_values.PushBack(xiiVarianceTypeAngle{0.2f, xiiAngle::Degree(20.0f)});
      test.m_values.PushBack(xiiVarianceTypeAngle{0.3f, xiiAngle::Degree(30.0f)});
      test.m_values.PushBack(xiiVarianceTypeAngle{0.4f, xiiAngle::Degree(40.0f)});
      test.m_values.PushBack(xiiVarianceTypeAngle{0.5f, xiiAngle::Degree(50.0f)});

      xiiVariant ret;
      funccall.Execute(&test, test.m_values, ret);
      XII_TEST_BOOL(ret.GetType() == xiiVariantType::TypedObject);
      XII_TEST_BOOL(ret == xiiVariant(xiiVarianceTypeAngle{0.6f, xiiAngle::Degree(60.0f)}));
      XII_TEST_BOOL(test.m_values[2] == xiiVariant(xiiVarianceTypeAngle{2.0f, xiiAngle::Degree(200.0f)}));
      XII_TEST_BOOL(test.m_values[4] == xiiVariant(xiiVarianceTypeAngle{4.0f, xiiAngle::Degree(400.0f)}));

      test.m_bPtrAreNull = true;
      test.m_values[4]   = xiiVariant();
      test.m_values[5]   = xiiVariant();
      ret                = xiiVariant();
      funccall.Execute(&test, test.m_values, ret);
      XII_TEST_BOOL(ret.GetType() == xiiVariantType::TypedObject);
      XII_TEST_BOOL(ret == xiiVariant(xiiVarianceTypeAngle{0.6f, xiiAngle::Degree(60.0f)}));
    }

    {
      xiiFunctionProperty<decltype(&FunctionTest::CustomTypeFunction2)> funccall2("", &FunctionTest::CustomTypeFunction2);

      FunctionTest         test;
      xiiVarianceTypeAngle v0{0.0f, xiiAngle::Degree(0.0f)};
      xiiVarianceTypeAngle v1{0.1f, xiiAngle::Degree(10.0f)};
      xiiVarianceTypeAngle v2{0.2f, xiiAngle::Degree(20.0f)};
      xiiVarianceTypeAngle v3{0.3f, xiiAngle::Degree(30.0f)};
      xiiVarianceTypeAngle v4{0.4f, xiiAngle::Degree(40.0f)};
      xiiVarianceTypeAngle v5{0.5f, xiiAngle::Degree(50.0f)};
      test.m_values.PushBack(&v0);
      test.m_values.PushBack(&v1);
      test.m_values.PushBack(&v2);
      test.m_values.PushBack(&v3);
      test.m_values.PushBack(&v4);
      test.m_values.PushBack(&v5);

      xiiVariant ret;
      funccall2.Execute(&test, test.m_values, ret);
      XII_TEST_BOOL(ret.GetType() == xiiVariantType::TypedObject);
      XII_TEST_BOOL(ret == xiiVariant(xiiVarianceTypeAngle{0.6f, xiiAngle::Degree(60.0f)}));
      XII_TEST_BOOL((*test.m_values[2].Get<xiiVarianceTypeAngle*>() == xiiVarianceTypeAngle{2.0f, xiiAngle::Degree(200.0f)}));
      XII_TEST_BOOL((*test.m_values[4].Get<xiiVarianceTypeAngle*>() == xiiVarianceTypeAngle{4.0f, xiiAngle::Degree(400.0f)}));

      test.m_bPtrAreNull = true;
      test.m_values[4]   = xiiVariant();
      test.m_values[5]   = xiiVariant();
      ret                = xiiVariant();
      funccall2.Execute(&test, test.m_values, ret);
      XII_TEST_BOOL(ret.GetType() == xiiVariantType::TypedObject);
      XII_TEST_BOOL(ret == xiiVariant(xiiVarianceTypeAngle{0.6f, xiiAngle::Degree(60.0f)}));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Member Functions - Strings")
  {
    xiiFunctionProperty<decltype(&FunctionTest::StringTypeFunction)> funccall("", &FunctionTest::StringTypeFunction);
    ParamSig                                                         testSet[] = {
      ParamSig(xiiGetStaticRTTI<const char*>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Const),
      ParamSig(xiiGetStaticRTTI<xiiString>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiStringView>(), xiiPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(
      &funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<const char*>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Const));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(xiiVariant(xiiString("String0")));
    test.m_values.PushBack(xiiVariant(xiiString("String1")));
    test.m_values.PushBack(xiiStringView("String2"));

    xiiVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::String);
    XII_TEST_BOOL(ret == xiiString("StringRet"));

    test.m_bPtrAreNull = true;
    test.m_values[0]   = xiiVariant();
    ret                = xiiVariant();
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::String);
    XII_TEST_BOOL(ret == xiiString("StringRet"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Member Functions - Enum")
  {
    xiiFunctionProperty<decltype(&FunctionTest::EnumFunction)> funccall("", &FunctionTest::EnumFunction);
    ParamSig                                                   testSet[] = {
      ParamSig(xiiGetStaticRTTI<xiiExampleEnum>(), xiiPropertyFlags::IsEnum),
      ParamSig(xiiGetStaticRTTI<xiiExampleEnum>(), xiiPropertyFlags::IsEnum | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiExampleEnum>(), xiiPropertyFlags::IsEnum | xiiPropertyFlags::Const | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiExampleEnum>(), xiiPropertyFlags::IsEnum | xiiPropertyFlags::Pointer),
      ParamSig(xiiGetStaticRTTI<xiiExampleEnum>(), xiiPropertyFlags::IsEnum | xiiPropertyFlags::Const | xiiPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<xiiExampleEnum>(), xiiPropertyFlags::IsEnum));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack((xiiInt64)xiiExampleEnum::Value1);
    test.m_values.PushBack((xiiInt64)xiiExampleEnum::Value2);
    test.m_values.PushBack((xiiInt64)xiiExampleEnum::Value3);
    test.m_values.PushBack((xiiInt64)xiiExampleEnum::Default);
    test.m_values.PushBack((xiiInt64)xiiExampleEnum::Value3);

    xiiVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Int64);
    XII_TEST_BOOL(ret == (xiiInt64)xiiExampleEnum::Value1);

    test.m_bPtrAreNull = true;
    test.m_values[3]   = xiiVariant();
    test.m_values[4]   = xiiVariant();
    ret                = xiiVariant();
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Int64);
    XII_TEST_BOOL(ret == (xiiInt64)xiiExampleEnum::Value1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Member Functions - Bitflags")
  {
    xiiFunctionProperty<decltype(&FunctionTest::BitflagsFunction)> funccall("", &FunctionTest::BitflagsFunction);
    ParamSig                                                       testSet[] = {
      ParamSig(xiiGetStaticRTTI<xiiExampleBitflags>(), xiiPropertyFlags::Bitflags),
      ParamSig(xiiGetStaticRTTI<xiiExampleBitflags>(), xiiPropertyFlags::Bitflags | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiExampleBitflags>(), xiiPropertyFlags::Bitflags | xiiPropertyFlags::Const | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiExampleBitflags>(), xiiPropertyFlags::Bitflags | xiiPropertyFlags::Pointer),
      ParamSig(xiiGetStaticRTTI<xiiExampleBitflags>(), xiiPropertyFlags::Bitflags | xiiPropertyFlags::Const | xiiPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<xiiExampleBitflags>(), xiiPropertyFlags::Bitflags));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack((xiiInt64)(0));
    test.m_values.PushBack((xiiInt64)(xiiExampleBitflags::Value2));
    test.m_values.PushBack((xiiInt64)(xiiExampleBitflags::Value3 | xiiExampleBitflags::Value2).GetValue());
    test.m_values.PushBack((xiiInt64)(xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2 | xiiExampleBitflags::Value3).GetValue());
    test.m_values.PushBack((xiiInt64)(xiiExampleBitflags::Value3));

    xiiVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Int64);
    XII_TEST_BOOL(ret == (xiiInt64)(xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2).GetValue());

    test.m_bPtrAreNull = true;
    test.m_values[3]   = xiiVariant();
    test.m_values[4]   = xiiVariant();
    ret                = xiiVariant();
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Int64);
    XII_TEST_BOOL(ret == (xiiInt64)(xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2).GetValue());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Member Functions - Structs")
  {
    xiiFunctionProperty<decltype(&FunctionTest::StructFunction)> funccall("", &FunctionTest::StructFunction);
    ParamSig                                                     testSet[] = {
      ParamSig(xiiGetStaticRTTI<xiiTestStruct3>(), xiiPropertyFlags::Class),
      ParamSig(xiiGetStaticRTTI<xiiTestStruct3>(), xiiPropertyFlags::Class),
      ParamSig(xiiGetStaticRTTI<xiiTestStruct3>(), xiiPropertyFlags::Class | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiTestStruct3>(), xiiPropertyFlags::Class | xiiPropertyFlags::Const | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiTestStruct3>(), xiiPropertyFlags::Class | xiiPropertyFlags::Pointer),
      ParamSig(xiiGetStaticRTTI<xiiTestStruct3>(), xiiPropertyFlags::Class | xiiPropertyFlags::Const | xiiPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<xiiTestStruct3>(), xiiPropertyFlags::Class));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Member);

    FunctionTest   test;
    xiiTestStruct3 retS;
    retS.m_fFloat1 = 0;
    retS.m_UInt8   = 0;
    xiiTestStruct3 value;
    value.m_fFloat1 = 0;
    value.m_UInt8   = 0;
    xiiTestStruct3 rs;
    rs.m_fFloat1 = 42;
    xiiTestStruct3 ps;
    ps.m_fFloat1 = 18;

    test.m_values.PushBack(xiiVariant(&value));
    test.m_values.PushBack(xiiVariant(&value));
    test.m_values.PushBack(xiiVariant(&rs));
    test.m_values.PushBack(xiiVariant(&value));
    test.m_values.PushBack(xiiVariant(&ps));
    test.m_values.PushBack(xiiVariant(&value));

    // xiiVariantAdapter<xiiTestStruct3 const*> aa(xiiVariant(&value));
    //auto bla = xiiIsStandardType<xiiTestStruct3 const*>::value;

    xiiVariant ret(&retS);
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_FLOAT(retS.m_fFloat1, 42, 0);
    XII_TEST_INT(retS.m_UInt8, 42);

    XII_TEST_FLOAT(rs.m_fFloat1, 999, 0);
    XII_TEST_INT(rs.m_UInt8, 666);

    XII_TEST_DOUBLE(ps.m_fFloat1, 666, 0);
    XII_TEST_INT(ps.m_UInt8, 999);

    test.m_bPtrAreNull = true;
    test.m_values[4]   = xiiVariant();
    test.m_values[5]   = xiiVariant();
    funccall.Execute(&test, test.m_values, ret);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Member Functions - Reflected Classes")
  {
    xiiFunctionProperty<decltype(&FunctionTest::ReflectedClassFunction)> funccall("", &FunctionTest::ReflectedClassFunction);
    ParamSig                                                             testSet[] = {
      ParamSig(xiiGetStaticRTTI<xiiTestClass1>(), xiiPropertyFlags::Class),
      ParamSig(xiiGetStaticRTTI<xiiTestClass1>(), xiiPropertyFlags::Class),
      ParamSig(xiiGetStaticRTTI<xiiTestClass1>(), xiiPropertyFlags::Class | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiTestClass1>(), xiiPropertyFlags::Class | xiiPropertyFlags::Const | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiTestClass1>(), xiiPropertyFlags::Class | xiiPropertyFlags::Pointer),
      ParamSig(xiiGetStaticRTTI<xiiTestClass1>(), xiiPropertyFlags::Class | xiiPropertyFlags::Const | xiiPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<xiiTestClass1>(), xiiPropertyFlags::Class));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Member);

    FunctionTest  test;
    xiiTestClass1 retS;
    retS.m_Color = xiiColor::Chocolate;
    xiiTestClass1 value;
    value.m_Color = xiiColor::AliceBlue;
    xiiTestClass1 rs;
    rs.m_Color = xiiColor::Beige;
    xiiTestClass1 ps;
    ps.m_Color = xiiColor::DarkBlue;

    test.m_values.PushBack(xiiVariant(&value));
    test.m_values.PushBack(xiiVariant(&value));
    test.m_values.PushBack(xiiVariant(&rs));
    test.m_values.PushBack(xiiVariant(&value));
    test.m_values.PushBack(xiiVariant(&ps));
    test.m_values.PushBack(xiiVariant(&value));

    rs.m_Color.SetRGB(1, 2, 3);
    rs.m_MyVector.Set(1, 2, 3);


    xiiVariant ret(&retS);
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(retS.m_Color == xiiColor(42, 42, 42));
    XII_TEST_BOOL(retS.m_MyVector == xiiVec3(42, 42, 42));

    XII_TEST_BOOL(rs.m_Color == xiiColor(1, 2, 3));
    XII_TEST_BOOL(rs.m_MyVector == xiiVec3(1, 2, 3));

    XII_TEST_BOOL(ps.m_Color == xiiColor(1, 2, 3));
    XII_TEST_BOOL(ps.m_MyVector == xiiVec3(1, 2, 3));

    test.m_bPtrAreNull = true;
    test.m_values[4]   = xiiVariant();
    test.m_values[5]   = xiiVariant();
    funccall.Execute(&test, test.m_values, ret);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Member Functions - Variant")
  {
    xiiFunctionProperty<decltype(&FunctionTest::VariantFunction)> funccall("", &FunctionTest::VariantFunction);
    ParamSig                                                      testSet[] = {
      ParamSig(xiiGetStaticRTTI<xiiVariant>(), xiiPropertyFlags::StandardType),
      ParamSig(xiiGetStaticRTTI<xiiVariant>(), xiiPropertyFlags::StandardType),
      ParamSig(xiiGetStaticRTTI<xiiVariant>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiVariant>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Const | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiVariant>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Pointer),
      ParamSig(xiiGetStaticRTTI<xiiVariant>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Const | xiiPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<xiiVariant>(), xiiPropertyFlags::StandardType));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(1);
    test.m_values.PushBack(xiiVec2(2));
    test.m_values.PushBack(xiiVec3(3));
    test.m_values.PushBack(xiiVec4(4));
    test.m_values.PushBack(xiiVec2U32(5));
    test.m_values.PushBack(xiiVec3U32(6));

    xiiVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Int32);
    XII_TEST_BOOL(ret == 5);
    XII_TEST_BOOL(test.m_values[2] == xiiVec3(1, 2, 3));
    XII_TEST_BOOL(test.m_values[4] == xiiVec2U32(1, 2));

    test.m_bPtrAreNull = true;
    test.m_values[4]   = xiiVariant();
    test.m_values[5]   = xiiVariant();
    ret                = xiiVariant();
    funccall.Execute(&test, test.m_values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Int32);
    XII_TEST_BOOL(ret == 5);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Static Functions")
  {
    // Void return
    xiiFunctionProperty<decltype(&FunctionTest::StaticFunction)> funccall("", &FunctionTest::StaticFunction);
    ParamSig                                                     testSet[] = {
      ParamSig(xiiGetStaticRTTI<bool>(), xiiPropertyFlags::StandardType),
      ParamSig(xiiGetStaticRTTI<xiiVariant>(), xiiPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(&funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<void>(), xiiPropertyFlags::Void));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::StaticMember);

    xiiDynamicArray<xiiVariant> values;
    values.PushBack(true);
    values.PushBack(4.0f);
    xiiVariant ret;
    funccall.Execute(nullptr, values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Invalid);

    // Zero parameter
    xiiFunctionProperty<decltype(&FunctionTest::StaticFunction2)> funccall2("", &FunctionTest::StaticFunction2);
    VerifyFunctionSignature(&funccall2, xiiArrayPtr<ParamSig>(), ParamSig(xiiGetStaticRTTI<int>(), xiiPropertyFlags::StandardType));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::StaticMember);
    values.Clear();
    funccall2.Execute(nullptr, values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Int32);
    XII_TEST_BOOL(ret == 42);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor Functions - StandardTypes")
  {
    xiiConstructorFunctionProperty<xiiVec4, float, float, float, float> funccall;
    ParamSig                                                            testSet[] = {
      ParamSig(xiiGetStaticRTTI<float>(), xiiPropertyFlags::StandardType),
      ParamSig(xiiGetStaticRTTI<float>(), xiiPropertyFlags::StandardType),
      ParamSig(xiiGetStaticRTTI<float>(), xiiPropertyFlags::StandardType),
      ParamSig(xiiGetStaticRTTI<float>(), xiiPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(
      &funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<xiiVec4>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Pointer));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Constructor);

    xiiDynamicArray<xiiVariant> values;
    values.PushBack(1.0f);
    values.PushBack(2.0f);
    values.PushBack(3.0f);
    values.PushBack(4.0f);
    xiiVariant ret;
    funccall.Execute(nullptr, values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::Vector4);
    XII_TEST_BOOL(ret == xiiVec4(1.0f, 2.0f, 3.0f, 4.0f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor Functions - Struct")
  {
    xiiConstructorFunctionProperty<xiiTestStruct3, double, xiiInt16> funccall;
    ParamSig                                                         testSet[] = {
      ParamSig(xiiGetStaticRTTI<double>(), xiiPropertyFlags::StandardType),
      ParamSig(xiiGetStaticRTTI<xiiInt16>(), xiiPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(
      &funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<xiiTestStruct3>(), xiiPropertyFlags::Class | xiiPropertyFlags::Pointer));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Constructor);

    xiiDynamicArray<xiiVariant> values;
    values.PushBack(59.0);
    values.PushBack((xiiInt16)666);
    xiiVariant ret;
    funccall.Execute(nullptr, values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::TypedPointer);
    xiiTestStruct3* pRet = static_cast<xiiTestStruct3*>(ret.ConvertTo<void*>());
    XII_TEST_BOOL(pRet != nullptr);

    XII_TEST_FLOAT(pRet->m_fFloat1, 59.0, 0);
    XII_TEST_INT(pRet->m_UInt8, 666);
    XII_TEST_INT(pRet->GetIntPublic(), 32);

    XII_DEFAULT_DELETE(pRet);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor Functions - Reflected Classes")
  {
    // The function signature does not actually need to match the ctor 100% as long as implicit conversion is possible.
    xiiConstructorFunctionProperty<xiiTestClass1, const xiiColor&, const xiiTestStruct&> funccall;
    ParamSig                                                                             testSet[] = {
      ParamSig(xiiGetStaticRTTI<xiiColor>(), xiiPropertyFlags::StandardType | xiiPropertyFlags::Const | xiiPropertyFlags::Reference),
      ParamSig(xiiGetStaticRTTI<xiiTestStruct>(), xiiPropertyFlags::Class | xiiPropertyFlags::Const | xiiPropertyFlags::Reference),
    };
    VerifyFunctionSignature(
      &funccall, xiiArrayPtr<ParamSig>(testSet), ParamSig(xiiGetStaticRTTI<xiiTestClass1>(), xiiPropertyFlags::Class | xiiPropertyFlags::Pointer));
    XII_TEST_BOOL(funccall.GetFunctionType() == xiiFunctionType::Constructor);

    xiiDynamicArray<xiiVariant> values;
    xiiTestStruct               s;
    s.m_fFloat1 = 1.0f;
    s.m_UInt8   = 255;
    values.PushBack(xiiColor::CornflowerBlue);
    values.PushBack(xiiVariant(&s));
    xiiVariant ret;
    funccall.Execute(nullptr, values, ret);
    XII_TEST_BOOL(ret.GetType() == xiiVariantType::TypedPointer);
    xiiTestClass1* pRet = static_cast<xiiTestClass1*>(ret.ConvertTo<void*>());
    XII_TEST_BOOL(pRet != nullptr);

    XII_TEST_BOOL(pRet->m_Color == xiiColor::CornflowerBlue);
    XII_TEST_BOOL(pRet->m_Struct == s);
    XII_TEST_BOOL(pRet->m_MyVector == xiiVec3(1, 2, 3));

    XII_DEFAULT_DELETE(pRet);
  }
}
