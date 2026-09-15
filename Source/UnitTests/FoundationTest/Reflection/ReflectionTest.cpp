/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

template <typename T>
void TestSerialization(const T& source)
{
  xiiDefaultMemoryStreamStorage StreamStorage;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "WriteObjectToDDL")
  {
    xiiMemoryStreamWriter FileOut(&StreamStorage);

    xiiReflectionSerializer::WriteObjectToDDL(FileOut, xiiGetStaticRTTI<T>(), &source, false, xiiOpenDdlWriter::TypeStringMode::Compliant);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadObjectPropertiesFromDDL")
  {
    xiiMemoryStreamReader FileIn(&StreamStorage);
    T                     data;
    xiiReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *xiiGetStaticRTTI<T>(), &data);

    XII_TEST_BOOL(data == source);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadObjectFromDDL")
  {
    xiiMemoryStreamReader FileIn(&StreamStorage);

    const xiiRTTI* pRtti;
    void*          pObject = xiiReflectionSerializer::ReadObjectFromDDL(FileIn, pRtti);

    T& c2 = *((T*)pObject);

    XII_TEST_BOOL(c2 == source);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  xiiDefaultMemoryStreamStorage StreamStorageBinary;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "WriteObjectToBinary")
  {
    xiiMemoryStreamWriter FileOut(&StreamStorageBinary);

    xiiReflectionSerializer::WriteObjectToBinary(FileOut, xiiGetStaticRTTI<T>(), &source);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadObjectPropertiesFromBinary")
  {
    xiiMemoryStreamReader FileIn(&StreamStorageBinary);
    T                     data;
    xiiReflectionSerializer::ReadObjectPropertiesFromBinary(FileIn, *xiiGetStaticRTTI<T>(), &data);

    XII_TEST_BOOL(data == source);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadObjectFromBinary")
  {
    xiiMemoryStreamReader FileIn(&StreamStorageBinary);

    const xiiRTTI* pRtti;
    void*          pObject = xiiReflectionSerializer::ReadObjectFromBinary(FileIn, pRtti);

    T& c2 = *((T*)pObject);

    XII_TEST_BOOL(c2 == source);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clone")
  {
    {
      T clone;
      xiiReflectionSerializer::Clone(&source, &clone, xiiGetStaticRTTI<T>());
      XII_TEST_BOOL(clone == source);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(&clone, &source, xiiGetStaticRTTI<T>()));
    }

    {
      T* pClone = xiiReflectionSerializer::Clone(&source);
      XII_TEST_BOOL(*pClone == source);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(pClone, &source));
      xiiGetStaticRTTI<T>()->GetAllocator()->Deallocate(pClone);
    }
  }
}


XII_CREATE_SIMPLE_TEST_GROUP(Reflection);

XII_CREATE_SIMPLE_TEST(Reflection, Types)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Iterate All")
  {
    bool bFoundStruct = false;
    bool bFoundClass1 = false;
    bool bFoundClass2 = false;

    xiiRTTI::ForEachType([&](const xiiRTTI* pRtti) {
      if (pRtti->GetTypeName() == "xiiTestStruct")
        bFoundStruct = true;
      if (pRtti->GetTypeName() == "xiiTestClass1")
        bFoundClass1 = true;
      if (pRtti->GetTypeName() == "xiiTestClass2")
        bFoundClass2 = true;

      XII_TEST_STRING(pRtti->GetPluginName(), "Static");
    });

    XII_TEST_BOOL(bFoundStruct);
    XII_TEST_BOOL(bFoundClass1);
    XII_TEST_BOOL(bFoundClass2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsDerivedFrom")
  {
    xiiDynamicArray<const xiiRTTI*> allTypes;
    xiiRTTI::ForEachType([&](const xiiRTTI* pRtti) { allTypes.PushBack(pRtti); });

    // ground truth - traversing up the parent list
    auto ManualIsDerivedFrom = [](const xiiRTTI* t, const xiiRTTI* pBaseType) -> bool {
      while (t != nullptr)
      {
        if (t == pBaseType)
          return true;

        t = t->GetParentType();
      }

      return false;
    };

    // test each type against every other:
    for (const xiiRTTI* typeA : allTypes)
    {
      for (const xiiRTTI* typeB : allTypes)
      {
        bool derived     = typeA->IsDerivedFrom(typeB);
        bool manualCheck = ManualIsDerivedFrom(typeA, typeB);
        XII_TEST_BOOL(derived == manualCheck);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PropertyFlags")
  {
    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<void>() == (xiiPropertyFlags::Void));
    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<const char*>() == (xiiPropertyFlags::StandardType | xiiPropertyFlags::Const));
    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<int>() == xiiPropertyFlags::StandardType);
    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<int&>() == (xiiPropertyFlags::StandardType | xiiPropertyFlags::Reference));
    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<int*>() == (xiiPropertyFlags::StandardType | xiiPropertyFlags::Pointer));

    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<const int>() == (xiiPropertyFlags::StandardType | xiiPropertyFlags::Const));
    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<const int&>() == (xiiPropertyFlags::StandardType | xiiPropertyFlags::Reference | xiiPropertyFlags::Const));
    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<const int*>() == (xiiPropertyFlags::StandardType | xiiPropertyFlags::Pointer | xiiPropertyFlags::Const));

    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<xiiVariant>() == (xiiPropertyFlags::StandardType));

    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<xiiExampleEnum::Enum>() == xiiPropertyFlags::IsEnum);
    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<xiiEnum<xiiExampleEnum>>() == xiiPropertyFlags::IsEnum);
    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<xiiBitflags<xiiExampleBitflags>>() == xiiPropertyFlags::Bitflags);

    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<xiiTestStruct3>() == xiiPropertyFlags::Class);
    XII_TEST_BOOL(xiiPropertyFlags::GetParameterFlags<xiiTestClass2>() == xiiPropertyFlags::Class);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TypeFlags")
  {
    XII_TEST_INT(xiiGetStaticRTTI<bool>()->GetTypeFlags().GetValue(), xiiTypeFlags::StandardType);
    XII_TEST_INT(xiiGetStaticRTTI<xiiUuid>()->GetTypeFlags().GetValue(), xiiTypeFlags::StandardType);
    XII_TEST_INT(xiiGetStaticRTTI<const char*>()->GetTypeFlags().GetValue(), xiiTypeFlags::StandardType);
    XII_TEST_INT(xiiGetStaticRTTI<xiiString>()->GetTypeFlags().GetValue(), xiiTypeFlags::StandardType);
    XII_TEST_INT(xiiGetStaticRTTI<xiiMat4>()->GetTypeFlags().GetValue(), xiiTypeFlags::StandardType);
    XII_TEST_INT(xiiGetStaticRTTI<xiiVariant>()->GetTypeFlags().GetValue(), xiiTypeFlags::StandardType);

    XII_TEST_INT(xiiGetStaticRTTI<xiiAbstractTestClass>()->GetTypeFlags().GetValue(), (xiiTypeFlags::Class | xiiTypeFlags::Abstract).GetValue());
    XII_TEST_INT(xiiGetStaticRTTI<xiiAbstractTestStruct>()->GetTypeFlags().GetValue(), (xiiTypeFlags::Class | xiiTypeFlags::Abstract).GetValue());

    XII_TEST_INT(xiiGetStaticRTTI<xiiTestStruct3>()->GetTypeFlags().GetValue(), xiiTypeFlags::Class);
    XII_TEST_INT(xiiGetStaticRTTI<xiiTestClass2>()->GetTypeFlags().GetValue(), xiiTypeFlags::Class);

    XII_TEST_INT(xiiGetStaticRTTI<xiiExampleEnum>()->GetTypeFlags().GetValue(), xiiTypeFlags::IsEnum);
    XII_TEST_INT(xiiGetStaticRTTI<xiiExampleBitflags>()->GetTypeFlags().GetValue(), xiiTypeFlags::Bitflags);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindTypeByName")
  {
    const xiiRTTI* pFloat = xiiRTTI::FindTypeByName("float");
    XII_TEST_BOOL(pFloat != nullptr);
    XII_TEST_STRING(pFloat->GetTypeName(), "float");

    const xiiRTTI* pStruct = xiiRTTI::FindTypeByName("xiiTestStruct");
    XII_TEST_BOOL(pStruct != nullptr);
    XII_TEST_STRING(pStruct->GetTypeName(), "xiiTestStruct");

    const xiiRTTI* pClass2 = xiiRTTI::FindTypeByName("xiiTestClass2");
    XII_TEST_BOOL(pClass2 != nullptr);
    XII_TEST_STRING(pClass2->GetTypeName(), "xiiTestClass2");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindTypeByNameHash")
  {
    const xiiRTTI* pFloat  = xiiRTTI::FindTypeByName("float");
    const xiiRTTI* pFloat2 = xiiRTTI::FindTypeByNameHash(pFloat->GetTypeNameHash());
    XII_TEST_BOOL(pFloat == pFloat2);

    const xiiRTTI* pStruct  = xiiRTTI::FindTypeByName("xiiTestStruct");
    const xiiRTTI* pStruct2 = xiiRTTI::FindTypeByNameHash(pStruct->GetTypeNameHash());
    XII_TEST_BOOL(pStruct == pStruct2);

    const xiiRTTI* pClass  = xiiRTTI::FindTypeByName("xiiTestClass2");
    const xiiRTTI* pClass2 = xiiRTTI::FindTypeByNameHash(pClass->GetTypeNameHash());
    XII_TEST_BOOL(pClass == pClass2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetProperties")
  {
    {
      const xiiRTTI* pType = xiiRTTI::FindTypeByName("xiiTestStruct");

      auto Props = pType->GetProperties();
      XII_TEST_INT(Props.GetCount(), 11);
      XII_TEST_STRING(Props[0]->GetPropertyName(), "Float");
      XII_TEST_STRING(Props[1]->GetPropertyName(), "Vector");
      XII_TEST_STRING(Props[2]->GetPropertyName(), "Int");
      XII_TEST_STRING(Props[3]->GetPropertyName(), "UInt8");
      XII_TEST_STRING(Props[4]->GetPropertyName(), "Variant");
      XII_TEST_STRING(Props[5]->GetPropertyName(), "Angle");
      XII_TEST_STRING(Props[6]->GetPropertyName(), "Angled");
      XII_TEST_STRING(Props[7]->GetPropertyName(), "DataBuffer");
      XII_TEST_STRING(Props[8]->GetPropertyName(), "vVec3I");
      XII_TEST_STRING(Props[9]->GetPropertyName(), "VarianceAngle");
      XII_TEST_STRING(Props[10]->GetPropertyName(), "VarianceAngled");
    }

    {
      const xiiRTTI* pType = xiiRTTI::FindTypeByName("xiiTestClass2");

      auto Props = pType->GetProperties();
      XII_TEST_INT(Props.GetCount(), 8);
      XII_TEST_STRING(Props[0]->GetPropertyName(), "CharPtr");
      XII_TEST_STRING(Props[1]->GetPropertyName(), "String");
      XII_TEST_STRING(Props[2]->GetPropertyName(), "StringView");
      XII_TEST_STRING(Props[3]->GetPropertyName(), "Time");
      XII_TEST_STRING(Props[4]->GetPropertyName(), "Enum");
      XII_TEST_STRING(Props[5]->GetPropertyName(), "Bitflags");
      XII_TEST_STRING(Props[6]->GetPropertyName(), "Array");
      XII_TEST_STRING(Props[7]->GetPropertyName(), "Variant");

      xiiHybridArray<const xiiAbstractProperty*, 32> AllProps;
      pType->GetAllProperties(AllProps);

      XII_TEST_INT(AllProps.GetCount(), 11);
      XII_TEST_STRING(AllProps[0]->GetPropertyName(), "SubStruct");
      XII_TEST_STRING(AllProps[1]->GetPropertyName(), "Color");
      XII_TEST_STRING(AllProps[2]->GetPropertyName(), "SubVector");
      XII_TEST_STRING(AllProps[3]->GetPropertyName(), "CharPtr");
      XII_TEST_STRING(AllProps[4]->GetPropertyName(), "String");
      XII_TEST_STRING(AllProps[5]->GetPropertyName(), "StringView");
      XII_TEST_STRING(AllProps[6]->GetPropertyName(), "Time");
      XII_TEST_STRING(AllProps[7]->GetPropertyName(), "Enum");
      XII_TEST_STRING(AllProps[8]->GetPropertyName(), "Bitflags");
      XII_TEST_STRING(AllProps[9]->GetPropertyName(), "Array");
      XII_TEST_STRING(AllProps[10]->GetPropertyName(), "Variant");
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Casts")
  {
    xiiTestClass2        test;
    xiiTestClass1*       pTestClass1      = &test;
    const xiiTestClass1* pConstTestClass1 = &test;

    xiiTestClass2*       pTestClass2      = xiiStaticCast<xiiTestClass2*>(pTestClass1);
    const xiiTestClass2* pConstTestClass2 = xiiStaticCast<const xiiTestClass2*>(pConstTestClass1);

    pTestClass2      = xiiDynamicCast<xiiTestClass2*>(pTestClass1);
    pConstTestClass2 = xiiDynamicCast<const xiiTestClass2*>(pConstTestClass1);
    XII_TEST_BOOL(pTestClass2 != nullptr);
    XII_TEST_BOOL(pConstTestClass2 != nullptr);

    xiiTestClass1 otherTest;
    pTestClass1      = &otherTest;
    pConstTestClass1 = &otherTest;

    pTestClass2      = xiiDynamicCast<xiiTestClass2*>(pTestClass1);
    pConstTestClass2 = xiiDynamicCast<const xiiTestClass2*>(pConstTestClass1);
    XII_TEST_BOOL(pTestClass2 == nullptr);
    XII_TEST_BOOL(pConstTestClass2 == nullptr);
  }

#if XII_ENABLED(XII_SUPPORTS_DYNAMIC_PLUGINS) && XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Types From Plugin")
  {
    xiiResult loadPlugin = xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin1);
    XII_TEST_BOOL(loadPlugin == XII_SUCCESS);

    if (loadPlugin.Failed())
      return;

    const xiiRTTI* pStruct2 = xiiRTTI::FindTypeByName("xiiTestStruct2");
    XII_TEST_BOOL(pStruct2 != nullptr);

    if (pStruct2)
    {
      XII_TEST_STRING(pStruct2->GetTypeName(), "xiiTestStruct2");
    }

    bool bFoundStruct2 = false;

    xiiRTTI::ForEachType([&](const xiiRTTI* pRtti) {
      if (pRtti->GetTypeName() == "xiiTestStruct2")
      {
        bFoundStruct2 = true;

        XII_TEST_STRING(pRtti->GetPluginName(), xiiFoundationTest_Plugin1);

        void* pInstance = pRtti->GetAllocator()->Allocate<void>();
        XII_TEST_BOOL(pInstance != nullptr);

        const xiiAbstractProperty* pProp = pRtti->FindPropertyByName("Float2");

        XII_TEST_BOOL(pProp != nullptr);

        XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Member);
        auto pAbsMember = static_cast<const xiiAbstractMemberProperty*>(pProp);

        XII_TEST_BOOL(pAbsMember->GetSpecificType() == xiiGetStaticRTTI<float>());

        auto pMember = static_cast<const xiiTypedMemberProperty<float>*>(pAbsMember);

        XII_TEST_FLOAT(pMember->GetValue(pInstance), 42.0f, 0);
        pMember->SetValue(pInstance, 43.0f);
        XII_TEST_FLOAT(pMember->GetValue(pInstance), 43.0f, 0);

        pRtti->GetAllocator()->Deallocate(pInstance);
      }
      else
      {
        XII_TEST_STRING(pRtti->GetPluginName(), "Static");
      }
    });

    XII_TEST_BOOL(bFoundStruct2);

    xiiPlugin::UnloadAllPlugins();
  }
#endif
}


XII_CREATE_SIMPLE_TEST(Reflection, Hierarchies)
{
  xiiTestClass2Allocator::m_iAllocs   = 0;
  xiiTestClass2Allocator::m_iDeallocs = 0;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTestStruct")
  {
    const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiTestStruct>();

    XII_TEST_STRING(pRtti->GetTypeName(), "xiiTestStruct");
    XII_TEST_INT(pRtti->GetTypeSize(), sizeof(xiiTestStruct));
    XII_TEST_BOOL(pRtti->GetVariantType() == xiiVariant::Type::Invalid);

    XII_TEST_BOOL(pRtti->GetParentType() == nullptr);

    XII_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTestClass1")
  {
    const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiTestClass1>();

    XII_TEST_STRING(pRtti->GetTypeName(), "xiiTestClass1");
    XII_TEST_INT(pRtti->GetTypeSize(), sizeof(xiiTestClass1));
    XII_TEST_BOOL(pRtti->GetVariantType() == xiiVariant::Type::Invalid);

    XII_TEST_BOOL(pRtti->GetParentType() == xiiGetStaticRTTI<xiiReflectedClass>());

    XII_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());

    xiiTestClass1* pInstance = pRtti->GetAllocator()->Allocate<xiiTestClass1>();
    XII_TEST_BOOL(pInstance != nullptr);

    XII_TEST_BOOL(pInstance->GetDynamicRTTI() == xiiGetStaticRTTI<xiiTestClass1>());
    pInstance->GetDynamicRTTI()->GetAllocator()->Deallocate(pInstance);

    XII_TEST_BOOL(pRtti->IsDerivedFrom<xiiReflectedClass>());
    XII_TEST_BOOL(pRtti->IsDerivedFrom(xiiGetStaticRTTI<xiiReflectedClass>()));

    XII_TEST_BOOL(pRtti->IsDerivedFrom<xiiTestClass1>());
    XII_TEST_BOOL(pRtti->IsDerivedFrom(xiiGetStaticRTTI<xiiTestClass1>()));

    XII_TEST_BOOL(!pRtti->IsDerivedFrom<xiiVec3>());
    XII_TEST_BOOL(!pRtti->IsDerivedFrom(xiiGetStaticRTTI<xiiVec3>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTestClass2")
  {
    const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiTestClass2>();

    XII_TEST_STRING(pRtti->GetTypeName(), "xiiTestClass2");
    XII_TEST_INT(pRtti->GetTypeSize(), sizeof(xiiTestClass2));
    XII_TEST_BOOL(pRtti->GetVariantType() == xiiVariant::Type::Invalid);

    XII_TEST_BOOL(pRtti->GetParentType() == xiiGetStaticRTTI<xiiTestClass1>());

    XII_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());

    XII_TEST_INT(xiiTestClass2Allocator::m_iAllocs, 0);
    XII_TEST_INT(xiiTestClass2Allocator::m_iDeallocs, 0);

    xiiTestClass2* pInstance = pRtti->GetAllocator()->Allocate<xiiTestClass2>();
    XII_TEST_BOOL(pInstance != nullptr);

    XII_TEST_BOOL(pInstance->GetDynamicRTTI() == xiiGetStaticRTTI<xiiTestClass2>());

    XII_TEST_INT(xiiTestClass2Allocator::m_iAllocs, 1);
    XII_TEST_INT(xiiTestClass2Allocator::m_iDeallocs, 0);

    pInstance->GetDynamicRTTI()->GetAllocator()->Deallocate(pInstance);

    XII_TEST_INT(xiiTestClass2Allocator::m_iAllocs, 1);
    XII_TEST_INT(xiiTestClass2Allocator::m_iDeallocs, 1);

    XII_TEST_BOOL(pRtti->IsDerivedFrom<xiiTestClass1>());
    XII_TEST_BOOL(pRtti->IsDerivedFrom(xiiGetStaticRTTI<xiiTestClass1>()));

    XII_TEST_BOOL(pRtti->IsDerivedFrom<xiiTestClass2>());
    XII_TEST_BOOL(pRtti->IsDerivedFrom(xiiGetStaticRTTI<xiiTestClass2>()));

    XII_TEST_BOOL(pRtti->IsDerivedFrom<xiiReflectedClass>());
    XII_TEST_BOOL(pRtti->IsDerivedFrom(xiiGetStaticRTTI<xiiReflectedClass>()));

    XII_TEST_BOOL(!pRtti->IsDerivedFrom<xiiVec3>());
    XII_TEST_BOOL(!pRtti->IsDerivedFrom(xiiGetStaticRTTI<xiiVec3>()));
  }
}


template <typename T, typename T2>
void TestMemberProperty(const char* szPropName, void* pObject, const xiiRTTI* pRtti, xiiBitflags<xiiPropertyFlags> expectedFlags, T2 expectedValue, T2 testValue, bool bTestDefaultValue = true)
{
  const xiiAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  if (!XII_TEST_BOOL(pProp != nullptr))
    return;

  XII_ANALYSIS_ASSUME(pProp != nullptr);

  XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Member);

  XII_TEST_BOOL(pProp->GetSpecificType() == xiiGetStaticRTTI<T>());
  auto pMember = static_cast<const xiiTypedMemberProperty<T>*>(pProp);

  XII_TEST_INT(pMember->GetFlags().GetValue(), expectedFlags.GetValue());

  T value = pMember->GetValue(pObject);
  XII_TEST_BOOL(expectedValue == value);

  if (bTestDefaultValue)
  {
    // Default value
    xiiVariant defaultValue = xiiReflectionUtils::GetDefaultValue(pProp);
    XII_TEST_BOOL(xiiVariant(expectedValue) == defaultValue);
  }

  if (!pMember->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
  {
    pMember->SetValue(pObject, testValue);

    XII_TEST_BOOL(testValue == pMember->GetValue(pObject));

    xiiReflectionUtils::SetMemberPropertyValue(pMember, pObject, xiiVariant(expectedValue));
    xiiVariant res = xiiReflectionUtils::GetMemberPropertyValue(pMember, pObject);

    XII_TEST_BOOL(res == xiiVariant(expectedValue));
    XII_TEST_BOOL(res != xiiVariant(testValue));

    xiiReflectionUtils::SetMemberPropertyValue(pMember, pObject, xiiVariant(testValue));
    res = xiiReflectionUtils::GetMemberPropertyValue(pMember, pObject);

    XII_TEST_BOOL(res != xiiVariant(expectedValue));
    XII_TEST_BOOL(res == xiiVariant(testValue));
  }
}

XII_CREATE_SIMPLE_TEST(Reflection, MemberProperties)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTestStruct")
  {
    xiiTestStruct  data;
    const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiTestStruct>();

    TestMemberProperty<float>("Float", &data, pRtti, xiiPropertyFlags::StandardType, 1.1f, 5.0f);
    TestMemberProperty<xiiInt32>("Int", &data, pRtti, xiiPropertyFlags::StandardType, 2, -8);
    TestMemberProperty<xiiVec3>("Vector", &data, pRtti, xiiPropertyFlags::StandardType | xiiPropertyFlags::ReadOnly, xiiVec3(3, 4, 5), xiiVec3(0, -1.0f, 3.14f));
    TestMemberProperty<xiiVariant>("Variant", &data, pRtti, xiiPropertyFlags::StandardType, xiiVariant("Test"), xiiVariant(xiiVec3(0, -1.0f, 3.14f)));
    TestMemberProperty<xiiAngle>("Angle", &data, pRtti, xiiPropertyFlags::StandardType, xiiAngle::MakeFromDegree(0.5f), xiiAngle::MakeFromDegree(1.0f));
    TestMemberProperty<xiiAngled>("Angled", &data, pRtti, xiiPropertyFlags::StandardType, xiiAngled::MakeFromDegree(0.5), xiiAngled::MakeFromDegree(1.0));

    {
      xiiVarianceTypeAngle expectedVA = xiiVarianceTypeAngle(xiiAngle::MakeFromDegree(90.0f), 0.5f);
      xiiVarianceTypeAngle testVA     = xiiVarianceTypeAngle(xiiAngle::MakeFromDegree(45.0f), 0.1f);
      TestMemberProperty<xiiVarianceTypeAngle>("VarianceAngle", &data, pRtti, xiiPropertyFlags::Class, expectedVA, testVA);
    }

    {
      xiiVarianceTypeAngled expectedVA = xiiVarianceTypeAngled(xiiAngled::MakeFromDegree(90.0), 0.5);
      xiiVarianceTypeAngled testVA     = xiiVarianceTypeAngled(xiiAngled::MakeFromDegree(45.0), 0.1);
      TestMemberProperty<xiiVarianceTypeAngled>("VarianceAngled", &data, pRtti, xiiPropertyFlags::Class, expectedVA, testVA);
    }

    xiiDataBuffer expected;
    expected.PushBack(255);
    expected.PushBack(0);
    expected.PushBack(127);

    xiiDataBuffer newValue;
    newValue.PushBack(1);
    newValue.PushBack(2);

    TestMemberProperty<xiiDataBuffer>("DataBuffer", &data, pRtti, xiiPropertyFlags::StandardType, expected, newValue);
    TestMemberProperty<xiiVec3I32>("vVec3I", &data, pRtti, xiiPropertyFlags::StandardType, xiiVec3I32(1, 2, 3), xiiVec3I32(5, 6, 7));

    TestSerialization<xiiTestStruct>(data);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTestClass2")
  {
    xiiTestClass2  Instance;
    const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiTestClass2>();

    {
      TestMemberProperty<const char*>("CharPtr", &Instance, pRtti, xiiPropertyFlags::StandardType | xiiPropertyFlags::Const, xiiString("AAA"), xiiString("aaaa"));

      TestMemberProperty<xiiString>("String", &Instance, pRtti, xiiPropertyFlags::StandardType, xiiString("BBB"), xiiString("bbbb"));

      TestMemberProperty<xiiStringView>("StringView", &Instance, pRtti, xiiPropertyFlags::StandardType, "CCC"_xiisv, "cccc"_xiisv);

      Instance.SetStringView("CCC");
      TestMemberProperty<xiiStringView>("StringView", &Instance, pRtti, xiiPropertyFlags::StandardType, xiiString("CCC"), xiiString("cccc"));

      const xiiAbstractProperty* pProp = pRtti->FindPropertyByName("SubVector", false);
      XII_TEST_BOOL(pProp == nullptr);
    }

    {
      TestMemberProperty<xiiVec3>("SubVector", &Instance, pRtti, xiiPropertyFlags::StandardType | xiiPropertyFlags::ReadOnly, xiiVec3(3, 4, 5), xiiVec3(3, 4, 5));
      const xiiAbstractProperty* pProp = pRtti->FindPropertyByName("SubStruct", false);
      XII_TEST_BOOL(pProp == nullptr);
    }

    {
      const xiiAbstractProperty* pProp = pRtti->FindPropertyByName("SubStruct");
      if (XII_TEST_BOOL(pProp != nullptr))
      {
        XII_ANALYSIS_ASSUME(pProp != nullptr);

        XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Member);
        xiiAbstractMemberProperty* pAbs = (xiiAbstractMemberProperty*)pProp;

        const xiiRTTI* pStruct    = pAbs->GetSpecificType();
        void*          pSubStruct = pAbs->GetPropertyPointer(&Instance);

        XII_TEST_BOOL(pSubStruct != nullptr);

        TestMemberProperty<float>("Float", pSubStruct, pStruct, xiiPropertyFlags::StandardType, 33.3f, 44.4f, false);
      }
    }

    TestSerialization<xiiTestClass2>(Instance);
  }
}


XII_CREATE_SIMPLE_TEST(Reflection, Enum)
{
  const xiiRTTI* pEnumRTTI = xiiGetStaticRTTI<xiiExampleEnum>();
  const xiiRTTI* pRTTI     = xiiGetStaticRTTI<xiiTestEnumStruct>();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Enum Constants")
  {
    XII_TEST_BOOL(pEnumRTTI->IsDerivedFrom<xiiEnumBase>());
    auto props = pEnumRTTI->GetProperties();
    XII_TEST_INT(props.GetCount(), 4); // Default + 3

    for (auto pProp : props)
    {
      XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Constant);
      XII_TEST_BOOL(pProp->GetSpecificType() == xiiGetStaticRTTI<xiiInt8>());
    }
    XII_TEST_INT(xiiExampleEnum::Default, xiiReflectionUtils::DefaultEnumerationValue(pEnumRTTI));

    XII_TEST_STRING(props[0]->GetPropertyName(), "xiiExampleEnum::Default");
    XII_TEST_STRING(props[1]->GetPropertyName(), "xiiExampleEnum::Value1");
    XII_TEST_STRING(props[2]->GetPropertyName(), "xiiExampleEnum::Value2");
    XII_TEST_STRING(props[3]->GetPropertyName(), "xiiExampleEnum::Value3");

    auto pTypedConstantProp0 = static_cast<const xiiTypedConstantProperty<xiiInt8>*>(props[0]);
    auto pTypedConstantProp1 = static_cast<const xiiTypedConstantProperty<xiiInt8>*>(props[1]);
    auto pTypedConstantProp2 = static_cast<const xiiTypedConstantProperty<xiiInt8>*>(props[2]);
    auto pTypedConstantProp3 = static_cast<const xiiTypedConstantProperty<xiiInt8>*>(props[3]);
    XII_TEST_INT(pTypedConstantProp0->GetValue(), xiiExampleEnum::Default);
    XII_TEST_INT(pTypedConstantProp1->GetValue(), xiiExampleEnum::Value1);
    XII_TEST_INT(pTypedConstantProp2->GetValue(), xiiExampleEnum::Value2);
    XII_TEST_INT(pTypedConstantProp3->GetValue(), xiiExampleEnum::Value3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Enum Property")
  {
    xiiTestEnumStruct data;
    auto              props = pRTTI->GetProperties();
    XII_TEST_INT(props.GetCount(), 4);

    for (auto pProp : props)
    {
      XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Member);
      XII_TEST_INT(pProp->GetFlags().GetValue(), xiiPropertyFlags::IsEnum);
      XII_TEST_BOOL(pProp->GetSpecificType() == pEnumRTTI);
      auto pEnumProp = static_cast<const xiiAbstractEnumerationProperty*>(pProp);
      XII_TEST_BOOL(pEnumProp->GetValue(&data) == xiiExampleEnum::Value1);

      const xiiRTTI* pEnumPropertyRTTI = pEnumProp->GetSpecificType();
      // Set and get all valid enum values.
      for (auto pProp2 : pEnumPropertyRTTI->GetProperties().GetSubArray(1))
      {
        auto pConstantProp = static_cast<const xiiTypedConstantProperty<xiiInt8>*>(pProp2);
        pEnumProp->SetValue(&data, pConstantProp->GetValue());
        XII_TEST_INT(pEnumProp->GetValue(&data), pConstantProp->GetValue());

        // Enum <-> string
        xiiStringBuilder sValue;
        XII_TEST_BOOL(xiiReflectionUtils::EnumerationToString(pEnumPropertyRTTI, pConstantProp->GetValue(), sValue));
        XII_TEST_STRING(sValue, pConstantProp->GetPropertyName());

        // Setting the value via a string also works.
        pEnumProp->SetValue(&data, xiiExampleEnum::Value1);
        xiiReflectionUtils::SetMemberPropertyValue(pEnumProp, &data, sValue.GetData());
        XII_TEST_INT(pEnumProp->GetValue(&data), pConstantProp->GetValue());

        xiiInt64 iValue = 0;
        XII_TEST_BOOL(xiiReflectionUtils::StringToEnumeration(pEnumPropertyRTTI, sValue, iValue));
        XII_TEST_INT(iValue, pConstantProp->GetValue());

        // Testing the short enum name version
        XII_TEST_BOOL(xiiReflectionUtils::EnumerationToString(pEnumPropertyRTTI, pConstantProp->GetValue(), sValue, xiiReflectionUtils::EnumConversionMode::ValueNameOnly));
        XII_TEST_BOOL(sValue.IsEqual(pConstantProp->GetPropertyName()) || sValue.IsEqual(pConstantProp->GetPropertyName().FindLastSubString("::") + 2));

        XII_TEST_BOOL(xiiReflectionUtils::StringToEnumeration(pEnumPropertyRTTI, sValue, iValue));
        XII_TEST_INT(iValue, pConstantProp->GetValue());

        // Testing the short enum name version
        XII_TEST_BOOL(xiiReflectionUtils::EnumerationToString(pEnumPropertyRTTI, pConstantProp->GetValue(), sValue, xiiReflectionUtils::EnumConversionMode::ValueNameOnly));
        XII_TEST_BOOL(sValue.IsEqual(pConstantProp->GetPropertyName()) || sValue.IsEqual(pConstantProp->GetPropertyName().FindLastSubString("::") + 2));

        XII_TEST_BOOL(xiiReflectionUtils::StringToEnumeration(pEnumPropertyRTTI, sValue, iValue));
        XII_TEST_INT(iValue, pConstantProp->GetValue());

        XII_TEST_INT(iValue, xiiReflectionUtils::MakeEnumerationValid(pEnumPropertyRTTI, iValue));
        XII_TEST_INT(xiiExampleEnum::Default, xiiReflectionUtils::MakeEnumerationValid(pEnumPropertyRTTI, iValue + 666));
      }
    }

    XII_TEST_BOOL(data.m_enum == xiiExampleEnum::Value3);
    XII_TEST_BOOL(data.m_enumClass == xiiExampleEnum::Value3);

    XII_TEST_BOOL(data.GetEnum() == xiiExampleEnum::Value3);
    XII_TEST_BOOL(data.GetEnumClass() == xiiExampleEnum::Value3);

    TestSerialization<xiiTestEnumStruct>(data);
  }
}


XII_CREATE_SIMPLE_TEST(Reflection, Bitflags)
{
  const xiiRTTI* pBitflagsRTTI = xiiGetStaticRTTI<xiiExampleBitflags>();
  const xiiRTTI* pRTTI         = xiiGetStaticRTTI<xiiTestBitflagsStruct>();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Bitflags Constants")
  {
    XII_TEST_BOOL(pBitflagsRTTI->IsDerivedFrom<xiiBitflagsBase>());
    auto props = pBitflagsRTTI->GetProperties();
    XII_TEST_INT(props.GetCount(), 4); // Default + 3

    for (auto pProp : props)
    {
      XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Constant);
      XII_TEST_BOOL(pProp->GetSpecificType() == xiiGetStaticRTTI<xiiUInt64>());
    }
    XII_TEST_INT(xiiExampleBitflags::Default, xiiReflectionUtils::DefaultEnumerationValue(pBitflagsRTTI));

    XII_TEST_STRING(props[0]->GetPropertyName(), "xiiExampleBitflags::Default");
    XII_TEST_STRING(props[1]->GetPropertyName(), "xiiExampleBitflags::Value1");
    XII_TEST_STRING(props[2]->GetPropertyName(), "xiiExampleBitflags::Value2");
    XII_TEST_STRING(props[3]->GetPropertyName(), "xiiExampleBitflags::Value3");

    auto pTypedConstantProp0 = static_cast<const xiiTypedConstantProperty<xiiUInt64>*>(props[0]);
    auto pTypedConstantProp1 = static_cast<const xiiTypedConstantProperty<xiiUInt64>*>(props[1]);
    auto pTypedConstantProp2 = static_cast<const xiiTypedConstantProperty<xiiUInt64>*>(props[2]);
    auto pTypedConstantProp3 = static_cast<const xiiTypedConstantProperty<xiiUInt64>*>(props[3]);
    XII_TEST_BOOL(pTypedConstantProp0->GetValue() == xiiExampleBitflags::Default);
    XII_TEST_BOOL(pTypedConstantProp1->GetValue() == xiiExampleBitflags::Value1);
    XII_TEST_BOOL(pTypedConstantProp2->GetValue() == xiiExampleBitflags::Value2);
    XII_TEST_BOOL(pTypedConstantProp3->GetValue() == xiiExampleBitflags::Value3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Bitflags Property")
  {
    xiiTestBitflagsStruct data;
    auto                  props = pRTTI->GetProperties();
    XII_TEST_INT(props.GetCount(), 2);

    for (auto pProp : props)
    {
      XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Member);
      XII_TEST_BOOL(pProp->GetSpecificType() == pBitflagsRTTI);
      XII_TEST_INT(pProp->GetFlags().GetValue(), xiiPropertyFlags::Bitflags);
      auto pBitflagsProp = static_cast<const xiiAbstractEnumerationProperty*>(pProp);
      XII_TEST_BOOL(pBitflagsProp->GetValue(&data) == xiiExampleBitflags::Value1);

      const xiiRTTI* pBitflagsPropertyRTTI = pBitflagsProp->GetSpecificType();

      // Set and get all valid bitflags values. (skip default value)
      xiiUInt64 constants[] = {
        static_cast<const xiiTypedConstantProperty<xiiUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[1])->GetValue(),
        static_cast<const xiiTypedConstantProperty<xiiUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[2])->GetValue(),
        static_cast<const xiiTypedConstantProperty<xiiUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[3])->GetValue(),
      };

      const char* stringValues[] = {"",
                                    "xiiExampleBitflags::Value1",
                                    "xiiExampleBitflags::Value2",
                                    "xiiExampleBitflags::Value1|xiiExampleBitflags::Value2",
                                    "xiiExampleBitflags::Value3",
                                    "xiiExampleBitflags::Value1|xiiExampleBitflags::Value3",
                                    "xiiExampleBitflags::Value2|xiiExampleBitflags::Value3",
                                    "xiiExampleBitflags::Value1|xiiExampleBitflags::Value2|xiiExampleBitflags::Value3"};

      const char* stringValuesShort[] = {"",
                                         "Value1",
                                         "Value2",
                                         "Value1|Value2",
                                         "Value3",
                                         "Value1|Value3",
                                         "Value2|Value3",
                                         "Value1|Value2|Value3"};
      for (xiiInt32 i = 0; i < 8; ++i)
      {
        xiiUInt64 uiBitflagValue = 0;
        uiBitflagValue |= (i & XII_BIT(0)) != 0 ? constants[0] : 0;
        uiBitflagValue |= (i & XII_BIT(1)) != 0 ? constants[1] : 0;
        uiBitflagValue |= (i & XII_BIT(2)) != 0 ? constants[2] : 0;

        pBitflagsProp->SetValue(&data, uiBitflagValue);
        XII_TEST_INT(pBitflagsProp->GetValue(&data), uiBitflagValue);

        // Bitflags <-> string
        xiiStringBuilder sValue;
        XII_TEST_BOOL(xiiReflectionUtils::EnumerationToString(pBitflagsPropertyRTTI, uiBitflagValue, sValue));
        XII_TEST_STRING(sValue, stringValues[i]);

        // Setting the value via a string also works.
        pBitflagsProp->SetValue(&data, 0);
        xiiReflectionUtils::SetMemberPropertyValue(pBitflagsProp, &data, sValue.GetData());
        XII_TEST_INT(pBitflagsProp->GetValue(&data), uiBitflagValue);

        xiiInt64 iValue = 0;
        XII_TEST_BOOL(xiiReflectionUtils::StringToEnumeration(pBitflagsPropertyRTTI, sValue, iValue));
        XII_TEST_INT(iValue, uiBitflagValue);

        // Testing the short enum name version
        XII_TEST_BOOL(xiiReflectionUtils::EnumerationToString(pBitflagsPropertyRTTI, uiBitflagValue, sValue, xiiReflectionUtils::EnumConversionMode::ValueNameOnly));
        XII_TEST_BOOL(sValue.IsEqual(stringValuesShort[i]));

        XII_TEST_BOOL(xiiReflectionUtils::StringToEnumeration(pBitflagsPropertyRTTI, sValue, iValue));
        XII_TEST_INT(iValue, uiBitflagValue);

        // Testing the short enum name version
        XII_TEST_BOOL(xiiReflectionUtils::EnumerationToString(pBitflagsPropertyRTTI, uiBitflagValue, sValue, xiiReflectionUtils::EnumConversionMode::ValueNameOnly));
        XII_TEST_BOOL(sValue.IsEqual(stringValuesShort[i]));

        XII_TEST_BOOL(xiiReflectionUtils::StringToEnumeration(pBitflagsPropertyRTTI, sValue, iValue));
        XII_TEST_INT(iValue, uiBitflagValue);

        XII_TEST_INT(iValue, xiiReflectionUtils::MakeEnumerationValid(pBitflagsPropertyRTTI, iValue));
        XII_TEST_INT(iValue, xiiReflectionUtils::MakeEnumerationValid(pBitflagsPropertyRTTI, iValue | XII_BIT(16)));
      }
    }

    XII_TEST_BOOL(data.m_bitflagsClass == (xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2 | xiiExampleBitflags::Value3));
    XII_TEST_BOOL(data.GetBitflagsClass() == (xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2 | xiiExampleBitflags::Value3));
    TestSerialization<xiiTestBitflagsStruct>(data);
  }
}


template <typename T>
void TestArrayPropertyVariant(const xiiAbstractArrayProperty* pArrayProp, void* pObject, const xiiRTTI* pRtti, T& value)
{
  T temp = {};

  // Reflection Utils
  xiiVariant value0 = xiiReflectionUtils::GetArrayPropertyValue(pArrayProp, pObject, 0);
  XII_TEST_BOOL(value0 == xiiVariant(value));
  // insert
  xiiReflectionUtils::InsertArrayPropertyValue(pArrayProp, pObject, xiiVariant(temp), 2);
  XII_TEST_INT(pArrayProp->GetCount(pObject), 3);
  xiiVariant value2 = xiiReflectionUtils::GetArrayPropertyValue(pArrayProp, pObject, 2);
  XII_TEST_BOOL(value0 != value2);
  xiiReflectionUtils::SetArrayPropertyValue(pArrayProp, pObject, 2, value);
  value2 = xiiReflectionUtils::GetArrayPropertyValue(pArrayProp, pObject, 2);
  XII_TEST_BOOL(value0 == value2);
  // remove again
  xiiReflectionUtils::RemoveArrayPropertyValue(pArrayProp, pObject, 2);
  XII_TEST_INT(pArrayProp->GetCount(pObject), 2);
}

template <>
void TestArrayPropertyVariant<xiiTestArrays>(const xiiAbstractArrayProperty* pArrayProp, void* pObject, const xiiRTTI* pRtti, xiiTestArrays& value)
{
}

template <>
void TestArrayPropertyVariant<xiiTestStruct3>(const xiiAbstractArrayProperty* pArrayProp, void* pObject, const xiiRTTI* pRtti, xiiTestStruct3& value)
{
}

template <typename T>
void TestArrayProperty(const char* szPropName, void* pObject, const xiiRTTI* pRtti, T& value)
{
  const xiiAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  XII_TEST_BOOL(pProp != nullptr);

  if (pProp == nullptr)
    return;

  XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Array);
  auto           pArrayProp = static_cast<const xiiAbstractArrayProperty*>(pProp);
  const xiiRTTI* pElemRtti  = pProp->GetSpecificType();
  XII_TEST_BOOL(pElemRtti == xiiGetStaticRTTI<T>());
  if (!pArrayProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
  {
    // If we don't know the element type T but we can allocate it, we can handle it anyway.
    if (pElemRtti->GetAllocator()->CanAllocate())
    {
      void* pData = pElemRtti->GetAllocator()->Allocate<void>();

      pArrayProp->SetCount(pObject, 2);
      XII_TEST_INT(pArrayProp->GetCount(pObject), 2);
      // Push default constructed object in both slots.
      pArrayProp->SetValue(pObject, 0, pData);
      pArrayProp->SetValue(pObject, 1, pData);

      // Retrieve it again and compare to function parameter, they should be different.
      pArrayProp->GetValue(pObject, 0, pData);
      XII_TEST_BOOL(*static_cast<T*>(pData) != value);
      pArrayProp->GetValue(pObject, 1, pData);
      XII_TEST_BOOL(*static_cast<T*>(pData) != value);

      pElemRtti->GetAllocator()->Deallocate(pData);
    }

    pArrayProp->Clear(pObject);
    XII_TEST_INT(pArrayProp->GetCount(pObject), 0);
    pArrayProp->SetCount(pObject, 2);
    pArrayProp->SetValue(pObject, 0, &value);
    pArrayProp->SetValue(pObject, 1, &value);

    // Insert default init values
    T temp = {};
    pArrayProp->Insert(pObject, 2, &temp);
    XII_TEST_INT(pArrayProp->GetCount(pObject), 3);
    pArrayProp->Insert(pObject, 0, &temp);
    XII_TEST_INT(pArrayProp->GetCount(pObject), 4);

    // Remove them again
    pArrayProp->Remove(pObject, 3);
    XII_TEST_INT(pArrayProp->GetCount(pObject), 3);
    pArrayProp->Remove(pObject, 0);
    XII_TEST_INT(pArrayProp->GetCount(pObject), 2);

    TestArrayPropertyVariant<T>(pArrayProp, pObject, pRtti, value);
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  XII_TEST_INT(pArrayProp->GetCount(pObject), 2);

  T v1 = {};
  pArrayProp->GetValue(pObject, 0, &v1);
  if constexpr (std::is_same<const char*, T>::value)
  {
    XII_TEST_BOOL(xiiStringUtils::IsEqual(v1, value));
  }
  else
  {
    XII_TEST_BOOL(v1 == value);
  }

  T v2 = {};
  pArrayProp->GetValue(pObject, 1, &v2);
  if constexpr (std::is_same<const char*, T>::value)
  {
    XII_TEST_BOOL(xiiStringUtils::IsEqual(v2, value));
  }
  else
  {
    XII_TEST_BOOL(v2 == value);
  }

  if (pElemRtti->GetAllocator()->CanAllocate())
  {
    // Current values should be different from default constructed version.
    void* pData = pElemRtti->GetAllocator()->Allocate<void>();

    XII_TEST_BOOL(*static_cast<T*>(pData) != v1);
    XII_TEST_BOOL(*static_cast<T*>(pData) != v2);

    pElemRtti->GetAllocator()->Deallocate(pData);
  }
}

XII_CREATE_SIMPLE_TEST(Reflection, Arrays)
{
  xiiTestArrays  containers;
  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiTestArrays>();
  XII_TEST_BOOL(pRtti != nullptr);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "POD Array")
  {
    double fValue = 5;
    TestArrayProperty<double>("Hybrid", &containers, pRtti, fValue);
    TestArrayProperty<double>("HybridRO", &containers, pRtti, fValue);

    TestArrayProperty<double>("AcHybrid", &containers, pRtti, fValue);
    TestArrayProperty<double>("AcHybridRO", &containers, pRtti, fValue);

    xiiStringView sValue0  = "Bla";
    xiiStringView sValue02 = "LongString------------------------------------------------------------------------------------";
    xiiString     sValue   = sValue0;
    xiiString     sValue2  = sValue02;

    TestArrayProperty<xiiString>("HybridChar", &containers, pRtti, sValue);
    TestArrayProperty<xiiString>("HybridCharRO", &containers, pRtti, sValue);

    TestArrayProperty<xiiStringView>("AcHybridChar", &containers, pRtti, sValue0);
    TestArrayProperty<xiiStringView>("AcHybridCharRO", &containers, pRtti, sValue0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Struct Array")
  {
    xiiTestStruct3 data;
    data.m_fFloat1 = 99.0f;
    data.m_UInt8   = 127;

    TestArrayProperty<xiiTestStruct3>("Dynamic", &containers, pRtti, data);
    TestArrayProperty<xiiTestStruct3>("DynamicRO", &containers, pRtti, data);

    TestArrayProperty<xiiTestStruct3>("AcDynamic", &containers, pRtti, data);
    TestArrayProperty<xiiTestStruct3>("AcDynamicRO", &containers, pRtti, data);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiReflectedClass Array")
  {
    xiiTestArrays data;
    data.m_Hybrid.PushBack(42.0);

    TestArrayProperty<xiiTestArrays>("Deque", &containers, pRtti, data);
    TestArrayProperty<xiiTestArrays>("DequeRO", &containers, pRtti, data);

    TestArrayProperty<xiiTestArrays>("AcDeque", &containers, pRtti, data);
    TestArrayProperty<xiiTestArrays>("AcDequeRO", &containers, pRtti, data);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Custom Variant Array")
  {
    // xiiVarianceTypeAngle
    {
      xiiVarianceTypeAngle data(xiiAngle::MakeFromDegree(45.0f), 0.1f);

      TestArrayProperty<xiiVarianceTypeAngle>("Custom", &containers, pRtti, data);
      TestArrayProperty<xiiVarianceTypeAngle>("CustomRO", &containers, pRtti, data);

      TestArrayProperty<xiiVarianceTypeAngle>("AcCustom", &containers, pRtti, data);
      TestArrayProperty<xiiVarianceTypeAngle>("AcCustomRO", &containers, pRtti, data);
    }

    // xiiVarianceTypeAngled
    {
      xiiVarianceTypeAngled data(xiiAngled::MakeFromDegree(45.0), 0.1);

      TestArrayProperty<xiiVarianceTypeAngled>("Custom2", &containers, pRtti, data);
      TestArrayProperty<xiiVarianceTypeAngled>("CustomRO2", &containers, pRtti, data);

      TestArrayProperty<xiiVarianceTypeAngled>("AcCustom2", &containers, pRtti, data);
      TestArrayProperty<xiiVarianceTypeAngled>("AcCustomRO2", &containers, pRtti, data);
    }
  }

  TestSerialization<xiiTestArrays>(containers);
}

/// \brief Determines whether a type is a pointer.
template <typename T>
struct xiiIsPointer
{
  static constexpr bool value = false;
};

template <typename T>
struct xiiIsPointer<T*>
{
  static constexpr bool value = true;
};

template <typename T>
void TestSetProperty(const char* szPropName, void* pObject, const xiiRTTI* pRtti, T& ref_value1, T& ref_value2)
{
  const xiiAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  if (!XII_TEST_BOOL(pProp != nullptr))
    return;

  XII_ANALYSIS_ASSUME(pProp != nullptr);

  XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Set);
  auto           pSetProp  = static_cast<const xiiAbstractSetProperty*>(pProp);
  const xiiRTTI* pElemRtti = pProp->GetSpecificType();
  XII_TEST_BOOL(pElemRtti == xiiGetStaticRTTI<T>());

  if (!pSetProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
  {
    pSetProp->Clear(pObject);
    XII_TEST_BOOL(pSetProp->IsEmpty(pObject));

    pSetProp->Insert(pObject, &ref_value1);

    XII_TEST_BOOL(!pSetProp->IsEmpty(pObject));
    XII_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
    XII_TEST_BOOL(!pSetProp->Contains(pObject, &ref_value2));

    pSetProp->Insert(pObject, &ref_value2);

    XII_TEST_BOOL(!pSetProp->IsEmpty(pObject));
    XII_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
    XII_TEST_BOOL(pSetProp->Contains(pObject, &ref_value2));

    // Insert default init value
    if (!xiiIsPointer<T>::value)
    {
      T temp = T{};
      pSetProp->Insert(pObject, &temp);

      XII_TEST_BOOL(!pSetProp->IsEmpty(pObject));
      XII_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
      XII_TEST_BOOL(pSetProp->Contains(pObject, &ref_value2));
      XII_TEST_BOOL(pSetProp->Contains(pObject, &temp));

      // Remove it again
      pSetProp->Remove(pObject, &temp);
      XII_TEST_BOOL(!pSetProp->IsEmpty(pObject));
      XII_TEST_BOOL(!pSetProp->Contains(pObject, &temp));
    }
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  XII_TEST_BOOL(!pSetProp->IsEmpty(pObject));
  XII_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
  XII_TEST_BOOL(pSetProp->Contains(pObject, &ref_value2));


  xiiHybridArray<xiiVariant, 16> keys;
  pSetProp->GetValues(pObject, keys);
  XII_TEST_INT(keys.GetCount(), 2);
}

XII_CREATE_SIMPLE_TEST(Reflection, Sets)
{
  xiiTestSets    containers;
  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiTestSets>();
  XII_TEST_BOOL(pRtti != nullptr);

  // Disabled because MSVC 2017 has code generation issues in Release builds
  XII_TEST_BLOCK(xiiTestBlock::Disabled, "xiiSet")
  {
    xiiInt8 iValue1 = -5;
    xiiInt8 iValue2 = 127;
    TestSetProperty<xiiInt8>("Set", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<xiiInt8>("SetRO", &containers, pRtti, iValue1, iValue2);

    double fValue1 = 5;
    double fValue2 = -3;
    TestSetProperty<double>("AcSet", &containers, pRtti, fValue1, fValue2);
    TestSetProperty<double>("AcSetRO", &containers, pRtti, fValue1, fValue2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiHashSet")
  {
    xiiInt32 iValue1 = -5;
    xiiInt32 iValue2 = 127;
    TestSetProperty<xiiInt32>("HashSet", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<xiiInt32>("HashSetRO", &containers, pRtti, iValue1, iValue2);

    xiiInt64 fValue1 = 5;
    xiiInt64 fValue2 = -3;
    TestSetProperty<xiiInt64>("HashAcSet", &containers, pRtti, fValue1, fValue2);
    TestSetProperty<xiiInt64>("HashAcSetRO", &containers, pRtti, fValue1, fValue2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiDeque Pseudo Set")
  {
    int iValue1 = -5;
    int iValue2 = 127;

    TestSetProperty<int>("AcPseudoSet", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<int>("AcPseudoSetRO", &containers, pRtti, iValue1, iValue2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiSetPtr Pseudo Set")
  {
    xiiString sValue1 = "TestString1";
    xiiString sValue2 = "Test String Deus";

    TestSetProperty<xiiString>("AcPseudoSet2", &containers, pRtti, sValue1, sValue2);
    TestSetProperty<xiiString>("AcPseudoSet2RO", &containers, pRtti, sValue1, sValue2);

    xiiStringView sValue01 = "TestString1";
    xiiStringView sValue02 = "Test String Deus";
    TestSetProperty<xiiStringView>("AcPseudoSet2b", &containers, pRtti, sValue01, sValue02);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Custom Variant HashSet")
  {
    // xiiVarianceTypeAngle
    {
      xiiVarianceTypeAngle value1(xiiAngle::MakeFromDegree(-45.0f), -0.1f);
      xiiVarianceTypeAngle value2(xiiAngle::MakeFromDegree(45.0f), 0.1f);

      TestSetProperty<xiiVarianceTypeAngle>("CustomHashSet", &containers, pRtti, value1, value2);
      TestSetProperty<xiiVarianceTypeAngle>("CustomHashSetRO", &containers, pRtti, value1, value2);

      xiiVarianceTypeAngle value3(xiiAngle::MakeFromDegree(-90.0f), -0.2f);
      xiiVarianceTypeAngle value4(xiiAngle::MakeFromDegree(90.0f), 0.2f);
      TestSetProperty<xiiVarianceTypeAngle>("CustomHashAcSet", &containers, pRtti, value3, value4);
      TestSetProperty<xiiVarianceTypeAngle>("CustomHashAcSetRO", &containers, pRtti, value3, value4);
    }

    // xiiVarianceTypeAngled
    {
      xiiVarianceTypeAngled value1(xiiAngled::MakeFromDegree(-45.0), -0.1);
      xiiVarianceTypeAngled value2(xiiAngled::MakeFromDegree(45.0), 0.1);

      TestSetProperty<xiiVarianceTypeAngled>("CustomHashSet2", &containers, pRtti, value1, value2);
      TestSetProperty<xiiVarianceTypeAngled>("CustomHashSetRO2", &containers, pRtti, value1, value2);

      xiiVarianceTypeAngled value3(xiiAngled::MakeFromDegree(-90.0), -0.2);
      xiiVarianceTypeAngled value4(xiiAngled::MakeFromDegree(90.0), 0.2);
      TestSetProperty<xiiVarianceTypeAngled>("CustomHashAcSet2", &containers, pRtti, value3, value4);
      TestSetProperty<xiiVarianceTypeAngled>("CustomHashAcSetRO2", &containers, pRtti, value3, value4);
    }
  }
  TestSerialization<xiiTestSets>(containers);
}

template <typename T>
void TestMapProperty(const char* szPropName, void* pObject, const xiiRTTI* pRtti, T& ref_value1, T& ref_value2)
{
  const xiiAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  if (!XII_TEST_BOOL(pProp != nullptr))
    return;

  XII_ANALYSIS_ASSUME(pProp != nullptr);

  XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Map);

  auto           pMapProp  = static_cast<const xiiAbstractMapProperty*>(pProp);
  const xiiRTTI* pElemRtti = pProp->GetSpecificType();
  XII_TEST_BOOL(pElemRtti == xiiGetStaticRTTI<T>());
  XII_TEST_BOOL(xiiReflectionUtils::IsBasicType(pElemRtti) || pElemRtti == xiiGetStaticRTTI<xiiVariant>() || pElemRtti == xiiGetStaticRTTI<xiiVarianceTypeAngle>() || pElemRtti == xiiGetStaticRTTI<xiiVarianceTypeAngled>());

  if (!pMapProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
  {
    pMapProp->Clear(pObject);
    XII_TEST_BOOL(pMapProp->IsEmpty(pObject));

    pMapProp->Insert(pObject, "value1", &ref_value1);

    XII_TEST_BOOL(!pMapProp->IsEmpty(pObject));
    XII_TEST_BOOL(pMapProp->Contains(pObject, "value1"));
    XII_TEST_BOOL(!pMapProp->Contains(pObject, "value2"));

    T getValue;

    XII_TEST_BOOL(!pMapProp->GetValue(pObject, "value2", &getValue));
    XII_TEST_BOOL(pMapProp->GetValue(pObject, "value1", &getValue));
    XII_TEST_BOOL(getValue == ref_value1);

    pMapProp->Insert(pObject, "value2", &ref_value2);

    XII_TEST_BOOL(!pMapProp->IsEmpty(pObject));
    XII_TEST_BOOL(pMapProp->Contains(pObject, "value1"));
    XII_TEST_BOOL(pMapProp->Contains(pObject, "value2"));
    XII_TEST_BOOL(pMapProp->GetValue(pObject, "value1", &getValue));
    XII_TEST_BOOL(getValue == ref_value1);
    XII_TEST_BOOL(pMapProp->GetValue(pObject, "value2", &getValue));
    XII_TEST_BOOL(getValue == ref_value2);
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  T getValue2;
  XII_TEST_BOOL(!pMapProp->IsEmpty(pObject));
  XII_TEST_BOOL(pMapProp->Contains(pObject, "value1"));
  XII_TEST_BOOL(pMapProp->Contains(pObject, "value2"));
  XII_TEST_BOOL(pMapProp->GetValue(pObject, "value1", &getValue2));
  XII_TEST_BOOL(getValue2 == ref_value1);
  XII_TEST_BOOL(pMapProp->GetValue(pObject, "value2", &getValue2));
  XII_TEST_BOOL(getValue2 == ref_value2);

  xiiHybridArray<xiiString, 16> keys;
  pMapProp->GetKeys(pObject, keys);
  XII_TEST_INT(keys.GetCount(), 2);
  keys.Sort();
  XII_TEST_BOOL(keys[0] == "value1");
  XII_TEST_BOOL(keys[1] == "value2");
}

XII_CREATE_SIMPLE_TEST(Reflection, Maps)
{
  xiiTestMaps    containers;
  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiTestMaps>();
  XII_TEST_BOOL(pRtti != nullptr);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiMap")
  {
    int iValue1 = -5;
    int iValue2 = 127;
    TestMapProperty<int>("Map", &containers, pRtti, iValue1, iValue2);
    TestMapProperty<int>("MapRO", &containers, pRtti, iValue1, iValue2);

    xiiInt64 iValue1b = 5;
    xiiInt64 iValue2b = -3;
    TestMapProperty<xiiInt64>("AcMap", &containers, pRtti, iValue1b, iValue2b);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiHashMap")
  {
    double fValue1 = -5;
    double fValue2 = 127;
    TestMapProperty<double>("HashTable", &containers, pRtti, fValue1, fValue2);
    TestMapProperty<double>("HashTableRO", &containers, pRtti, fValue1, fValue2);

    xiiString sValue1 = "Bla";
    xiiString sValue2 = "Test";
    TestMapProperty<xiiString>("AcHashTable", &containers, pRtti, sValue1, sValue2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Accessor")
  {
    xiiVariant sValue1 = "Test";
    xiiVariant sValue2 = xiiVec4(1, 2, 3, 4);
    TestMapProperty<xiiVariant>("Accessor", &containers, pRtti, sValue1, sValue2);
    TestMapProperty<xiiVariant>("AccessorRO", &containers, pRtti, sValue1, sValue2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CustomVariant")
  {
    // xiiVarianceTypeAngle
    {
      xiiVarianceTypeAngle value1(xiiAngle::MakeFromDegree(-45.0f), -0.1f);
      xiiVarianceTypeAngle value2(xiiAngle::MakeFromDegree(45.0f), 0.1f);

      TestMapProperty<xiiVarianceTypeAngle>("CustomVariant", &containers, pRtti, value1, value2);
      TestMapProperty<xiiVarianceTypeAngle>("CustomVariantRO", &containers, pRtti, value1, value2);
    }

    // xiiVarianceTypeAngled
    {
      xiiVarianceTypeAngled value1(xiiAngled::MakeFromDegree(-45.0), -0.1);
      xiiVarianceTypeAngled value2(xiiAngled::MakeFromDegree(45.0), 0.1);

      TestMapProperty<xiiVarianceTypeAngled>("CustomVariant2", &containers, pRtti, value1, value2);
      TestMapProperty<xiiVarianceTypeAngled>("CustomVariantRO2", &containers, pRtti, value1, value2);
    }
  }
  TestSerialization<xiiTestMaps>(containers);
}


template <typename T>
void TestPointerMemberProperty(const char* szPropName, void* pObject, const xiiRTTI* pRtti, xiiBitflags<xiiPropertyFlags> expectedFlags, T* pExpectedValue)
{
  const xiiAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  if (!XII_TEST_BOOL(pProp != nullptr))
    return;

  XII_ANALYSIS_ASSUME(pProp != nullptr);

  XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Member);

  auto pAbsMember = static_cast<const xiiAbstractMemberProperty*>(pProp);
  XII_TEST_INT(pProp->GetFlags().GetValue(), expectedFlags.GetValue());
  XII_TEST_BOOL(pProp->GetSpecificType() == xiiGetStaticRTTI<T>());

  void* pData = nullptr;
  pAbsMember->GetValuePtr(pObject, &pData);
  XII_TEST_BOOL(pData == pExpectedValue);

  // Set value to null.
  {
    void* pDataNull = nullptr;
    pAbsMember->SetValuePtr(pObject, &pDataNull);

    void* pDataNull2 = nullptr;
    pAbsMember->GetValuePtr(pObject, &pDataNull2);

    XII_TEST_BOOL(pDataNull == pDataNull2);
  }

  // Set value to new instance.
  {
    void* pNewData = pAbsMember->GetSpecificType()->GetAllocator()->Allocate<void>();
    pAbsMember->SetValuePtr(pObject, &pNewData);

    void* pData2 = nullptr;
    pAbsMember->GetValuePtr(pObject, &pData2);

    XII_TEST_BOOL(pNewData == pData2);
  }

  // Delete old value
  pAbsMember->GetSpecificType()->GetAllocator()->Deallocate(pData);
}

XII_CREATE_SIMPLE_TEST(Reflection, Pointer)
{
  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiTestPtr>();
  if (!XII_TEST_BOOL(pRtti != nullptr))
    return;

  XII_ANALYSIS_ASSUME(pRtti != nullptr);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Member Property Ptr")
  {
    xiiTestPtr containers;
    {
      const xiiAbstractProperty* pProp = pRtti->FindPropertyByName("ConstCharPtr");
      if (XII_TEST_BOOL(pProp != nullptr))
      {
        XII_ANALYSIS_ASSUME(pProp != nullptr);

        XII_TEST_BOOL(pProp->GetCategory() == xiiPropertyCategory::Member);
        XII_TEST_INT(pProp->GetFlags().GetValue(), (xiiPropertyFlags::StandardType | xiiPropertyFlags::Const).GetValue());
        XII_TEST_BOOL(pProp->GetSpecificType() == xiiGetStaticRTTI<const char*>());
      }
    }

    TestPointerMemberProperty<xiiTestArrays>("ArraysPtr", &containers, pRtti, xiiPropertyFlags::Class | xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner, containers.m_pArrays);
    TestPointerMemberProperty<xiiTestArrays>("ArraysPtrDirect", &containers, pRtti, xiiPropertyFlags::Class | xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner, containers.m_pArraysDirect);
  }

  xiiTestPtr                    containers;
  xiiDefaultMemoryStreamStorage StreamStorage;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Serialize Property Ptr")
  {
    containers.m_sString = "Test";

    containers.m_pArrays = XII_DEFAULT_NEW(xiiTestArrays);
    containers.m_pArrays->m_Deque.PushBack(xiiTestArrays());

    containers.m_ArrayPtr.PushBack(XII_DEFAULT_NEW(xiiTestArrays));
    containers.m_ArrayPtr[0]->m_Hybrid.PushBack(5.0);

    containers.m_SetPtr.Insert(XII_DEFAULT_NEW(xiiTestSets));
    containers.m_SetPtr.GetIterator().Key()->m_Array.PushBack("BLA");
  }

  TestSerialization<xiiTestPtr>(containers);
}
