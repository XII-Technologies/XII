#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>

namespace
{
  class xiiTestComponentManager;

  class xiiTestComponent : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(xiiTestComponent, xiiComponent, xiiTestComponentManager);
  };

  class xiiTestComponentManager : public xiiComponentManager<class xiiTestComponent, xiiBlockStorageType::FreeList>
  {
  public:
    xiiTestComponentManager(xiiWorld* pWorld) :
      xiiComponentManager<xiiTestComponent, xiiBlockStorageType::FreeList>(pWorld)
    {
      m_qRotation.SetIdentity();
    }

    virtual void Initialize() override
    {
      auto desc                        = xiiWorldModule::UpdateFunctionDesc(xiiWorldModule::UpdateFunction(&xiiTestComponentManager::Update, this), "Update");
      desc.m_bOnlyUpdateWhenSimulating = false;

      RegisterUpdateFunction(desc);
    }

    void Update(const xiiWorldModule::UpdateContext& context)
    {
      xiiQuat qRot;
      qRot.SetFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(2.0f));

      m_qRotation = qRot * m_qRotation;

      for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
      {
        ComponentType* pComponent = it;
        if (pComponent->IsActiveAndInitialized())
        {
          auto pOwner = pComponent->GetOwner();
          pOwner->SetLocalRotation(m_qRotation);
        }
      }
    }

    xiiQuat m_qRotation;
  };

  // clang-format off
  XII_BEGIN_COMPONENT_TYPE(xiiTestComponent, 1, xiiComponentMode::Dynamic);
  XII_END_COMPONENT_TYPE;
  // clang-format on

  void AddObjectsToWorld(xiiWorld& ref_world, bool bDynamic, xiiUInt32 uiNumObjects, xiiUInt32 uiTreeLevelNumNodeDiv, xiiUInt32 uiTreeDepth, xiiInt32 iAttachCompsDepth, xiiGameObjectHandle hParent = xiiGameObjectHandle())
  {
    if (uiTreeDepth == 0)
      return;

    xiiGameObjectDesc gd;
    gd.m_bDynamic = bDynamic;
    gd.m_hParent  = hParent;

    float posX = 0.0f;
    float posY = uiTreeDepth * 5.0f;

    xiiTestComponentManager* pMan = ref_world.GetOrCreateComponentManager<xiiTestComponentManager>();

    for (xiiUInt32 i = 0; i < uiNumObjects; ++i)
    {
      gd.m_LocalPosition.Set(posX, posY, 0);
      posX += 5.0f;

      xiiGameObject* pObj;
      auto           hObj = ref_world.CreateObject(gd, pObj);

      if (iAttachCompsDepth > 0)
      {
        xiiTestComponent* comp;
        pMan->CreateComponent(pObj, comp);
      }

      AddObjectsToWorld(
        ref_world, bDynamic, xiiMath::Max(uiNumObjects / uiTreeLevelNumNodeDiv, 1U), uiTreeLevelNumNodeDiv, uiTreeDepth - 1, iAttachCompsDepth - 1, hObj);
    }
  }

  void MeasureCreationTime(
    bool      bDynamic,
    xiiUInt32 uiNumObjects,
    xiiUInt32 uiTreeLevelNumNodeDiv,
    xiiUInt32 uiTreeDepth,
    xiiInt32  iAttachCompsDepth,
    xiiWorld* pWorld = nullptr)
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);

    if (pWorld == nullptr)
    {
      pWorld = &world;
    }

    XII_LOCK(pWorld->GetWriteMarker());

    {
      xiiStopwatch sw;

      AddObjectsToWorld(*pWorld, bDynamic, uiNumObjects, uiTreeLevelNumNodeDiv, uiTreeDepth, iAttachCompsDepth);

      const xiiTime tDiff = sw.Checkpoint();

      xiiTestFramework::Output(xiiTestOutput::Duration, "Creating %u %s objects (depth: %u): %.2fms", pWorld->GetObjectCount(),
                               bDynamic ? "dynamic" : "static", uiTreeDepth, tDiff.GetMilliseconds());
    }
  }

} // namespace


#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
static const xiiTestBlock::Enum EnableInRelease = xiiTestBlock::DisabledNoWarning;
#else
static const xiiTestBlock::Enum EnableInRelease = xiiTestBlock::Enabled;
#endif

