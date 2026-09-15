/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

XII_CREATE_SIMPLE_TEST_GROUP(Reflection);

void VariantToPropertyTest(void* pIntStruct, const xiiRTTI* pRttiInt, xiiStringView sPropName, xiiVariant::Type::Enum type)
{
  const xiiAbstractMemberProperty* pProp = xiiReflectionUtils::GetMemberProperty(pRttiInt, sPropName);
  XII_TEST_BOOL(pProp != nullptr);
  if (pProp)
  {
    xiiVariant oldValue = xiiReflectionUtils::GetMemberPropertyValue(pProp, pIntStruct);
    XII_TEST_BOOL(oldValue.IsValid());
    XII_TEST_BOOL(oldValue.GetType() == type);

    xiiVariant defaultValue = xiiReflectionUtils::GetDefaultValue(pProp);
    XII_TEST_BOOL(defaultValue.GetType() == type);
    xiiReflectionUtils::SetMemberPropertyValue(pProp, pIntStruct, defaultValue);

    xiiVariant newValue = xiiReflectionUtils::GetMemberPropertyValue(pProp, pIntStruct);
    XII_TEST_BOOL(newValue.IsValid());
    XII_TEST_BOOL(newValue.GetType() == type);
    XII_TEST_BOOL(newValue == defaultValue);
    XII_TEST_BOOL(newValue != oldValue);
  }
}

