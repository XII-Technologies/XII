#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

void MirrorCheck(xiiTestDocument* pDoc, const xiiDocumentObject* pObject)
{
  // Create native object graph
  xiiAbstractObjectGraph graph;
  xiiAbstractObjectNode* pRootNode = nullptr;
  {
    xiiRttiConverterWriter rttiConverter(&graph, &pDoc->m_Context, true, true);
    pRootNode = rttiConverter.AddObjectToGraph(pObject->GetType(), pDoc->m_ObjectMirror.GetNativeObjectPointer(pObject), "Object");
  }

  // Create object manager graph
  xiiAbstractObjectGraph origGraph;
  xiiAbstractObjectNode* pOrigRootNode = nullptr;
  {
    xiiDocumentObjectConverterWriter writer(&origGraph, pDoc->GetObjectManager());
    pOrigRootNode = writer.AddObjectToGraph(pObject);
  }

  // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
  graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
  xiiDeque<xiiAbstractGraphDiffOperation> diffResult;

  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  XII_TEST_BOOL(diffResult.GetCount() == 0);
}


xiiVariant GetVariantFromType(xiiVariant::Type::Enum type)
{
  switch (type)
  {
    case xiiVariant::Type::Invalid:
      return xiiVariant();
    case xiiVariant::Type::Bool:
      return xiiVariant(true);
    case xiiVariant::Type::Int8:
      return xiiVariant((xiiInt8)-55);
    case xiiVariant::Type::UInt8:
      return xiiVariant((xiiUInt8)44);
    case xiiVariant::Type::Int16:
      return xiiVariant((xiiInt16)-444);
    case xiiVariant::Type::UInt16:
      return xiiVariant((xiiUInt16)666);
    case xiiVariant::Type::Int32:
      return xiiVariant((xiiInt32)-88880);
    case xiiVariant::Type::UInt32:
      return xiiVariant((xiiUInt32)123445);
    case xiiVariant::Type::Int64:
      return xiiVariant((xiiInt64)-888800000);
    case xiiVariant::Type::UInt64:
      return xiiVariant((xiiUInt64)123445000);
    case xiiVariant::Type::Float:
      return xiiVariant(1024.0f);
    case xiiVariant::Type::Double:
      return xiiVariant(-2048.0f);
    case xiiVariant::Type::Color:
      return xiiVariant(xiiColor(0.5f, 33.0f, 2.0f, 0.3f));
    case xiiVariant::Type::ColorGamma:
      return xiiVariant(xiiColorGammaUB(xiiColor(0.5f, 33.0f, 2.0f, 0.3f)));
    case xiiVariant::Type::Vector2:
      return xiiVariant(xiiVec2(2.0f, 4.0f));
    case xiiVariant::Type::Vector2d:
      return xiiVariant(xiiVec2d(2.0, 4.0));
    case xiiVariant::Type::Vector3:
      return xiiVariant(xiiVec3(2.0f, 4.0f, -8.0f));
    case xiiVariant::Type::Vector3d:
      return xiiVariant(xiiVec3d(2.0, 4.0, -8.0));
    case xiiVariant::Type::Vector4:
      return xiiVariant(xiiVec4(1.0f, 7.0f, 8.0f, -10.0f));
    case xiiVariant::Type::Vector4d:
      return xiiVariant(xiiVec4d(1.0, 7.0, 8.0, -10.0));
    case xiiVariant::Type::Vector2I:
      return xiiVariant(xiiVec2I32(1, 2));
    case xiiVariant::Type::Vector2I64:
      return xiiVariant(xiiVec2I64(1, 2));
    case xiiVariant::Type::Vector3I:
      return xiiVariant(xiiVec3I32(3, 4, 5));
    case xiiVariant::Type::Vector3I64:
      return xiiVariant(xiiVec3I64(3, 4, 5));
    case xiiVariant::Type::Vector4I:
      return xiiVariant(xiiVec4I32(6, 7, 8, 9));
    case xiiVariant::Type::Vector4I64:
      return xiiVariant(xiiVec4I64(6, 7, 8, 9));
    case xiiVariant::Type::Vector2U:
      return xiiVariant(xiiVec2U32(1, 2));
    case xiiVariant::Type::Vector2U64:
      return xiiVariant(xiiVec2U64(1, 2));
    case xiiVariant::Type::Vector3U:
      return xiiVariant(xiiVec3U32(3, 4, 5));
    case xiiVariant::Type::Vector3U64:
      return xiiVariant(xiiVec3U64(3, 4, 5));
    case xiiVariant::Type::Vector4U:
      return xiiVariant(xiiVec4U32(6, 7, 8, 9));
    case xiiVariant::Type::Vector4U64:
      return xiiVariant(xiiVec4U64(6, 7, 8, 9));
    case xiiVariant::Type::Quaternion:
    {
      xiiQuat quat;
      quat.SetFromEulerAngles(xiiAngle::MakeFromDegree(30), xiiAngle::MakeFromDegree(-15), xiiAngle::MakeFromDegree(20));
      return xiiVariant(quat);
    }
    case xiiVariant::Type::Quaterniond:
    {
      xiiQuatd quat;
      quat.SetFromEulerAngles(xiiAngled::MakeFromDegree(30), xiiAngled::MakeFromDegree(-15), xiiAngled::MakeFromDegree(20));
      return xiiVariant(quat);
    }
    case xiiVariant::Type::Matrix3:
    {
      xiiMat3 mat = xiiMat3::MakeIdentity();

      mat.SetRotationMatrix(xiiVec3(1.0f, 0.0f, 0.0f), xiiAngle::MakeFromDegree(30));
      return xiiVariant(mat);
    }
    case xiiVariant::Type::Matrix3d:
    {
      xiiMat3d mat = xiiMat3d::IdentityMatrix();

      mat.SetRotationMatrix(xiiVec3d(1.0, 0.0, 0.0), xiiAngled::MakeFromDegree(30));
      return xiiVariant(mat);
    }
    case xiiVariant::Type::Matrix4:
    {
      xiiMat4 mat = xiiMat4::MakeIdentity();

      mat.SetRotationMatrix(xiiVec3(0.0f, 1.0f, 0.0f), xiiAngle::MakeFromDegree(30));
      mat.SetTranslationVector(xiiVec3(1.0f, 2.0f, 3.0f));
      return xiiVariant(mat);
    }
    case xiiVariant::Type::Matrix4d:
    {
      xiiMat4d mat = xiiMat4d::IdentityMatrix();

      mat.SetRotationMatrix(xiiVec3d(0.0, 1.0, 0.0), xiiAngled::MakeFromDegree(30));
      mat.SetTranslationVector(xiiVec3d(1.0, 2.0, 3.0));
      return xiiVariant(mat);
    }
    case xiiVariant::Type::String:
      return xiiVariant("Test");
    case xiiVariant::Type::StringView:
      return xiiVariant(xiiStringView("Test"), false);
    case xiiVariant::Type::Time:
      return xiiVariant(xiiTime::MakeFromSeconds(123.0f));
    case xiiVariant::Type::Uuid:
    {
      xiiUuid guid;
      guid.CreateNewUuid();
      return xiiVariant(guid);
    }
    case xiiVariant::Type::Angle:
      return xiiVariant(xiiAngle::MakeFromDegree(30.0f));
    case xiiVariant::Type::Angled:
      return xiiVariant(xiiAngled::MakeFromDegree(30.0));
    case xiiVariant::Type::DataBuffer:
    {
      xiiDataBuffer data;
      data.PushBack(12);
      data.PushBack(55);
      data.PushBack(88);
      return xiiVariant(data);
    }
    case xiiVariant::Type::VariantArray:
      return xiiVariantArray();
    case xiiVariant::Type::VariantDictionary:
      return xiiVariantDictionary();
    case xiiVariant::Type::TypedPointer:
      return xiiVariant(xiiTypedPointer(nullptr, nullptr));
    case xiiVariant::Type::TypedObject:
      XII_ASSERT_NOT_IMPLEMENTED;

    default:
      XII_REPORT_FAILURE("Invalid case statement");
      return xiiVariant();
  }
  return xiiVariant();
}

void RecursiveModifyProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiObjectAccessorBase* pObjectAccessor)
{
  if (pProp->GetCategory() == xiiPropertyCategory::Member)
  {
    if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
    {
      if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
      {
        const xiiUuid oldGuid = pObjectAccessor->Get<xiiUuid>(pObject, pProp);
        xiiUuid       newGuid = xiiUuid::MakeUuid();
        if (oldGuid.IsValid())
        {
          XII_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(oldGuid)).m_Result.Succeeded());
        }

        XII_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, xiiVariant(), pProp->GetSpecificType(), newGuid).m_Result.Succeeded());

        const xiiDocumentObject* pChild = pObject->GetChild(newGuid);
        XII_ASSERT_DEV(pChild != nullptr, "References child object does not exist!");
      }
      else
      {
        xiiVariant value = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
        XII_TEST_BOOL(pObjectAccessor->SetValue(pObject, pProp, value).m_Result.Succeeded());
      }
    }
    else
    {
      if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags | xiiPropertyFlags::StandardType))
      {
        xiiVariant value = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
        XII_TEST_BOOL(pObjectAccessor->SetValue(pObject, pProp, value).m_Result.Succeeded());
      }
      else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
      {
        // Noting to do here, value cannot change
      }
    }
  }
  else if (pProp->GetCategory() == xiiPropertyCategory::Array || pProp->GetCategory() == xiiPropertyCategory::Set)
  {
    if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::StandardType | xiiPropertyFlags::Pointer) &&
        !pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
    {
      xiiInt32 iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
      for (xiiInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        pObjectAccessor->RemoveValue(pObject, pProp, i).AssertSuccess();
      }

      xiiVariant value1 = xiiReflectionUtils::GetDefaultValue(pProp, 0);
      xiiVariant value2 = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
      XII_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value1, 0).m_Result.Succeeded());
      XII_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value2, 1).m_Result.Succeeded());
    }
    else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
    {
      xiiInt32                       iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      xiiHybridArray<xiiVariant, 16> currentValues;
      pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
      for (xiiInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        XII_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<xiiUuid>())).m_Result.Succeeded());
      }

      if (pProp->GetCategory() == xiiPropertyCategory::Array)
      {
        xiiUuid newGuid = xiiUuid::MakeUuid();
        XII_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, 0, pProp->GetSpecificType(), newGuid).m_Result.Succeeded());
      }
    }
  }
  else if (pProp->GetCategory() == xiiPropertyCategory::Map)
  {
    if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::StandardType | xiiPropertyFlags::Pointer) &&
        !pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
    {
      xiiInt32                       iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
      xiiHybridArray<xiiVariant, 16> keys;
      pObjectAccessor->GetKeys(pObject, pProp, keys).AssertSuccess();
      for (const xiiVariant& key : keys)
      {
        pObjectAccessor->RemoveValue(pObject, pProp, key).AssertSuccess();
      }

      xiiVariant value1 = xiiReflectionUtils::GetDefaultValue(pProp, "Dummy");
      xiiVariant value2 = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
      XII_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value1, "value1").m_Result.Succeeded());
      XII_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value2, "value2").m_Result.Succeeded());
    }
    else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
    {
      xiiInt32                       iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      xiiHybridArray<xiiVariant, 16> currentValues;
      pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
      for (xiiInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        XII_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<xiiUuid>())).m_Result.Succeeded());
      }

      xiiUuid newGuid = xiiUuid::MakeUuid();
      XII_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, "value1", pProp->GetSpecificType(), newGuid).m_Result.Succeeded());
    }
  }
}

