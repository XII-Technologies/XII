/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  static xiiSpatialData::Category s_SpecialTestCategory = xiiSpatialData::RegisterCategory("SpecialTestCategory", xiiSpatialData::Flags::None);

  using TestBoundsComponentManager = xiiComponentManager<class TestBoundsComponent, xiiBlockStorageType::Compact>;

  class TestBoundsComponent : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(TestBoundsComponent, xiiComponent, TestBoundsComponentManager);

  public:
    virtual void Initialize() override { GetOwner()->UpdateLocalBounds(); }

    void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg)
    {
      auto& rng = GetWorld()->GetRandomNumberGenerator();

      float x = (float)rng.DoubleMinMax(1.0, 100.0);
      float y = (float)rng.DoubleMinMax(1.0, 100.0);
      float z = (float)rng.DoubleMinMax(1.0, 100.0);

      xiiBoundingBox bounds = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3::MakeZero(), xiiVec3(x, y, z));

      xiiSpatialData::Category category = m_SpecialCategory;
      if (category == xiiInvalidSpatialDataCategory)
      {
        category = GetOwner()->IsDynamic() ? xiiDefaultSpatialDataCategories::RenderDynamic : xiiDefaultSpatialDataCategories::RenderStatic;
      }

      ref_msg.AddBounds(xiiBoundingBoxSphere::MakeFromBox(bounds), category);
    }

    xiiSpatialData::Category m_SpecialCategory = xiiInvalidSpatialDataCategory;
  };

  // clang-format off
  XII_BEGIN_COMPONENT_TYPE(TestBoundsComponent, 1, xiiComponentMode::Static)
  {
    XII_BEGIN_MESSAGEHANDLERS
    {
      XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds)
    }
    XII_END_MESSAGEHANDLERS;
  }
  XII_END_COMPONENT_TYPE;
  // clang-format on

  static xiiGameObject* CreateObjectAndTestComponent(xiiWorld& inout_world, bool bDynamic)
  {
    auto&                  rng   = inout_world.GetRandomNumberGenerator();
    constexpr const double range = 10000.0;

    float x = (float)rng.DoubleMinMax(-range, range);
    float y = (float)rng.DoubleMinMax(-range, range);
    float z = (float)rng.DoubleMinMax(-range, range);

    xiiGameObjectDescription desc;
    desc.m_bDynamic      = bDynamic;
    desc.m_LocalPosition = xiiVec3(x, y, z);

    xiiGameObject* pObject = nullptr;
    inout_world.CreateObject(desc, pObject);

    TestBoundsComponent* pComponent = nullptr;
    TestBoundsComponent::CreateComponent(pObject, pComponent);

    return pObject;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(World, SpatialSystem)
{
  xiiWorldDescription worldDesc("Test");
  worldDesc.m_uiRandomNumberGeneratorSeed = 5;

  xiiWorld world(worldDesc);
  XII_LOCK(world.GetWriteMarker());

  for (xiiUInt32 i = 0; i < 1000; ++i)
  {
    CreateObjectAndTestComponent(world, i >= 500);
  }

  world.Update();

  xiiSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = xiiDefaultSpatialDataCategories::RenderStatic.GetBitmask();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindObjectsInSphere")
  {
    xiiBoundingSphere testSphere = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3(100.0f, 60.0f, 400.0f), 3000.0f);

    xiiDynamicArray<xiiGameObject*> objectsInSphere;
    xiiHashSet<xiiGameObject*>      uniqueObjects;
    world.GetSpatialSystem()->FindObjectsInSphere(testSphere, queryParams, objectsInSphere);

    for (auto pObject : objectsInSphere)
    {
      xiiBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      XII_TEST_BOOL(testSphere.Overlaps(objSphere));
      XII_TEST_BOOL(!uniqueObjects.Insert(pObject));
      XII_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      xiiBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        XII_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((xiiGameObject*)it));
      }
    }

    objectsInSphere.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem()->FindObjectsInSphere(testSphere, queryParams, [&](xiiGameObject* pObject) {
      objectsInSphere.PushBack(pObject);
      XII_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return xiiVisitorExecution::Continue; });

    for (auto pObject : objectsInSphere)
    {
      xiiBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      XII_TEST_BOOL(testSphere.Overlaps(objSphere));
      XII_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      xiiBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        XII_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((xiiGameObject*)it));
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindObjectsInBox")
  {
    xiiBoundingBox testBox = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3(100.0f, 60.0f, 400.0f), xiiVec3(3000.0f));

    xiiDynamicArray<xiiGameObject*> objectsInBox;
    xiiHashSet<xiiGameObject*>      uniqueObjects;
    world.GetSpatialSystem()->FindObjectsInBox(testBox, queryParams, objectsInBox);

    for (auto pObject : objectsInBox)
    {
      xiiBoundingBox objBox = pObject->GetGlobalBounds().GetBox();

      XII_TEST_BOOL(testBox.Overlaps(objBox));
      XII_TEST_BOOL(!uniqueObjects.Insert(pObject));
      XII_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      xiiBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        XII_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((xiiGameObject*)it));
      }
    }

    objectsInBox.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem()->FindObjectsInBox(testBox, queryParams, [&](xiiGameObject* pObject) {
      objectsInBox.PushBack(pObject);
      XII_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return xiiVisitorExecution::Continue; });

    for (auto pObject : objectsInBox)
    {
      xiiBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      XII_TEST_BOOL(testBox.Overlaps(objSphere));
      XII_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      xiiBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        XII_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((xiiGameObject*)it));
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindVisibleObjects")
  {
    constexpr uint32_t numUpdates = 13;

    // update a few times to increase internal frame counter
    for (uint32_t i = 0; i < numUpdates; ++i)
    {
      world.Update();
    }

    // newly created objects should be considered visible in the first frame after creation
    {
      xiiGameObject* pNewObject = CreateObjectAndTestComponent(world, false);

      world.Update();

      auto visState = pNewObject->GetVisibilityState();
      XII_TEST_BOOL(visState == xiiVisibilityState::Direct);
    }

    // update a few more times to increase internal frame counter
    for (uint32_t i = 0; i < numUpdates; ++i)
    {
      world.Update();
    }

    queryParams.m_uiCategoryBitmask = xiiDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

    xiiMat4 lookAt     = xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3::MakeZero(), xiiVec3::MakeAxisX(), xiiVec3::MakeAxisZ());
    xiiMat4 projection = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle::MakeFromDegree(80.0f), 1.0f, 1.0f, 10000.0f);

    xiiFrustum testFrustum = xiiFrustum::MakeFromMVP(projection * lookAt);

    xiiDynamicArray<const xiiGameObject*> visibleObjects;
    xiiHashSet<const xiiGameObject*>      uniqueObjects;
    world.GetSpatialSystem()->FindVisibleObjects(testFrustum, queryParams, visibleObjects, {}, xiiVisibilityState::Direct);

    XII_TEST_BOOL(!visibleObjects.IsEmpty());

    for (auto pObject : visibleObjects)
    {
      XII_TEST_BOOL(testFrustum.Overlaps(pObject->GetGlobalBoundsSimd().GetSphere()));
      XII_TEST_BOOL(!uniqueObjects.Insert(pObject));
      XII_TEST_BOOL(pObject->IsDynamic());

      xiiVisibilityState::Enum visType = pObject->GetVisibilityState();
      XII_TEST_BOOL(visType == xiiVisibilityState::Direct);
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      xiiGameObject* pObject = it;

      if (testFrustum.GetObjectPosition(pObject->GetGlobalBounds().GetSphere()) == xiiVolumePosition::Outside)
      {
        xiiVisibilityState::Enum visType = pObject->GetVisibilityState();
        XII_TEST_BOOL(visType == xiiVisibilityState::Invisible);
      }
    }

    // Move some objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      constexpr const double range = 500.0f;

      if (it->IsDynamic())
      {
        xiiVec3 pos = it->GetLocalPosition();

        auto& rng = world.GetRandomNumberGenerator();
        pos.x += (float)rng.DoubleMinMax(-range, range);
        pos.y += (float)rng.DoubleMinMax(-range, range);
        pos.z += (float)rng.DoubleMinMax(-range, range);

        it->SetLocalPosition(pos);
      }
    }

    world.Update();

    // Check that last frame visible doesn't reset entirely after moving
    for (const xiiGameObject* pObject : visibleObjects)
    {
      xiiVisibilityState::Enum visType = pObject->GetVisibilityState();
      XII_TEST_BOOL(visType == xiiVisibilityState::Direct);
    }
  }

  if (false)
  {
    xiiStringBuilder outputPath = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);

    xiiProfilingUtils::SaveProfilingCapture(":output/profiling.json").IgnoreResult();
  }

  // Test multiple categories for spatial data
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MultipleCategories")
  {
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      xiiGameObject* pObject = it;

      TestBoundsComponent* pComponent = nullptr;
      TestBoundsComponent::CreateComponent(pObject, pComponent);
      pComponent->m_SpecialCategory = s_SpecialTestCategory;
    }

    world.Update();

    xiiDynamicArray<xiiGameObjectHandle> allObjects;
    allObjects.Reserve(world.GetObjectCount());

    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      allObjects.PushBack(it->GetHandle());
    }

    for (xiiUInt32 i = allObjects.GetCount(); i-- > 0;)
    {
      world.DeleteObjectNow(allObjects[i]);
    }

    world.Update();
  }
}
