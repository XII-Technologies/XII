#include <CoreTest/CoreTestPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  static xiiSpatialData::Category s_SpecialTestCategory = xiiSpatialData::RegisterCategory("SpecialTestCategory", xiiSpatialData::Flags::None);

  using TestBoundsComponentManager = class TestBoundsComponent;

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

      xiiBoundingBox bounds;
      bounds.SetCenterAndHalfExtents(xiiVec3::ZeroVector(), xiiVec3(x, y, z));

      xiiSpatialData::Category category = m_SpecialCategory;
      if (category == xiiInvalidSpatialDataCategory)
      {
        category = GetOwner()->IsDynamic() ? xiiDefaultSpatialDataCategories::RenderDynamic : xiiDefaultSpatialDataCategories::RenderStatic;
      }

      ref_msg.AddBounds(bounds, category);
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
} // namespace

XII_CREATE_SIMPLE_TEST(World, SpatialSystem)
{
  xiiWorldDesc worldDesc("Test");
  worldDesc.m_uiRandomNumberGeneratorSeed = 5;

  xiiWorld world(worldDesc);
  XII_LOCK(world.GetWriteMarker());

  auto& rng = world.GetRandomNumberGenerator();

  xiiDynamicArray<xiiGameObject*> objects;
  objects.Reserve(1000);

  for (xiiUInt32 i = 0; i < 1000; ++i)
  {
    constexpr const double range = 10000.0;

    float x = (float)rng.DoubleMinMax(-range, range);
    float y = (float)rng.DoubleMinMax(-range, range);
    float z = (float)rng.DoubleMinMax(-range, range);

    xiiGameObjectDesc desc;
    desc.m_bDynamic      = (i >= 500);
    desc.m_LocalPosition = xiiVec3(x, y, z);

    xiiGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    objects.PushBack(pObject);

    TestBoundsComponent* pComponent = nullptr;
    TestBoundsComponent::CreateComponent(pObject, pComponent);
  }

  world.Update();

  xiiSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = xiiDefaultSpatialDataCategories::RenderStatic.GetBitmask();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindObjectsInSphere")
  {
    xiiBoundingSphere testSphere(xiiVec3(100.0f, 60.0f, 400.0f), 3000.0f);

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

      return xiiVisitorExecution::Continue;
    });

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
    xiiBoundingBox testBox;
    testBox.SetCenterAndHalfExtents(xiiVec3(100.0f, 60.0f, 400.0f), xiiVec3(3000.0f));

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

      return xiiVisitorExecution::Continue;
    });

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

    queryParams.m_uiCategoryBitmask = xiiDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

    xiiMat4 lookAt     = xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3::ZeroVector(), xiiVec3::UnitXAxis(), xiiVec3::UnitZAxis());
    xiiMat4 projection = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle::Degree(80.0f), 1.0f, 1.0f, 10000.0f);

    xiiFrustum testFrustum;
    testFrustum.SetFrustum(projection * lookAt);

    xiiDynamicArray<const xiiGameObject*> visibleObjects;
    xiiHashSet<const xiiGameObject*>      uniqueObjects;
    world.GetSpatialSystem()->FindVisibleObjects(testFrustum, queryParams, visibleObjects, {});

    XII_TEST_BOOL(!visibleObjects.IsEmpty());

    for (auto pObject : visibleObjects)
    {
      XII_TEST_BOOL(testFrustum.Overlaps(pObject->GetGlobalBoundsSimd().GetSphere()));
      XII_TEST_BOOL(!uniqueObjects.Insert(pObject));
      XII_TEST_BOOL(pObject->IsDynamic());
      XII_TEST_BOOL(pObject->GetNumFramesSinceVisible() == 0);
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      xiiGameObject* pObject = it;

      if (testFrustum.GetObjectPosition(pObject->GetGlobalBounds().GetSphere()) == xiiVolumePosition::Outside)
      {
        XII_TEST_BOOL(pObject->GetNumFramesSinceVisible() >= numUpdates);
      }
    }

    // Move some objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      constexpr const double range = 500.0f;

      if (it->IsDynamic())
      {
        xiiVec3 pos = it->GetLocalPosition();

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
      XII_TEST_BOOL(pObject->GetNumFramesSinceVisible() == 1);
    }
  }

  if (false)
  {
    xiiStringBuilder outputPath = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", xiiFileSystem::AllowWrites) == XII_SUCCESS);

    xiiFileWriter fileWriter;
    if (fileWriter.Open(":output/profiling.json") == XII_SUCCESS)
    {
      xiiProfilingSystem::ProfilingData profilingData;
      xiiProfilingSystem::Capture(profilingData);
      profilingData.Write(fileWriter).IgnoreResult();
      xiiLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    }
  }

  // Test multiple categories for spatial data
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MultipleCategories")
  {
    for (xiiUInt32 i = 0; i < objects.GetCount(); ++i)
    {
      xiiGameObject* pObject = objects[i];

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
