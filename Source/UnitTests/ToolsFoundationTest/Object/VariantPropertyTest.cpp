#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

static xiiHybridArray<xiiDocumentObjectPropertyEvent, 2> s_Changes;
void                                                     TestPropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  s_Changes.PushBack(e);
}

void TestArray(xiiVariantSubAccessor& ref_accessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiDelegate<xiiVariant()>& getNativeValue)
{
  auto VerifyChange = [&]() {
    // Any operation should collapse to the xiiVariant being set as a whole.
    XII_TEST_INT(s_Changes.GetCount(), 1);
    XII_TEST_BOOL(s_Changes[0].m_EventType == xiiDocumentObjectPropertyEvent::Type::PropertySet);
    XII_TEST_BOOL(s_Changes[0].m_pObject == pObject);
    XII_TEST_BOOL(s_Changes[0].m_sProperty == pProp->GetPropertyName());
    s_Changes.Clear();
  };

  s_Changes.Clear();
  ref_accessor.StartTransaction("Insert Element");
  xiiInt32   iCount = 0;
  xiiVariant value  = xiiColor(1, 2, 3);
  XII_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  XII_TEST_INT(iCount, 0);
  XII_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value, 0));
  XII_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  XII_TEST_INT(iCount, 1);
  XII_TEST_BOOL(getNativeValue()[0] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();
  s_Changes.Clear();

  xiiVariant outValue;
  XII_TEST_STATUS(ref_accessor.GetValue(pObject, pProp, outValue, 0));
  XII_TEST_BOOL(value == outValue);

  ref_accessor.StartTransaction("Set Element");
  value = xiiVariantDictionary();
  XII_TEST_STATUS(ref_accessor.SetValue(pObject, pProp, value, 0));
  XII_TEST_BOOL(getNativeValue()[0] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Insert Element");
  xiiVariant value2 = "Test";
  XII_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value2, 1));
  XII_TEST_BOOL(getNativeValue()[0] == value);
  XII_TEST_BOOL(getNativeValue()[1] == value2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Move Element");
  XII_TEST_STATUS(ref_accessor.MoveValue(pObject, pProp, 1, 0));
  XII_TEST_BOOL(getNativeValue()[0] == value2);
  XII_TEST_BOOL(getNativeValue()[1] == value);
  XII_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  XII_TEST_INT(iCount, 2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Remove Element");
  XII_TEST_STATUS(ref_accessor.RemoveValue(pObject, pProp, 0));
  XII_TEST_BOOL(getNativeValue()[0] == value);
  XII_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  XII_TEST_INT(iCount, 1);
  ref_accessor.FinishTransaction();
  VerifyChange();
}

void TestDictionary(xiiVariantSubAccessor& ref_accessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiDelegate<xiiVariant()>& getNativeValue)
{
  auto VerifyChange = [&]() {
    // Any operation should collapse to the xiiVariant being set as a whole.
    XII_TEST_INT(s_Changes.GetCount(), 1);
    XII_TEST_BOOL(s_Changes[0].m_EventType == xiiDocumentObjectPropertyEvent::Type::PropertySet);
    XII_TEST_BOOL(s_Changes[0].m_pObject == pObject);
    XII_TEST_BOOL(s_Changes[0].m_sProperty == pProp->GetPropertyName());
    s_Changes.Clear();
  };

  s_Changes.Clear();
  ref_accessor.StartTransaction("Insert Element");
  xiiInt32   iCount = 0;
  xiiVariant value  = xiiColor(1, 2, 3);
  XII_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  XII_TEST_INT(iCount, 0);
  XII_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value, "A"));
  XII_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  XII_TEST_INT(iCount, 1);
  XII_TEST_BOOL(getNativeValue()["A"] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();
  s_Changes.Clear();

  xiiVariant outValue;
  XII_TEST_STATUS(ref_accessor.GetValue(pObject, pProp, outValue, "A"));
  XII_TEST_BOOL(value == outValue);

  ref_accessor.StartTransaction("Set Element");
  value = 42u;
  XII_TEST_STATUS(ref_accessor.SetValue(pObject, pProp, value, "A"));
  XII_TEST_BOOL(getNativeValue()["A"] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Insert Element");
  xiiVariant value2 = xiiVariantArray();
  XII_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value2, "B"));
  XII_TEST_BOOL(getNativeValue()["A"] == value);
  XII_TEST_BOOL(getNativeValue()["B"] == value2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Remove Element");
  XII_TEST_STATUS(ref_accessor.RemoveValue(pObject, pProp, "A"));
  XII_TEST_BOOL(getNativeValue()["B"] == value2);
  XII_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  XII_TEST_INT(iCount, 1);
  ref_accessor.FinishTransaction();
  VerifyChange();
}

