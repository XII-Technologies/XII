/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>

namespace
{
  using TestComponentBaseManager = xiiComponentManagerSimple<class TestComponentBase, xiiComponentUpdateType::Always>;

  class TestComponentBase : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(TestComponentBase, xiiComponent, TestComponentBaseManager);

  public:
    void Update() { ++s_iUpdateCounter; }

    static int s_iUpdateCounter;
  };

  int TestComponentBase::s_iUpdateCounter = 0;

  XII_BEGIN_COMPONENT_TYPE(TestComponentBase, 1, xiiComponentMode::Static)
  XII_END_COMPONENT_TYPE

  //////////////////////////////////////////////////////////////////////////

  using TestComponentDerived1Manager = xiiComponentManagerSimple<class TestComponentDerived1, xiiComponentUpdateType::Always>;

  class TestComponentDerived1 : public TestComponentBase
  {
    XII_DECLARE_COMPONENT_TYPE(TestComponentDerived1, TestComponentBase, TestComponentDerived1Manager);

  public:
    void Update() { ++s_iUpdateCounter; }

    static int s_iUpdateCounter;
  };

  int TestComponentDerived1::s_iUpdateCounter = 0;

  XII_BEGIN_COMPONENT_TYPE(TestComponentDerived1, 1, xiiComponentMode::Static)
  XII_END_COMPONENT_TYPE
} // namespace


XII_CREATE_SIMPLE_TEST(World, DerivedComponents)
{
  xiiWorldDesc worldDesc("Test");
  xiiWorld     world(worldDesc);
  XII_LOCK(world.GetWriteMarker());

  TestComponentBaseManager*     pManagerBase     = world.GetOrCreateComponentManager<TestComponentBaseManager>();
  TestComponentDerived1Manager* pManagerDerived1 = world.GetOrCreateComponentManager<TestComponentDerived1Manager>();

  xiiGameObjectDesc   desc;
  xiiGameObject*      pObject;
  xiiGameObjectHandle hObject = world.CreateObject(desc, pObject);
  XII_TEST_BOOL(!hObject.IsInvalidated());

  xiiGameObject* pObject2;
  world.CreateObject(desc, pObject2);

  TestComponentBase::s_iUpdateCounter     = 0;
  TestComponentDerived1::s_iUpdateCounter = 0;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Derived Component Update")
  {
    TestComponentBase* pComponentBase = nullptr;
    xiiComponentHandle hComponentBase = TestComponentBase::CreateComponent(pObject, pComponentBase);

    TestComponentBase* pTestBase = nullptr;
    XII_TEST_BOOL(world.TryGetComponent(hComponentBase, pTestBase));
    XII_TEST_BOOL(pTestBase == pComponentBase);
    XII_TEST_BOOL(pComponentBase->GetHandle() == hComponentBase);
    XII_TEST_BOOL(pComponentBase->GetOwningManager() == pManagerBase);

    TestComponentDerived1* pComponentDerived1 = nullptr;
    xiiComponentHandle     hComponentDerived1 = TestComponentDerived1::CreateComponent(pObject2, pComponentDerived1);

    TestComponentDerived1* pTestDerived1 = nullptr;
    XII_TEST_BOOL(world.TryGetComponent(hComponentDerived1, pTestDerived1));
    XII_TEST_BOOL(pTestDerived1 == pComponentDerived1);
    XII_TEST_BOOL(pComponentDerived1->GetHandle() == hComponentDerived1);
    XII_TEST_BOOL(pComponentDerived1->GetOwningManager() == pManagerDerived1);

    world.Update();

    XII_TEST_INT(TestComponentBase::s_iUpdateCounter, 1);
    XII_TEST_INT(TestComponentDerived1::s_iUpdateCounter, 1);

    // Get component manager via rtti
    XII_TEST_BOOL(world.GetManagerForComponentType(xiiGetStaticRTTI<TestComponentBase>()) == pManagerBase);
    XII_TEST_BOOL(world.GetManagerForComponentType(xiiGetStaticRTTI<TestComponentDerived1>()) == pManagerDerived1);
  }
}
