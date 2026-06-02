/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>

namespace
{
  class TestComponent;
  class TestComponentManager : public xiiComponentManager<TestComponent, xiiBlockStorageType::FreeList>
  {
  public:
    TestComponentManager(xiiWorld* pWorld) :
      xiiComponentManager<TestComponent, xiiBlockStorageType::FreeList>(pWorld)
    {
    }

    virtual void Initialize() override
    {
      auto desc         = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update, this);
      auto desc2        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update2, this);
      auto desc3        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update3, this);
      desc3.m_fPriority = 1000.0f;

      auto desc4        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::AUpdate3, this);
      desc4.m_fPriority = 1000.0f;

      desc.m_DependsOn.PushBack(xiiMakeHashedString("TestComponentManager::Update2")); // update2 will be called before update
      desc.m_DependsOn.PushBack(xiiMakeHashedString("TestComponentManager::Update3")); // update3 will be called before update

      auto descAsync                    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::UpdateAsync, this);
      descAsync.m_Phase                 = xiiWorldUpdatePhase::Async;
      descAsync.m_uiAsyncPhaseBatchSize = 20;

      // Update functions are now registered in reverse order, so we can test whether dependencies work.
      this->RegisterUpdateFunction(descAsync);
      this->RegisterUpdateFunction(desc4);
      this->RegisterUpdateFunction(desc3);
      this->RegisterUpdateFunction(desc2);
      this->RegisterUpdateFunction(desc);
    }

    void Update(const xiiWorldModule::UpdateContext& context);
    void Update2(const xiiWorldModule::UpdateContext& context);
    void Update3(const xiiWorldModule::UpdateContext& context);
    void AUpdate3(const xiiWorldModule::UpdateContext& context);
    void UpdateAsync(const xiiWorldModule::UpdateContext& context);
  };

  class TestComponent : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(TestComponent, xiiComponent, TestComponentManager);

  public:
    TestComponent()  = default;
    ~TestComponent() = default;

    virtual void Initialize() override { ++s_iInitCounter; }

    virtual void Deinitialize() override { --s_iInitCounter; }

    virtual void OnActivated() override
    {
      ++s_iActivateCounter;

      SpawnOther();
    }

    virtual void OnDeactivated() override { --s_iActivateCounter; }

    virtual void OnSimulationStarted() override { ++s_iSimulationStartedCounter; }

    void Update() { m_iSomeData *= 5; }

    void Update2() { m_iSomeData += 3; }

    void SpawnOther();

    xiiInt32 m_iSomeData = 1;

    static xiiInt32 s_iInitCounter;
    static xiiInt32 s_iActivateCounter;
    static xiiInt32 s_iSimulationStartedCounter;

    static bool s_bSpawnOther;
  };

  xiiInt32 TestComponent::s_iInitCounter              = 0;
  xiiInt32 TestComponent::s_iActivateCounter          = 0;
  xiiInt32 TestComponent::s_iSimulationStartedCounter = 0;
  bool     TestComponent::s_bSpawnOther               = false;

  XII_BEGIN_COMPONENT_TYPE(TestComponent, 1, xiiComponentMode::Static)
  XII_END_COMPONENT_TYPE

  void TestComponentManager::Update(const xiiWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  void TestComponentManager::Update2(const xiiWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update2();
    }
  }

  void TestComponentManager::Update3(const xiiWorldModule::UpdateContext& context) {}

  void TestComponentManager::AUpdate3(const xiiWorldModule::UpdateContext& context) {}

  void TestComponentManager::UpdateAsync(const xiiWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  using TestComponent2Manager = xiiComponentManager<class TestComponent2, xiiBlockStorageType::FreeList>;

  class TestComponent2 : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(TestComponent2, xiiComponent, TestComponent2Manager);

    virtual void OnActivated() override { TestComponent::s_iActivateCounter++; }
  };

  XII_BEGIN_COMPONENT_TYPE(TestComponent2, 1, xiiComponentMode::Static)
  XII_END_COMPONENT_TYPE

  void TestComponent::SpawnOther()
  {
    if (s_bSpawnOther)
    {
      xiiGameObjectDesc desc;
      desc.m_hParent = GetOwner()->GetHandle();

      xiiGameObject* pChild = nullptr;
      GetWorld()->CreateObject(desc, pChild);

      TestComponent2* pChildComponent = nullptr;
      TestComponent2::CreateComponent(pChild, pChildComponent);
    }
  }
} // namespace


