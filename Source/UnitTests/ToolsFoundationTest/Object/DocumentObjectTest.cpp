#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

XII_CREATE_SIMPLE_TEST_GROUP(DocumentObject);

XII_CREATE_SIMPLE_TEST(DocumentObject, DocumentObjectManager)
{
  xiiTestDocumentObjectManager manager;
  xiiDocumentObject*           pObject              = nullptr;
  xiiDocumentObject*           pChildObject         = nullptr;
  xiiDocumentObject*           pChildren[4]         = {nullptr};
  xiiDocumentObject*           pSubElementObject[4] = {nullptr};

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DocumentObject")
  {
    XII_TEST_BOOL(manager.CanAdd(xiiObjectTest::GetStaticRTTI(), nullptr, "", 0).Succeeded());
    pObject = manager.CreateObject(xiiObjectTest::GetStaticRTTI());
    manager.AddObject(pObject, nullptr, "", 0);

    const char* szProperty = "SubObjectSet";
    XII_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, szProperty, 0).Failed());
    XII_TEST_BOOL(manager.CanAdd(xiiObjectTest::GetStaticRTTI(), pObject, szProperty, 0).Succeeded());
    pChildObject = manager.CreateObject(xiiObjectTest::GetStaticRTTI());
    manager.AddObject(pChildObject, pObject, "SubObjectSet", 0);
    XII_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 1);

    XII_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0).Succeeded());
    XII_TEST_BOOL(manager.CanAdd(ExtendedOuterClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0).Succeeded());
    XII_TEST_BOOL(!manager.CanAdd(xiiReflectedClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0).Succeeded());

    for (xiiInt32 i = 0; i < XII_ARRAY_SIZE(pChildren); i++)
    {
      XII_TEST_BOOL(manager.CanAdd(xiiObjectTest::GetStaticRTTI(), pChildObject, szProperty, i).Succeeded());
      pChildren[i] = manager.CreateObject(xiiObjectTest::GetStaticRTTI());
      manager.AddObject(pChildren[i], pChildObject, szProperty, i);
      XII_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), i + 1);
    }
    XII_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), 4);

    XII_TEST_BOOL_MSG(manager.CanMove(pObject, pChildObject, szProperty, 0).Failed(), "Can't move to own child");
    XII_TEST_BOOL_MSG(manager.CanMove(pChildren[1], pChildObject, szProperty, 1).Failed(), "Can't move before onself");
    XII_TEST_BOOL_MSG(manager.CanMove(pChildren[1], pChildObject, szProperty, 2).Failed(), "Can't move after oneself");
    XII_TEST_BOOL_MSG(manager.CanMove(pChildren[1], pChildren[1], szProperty, 0).Failed(), "Can't move into yourself");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DocumentSubElementObject")
  {
    const char* szProperty = "ClassArray";
    for (xiiInt32 i = 0; i < XII_ARRAY_SIZE(pSubElementObject); i++)
    {
      XII_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, szProperty, i).Succeeded());
      pSubElementObject[i] = manager.CreateObject(OuterClass::GetStaticRTTI());
      manager.AddObject(pSubElementObject[i], pObject, szProperty, i);
      XII_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), i + 1);
    }

    XII_TEST_BOOL(manager.CanRemove(pSubElementObject[0]).Succeeded());
    manager.RemoveObject(pSubElementObject[0]);
    manager.DestroyObject(pSubElementObject[0]);
    pSubElementObject[0] = nullptr;
    XII_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 3);

    xiiVariant value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    XII_TEST_BOOL(value.IsA<xiiUuid>() && value.Get<xiiUuid>() == pSubElementObject[1]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    XII_TEST_BOOL(value.IsA<xiiUuid>() && value.Get<xiiUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 2);
    XII_TEST_BOOL(value.IsA<xiiUuid>() && value.Get<xiiUuid>() == pSubElementObject[3]->GetGuid());

    XII_TEST_BOOL(manager.CanMove(pSubElementObject[1], pObject, szProperty, 2).Succeeded());
    manager.MoveObject(pSubElementObject[1], pObject, szProperty, 2);
    XII_TEST_BOOL(manager.CanMove(pSubElementObject[3], pObject, szProperty, 0).Succeeded());
    manager.MoveObject(pSubElementObject[3], pObject, szProperty, 0);

    value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    XII_TEST_BOOL(value.IsA<xiiUuid>() && value.Get<xiiUuid>() == pSubElementObject[3]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    XII_TEST_BOOL(value.IsA<xiiUuid>() && value.Get<xiiUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 2);
    XII_TEST_BOOL(value.IsA<xiiUuid>() && value.Get<xiiUuid>() == pSubElementObject[1]->GetGuid());

    XII_TEST_BOOL(manager.CanRemove(pSubElementObject[3]).Succeeded());
    manager.RemoveObject(pSubElementObject[3]);
    manager.DestroyObject(pSubElementObject[3]);
    pSubElementObject[3] = nullptr;
    XII_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 2);

    value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    XII_TEST_BOOL(value.IsA<xiiUuid>() && value.Get<xiiUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    XII_TEST_BOOL(value.IsA<xiiUuid>() && value.Get<xiiUuid>() == pSubElementObject[1]->GetGuid());

    XII_TEST_BOOL(manager.CanMove(pSubElementObject[1], pChildObject, szProperty, 0).Succeeded());
    manager.MoveObject(pSubElementObject[1], pChildObject, szProperty, 0);
    XII_TEST_BOOL(manager.CanMove(pSubElementObject[2], pChildObject, szProperty, 0).Succeeded());
    manager.MoveObject(pSubElementObject[2], pChildObject, szProperty, 0);

    XII_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 0);
    XII_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), 2);

    value = pChildObject->GetTypeAccessor().GetValue(szProperty, 0);
    XII_TEST_BOOL(value.IsA<xiiUuid>() && value.Get<xiiUuid>() == pSubElementObject[2]->GetGuid());
    value = pChildObject->GetTypeAccessor().GetValue(szProperty, 1);
    XII_TEST_BOOL(value.IsA<xiiUuid>() && value.Get<xiiUuid>() == pSubElementObject[1]->GetGuid());
  }

  manager.DestroyAllObjects();
}