void RecursiveModifyObject(const xiiDocumentObject* pObject, xiiObjectAccessorBase* pAccessor)
{
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(properties);
  for (auto pProp : properties)
  {
    RecursiveModifyProperty(pObject, pProp, pAccessor);
  }

  for (const xiiDocumentObject* pSubObject : pObject->GetChildren())
  {
    RecursiveModifyObject(pSubObject, pAccessor);
  }
}

XII_CREATE_SIMPLE_TEST(DocumentObject, ObjectMirror)
{
  xiiTestDocument doc("Test", true);
  doc.InitializeAfterLoading(false);
  xiiObjectAccessorBase* pAccessor = doc.GetObjectAccessor();
  xiiUuid                mirrorGuid;

  pAccessor->StartTransaction("Init");
  xiiStatus                status  = pAccessor->AddObject(nullptr, (const xiiAbstractProperty*)nullptr, -1, xiiGetStaticRTTI<xiiMirrorTest>(), mirrorGuid);
  const xiiDocumentObject* pObject = pAccessor->GetObject(mirrorGuid);
  XII_TEST_BOOL(status.m_Result.Succeeded());
  pAccessor->FinishTransaction();

  MirrorCheck(&doc, pObject);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Document Changes")
  {
    pAccessor->StartTransaction("Document Changes");
    RecursiveModifyObject(pObject, pAccessor);
    pAccessor->FinishTransaction();

    MirrorCheck(&doc, pObject);
  }
  {
    pAccessor->StartTransaction("Document Changes");
    RecursiveModifyObject(pObject, pAccessor);
    pAccessor->FinishTransaction();

    MirrorCheck(&doc, pObject);
  }
}