XII_CREATE_SIMPLE_TEST(Reflection, ReflectionUtils)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Integer Properties")
  {
    xiiIntegerStruct intStruct;
    const xiiRTTI*   pRttiInt = xiiRTTI::FindTypeByName("xiiIntegerStruct");
    XII_TEST_BOOL(pRttiInt != nullptr);

    VariantToPropertyTest(&intStruct, pRttiInt, "Int8", xiiVariant::Type::Int8);
    XII_TEST_INT(0, intStruct.GetInt8());
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt8", xiiVariant::Type::UInt8);
    XII_TEST_INT(0, intStruct.GetUInt8());

    VariantToPropertyTest(&intStruct, pRttiInt, "Int16", xiiVariant::Type::Int16);
    XII_TEST_INT(0, intStruct.m_iInt16);
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt16", xiiVariant::Type::UInt16);
    XII_TEST_INT(0, intStruct.m_iUInt16);

    VariantToPropertyTest(&intStruct, pRttiInt, "Int32", xiiVariant::Type::Int32);
    XII_TEST_INT(0, intStruct.GetInt32());
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt32", xiiVariant::Type::UInt32);
    XII_TEST_INT(0, intStruct.GetUInt32());

    VariantToPropertyTest(&intStruct, pRttiInt, "Int64", xiiVariant::Type::Int64);
    XII_TEST_INT(0, intStruct.m_iInt64);
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt64", xiiVariant::Type::UInt64);
    XII_TEST_INT(0, intStruct.m_iUInt64);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Float Properties")
  {
    xiiFloatStruct floatStruct;
    const xiiRTTI* pRttiFloat = xiiRTTI::FindTypeByName("xiiFloatStruct");
    XII_TEST_BOOL(pRttiFloat != nullptr);

    VariantToPropertyTest(&floatStruct, pRttiFloat, "Float", xiiVariant::Type::Float);
    XII_TEST_FLOAT(0, floatStruct.GetFloat(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Double", xiiVariant::Type::Double);
    XII_TEST_FLOAT(0, floatStruct.GetDouble(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Time", xiiVariant::Type::Time);
    XII_TEST_FLOAT(0, floatStruct.GetTime().GetSeconds(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Angle", xiiVariant::Type::Angle);
    XII_TEST_FLOAT(0, floatStruct.GetAngle().GetDegree(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Angled", xiiVariant::Type::Angled);
    XII_TEST_DOUBLE(0, floatStruct.GetAngled().GetDegree(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Misc Properties")
  {
    xiiPODClass    podClass;
    const xiiRTTI* pRttiPOD = xiiRTTI::FindTypeByName("xiiPODClass");
    XII_TEST_BOOL(pRttiPOD != nullptr);

    VariantToPropertyTest(&podClass, pRttiPOD, "Bool", xiiVariant::Type::Bool);
    XII_TEST_BOOL(podClass.GetBool() == false);
    VariantToPropertyTest(&podClass, pRttiPOD, "Color", xiiVariant::Type::Color);
    XII_TEST_BOOL(podClass.GetColor() == xiiColor(1.0f, 1.0f, 1.0f, 1.0f));
    VariantToPropertyTest(&podClass, pRttiPOD, "String", xiiVariant::Type::String);
    XII_TEST_STRING(podClass.GetString(), "");
    VariantToPropertyTest(&podClass, pRttiPOD, "Buffer", xiiVariant::Type::DataBuffer);
    XII_TEST_BOOL(podClass.GetBuffer() == xiiDataBuffer());
    VariantToPropertyTest(&podClass, pRttiPOD, "VarianceAngle", xiiVariant::Type::TypedObject);
    XII_TEST_BOOL(podClass.GetCustom() == xiiVarianceTypeAngle{});
    VariantToPropertyTest(&podClass, pRttiPOD, "VarianceAngled", xiiVariant::Type::TypedObject);
    XII_TEST_BOOL(podClass.GetCustom2() == xiiVarianceTypeAngled{});
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Math Properties")
  {
    xiiMathClass   mathClass;
    const xiiRTTI* pRttiMath = xiiRTTI::FindTypeByName("xiiMathClass");
    XII_TEST_BOOL(pRttiMath != nullptr);

    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2I", xiiVariant::Type::Vector2I);
    XII_TEST_BOOL(mathClass.m_Vec2I == xiiVec2I32(0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3I", xiiVariant::Type::Vector3I);
    XII_TEST_BOOL(mathClass.m_Vec3I == xiiVec3I32(0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4I", xiiVariant::Type::Vector4I);
    XII_TEST_BOOL(mathClass.m_Vec4I == xiiVec4I32(0, 0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2U", xiiVariant::Type::Vector2U);
    XII_TEST_BOOL(mathClass.m_Vec2U == xiiVec2U32(0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3U", xiiVariant::Type::Vector3U);
    XII_TEST_BOOL(mathClass.m_Vec3U == xiiVec3U32(0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4U", xiiVariant::Type::Vector4U);
    XII_TEST_BOOL(mathClass.m_Vec4U == xiiVec4U32(0, 0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2I64", xiiVariant::Type::Vector2I64);
    XII_TEST_BOOL(mathClass.m_Vec2I64 == xiiVec2I64(0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3I64", xiiVariant::Type::Vector3I64);
    XII_TEST_BOOL(mathClass.m_Vec3I64 == xiiVec3I64(0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4I64", xiiVariant::Type::Vector4I64);
    XII_TEST_BOOL(mathClass.m_Vec4I64 == xiiVec4I64(0, 0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2U64", xiiVariant::Type::Vector2U64);
    XII_TEST_BOOL(mathClass.m_Vec2U64 == xiiVec2U64(0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3U64", xiiVariant::Type::Vector3U64);
    XII_TEST_BOOL(mathClass.m_Vec3U64 == xiiVec3U64(0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4U64", xiiVariant::Type::Vector4U64);
    XII_TEST_BOOL(mathClass.m_Vec4U64 == xiiVec4U64(0, 0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2", xiiVariant::Type::Vector2);
    XII_TEST_BOOL(mathClass.GetVec2() == xiiVec2(0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3", xiiVariant::Type::Vector3);
    XII_TEST_BOOL(mathClass.GetVec3() == xiiVec3(0.0f, 0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4", xiiVariant::Type::Vector4);
    XII_TEST_BOOL(mathClass.GetVec4() == xiiVec4(0.0f, 0.0f, 0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Quat", xiiVariant::Type::Quaternion);
    XII_TEST_BOOL(mathClass.GetQuat() == xiiQuat(0.0f, 0.0f, 0.0f, 1.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat3", xiiVariant::Type::Matrix3);
    XII_TEST_BOOL(mathClass.GetMat3() == xiiMat3::MakeIdentity());
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat4", xiiVariant::Type::Matrix4);
    XII_TEST_BOOL(mathClass.GetMat4() == xiiMat4::MakeIdentity());
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2d", xiiVariant::Type::Vector2d);
    XII_TEST_BOOL(mathClass.GetVec2d() == xiiVec2d(0.0, 0.0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3d", xiiVariant::Type::Vector3d);
    XII_TEST_BOOL(mathClass.GetVec3d() == xiiVec3d(0.0, 0.0, 0.0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4d", xiiVariant::Type::Vector4d);
    XII_TEST_BOOL(mathClass.GetVec4d() == xiiVec4d(0.0, 0.0, 0.0, 0.0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Quatd", xiiVariant::Type::Quaterniond);
    XII_TEST_BOOL(mathClass.GetQuatd() == xiiQuatd(0.0, 0.0, 0.0, 1.0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat3d", xiiVariant::Type::Matrix3d);
    XII_TEST_BOOL(mathClass.GetMat3d() == xiiMat3d::MakeIdentity());
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat4d", xiiVariant::Type::Matrix4d);
    XII_TEST_BOOL(mathClass.GetMat4d() == xiiMat4d::MakeIdentity());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Enumeration Properties")
  {
    xiiEnumerationsClass enumClass;
    const xiiRTTI*       pRttiEnum = xiiRTTI::FindTypeByName("xiiEnumerationsClass");
    XII_TEST_BOOL(pRttiEnum != nullptr);

    VariantToPropertyTest(&enumClass, pRttiEnum, "Enum", xiiVariant::Type::Int64);
    XII_TEST_BOOL(enumClass.GetEnum() == xiiExampleEnum::Value1);
    VariantToPropertyTest(&enumClass, pRttiEnum, "Bitflags", xiiVariant::Type::Int64);
    XII_TEST_BOOL(enumClass.GetBitflags() == xiiExampleBitflags::Value1);
  }
}

void AccessorPropertyTest(xiiIReflectedTypeAccessor& ref_accessor, xiiStringView sProperty, xiiVariant::Type::Enum type)
{
  xiiVariant oldValue = ref_accessor.GetValue(sProperty);
  XII_TEST_BOOL(oldValue.IsValid());
  XII_TEST_BOOL(oldValue.GetType() == type);

  const xiiAbstractProperty* pProp        = ref_accessor.GetType()->FindPropertyByName(sProperty);
  xiiVariant                 defaultValue = xiiReflectionUtils::GetDefaultValue(pProp);
  XII_TEST_BOOL(defaultValue.GetType() == type);
  bool bSetSuccess = ref_accessor.SetValue(sProperty, defaultValue);
  XII_TEST_BOOL(bSetSuccess);

  xiiVariant newValue = ref_accessor.GetValue(sProperty);
  XII_TEST_BOOL(newValue.IsValid());
  XII_TEST_BOOL(newValue.GetType() == type);
  XII_TEST_BOOL(newValue == defaultValue);
}

xiiUInt32 AccessorPropertiesTest(xiiIReflectedTypeAccessor& ref_accessor, const xiiRTTI* pType)
{
  xiiUInt32 uiPropertiesSet = 0;
  if (!XII_TEST_BOOL(pType != nullptr))
    return 0;

  XII_ANALYSIS_ASSUME(pType != nullptr);

  // Call for base class
  if (pType->GetParentType() != nullptr)
  {
    uiPropertiesSet += AccessorPropertiesTest(ref_accessor, pType->GetParentType());
  }

  // Test properties
  xiiUInt32 uiPropCount = pType->GetProperties().GetCount();
  for (xiiUInt32 i = 0; i < uiPropCount; ++i)
  {
    const xiiAbstractProperty* pProp        = pType->GetProperties()[i];
    const bool                 bIsValueType = xiiReflectionUtils::IsValueType(pProp);

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Member:
      {
        auto pProp3 = static_cast<const xiiAbstractMemberProperty*>(pProp);
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::IsEnum))
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), xiiVariant::Type::Int64);
          uiPropertiesSet++;
        }
        else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Bitflags))
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), xiiVariant::Type::Int64);
          uiPropertiesSet++;
        }
        else if (bIsValueType)
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), pProp3->GetSpecificType()->GetVariantType());
          uiPropertiesSet++;
        }
        else // xiiPropertyFlags::Class
        {
          // Recurs into sub-classes
          const xiiUuid&     subObjectGuid        = ref_accessor.GetValue(pProp->GetPropertyName()).Get<xiiUuid>();
          xiiDocumentObject* pEmbeddedClassObject = const_cast<xiiDocumentObject*>(ref_accessor.GetOwner()->GetChild(subObjectGuid));
          uiPropertiesSet += AccessorPropertiesTest(pEmbeddedClassObject->GetTypeAccessor(), pProp3->GetSpecificType());
        }
      }
      break;
      case xiiPropertyCategory::Array:
      {
        // xiiAbstractArrayProperty* pProp3 = static_cast<xiiAbstractArrayProperty*>(pProp);
        // TODO
      }
      break;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }
  return uiPropertiesSet;
}

xiiUInt32 AccessorPropertiesTest(xiiIReflectedTypeAccessor& ref_accessor)
{
  const xiiRTTI* handle = ref_accessor.GetType();
  return AccessorPropertiesTest(ref_accessor, handle);
}

static xiiUInt32 GetTypeCount()
{
  xiiUInt32 uiCount = 0;
  xiiRTTI::ForEachType([&](const xiiRTTI* pRtti) { uiCount++; });
  return uiCount;
}

static const xiiRTTI* RegisterType(xiiStringView sTypeName)
{
  const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(sTypeName);
  XII_TEST_BOOL(pRtti != nullptr);

  xiiReflectedTypeDescriptor desc;
  xiiToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRtti, desc);
  return xiiPhantomRttiManager::RegisterType(desc);
}

XII_CREATE_SIMPLE_TEST(Reflection, ReflectedType)
{
  xiiTestDocumentObjectManager manager;

  /*const xiiRTTI* pRttiBase =*/RegisterType("xiiReflectedClass");
  /*const xiiRTTI* pRttiEnumBase =*/RegisterType("xiiEnumBase");
  /*const xiiRTTI* pRttiBitflagsBase =*/RegisterType("xiiBitflagsBase");

  const xiiRTTI* pRttiInt   = RegisterType("xiiIntegerStruct");
  const xiiRTTI* pRttiFloat = RegisterType("xiiFloatStruct");
  const xiiRTTI* pRttiPOD   = RegisterType("xiiPODClass");
  const xiiRTTI* pRttiMath  = RegisterType("xiiMathClass");
  /*const xiiRTTI* pRttiEnum =*/RegisterType("xiiExampleEnum");
  /*const xiiRTTI* pRttiFlags =*/RegisterType("xiiExampleBitflags");
  const xiiRTTI* pRttiEnumerations = RegisterType("xiiEnumerationsClass");

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiReflectedTypeStorageAccessor")
  {
    {
      xiiDocumentObject* pObject = manager.CreateObject(pRttiInt);
      XII_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 8);
      manager.DestroyObject(pObject);
    }
    {
      xiiDocumentObject* pObject = manager.CreateObject(pRttiFloat);
      XII_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 5);
      manager.DestroyObject(pObject);
    }
    {
      xiiDocumentObject* pObject = manager.CreateObject(pRttiPOD);
      XII_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 20);
      manager.DestroyObject(pObject);
    }
    {
      xiiDocumentObject* pObject = manager.CreateObject(pRttiMath);
      XII_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 44);
      manager.DestroyObject(pObject);
    }
    {
      xiiDocumentObject* pObject = manager.CreateObject(pRttiEnumerations);
      XII_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 2);
      manager.DestroyObject(pObject);
    }
  }
}


XII_CREATE_SIMPLE_TEST(Reflection, ReflectedTypeReloading)
{
  xiiTestDocumentObjectManager manager;

  const xiiRTTI*             pRttiInner  = xiiRTTI::FindTypeByName("InnerStruct");
  const xiiRTTI*             pRttiInnerP = nullptr;
  xiiReflectedTypeDescriptor descInner;

  const xiiRTTI*             pRttiOuter  = xiiRTTI::FindTypeByName("OuterClass");
  const xiiRTTI*             pRttiOuterP = nullptr;
  xiiReflectedTypeDescriptor descOuter;

  xiiUInt32 uiRegisteredBaseTypes = GetTypeCount();
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RegisterType")
  {
    XII_TEST_BOOL(pRttiInner != nullptr);
    xiiToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiInner, descInner);
    descInner.m_sTypeName = "InnerStructP";
    pRttiInnerP           = xiiPhantomRttiManager::RegisterType(descInner);
    XII_TEST_BOOL(pRttiInnerP != nullptr);

    XII_TEST_BOOL(pRttiOuter != nullptr);
    xiiToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
    descOuter.m_sTypeName             = "OuterClassP";
    descOuter.m_Properties[0].m_sType = "InnerStructP";
    pRttiOuterP                       = xiiPhantomRttiManager::RegisterType(descOuter);
    XII_TEST_BOOL(pRttiOuterP != nullptr);
  }

  {
    xiiDocumentObject* pInnerObject = manager.CreateObject(pRttiInnerP);
    manager.AddObject(pInnerObject, nullptr, "Children", -1);
    xiiIReflectedTypeAccessor& innerAccessor = pInnerObject->GetTypeAccessor();

    xiiDocumentObject* pOuterObject = manager.CreateObject(pRttiOuterP);
    manager.AddObject(pOuterObject, nullptr, "Children", -1);
    xiiIReflectedTypeAccessor& outerAccessor = pOuterObject->GetTypeAccessor();

    xiiUuid                    innerGuid             = outerAccessor.GetValue("Inner").Get<xiiUuid>();
    xiiDocumentObject*         pEmbeddedInnerObject  = manager.GetObject(innerGuid);
    xiiIReflectedTypeAccessor& embeddedInnerAccessor = pEmbeddedInnerObject->GetTypeAccessor();

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetValues")
    {
      // Just set a few values to make sure they don't get messed up by the following operations.
      XII_TEST_BOOL(innerAccessor.SetValue("IP1", 1.4f));
      XII_TEST_BOOL(outerAccessor.SetValue("OP1", 0.9f));
      XII_TEST_BOOL(embeddedInnerAccessor.SetValue("IP1", 1.4f));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "AddProperty")
    {
      // Say we reload the engine and the InnerStruct now has a second property: IP2.
      descInner.m_Properties.PushBack(xiiReflectedPropertyDescriptor(xiiPropertyCategory::Member, "IP2", "xiiVec4",
                                                                     xiiBitflags<xiiPropertyFlags>(xiiPropertyFlags::StandardType), xiiArrayPtr<xiiPropertyAttribute* const>()));
      const xiiRTTI* NewInnerHandle = xiiPhantomRttiManager::RegisterType(descInner);
      XII_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Check that the new property is present.
      AccessorPropertyTest(innerAccessor, "IP2", xiiVariant::Type::Vector4);

      AccessorPropertyTest(embeddedInnerAccessor, "IP2", xiiVariant::Type::Vector4);

      // Test that the old properties are still valid.
      XII_TEST_BOOL(innerAccessor.GetValue("IP1") == 1.4f);
      XII_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);
      XII_TEST_BOOL(embeddedInnerAccessor.GetValue("IP1") == 1.4f);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "ChangeProperty")
    {
      // Out original inner float now is a Int32!
      descInner.m_Properties[0].m_sType = "xiiInt32";
      const xiiRTTI* NewInnerHandle     = xiiPhantomRttiManager::RegisterType(descInner);
      XII_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Test if the previous value was converted correctly to its new type.
      xiiVariant innerValue = innerAccessor.GetValue("IP1");
      XII_TEST_BOOL(innerValue.IsValid());
      XII_TEST_BOOL(innerValue.GetType() == xiiVariant::Type::Int32);
      XII_TEST_INT(innerValue.Get<xiiInt32>(), 1);

      xiiVariant outerValue = embeddedInnerAccessor.GetValue("IP1");
      XII_TEST_BOOL(outerValue.IsValid());
      XII_TEST_BOOL(outerValue.GetType() == xiiVariant::Type::Int32);
      XII_TEST_INT(outerValue.Get<xiiInt32>(), 1);

      // Test that the old properties are still valid.
      XII_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);

      AccessorPropertyTest(innerAccessor, "IP2", xiiVariant::Type::Vector4);
      AccessorPropertyTest(embeddedInnerAccessor, "IP2", xiiVariant::Type::Vector4);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "DeleteProperty")
    {
      // Lets now delete the original inner property IP1.
      descInner.m_Properties.RemoveAtAndCopy(0);
      const xiiRTTI* NewInnerHandle = xiiPhantomRttiManager::RegisterType(descInner);
      XII_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Check that IP1 is really gone.
      XII_TEST_BOOL(!innerAccessor.GetValue("IP1").IsValid());
      XII_TEST_BOOL(!embeddedInnerAccessor.GetValue("IP1").IsValid());

      // Test that the old properties are still valid.
      XII_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);

      AccessorPropertyTest(innerAccessor, "IP2", xiiVariant::Type::Vector4);
      AccessorPropertyTest(embeddedInnerAccessor, "IP2", xiiVariant::Type::Vector4);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "RevertProperties")
    {
      // Reset all classes to their initial state.
      xiiToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiInner, descInner);
      descInner.m_sTypeName = "InnerStructP";
      xiiPhantomRttiManager::RegisterType(descInner);

      xiiToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
      descInner.m_sTypeName             = "OuterStructP";
      descOuter.m_Properties[0].m_sType = "InnerStructP";
      xiiPhantomRttiManager::RegisterType(descOuter);

      // Test that the old properties are back again.
      xiiStringBuilder path       = "IP1";
      xiiVariant       innerValue = innerAccessor.GetValue(path);
      XII_TEST_BOOL(innerValue.IsValid());
      XII_TEST_BOOL(innerValue.GetType() == xiiVariant::Type::Float);
      XII_TEST_FLOAT(innerValue.Get<float>(), 1.0f, 0.0f);

      xiiVariant outerValue = embeddedInnerAccessor.GetValue("IP1");
      XII_TEST_BOOL(outerValue.IsValid());
      XII_TEST_BOOL(outerValue.GetType() == xiiVariant::Type::Float);
      XII_TEST_FLOAT(outerValue.Get<float>(), 1.0f, 0.0f);
      XII_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);
    }

    manager.RemoveObject(pInnerObject);
    manager.DestroyObject(pInnerObject);

    manager.RemoveObject(pOuterObject);
    manager.DestroyObject(pOuterObject);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "UnregisterType")
  {
    XII_TEST_INT(GetTypeCount(), uiRegisteredBaseTypes + 2);
    xiiPhantomRttiManager::UnregisterType(pRttiOuterP);
    xiiPhantomRttiManager::UnregisterType(pRttiInnerP);
    XII_TEST_INT(GetTypeCount(), uiRegisteredBaseTypes);
  }
}
