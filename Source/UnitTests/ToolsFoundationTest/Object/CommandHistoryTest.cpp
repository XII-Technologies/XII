#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

XII_CREATE_SIMPLE_TEST(DocumentObject, CommandHistory)
{
  xiiTestDocument doc("Test", true);
  doc.InitializeAfterLoading(false);
  xiiObjectAccessorBase* pAccessor = doc.GetObjectAccessor();

  auto CreateObject = [&doc, &pAccessor](const xiiRTTI* pType) -> const xiiDocumentObject* {
    xiiUuid objGuid;
    pAccessor->StartTransaction("Add Object");
    XII_TEST_STATUS(pAccessor->AddObject(nullptr, (const xiiAbstractProperty*)nullptr, -1, pType, objGuid));
    pAccessor->FinishTransaction();
    return pAccessor->GetObject(objGuid);
  };

  auto StoreOriginalState = [&doc](xiiAbstractObjectGraph& ref_graph, const xiiDocumentObject* pRoot) {
    xiiDocumentObjectConverterWriter writer(&ref_graph, doc.GetObjectManager(), [](const xiiDocumentObject*, const xiiAbstractProperty* p) { return p->GetAttributeByType<xiiHiddenAttribute>() == nullptr; });
    xiiAbstractObjectNode*           pAbstractObj = writer.AddObjectToGraph(pRoot);
  };

  auto CompareAgainstOriginalState = [&doc](xiiAbstractObjectGraph& ref_original, const xiiDocumentObject* pRoot) {
    xiiAbstractObjectGraph           graph;
    xiiDocumentObjectConverterWriter writer2(&graph, doc.GetObjectManager(), [](const xiiDocumentObject*, const xiiAbstractProperty* p) { return p->GetAttributeByType<xiiHiddenAttribute>() == nullptr; });
    xiiAbstractObjectNode*           pAbstractObj2 = writer2.AddObjectToGraph(pRoot);

    xiiDeque<xiiAbstractGraphDiffOperation> diff;
    graph.CreateDiffWithBaseGraph(ref_original, diff);
    XII_TEST_BOOL(diff.GetCount() == 0);
  };

  const xiiDocumentObject* pRoot = CreateObject(xiiGetStaticRTTI<xiiMirrorTest>());

  xiiUuid mathGuid   = pAccessor->Get<xiiUuid>(pRoot, "Math");
  xiiUuid objectGuid = pAccessor->Get<xiiUuid>(pRoot, "Object");

  const xiiDocumentObject* pMath       = pAccessor->GetObject(mathGuid);
  const xiiDocumentObject* pObjectTest = pAccessor->GetObject(objectGuid);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetValue")
  {

    XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), 1);

    auto TestSetValue = [&](const xiiDocumentObject* pObject, const char* szProperty, xiiVariant value) {
      xiiAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      xiiUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();

      pAccessor->StartTransaction("SetValue");
      XII_TEST_STATUS(pAccessor->SetValue(pObject, szProperty, value));
      pAccessor->FinishTransaction();
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      xiiVariant newValue;
      XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue));
      XII_TEST_BOOL(newValue == value);

      XII_TEST_STATUS(doc.GetCommandHistory()->Undo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      CompareAgainstOriginalState(graph, pObject);

      XII_TEST_STATUS(doc.GetCommandHistory()->Redo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue));
      XII_TEST_BOOL(newValue == value);
    };

    // Math
    TestSetValue(pMath, "Vec2", xiiVec2(1, 2));
    TestSetValue(pMath, "Vec3", xiiVec3(1, 2, 3));
    TestSetValue(pMath, "Vec4", xiiVec4(1, 2, 3, 4));
    TestSetValue(pMath, "Vec2I", xiiVec2I32(1, 2));
    TestSetValue(pMath, "Vec3I", xiiVec3I32(1, 2, 3));
    TestSetValue(pMath, "Vec4I", xiiVec4I32(1, 2, 3, 4));
    xiiQuat qValue = xiiQuat::MakeFromEulerAngles(xiiAngle::MakeFromDegree(30), xiiAngle::MakeFromDegree(30), xiiAngle::MakeFromDegree(30));
    TestSetValue(pMath, "Quat", qValue);
    xiiMat3 mValue;
    mValue = xiiMat3::MakeRotationX(xiiAngle::MakeFromDegree(30));
    TestSetValue(pMath, "Mat3", mValue);
    xiiMat4 mValue2;
    mValue2.SetIdentity();
    mValue2 = xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(30));
    TestSetValue(pMath, "Mat4", mValue2);

    // Integer
    const xiiDocumentObject* pInteger = CreateObject(xiiGetStaticRTTI<xiiIntegerStruct>());
    TestSetValue(pInteger, "Int8", xiiInt8(-5));
    TestSetValue(pInteger, "UInt8", xiiUInt8(5));
    TestSetValue(pInteger, "Int16", xiiInt16(-5));
    TestSetValue(pInteger, "UInt16", xiiUInt16(5));
    TestSetValue(pInteger, "Int32", xiiInt32(-5));
    TestSetValue(pInteger, "UInt32", xiiUInt32(5));
    TestSetValue(pInteger, "Int64", xiiInt64(-5));
    TestSetValue(pInteger, "UInt64", xiiUInt64(5));

    // Test automatic type conversions
    TestSetValue(pInteger, "Int8", xiiInt16(-5));
    TestSetValue(pInteger, "Int8", xiiInt32(-5));
    TestSetValue(pInteger, "Int8", xiiInt64(-5));
    TestSetValue(pInteger, "Int8", float(-5));
    TestSetValue(pInteger, "Int8", xiiUInt8(5));

    TestSetValue(pInteger, "Int64", xiiInt32(-5));
    TestSetValue(pInteger, "Int64", xiiInt16(-5));
    TestSetValue(pInteger, "Int64", xiiInt8(-5));
    TestSetValue(pInteger, "Int64", float(-5));
    TestSetValue(pInteger, "Int64", xiiUInt8(5));

    TestSetValue(pInteger, "UInt64", xiiUInt32(5));
    TestSetValue(pInteger, "UInt64", xiiUInt16(5));
    TestSetValue(pInteger, "UInt64", xiiUInt8(5));
    TestSetValue(pInteger, "UInt64", float(5));
    TestSetValue(pInteger, "UInt64", xiiInt8(5));

    // Float
    const xiiDocumentObject* pFloat = CreateObject(xiiGetStaticRTTI<xiiFloatStruct>());
    TestSetValue(pFloat, "Float", -5.0f);
    TestSetValue(pFloat, "Double", -5.0);
    TestSetValue(pFloat, "Time", xiiTime::MakeFromMinutes(3.0f));
    TestSetValue(pFloat, "Angle", xiiAngle::MakeFromDegree(45.0f));

    TestSetValue(pFloat, "Float", 5.0);
    TestSetValue(pFloat, "Float", xiiInt8(-5));
    TestSetValue(pFloat, "Float", xiiUInt8(5));

    // Misc PODs
    const xiiDocumentObject* pPOD = CreateObject(xiiGetStaticRTTI<xiiPODClass>());
    TestSetValue(pPOD, "Bool", true);
    TestSetValue(pPOD, "Bool", false);
    TestSetValue(pPOD, "Color", xiiColor(1.0f, 2.0f, 3.0f, 4.0f));
    TestSetValue(pPOD, "ColorUB", xiiColorGammaUB(200, 100, 255));
    TestSetValue(pPOD, "String", "Test");
    xiiVarianceTypeAngle customFloat;
    customFloat.m_Value     = xiiAngle::MakeFromDegree(45.0f);
    customFloat.m_fVariance = 1.0f;
    TestSetValue(pPOD, "VarianceAngle", customFloat);

    // Enumerations
    const xiiDocumentObject* pEnum = CreateObject(xiiGetStaticRTTI<xiiEnumerationsClass>());
    TestSetValue(pEnum, "Enum", (xiiInt8)xiiExampleEnum::Value2);
    TestSetValue(pEnum, "Enum", (xiiInt64)xiiExampleEnum::Value2);
    TestSetValue(pEnum, "Bitflags", (xiiUInt8)xiiExampleBitflags::Value2);
    TestSetValue(pEnum, "Bitflags", (xiiInt64)xiiExampleBitflags::Value2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "InsertValue")
  {
    auto TestInsertValue = [&](const xiiDocumentObject* pObject, const char* szProperty, xiiVariant value, xiiVariant index) {
      xiiAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const xiiUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const xiiInt32  iArraySize        = pAccessor->GetCount(pObject, szProperty);

      pAccessor->StartTransaction("InsertValue");
      XII_TEST_STATUS(pAccessor->InsertValue(pObject, szProperty, value, index));
      pAccessor->FinishTransaction();
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize + 1);
      xiiVariant newValue;
      XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue, index));
      XII_TEST_BOOL(newValue == value);

      XII_TEST_STATUS(doc.GetCommandHistory()->Undo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      XII_TEST_STATUS(doc.GetCommandHistory()->Redo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize + 1);
      XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue, index));
      XII_TEST_BOOL(newValue == value);
    };

    TestInsertValue(pObjectTest, "StandardTypeArray", double(0), 0);
    TestInsertValue(pObjectTest, "StandardTypeArray", double(2), 1);
    TestInsertValue(pObjectTest, "StandardTypeArray", double(1), 1);

    TestInsertValue(pObjectTest, "StandardTypeSet", "A", 0);
    TestInsertValue(pObjectTest, "StandardTypeSet", "C", 1);
    TestInsertValue(pObjectTest, "StandardTypeSet", "B", 1);

    TestInsertValue(pObjectTest, "StandardTypeMap", double(0), "A");
    TestInsertValue(pObjectTest, "StandardTypeMap", double(2), "C");
    TestInsertValue(pObjectTest, "StandardTypeMap", double(1), "B");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MoveValue")
  {
    auto TestMoveValue = [&](const xiiDocumentObject* pObject, const char* szProperty, xiiVariant oldIndex, xiiVariant newIndex, xiiArrayPtr<xiiVariant> expectedOutcome) {
      xiiAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const xiiUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const xiiInt32  iArraySize        = pAccessor->GetCount(pObject, szProperty);
      XII_TEST_INT(iArraySize, expectedOutcome.GetCount());

      xiiDynamicArray<xiiVariant> values;
      XII_TEST_STATUS(pAccessor->GetValues(pObject, szProperty, values));
      XII_TEST_INT(iArraySize, values.GetCount());

      pAccessor->StartTransaction("MoveValue");
      XII_TEST_STATUS(pAccessor->MoveValue(pObject, szProperty, oldIndex, newIndex));
      pAccessor->FinishTransaction();
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize);

      for (xiiInt32 i = 0; i < iArraySize; i++)
      {
        xiiVariant newValue;
        XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue, i));
        XII_TEST_BOOL(newValue == expectedOutcome[i]);
      }

      XII_TEST_STATUS(doc.GetCommandHistory()->Undo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      XII_TEST_STATUS(doc.GetCommandHistory()->Redo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize);

      for (xiiInt32 i = 0; i < iArraySize; i++)
      {
        xiiVariant newValue;
        XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue, i));
        XII_TEST_BOOL(newValue == expectedOutcome[i]);
      }
    };

    {
      xiiVariant expectedValues[3] = {0, 1, 2};
      // Move first element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 0, xiiArrayPtr<xiiVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 1, xiiArrayPtr<xiiVariant>(expectedValues));
      // Move last element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 2, xiiArrayPtr<xiiVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 3, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      // Move first element to the end.
      xiiVariant expectedValues[3] = {1, 2, 0};
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 3, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      // Move last element to the front.
      xiiVariant expectedValues[3] = {0, 1, 2};
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 0, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      // Move first element to the middle
      xiiVariant expectedValues[3] = {1, 0, 2};
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 2, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      // Move last element to the middle
      xiiVariant expectedValues[3] = {1, 2, 0};
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 1, xiiArrayPtr<xiiVariant>(expectedValues));
    }

    {
      xiiVariant expectedValues[3] = {"A", "B", "C"};
      // Move first element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 0, xiiArrayPtr<xiiVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 1, xiiArrayPtr<xiiVariant>(expectedValues));
      // Move last element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 2, xiiArrayPtr<xiiVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 3, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      // Move first element to the end.
      xiiVariant expectedValues[3] = {"B", "C", "A"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 3, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      // Move last element to the front.
      xiiVariant expectedValues[3] = {"A", "B", "C"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 0, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      // Move first element to the middle
      xiiVariant expectedValues[3] = {"B", "A", "C"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 2, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      // Move last element to the middle
      xiiVariant expectedValues[3] = {"B", "C", "A"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 1, xiiArrayPtr<xiiVariant>(expectedValues));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveValue")
  {
    auto TestRemoveValue = [&](const xiiDocumentObject* pObject, const char* szProperty, xiiVariant index, xiiArrayPtr<xiiVariant> expectedOutcome) {
      xiiAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const xiiUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const xiiInt32  iArraySize        = pAccessor->GetCount(pObject, szProperty);
      XII_TEST_INT(iArraySize - 1, expectedOutcome.GetCount());

      xiiDynamicArray<xiiVariant> values;
      xiiDynamicArray<xiiVariant> keys;
      {
        XII_TEST_STATUS(pAccessor->GetValues(pObject, szProperty, values));
        XII_TEST_INT(iArraySize, values.GetCount());

        XII_TEST_STATUS(pAccessor->GetKeys(pObject, szProperty, keys));
        XII_TEST_INT(iArraySize, keys.GetCount());
        xiiUInt32 uiIndex = keys.IndexOf(index);
        keys.RemoveAtAndSwap(uiIndex);
        values.RemoveAtAndSwap(uiIndex);
        XII_TEST_INT(iArraySize - 1, keys.GetCount());
      }

      pAccessor->StartTransaction("RemoveValue");
      XII_TEST_STATUS(pAccessor->RemoveValue(pObject, szProperty, index));
      pAccessor->FinishTransaction();
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize - 1);

      if (pObject->GetType()->FindPropertyByName(szProperty)->GetCategory() == xiiPropertyCategory::Map)
      {
        for (xiiInt32 i = 0; i < iArraySize - 1; i++)
        {
          const xiiVariant& key   = keys[i];
          const xiiVariant& value = values[i];
          xiiVariant        newValue;
          XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue, key));
          XII_TEST_BOOL(newValue == value);
          XII_TEST_BOOL(expectedOutcome.Contains(newValue));
        }
      }
      else
      {
        for (xiiInt32 i = 0; i < iArraySize - 1; i++)
        {
          xiiVariant newValue;
          XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue, i));
          XII_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }

      XII_TEST_STATUS(doc.GetCommandHistory()->Undo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      XII_TEST_STATUS(doc.GetCommandHistory()->Redo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize - 1);

      if (pObject->GetType()->FindPropertyByName(szProperty)->GetCategory() == xiiPropertyCategory::Map)
      {
        for (xiiInt32 i = 0; i < iArraySize - 1; i++)
        {
          const xiiVariant& key   = keys[i];
          const xiiVariant& value = values[i];
          xiiVariant        newValue;
          XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue, key));
          XII_TEST_BOOL(newValue == value);
          XII_TEST_BOOL(expectedOutcome.Contains(newValue));
        }
      }
      else
      {
        for (xiiInt32 i = 0; i < iArraySize - 1; i++)
        {
          xiiVariant newValue;
          XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue, i));
          XII_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }
    };

    // StandardTypeArray
    {
      xiiVariant expectedValues[2] = {2, 0};
      TestRemoveValue(pObjectTest, "StandardTypeArray", 0, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      xiiVariant expectedValues[1] = {2};
      TestRemoveValue(pObjectTest, "StandardTypeArray", 1, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      TestRemoveValue(pObjectTest, "StandardTypeArray", 0, xiiArrayPtr<xiiVariant>());
    }
    // StandardTypeSet
    {
      xiiVariant expectedValues[2] = {"B", "C"};
      TestRemoveValue(pObjectTest, "StandardTypeSet", 2, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      xiiVariant expectedValues[1] = {"C"};
      TestRemoveValue(pObjectTest, "StandardTypeSet", 0, xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      TestRemoveValue(pObjectTest, "StandardTypeSet", 0, xiiArrayPtr<xiiVariant>());
    }
    // StandardTypeMap
    {
      xiiVariant expectedValues[2] = {1, 2};
      TestRemoveValue(pObjectTest, "StandardTypeMap", "A", xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      xiiVariant expectedValues[1] = {1};
      TestRemoveValue(pObjectTest, "StandardTypeMap", "C", xiiArrayPtr<xiiVariant>(expectedValues));
    }
    {
      TestRemoveValue(pObjectTest, "StandardTypeMap", "B", xiiArrayPtr<xiiVariant>());
    }
  }

  auto CreateGuid = [](const char* szType, xiiInt32 iIndex) -> xiiUuid {
    xiiUuid A = xiiUuid::MakeStableUuidFromString(szType);
    xiiUuid B = xiiUuid::MakeStableUuidFromInt(iIndex);
    A.CombineWithSeed(B);
    return A;
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "AddObject")
  {
    auto TestAddObject = [&](const xiiDocumentObject* pObject, const char* szProperty, xiiVariant index, const xiiRTTI* pType, xiiUuid& inout_object) {
      xiiAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const xiiUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const xiiInt32  iArraySize        = pAccessor->GetCount(pObject, szProperty);

      pAccessor->StartTransaction("TestAddObject");
      XII_TEST_STATUS(pAccessor->AddObject(pObject, szProperty, index, pType, inout_object));
      pAccessor->FinishTransaction();
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize + 1);
      xiiVariant newValue;
      XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue, index));
      XII_TEST_BOOL(newValue == inout_object);

      XII_TEST_STATUS(doc.GetCommandHistory()->Undo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      XII_TEST_STATUS(doc.GetCommandHistory()->Redo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pObject, szProperty), iArraySize + 1);
      XII_TEST_STATUS(pAccessor->GetValue(pObject, szProperty, newValue, index));
      XII_TEST_BOOL(newValue == inout_object);
    };

    xiiUuid A = CreateGuid("ClassArray", 0);
    xiiUuid B = CreateGuid("ClassArray", 1);
    xiiUuid C = CreateGuid("ClassArray", 2);

    TestAddObject(pObjectTest, "ClassArray", 0, xiiGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassArray", 1, xiiGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassArray", 1, xiiGetStaticRTTI<OuterClass>(), B);

    A = CreateGuid("ClassPtrArray", 0);
    B = CreateGuid("ClassPtrArray", 1);
    C = CreateGuid("ClassPtrArray", 2);

    TestAddObject(pObjectTest, "ClassPtrArray", 0, xiiGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassPtrArray", 1, xiiGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassPtrArray", 1, xiiGetStaticRTTI<OuterClass>(), B);

    A = CreateGuid("SubObjectSet", 0);
    B = CreateGuid("SubObjectSet", 1);
    C = CreateGuid("SubObjectSet", 2);

    TestAddObject(pObjectTest, "SubObjectSet", 0, xiiGetStaticRTTI<xiiObjectTest>(), A);
    TestAddObject(pObjectTest, "SubObjectSet", 1, xiiGetStaticRTTI<xiiObjectTest>(), C);
    TestAddObject(pObjectTest, "SubObjectSet", 1, xiiGetStaticRTTI<xiiObjectTest>(), B);

    A = CreateGuid("ClassMap", 0);
    B = CreateGuid("ClassMap", 1);
    C = CreateGuid("ClassMap", 2);

    TestAddObject(pObjectTest, "ClassMap", "A", xiiGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassMap", "C", xiiGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassMap", "B", xiiGetStaticRTTI<OuterClass>(), B);

    A = CreateGuid("ClassPtrMap", 0);
    B = CreateGuid("ClassPtrMap", 1);
    C = CreateGuid("ClassPtrMap", 2);

    TestAddObject(pObjectTest, "ClassPtrMap", "A", xiiGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassPtrMap", "C", xiiGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassPtrMap", "B", xiiGetStaticRTTI<OuterClass>(), B);
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MoveObject")
  {
    auto TestMoveObjectFailure = [&](const xiiDocumentObject* pObject, const char* szProperty, xiiVariant newIndex) {
      pAccessor->StartTransaction("MoveObject");
      XII_TEST_BOOL(pAccessor->MoveObject(pObject, pObject->GetParent(), szProperty, newIndex).Failed());
      pAccessor->CancelTransaction();
    };

    auto TestMoveObject = [&](const xiiDocumentObject* pObject, const char* szProperty, xiiVariant newIndex, xiiArrayPtr<xiiUuid> expectedOutcome) {
      xiiAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject->GetParent());

      const xiiUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const xiiInt32  iArraySize        = pAccessor->GetCount(pObject->GetParent(), szProperty);
      XII_TEST_INT(iArraySize, expectedOutcome.GetCount());

      xiiDynamicArray<xiiVariant> values;
      XII_TEST_STATUS(pAccessor->GetValues(pObject->GetParent(), szProperty, values));
      XII_TEST_INT(iArraySize, values.GetCount());

      pAccessor->StartTransaction("MoveObject");
      XII_TEST_STATUS(pAccessor->MoveObject(pObject, pObject->GetParent(), szProperty, newIndex));
      pAccessor->FinishTransaction();
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pObject->GetParent(), szProperty), iArraySize);

      for (xiiInt32 i = 0; i < iArraySize; i++)
      {
        xiiVariant newValue;
        XII_TEST_STATUS(pAccessor->GetValue(pObject->GetParent(), szProperty, newValue, i));
        XII_TEST_BOOL(newValue == expectedOutcome[i]);
      }

      XII_TEST_STATUS(doc.GetCommandHistory()->Undo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      XII_TEST_INT(pAccessor->GetCount(pObject->GetParent(), szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject->GetParent());

      XII_TEST_STATUS(doc.GetCommandHistory()->Redo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pObject->GetParent(), szProperty), iArraySize);

      for (xiiInt32 i = 0; i < iArraySize; i++)
      {
        xiiVariant newValue;
        XII_TEST_STATUS(pAccessor->GetValue(pObject->GetParent(), szProperty, newValue, i));
        XII_TEST_BOOL(newValue == expectedOutcome[i]);
      }
    };

    xiiUuid                  A  = CreateGuid("ClassArray", 0);
    xiiUuid                  B  = CreateGuid("ClassArray", 1);
    xiiUuid                  C  = CreateGuid("ClassArray", 2);
    const xiiDocumentObject* pA = pAccessor->GetObject(A);
    const xiiDocumentObject* pB = pAccessor->GetObject(B);
    const xiiDocumentObject* pC = pAccessor->GetObject(C);

    {
      // Move first element before or after itself (no-op)
      TestMoveObjectFailure(pA, "ClassArray", 0);
      TestMoveObjectFailure(pA, "ClassArray", 1);
      // Move last element before or after itself (no-op)
      TestMoveObjectFailure(pC, "ClassArray", 2);
      TestMoveObjectFailure(pC, "ClassArray", 3);
    }
    {
      // Move first element to the end.
      xiiUuid expectedValues[3] = {B, C, A};
      TestMoveObject(pA, "ClassArray", 3, xiiArrayPtr<xiiUuid>(expectedValues));
    }
    {
      // Move last element to the front.
      xiiUuid expectedValues[3] = {A, B, C};
      TestMoveObject(pA, "ClassArray", 0, xiiArrayPtr<xiiUuid>(expectedValues));
    }
    {
      // Move first element to the middle
      xiiUuid expectedValues[3] = {B, A, C};
      TestMoveObject(pA, "ClassArray", 2, xiiArrayPtr<xiiUuid>(expectedValues));
    }
    {
      // Move last element to the middle
      xiiUuid expectedValues[3] = {B, C, A};
      TestMoveObject(pC, "ClassArray", 1, xiiArrayPtr<xiiUuid>(expectedValues));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveObject")
  {
    auto TestRemoveObject = [&](const xiiDocumentObject* pObject, xiiArrayPtr<xiiUuid> expectedOutcome) {
      auto      pParent   = pObject->GetParent();
      xiiString sProperty = pObject->GetParentProperty();

      xiiAbstractObjectGraph graph;
      StoreOriginalState(graph, pParent);
      const xiiUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const xiiInt32  iArraySize        = pAccessor->GetCount(pParent, sProperty);
      XII_TEST_INT(iArraySize - 1, expectedOutcome.GetCount());


      xiiDynamicArray<xiiVariant> values;
      xiiDynamicArray<xiiVariant> keys;
      {
        XII_TEST_STATUS(pAccessor->GetValues(pParent, sProperty, values));
        XII_TEST_INT(iArraySize, values.GetCount());

        XII_TEST_STATUS(pAccessor->GetKeys(pParent, sProperty, keys));
        XII_TEST_INT(iArraySize, keys.GetCount());
        xiiUInt32 uiIndex = keys.IndexOf(pObject->GetPropertyIndex());
        keys.RemoveAtAndSwap(uiIndex);
        values.RemoveAtAndSwap(uiIndex);
        XII_TEST_INT(iArraySize - 1, keys.GetCount());
      }

      pAccessor->StartTransaction("RemoveValue");
      XII_TEST_STATUS(pAccessor->RemoveObject(pObject));
      pAccessor->FinishTransaction();
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pParent, sProperty), iArraySize - 1);

      if (pParent->GetType()->FindPropertyByName(sProperty)->GetCategory() == xiiPropertyCategory::Map)
      {
        for (xiiInt32 i = 0; i < iArraySize - 1; i++)
        {
          const xiiVariant& key   = keys[i];
          const xiiVariant& value = values[i];
          xiiVariant        newValue;
          XII_TEST_STATUS(pAccessor->GetValue(pParent, sProperty, newValue, key));
          XII_TEST_BOOL(newValue == value);
          XII_TEST_BOOL(expectedOutcome.Contains(newValue.Get<xiiUuid>()));
        }
      }
      else
      {
        for (xiiInt32 i = 0; i < iArraySize - 1; i++)
        {
          xiiVariant newValue;
          XII_TEST_STATUS(pAccessor->GetValue(pParent, sProperty, newValue, i));
          XII_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }

      XII_TEST_STATUS(doc.GetCommandHistory()->Undo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      XII_TEST_INT(pAccessor->GetCount(pParent, sProperty), iArraySize);
      CompareAgainstOriginalState(graph, pParent);

      XII_TEST_STATUS(doc.GetCommandHistory()->Redo());
      XII_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      XII_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      XII_TEST_INT(pAccessor->GetCount(pParent, sProperty), iArraySize - 1);

      if (pParent->GetType()->FindPropertyByName(sProperty)->GetCategory() == xiiPropertyCategory::Map)
      {
        for (xiiInt32 i = 0; i < iArraySize - 1; i++)
        {
          const xiiVariant& key   = keys[i];
          const xiiVariant& value = values[i];
          xiiVariant        newValue;
          XII_TEST_STATUS(pAccessor->GetValue(pParent, sProperty, newValue, key));
          XII_TEST_BOOL(newValue == value);
          XII_TEST_BOOL(expectedOutcome.Contains(newValue.Get<xiiUuid>()));
        }
      }
      else
      {
        for (xiiInt32 i = 0; i < iArraySize - 1; i++)
        {
          xiiVariant newValue;
          XII_TEST_STATUS(pAccessor->GetValue(pParent, sProperty, newValue, i));
          XII_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }
    };

    auto ClearContainer = [&](const char* szContainer) {
      xiiUuid                  A  = CreateGuid(szContainer, 0);
      xiiUuid                  B  = CreateGuid(szContainer, 1);
      xiiUuid                  C  = CreateGuid(szContainer, 2);
      const xiiDocumentObject* pA = pAccessor->GetObject(A);
      const xiiDocumentObject* pB = pAccessor->GetObject(B);
      const xiiDocumentObject* pC = pAccessor->GetObject(C);
      {
        xiiUuid expectedValues[2] = {B, C};
        TestRemoveObject(pA, xiiArrayPtr<xiiUuid>(expectedValues));
      }
      {
        xiiUuid expectedValues[1] = {C};
        TestRemoveObject(pB, xiiArrayPtr<xiiUuid>(expectedValues));
      }
      {
        TestRemoveObject(pC, xiiArrayPtr<xiiUuid>());
      }
    };

    ClearContainer("ClassArray");
    ClearContainer("ClassPtrArray");
    ClearContainer("SubObjectSet");
    ClearContainer("ClassMap");
    ClearContainer("ClassPtrMap");
  }
}