XII_CREATE_SIMPLE_TEST(World, Profile_Creation)
{
  XII_TEST_BLOCK(EnableInRelease, "Create many objects")
  {
    // it makes no difference whether we create static or dynamic objects
    static bool bDynamic = true;
    bDynamic             = !bDynamic;

    MeasureCreationTime(bDynamic, 10, 1, 4, 0);
    MeasureCreationTime(bDynamic, 10, 1, 5, 0);
    MeasureCreationTime(bDynamic, 100, 1, 2, 0);
    MeasureCreationTime(bDynamic, 10000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 100000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 1000000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 100, 1, 3, 0);
    MeasureCreationTime(bDynamic, 3, 1, 12, 0);
    MeasureCreationTime(bDynamic, 1, 1, 80, 0);
  }
}

XII_CREATE_SIMPLE_TEST(World, Profile_Deletion)
{
  XII_TEST_BLOCK(EnableInRelease, "Delete many objects")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 2, &world);

    xiiStopwatch sw;

    XII_LOCK(world.GetWriteMarker());
    xiiUInt32 uiNumObjects = world.GetObjectCount();

    world.Clear();
    world.Update();

    const xiiTime tDiff = sw.Checkpoint();
    xiiTestFramework::Output(xiiTestOutput::Duration, "Deleting %u objects: %.2fms", uiNumObjects, tDiff.GetMilliseconds());
  }
}

XII_CREATE_SIMPLE_TEST(World, Profile_Update)
{
  XII_TEST_BLOCK(EnableInRelease, "Update 1,000,000 static objects")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    MeasureCreationTime(false, 100, 1, 3, 0, &world);

    xiiStopwatch sw;

    // first round always has some overhead
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      XII_LOCK(world.GetWriteMarker());
      world.Update();

      const xiiTime tDiff = sw.Checkpoint();

      xiiTestFramework::Output(xiiTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  XII_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic objects")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 0, &world);

    xiiStopwatch sw;

    // first round always has some overhead
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      XII_LOCK(world.GetWriteMarker());
      world.Update();

      const xiiTime tDiff = sw.Checkpoint();

      xiiTestFramework::Output(xiiTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  XII_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic objects with components")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 2, &world);

    xiiStopwatch sw;

    // first round always has some overhead
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      XII_LOCK(world.GetWriteMarker());
      world.Update();

      const xiiTime tDiff = sw.Checkpoint();

      xiiTestFramework::Output(xiiTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  XII_TEST_BLOCK(EnableInRelease, "Update 250,000 dynamic objects")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    MeasureCreationTime(true, 200, 5, 6, 0, &world);

    xiiStopwatch sw;

    // first round always has some overhead
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      XII_LOCK(world.GetWriteMarker());
      world.Update();

      const xiiTime tDiff = sw.Checkpoint();

      xiiTestFramework::Output(xiiTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  XII_TEST_BLOCK(EnableInRelease, "MT Update 250,000 dynamic objects")
  {
    xiiWorldDesc worldDesc("Test");
    worldDesc.m_bAutoCreateSpatialSystem = false; // allows multi-threaded update
    xiiWorld world(worldDesc);
    MeasureCreationTime(true, 200, 5, 6, 0, &world);

    xiiStopwatch sw;

    // first round always has some overhead
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      XII_LOCK(world.GetWriteMarker());
      world.Update();

      const xiiTime tDiff = sw.Checkpoint();

      xiiTestFramework::Output(xiiTestOutput::Duration, "Updating %u objects (MT): %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  XII_TEST_BLOCK(EnableInRelease, "MT Update 1,000,000 dynamic objects")
  {
    xiiWorldDesc worldDesc("Test");
    worldDesc.m_bAutoCreateSpatialSystem = false; // allows multi-threaded update
    xiiWorld world(worldDesc);
    MeasureCreationTime(true, 100, 1, 3, 1, &world);

    xiiStopwatch sw;

    // first round always has some overhead
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      XII_LOCK(world.GetWriteMarker());
      world.Update();

      const xiiTime tDiff = sw.Checkpoint();

      xiiTestFramework::Output(xiiTestOutput::Duration, "Updating %u objects (MT): %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }
}