XII_CREATE_SIMPLE_TEST(World, Components)
{
  xiiWorldDescription worldDesc("Test");
  xiiWorld            world(worldDesc);
  XII_LOCK(world.GetWriteMarker());

  TestComponentManager* pManager = world.GetOrCreateComponentManager<TestComponentManager>();

  xiiGameObject* pTestObject1;
  xiiGameObject* pTestObject2;

  {
    xiiGameObjectDesc   desc;
    xiiGameObjectHandle hObject = world.CreateObject(desc, pTestObject1);
    XII_TEST_BOOL(!hObject.IsInvalidated());
    world.CreateObject(desc, pTestObject2);
  }

  TestComponent* pTestComponent = nullptr;

  TestComponent::s_iInitCounter              = 0;
  TestComponent::s_iActivateCounter          = 0;
  TestComponent::s_iSimulationStartedCounter = 0;
  TestComponent::s_bSpawnOther               = false;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Component Init")
  {
    // test recursive write lock
    XII_LOCK(world.GetWriteMarker());

    xiiComponentHandle handle;
    XII_TEST_BOOL(!world.TryGetComponent(handle, pTestComponent));

    // Update with no components created
    world.Update();

    handle = TestComponent::CreateComponent(pTestObject1, pTestComponent);

    TestComponent* pTest = nullptr;
    XII_TEST_BOOL(world.TryGetComponent(handle, pTest));
    XII_TEST_BOOL(pTest == pTestComponent);
    XII_TEST_BOOL(pTestComponent->GetHandle() == handle);

    TestComponent2* pTest2 = nullptr;
    XII_TEST_BOOL(!world.TryGetComponent(handle, pTest2));

    XII_TEST_INT(pTestComponent->m_iSomeData, 1);
    XII_TEST_INT(TestComponent::s_iInitCounter, 0);

    for (xiiUInt32 i = 1; i < 100; ++i)
    {
      pManager->CreateComponent(pTestObject2, pTestComponent);
      pTestComponent->m_iSomeData = i + 1;
    }

    XII_TEST_INT(pManager->GetComponentCount(), 100);
    XII_TEST_INT(TestComponent::s_iInitCounter, 0);

    // Update with components created
    world.Update();

    XII_TEST_INT(pManager->GetComponentCount(), 100);
    XII_TEST_INT(TestComponent::s_iInitCounter, 100);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Component Update")
  {
    // test recursive read lock
    XII_LOCK(world.GetReadMarker());

    world.Update();

    xiiUInt32 uiCounter = 0;
    for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
    {
      XII_TEST_INT(it->m_iSomeData, (((uiCounter + 4) * 25) + 3) * 25);
      ++uiCounter;
    }

    XII_TEST_INT(uiCounter, 100);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Delete Component")
  {
    pManager->DeleteComponent(pTestComponent->GetHandle());
    XII_TEST_INT(pManager->GetComponentCount(), 99);
    XII_TEST_INT(TestComponent::s_iInitCounter, 99);

    // component should also be removed from the game object
    XII_TEST_INT(pTestObject2->GetComponents().GetCount(), 98);

    world.DeleteObjectNow(pTestObject2->GetHandle());
    world.Update();

    XII_TEST_INT(TestComponent::s_iInitCounter, 1);

    world.DeleteComponentManager<TestComponentManager>();
    pManager = nullptr;
    XII_TEST_INT(TestComponent::s_iInitCounter, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Delete Objects with Component")
  {
    xiiGameObjectDesc desc;

    xiiGameObject* pObjectA = nullptr;
    xiiGameObject* pObjectB = nullptr;
    xiiGameObject* pObjectC = nullptr;

    desc.m_sName.Assign("A");
    xiiGameObjectHandle hObjectA = world.CreateObject(desc, pObjectA);
    desc.m_sName.Assign("B");
    xiiGameObjectHandle hObjectB = world.CreateObject(desc, pObjectB);
    desc.m_sName.Assign("C");
    xiiGameObjectHandle hObjectC = world.CreateObject(desc, pObjectC);

    XII_TEST_BOOL(!hObjectA.IsInvalidated());
    XII_TEST_BOOL(!hObjectB.IsInvalidated());
    XII_TEST_BOOL(!hObjectC.IsInvalidated());

    TestComponent* pComponentA = nullptr;
    TestComponent* pComponentB = nullptr;
    TestComponent* pComponentC = nullptr;

    xiiComponentHandle hComponentA = TestComponent::CreateComponent(pObjectA, pComponentA);
    xiiComponentHandle hComponentB = TestComponent::CreateComponent(pObjectB, pComponentB);
    xiiComponentHandle hComponentC = TestComponent::CreateComponent(pObjectC, pComponentC);

    XII_TEST_BOOL(!hComponentA.IsInvalidated());
    XII_TEST_BOOL(!hComponentB.IsInvalidated());
    XII_TEST_BOOL(!hComponentC.IsInvalidated());

    world.DeleteObjectNow(pObjectB->GetHandle());

    XII_TEST_BOOL(pObjectA->IsActive());
    XII_TEST_BOOL(pComponentA->IsActive());
    XII_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    XII_TEST_BOOL(!pObjectB->IsActive());
    XII_TEST_BOOL(!pComponentB->IsActive());
    XII_TEST_BOOL(pComponentB->GetOwner() == nullptr);

    XII_TEST_BOOL(pObjectC->IsActive());
    XII_TEST_BOOL(pComponentC->IsActive());
    XII_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    world.Update();

    XII_TEST_BOOL(world.TryGetObject(hObjectA, pObjectA));
    XII_TEST_BOOL(world.TryGetObject(hObjectC, pObjectC));

    // Since we're not recompacting storage for components, pointer should still be valid.
    // XII_TEST_BOOL(world.TryGetComponent(hComponentA, pComponentA));
    // XII_TEST_BOOL(world.TryGetComponent(hComponentC, pComponentC));

    XII_TEST_BOOL(pObjectA->IsActive());
    XII_TEST_BOOL(pObjectA->GetName() == "A");
    XII_TEST_BOOL(pComponentA->IsActive());
    XII_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    XII_TEST_BOOL(pObjectC->IsActive());
    XII_TEST_BOOL(pObjectC->GetName() == "C");
    XII_TEST_BOOL(pComponentC->IsActive());
    XII_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    // creating a new component should reuse memory from component B
    TestComponent*     pComponentB2 = nullptr;
    xiiComponentHandle hComponentB2 = TestComponent::CreateComponent(pObjectB, pComponentB2);
    XII_TEST_BOOL(!hComponentB2.IsInvalidated());
    XII_TEST_BOOL(pComponentB2 == pComponentB);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Get Components")
  {
    const xiiWorld& constWorld = world;

    const TestComponentManager* pConstManager = constWorld.GetComponentManager<TestComponentManager>();

    for (auto it = pConstManager->GetComponents(); it.IsValid(); it.Next())
    {
      xiiComponentHandle hComponent = it->GetHandle();

      const TestComponent* pConstComponent = nullptr;
      XII_TEST_BOOL(constWorld.TryGetComponent(hComponent, pConstComponent));
      XII_TEST_BOOL(pConstComponent == (const TestComponent*)it);

      XII_TEST_BOOL(pConstManager->TryGetComponent(hComponent, pConstComponent));
      XII_TEST_BOOL(pConstComponent == (const TestComponent*)it);
    }

    world.DeleteComponentManager<TestComponentManager>();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Component Callbacks")
  {
    xiiGameObjectDesc desc;
    xiiGameObject*    pObject = nullptr;
    world.CreateObject(desc, pObject);

    // Simulation stopped, component active
    {
      world.SetWorldSimulationEnabled(false);
      TestComponent::s_iInitCounter              = 0;
      TestComponent::s_iActivateCounter          = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);

      XII_TEST_INT(TestComponent::s_iInitCounter, 0);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 0);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 1);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.SetWorldSimulationEnabled(true);
      world.Update();

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 1);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->SetActiveFlag(false);

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 0);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->SetActiveFlag(true);
      world.Update();

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 1);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 2);

      pComponent->DeleteComponent();
    }

    // Simulation stopped, component inactive
    {
      world.SetWorldSimulationEnabled(false);
      TestComponent::s_iInitCounter              = 0;
      TestComponent::s_iActivateCounter          = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);
      pComponent->SetActiveFlag(false);

      XII_TEST_INT(TestComponent::s_iInitCounter, 0);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 0);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 0);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 1);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(false);

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 0);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.SetWorldSimulationEnabled(true);
      world.Update();

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 0);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 1);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->DeleteComponent();
    }

    // Simulation started, component active
    {
      world.SetWorldSimulationEnabled(true);
      TestComponent::s_iInitCounter              = 0;
      TestComponent::s_iActivateCounter          = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);

      XII_TEST_INT(TestComponent::s_iInitCounter, 0);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 0);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 1);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->DeleteComponent();
    }

    // Simulation started, component inactive
    {
      world.SetWorldSimulationEnabled(true);
      TestComponent::s_iInitCounter              = 0;
      TestComponent::s_iActivateCounter          = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);
      pComponent->SetActiveFlag(false);

      XII_TEST_INT(TestComponent::s_iInitCounter, 0);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 0);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 0);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      XII_TEST_INT(TestComponent::s_iInitCounter, 1);
      XII_TEST_INT(TestComponent::s_iActivateCounter, 1);
      XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->DeleteComponent();
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Component dependent initialization")
  {
    xiiGameObjectDesc desc;
    xiiGameObject*    pObject = nullptr;
    world.CreateObject(desc, pObject);

    world.SetWorldSimulationEnabled(true);

    TestComponent::s_iInitCounter              = 0;
    TestComponent::s_iActivateCounter          = 0;
    TestComponent::s_iSimulationStartedCounter = 0;
    TestComponent::s_bSpawnOther               = true;

    TestComponent* pComponent = nullptr;
    TestComponent::CreateComponent(pObject, pComponent);

    world.Update();

    XII_TEST_INT(TestComponent::s_iInitCounter, 1);
    XII_TEST_INT(TestComponent::s_iActivateCounter, 2);
    XII_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);
  }
}