XII_CREATE_SIMPLE_TEST(DocumentObject, VariantPropertyTest)
{
  XII_SCOPE_EXIT(s_Changes.Clear(); s_Changes.Compact(););
  xiiTestDocument doc("Test", true);
  doc.InitializeAfterLoading(false);
  xiiObjectAccessorBase*      pAccessor = doc.GetObjectAccessor();
  const xiiDocumentObject*    pObject   = nullptr;
  const xiiVariantTestStruct* pNative   = nullptr;
  doc.GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&TestPropertyEventHandler));
  XII_SCOPE_EXIT(doc.GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&TestPropertyEventHandler)));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CreateObject")
  {
    xiiUuid objGuid;
    pAccessor->StartTransaction("Add Object");
    XII_TEST_STATUS(pAccessor->AddObject(nullptr, (const xiiAbstractProperty*)nullptr, -1, xiiGetStaticRTTI<xiiVariantTestStruct>(), objGuid));
    pAccessor->FinishTransaction();
    pObject = pAccessor->GetObject(objGuid);
    pNative = static_cast<xiiVariantTestStruct*>(doc.m_ObjectMirror.GetNativeObjectPointer(pObject));
  }

  const xiiAbstractProperty* pProp      = pObject->GetType()->FindPropertyByName("Variant");
  const xiiAbstractProperty* pPropArray = pObject->GetType()->FindPropertyByName("VariantArray");
  const xiiAbstractProperty* pPropDict  = pObject->GetType()->FindPropertyByName("VariantDictionary");

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TestVariant")
  {
    pAccessor->StartTransaction("Set as Array");
    XII_TEST_STATUS(pAccessor->SetValue(pObject, pProp, xiiVariantArray()));
    pAccessor->FinishTransaction();
    s_Changes.Clear();

    xiiVariantSubAccessor                        accessor(pAccessor, pProp);
    xiiMap<const xiiDocumentObject*, xiiVariant> subItemMap;
    subItemMap.Insert(pObject, xiiVariant());
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pProp, [&]() { return pNative->m_Variant; });
    // What remains is an xiiVariantDictionary at index 0 that we can recurse into.
    {
      xiiVariantSubAccessor                        accessor2(&accessor, pProp);
      xiiMap<const xiiDocumentObject*, xiiVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pProp, [&]() { return pNative->m_Variant[0]; });
    }
    pAccessor->StartTransaction("Set as Dict");
    XII_TEST_STATUS(pAccessor->SetValue(pObject, pProp, xiiVariantDictionary()));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    TestDictionary(accessor, pObject, pProp, [&]() { return pNative->m_Variant; });
    // What remains is an xiiVariantArray at index "B" that we can recurse into.
    {
      xiiVariantSubAccessor                        accessor2(&accessor, pProp);
      xiiMap<const xiiDocumentObject*, xiiVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pProp, [&]() { return pNative->m_Variant["B"]; });
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TestVariantArray")
  {
    pAccessor->StartTransaction("Insert Array");
    XII_TEST_STATUS(pAccessor->InsertValue(pObject, pPropArray, xiiVariantArray(), 0));
    pAccessor->FinishTransaction();

    xiiVariantSubAccessor                        accessor(pAccessor, pPropArray);
    xiiMap<const xiiDocumentObject*, xiiVariant> subItemMap;
    subItemMap.Insert(pObject, 0);
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pPropArray, [&]() { return pNative->m_VariantArray[0]; });
    // What remains is an xiiVariantDictionary at index 0 that we can recurse into.
    {
      xiiVariantSubAccessor                        accessor2(&accessor, pPropArray);
      xiiMap<const xiiDocumentObject*, xiiVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pPropArray, [&]() { return pNative->m_VariantArray[0][0]; });
    }
    pAccessor->StartTransaction("Insert Dictionary");
    XII_TEST_STATUS(pAccessor->InsertValue(pObject, pPropArray, xiiVariantDictionary(), 1));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    subItemMap.Insert(pObject, 1);
    accessor.SetSubItems(subItemMap);
    TestDictionary(accessor, pObject, pPropArray, [&]() { return pNative->m_VariantArray[1]; });
    // What remains is an xiiVariantArray at index "B" that we can recurse into.
    {
      xiiVariantSubAccessor                        accessor2(&accessor, pPropArray);
      xiiMap<const xiiDocumentObject*, xiiVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pPropArray, [&]() { return pNative->m_VariantArray[1]["B"]; });
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TestVariantDictionary")
  {
    pAccessor->StartTransaction("Insert Array");
    XII_TEST_STATUS(pAccessor->InsertValue(pObject, pPropDict, xiiVariantArray(), "AAA"));
    pAccessor->FinishTransaction();

    xiiVariantSubAccessor                        accessor(pAccessor, pPropDict);
    xiiMap<const xiiDocumentObject*, xiiVariant> subItemMap;
    subItemMap.Insert(pObject, "AAA");
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pPropDict, [&]() { return *pNative->m_VariantDictionary.GetValue("AAA"); });
    // What remains is an xiiVariantDictionary at index 0 that we can recurse into.
    {
      xiiVariantSubAccessor                        accessor2(&accessor, pPropDict);
      xiiMap<const xiiDocumentObject*, xiiVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pPropDict, [&]() { return (*pNative->m_VariantDictionary.GetValue("AAA"))[0]; });
    }
    pAccessor->StartTransaction("Insert Dictionary");
    XII_TEST_STATUS(pAccessor->InsertValue(pObject, pPropDict, xiiVariantDictionary(), "BBB"));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    subItemMap.Insert(pObject, "BBB");
    accessor.SetSubItems(subItemMap);
    TestDictionary(accessor, pObject, pPropDict, [&]() { return *pNative->m_VariantDictionary.GetValue("BBB"); });
    // What remains is an xiiVariantArray at index "B" that we can recurse into.
    {
      xiiVariantSubAccessor                        accessor2(&accessor, pPropDict);
      xiiMap<const xiiDocumentObject*, xiiVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pPropDict, [&]() { return (*pNative->m_VariantDictionary.GetValue("BBB"))["B"]; });
    }
  }
}
