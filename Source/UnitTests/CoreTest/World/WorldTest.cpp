#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/GraphicsUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(World);

namespace
{
  union TestWorldObjects
  {
    struct
    {
      xiiGameObject* pParent1;
      xiiGameObject* pParent2;
      xiiGameObject* pChild11;
      xiiGameObject* pChild21;
    };
    xiiGameObject* pObjects[4];
  };

  TestWorldObjects CreateTestWorld(xiiWorld& ref_world, bool bDynamic)
  {
    TestWorldObjects testWorldObjects;
    xiiMemoryUtils::ZeroFill(&testWorldObjects, 1);

    xiiQuat q;
    q.SetFromAxisAndAngle(xiiVec3(0.0f, 0.0f, 1.0f), xiiAngle::MakeFromDegree(90.0f));

    xiiGameObjectDesc desc;
    desc.m_bDynamic      = bDynamic;
    desc.m_LocalPosition = xiiVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling  = xiiVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent1");

    ref_world.CreateObject(desc, testWorldObjects.pParent1);

    desc.m_sName.Assign("Parent2");
    ref_world.CreateObject(desc, testWorldObjects.pParent2);

    desc.m_hParent = testWorldObjects.pParent1->GetHandle();
    desc.m_sName.Assign("Child11");
    ref_world.CreateObject(desc, testWorldObjects.pChild11);

    desc.m_hParent = testWorldObjects.pParent2->GetHandle();
    desc.m_sName.Assign("Child21");
    ref_world.CreateObject(desc, testWorldObjects.pChild21);

    return testWorldObjects;
  }

  void TestTransforms(const TestWorldObjects& o, xiiVec3 vOffset = xiiVec3(100.0f, 0.0f, 0.0f))
  {
    const float eps = xiiMath::DefaultEpsilon<float>();
    xiiQuat     q;
    q.SetFromAxisAndAngle(xiiVec3(0.0f, 0.0f, 1.0f), xiiAngle::MakeFromDegree(90.0f));

    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      XII_TEST_VEC3(o.pObjects[i]->GetGlobalPosition(), vOffset, 0);
      XII_TEST_BOOL(o.pObjects[i]->GetGlobalRotation().IsEqualRotation(q, eps * 10.0f));
      XII_TEST_VEC3(o.pObjects[i]->GetGlobalScaling(), xiiVec3(1.5f, 1.5f, 1.5f), 0);
    }

    for (xiiUInt32 i = 2; i < 4; ++i)
    {
      XII_TEST_VEC3(o.pObjects[i]->GetGlobalPosition(), vOffset + xiiVec3(0.0f, 150.0f, 0.0f), eps * 2.0f);
      XII_TEST_BOOL(o.pObjects[i]->GetGlobalRotation().IsEqualRotation(q * q, eps * 10.0f));
      XII_TEST_VEC3(o.pObjects[i]->GetGlobalScaling(), xiiVec3(2.25f, 2.25f, 2.25f), 0);
    }
  }

  void SanityCheckWorld(xiiWorld& ref_world)
  {
    struct Traverser
    {
      Traverser(xiiWorld& ref_world) :
        m_World(ref_world)
      {
      }

      xiiWorld&              m_World;
      xiiSet<xiiGameObject*> m_Found;

      xiiVisitorExecution::Enum Visit(xiiGameObject* pObject)
      {
        xiiGameObject* pObject2 = nullptr;
        XII_TEST_BOOL_MSG(m_World.TryGetObject(pObject->GetHandle(), pObject2), "Visited object that is not part of the world!");
        XII_TEST_BOOL_MSG(pObject2 == pObject, "Handle did not resolve to the same object!");
        XII_TEST_BOOL_MSG(!m_Found.Contains(pObject), "Object visited twice!");
        m_Found.Insert(pObject);

        const xiiUInt32 uiChildren  = pObject->GetChildCount();
        xiiUInt32       uiChildren2 = 0;
        for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
        {
          uiChildren2++;
          auto           handle = it->GetHandle();
          xiiGameObject* pChild = nullptr;
          XII_TEST_BOOL_MSG(m_World.TryGetObject(handle, pChild), "Could not resolve child!");
          xiiGameObject* pParent = pChild->GetParent();
          XII_TEST_BOOL_MSG(pParent == pObject, "pObject's child's parent does not point to pObject!");
        }
        XII_TEST_INT(uiChildren, uiChildren2);
        return xiiVisitorExecution::Continue;
      }
    };

    Traverser traverser(ref_world);
    ref_world.Traverse(xiiWorld::VisitorFunc(&Traverser::Visit, &traverser), xiiWorld::TraversalMethod::BreadthFirst);
  }

  class CustomCoordinateSystemProvider : public xiiCoordinateSystemProvider
  {
  public:
    CustomCoordinateSystemProvider(const xiiWorld* pWorld) :
      xiiCoordinateSystemProvider(pWorld)
    {
    }

    virtual void GetCoordinateSystem(const xiiVec3Real& vGlobalPosition, xiiCoordinateSystem& out_coordinateSystem) const override
    {
      const xiiMat3Real mTmp = xiiGraphicsUtils::CreateLookAtViewMatrix(-vGlobalPosition, xiiVec3Real(0, 0, 1), xiiHandedness::LeftHanded);

      out_coordinateSystem.m_vRightDir   = mTmp.GetRow(0);
      out_coordinateSystem.m_vUpDir      = mTmp.GetRow(1);
      out_coordinateSystem.m_vForwardDir = mTmp.GetRow(2);
    }
  };

  class VelocityTestModule : public xiiWorldModule
  {
    XII_ADD_DYNAMIC_REFLECTION(VelocityTestModule, xiiWorldModule);
    XII_DECLARE_WORLD_MODULE();

  public:
    VelocityTestModule(xiiWorld* pWorld) :
      xiiWorldModule(pWorld)
    {
    }

    virtual void Initialize() override
    {
      {
        auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(VelocityTestModule::SetLocalPos, this);
        RegisterUpdateFunction(desc);
      }

      {
        auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(VelocityTestModule::ResetGlobalPos, this);
        RegisterUpdateFunction(desc);
      }
    }

    void SetLocalPos(const UpdateContext&)
    {
      if (m_bSetLocalPos == false)
        return;

      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        xiiUInt32 i = it->GetHandle().GetInternalID().m_InstanceIndex;

        xiiVec3 newPos = xiiVec3(i * 10.0f, 0, 0);
        it->SetLocalPosition(newPos);

        xiiQuat newRot;
        newRot.SetFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(i * 30.0f));
        it->SetLocalRotation(newRot);

        if (i > 5)
        {
          it->UpdateGlobalTransform();
        }
        if (i > 8)
        {
          it->UpdateGlobalTransformAndBounds();
        }
      }
    }

    void ResetGlobalPos(const UpdateContext&)
    {
      if (m_bResetGlobalPos == false)
        return;

      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        it->SetGlobalPosition(xiiVec3::MakeZero());
      }
    }

    bool m_bSetLocalPos    = false;
    bool m_bResetGlobalPos = false;
  };

  // clang-format off
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(VelocityTestModule, 1, xiiRTTINoAllocator)
  XII_END_DYNAMIC_REFLECTED_TYPE;
  XII_IMPLEMENT_WORLD_MODULE(VelocityTestModule);
  // clang-format on
} // namespace

class xiiGameObjectTest
{
public:
  static void TestInternals(xiiGameObject* pObject, xiiGameObject* pParent, xiiUInt32 uiHierarchyLevel)
  {
    XII_TEST_INT(pObject->m_uiHierarchyLevel, uiHierarchyLevel);
    XII_TEST_BOOL(pObject->m_pTransformationData->m_pObject == pObject);

    if (pParent)
    {
      XII_TEST_BOOL(pObject->m_pTransformationData->m_pParentData->m_pObject == pParent);
    }

    XII_TEST_BOOL(pObject->m_pTransformationData->m_pParentData == (pParent != nullptr ? pParent->m_pTransformationData : nullptr));
    XII_TEST_BOOL(pObject->GetParent() == pParent);
  }
};

XII_CREATE_SIMPLE_TEST(World, World)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transforms dynamic")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    XII_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    xiiVec3 offset = xiiVec3(200.0f, 0.0f, 0.0f);
    o.pParent1->SetLocalPosition(offset);
    o.pParent2->SetLocalPosition(offset);

    world.Update();

    TestTransforms(o, offset);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transforms static")
  {
    xiiWorldDesc worldDesc("Test");
    worldDesc.m_bReportErrorWhenStaticObjectMoves = false;

    xiiWorld world(worldDesc);
    XII_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, false);

    xiiVec3 offset = xiiVec3(200.0f, 0.0f, 0.0f);
    o.pParent1->SetLocalPosition(offset);
    o.pParent2->SetLocalPosition(offset);

    // No need to call world update since global transform is updated immediately for static objects.
    // world.Update();

    TestTransforms(o, offset);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GameObject parenting")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    XII_LOCK(world.GetWriteMarker());

    const float eps = xiiMath::DefaultEpsilon<float>();
    xiiQuat     q;
    q.SetFromAxisAndAngle(xiiVec3(0.0f, 0.0f, 1.0f), xiiAngle::MakeFromDegree(90.0f));

    xiiGameObjectDesc desc;
    desc.m_LocalPosition = xiiVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling  = xiiVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent");

    xiiGameObject*      pParentObject;
    xiiGameObjectHandle parentObject = world.CreateObject(desc, pParentObject);

    XII_TEST_VEC3(pParentObject->GetLocalPosition(), desc.m_LocalPosition, 0);
    XII_TEST_BOOL(pParentObject->GetLocalRotation() == desc.m_LocalRotation);
    XII_TEST_VEC3(pParentObject->GetLocalScaling(), desc.m_LocalScaling, 0);

    XII_TEST_VEC3(pParentObject->GetGlobalPosition(), desc.m_LocalPosition, 0);
    XII_TEST_BOOL(pParentObject->GetGlobalRotation().IsEqualRotation(desc.m_LocalRotation, eps * 10.0f));
    XII_TEST_VEC3(pParentObject->GetGlobalScaling(), desc.m_LocalScaling, 0);

    XII_TEST_BOOL(pParentObject->GetName() == desc.m_sName.GetString());

    desc.m_LocalRotation.SetIdentity();
    desc.m_LocalScaling.Set(1.0f);
    desc.m_hParent = parentObject;

    xiiGameObjectHandle childObjects[10];
    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      xiiStringBuilder sb;
      sb.AppendFormat("Child_{0}", i);
      desc.m_sName.Assign(sb.GetData());

      desc.m_LocalPosition = xiiVec3(i * 10.0f, 0.0f, 0.0f);

      childObjects[i] = world.CreateObject(desc);
    }

    xiiUInt32 uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      xiiStringBuilder sb;
      sb.AppendFormat("Child_{0}", uiCounter);

      XII_TEST_BOOL(it->GetName() == sb);

      XII_TEST_VEC3(it->GetGlobalPosition(), xiiVec3(100.0f, uiCounter * 15.0f, 0.0f), eps * 2.0f); // 15 because parent is scaled by 1.5
      XII_TEST_BOOL(it->GetGlobalRotation().IsEqualRotation(q, eps * 10.0f));
      XII_TEST_VEC3(it->GetGlobalScaling(), xiiVec3(1.5f, 1.5f, 1.5f), 0.0f);

      ++uiCounter;
    }

    XII_TEST_INT(uiCounter, 10);
    XII_TEST_INT(pParentObject->GetChildCount(), 10);

    world.DeleteObjectNow(childObjects[0]);
    world.DeleteObjectNow(childObjects[3]);
    world.DeleteObjectNow(childObjects[9]);

    XII_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    XII_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    XII_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    xiiUInt32 indices[7] = {1, 2, 4, 5, 6, 7, 8};

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      xiiStringBuilder sb;
      sb.AppendFormat("Child_{0}", indices[uiCounter]);

      XII_TEST_BOOL(it->GetName() == sb);

      ++uiCounter;
    }

    XII_TEST_INT(uiCounter, 7);
    XII_TEST_INT(pParentObject->GetChildCount(), 7);

    // do one update step so dead objects get deleted
    world.Update();
    SanityCheckWorld(world);

    XII_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    XII_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    XII_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      xiiStringBuilder sb;
      sb.AppendFormat("Child_{0}", indices[uiCounter]);

      XII_TEST_BOOL(it->GetName() == sb);

      ++uiCounter;
    }

    XII_TEST_INT(uiCounter, 7);
    XII_TEST_INT(pParentObject->GetChildCount(), 7);

    world.DeleteObjectDelayed(parentObject);
    XII_TEST_BOOL(world.IsValidObject(parentObject));

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(indices); ++i)
    {
      XII_TEST_BOOL(world.IsValidObject(childObjects[indices[i]]));
    }

    // do one update step so dead objects get deleted
    world.Update();
    SanityCheckWorld(world);

    XII_TEST_BOOL(!world.IsValidObject(parentObject));

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      XII_TEST_BOOL(!world.IsValidObject(childObjects[i]));
    }

    XII_TEST_INT(world.GetObjectCount(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Re-parenting 1")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    XII_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    o.pParent1->AddChild(o.pParent2->GetHandle());
    o.pParent2->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // No need to update the world since re-parenting is now done immediately.
    // world.Update();

    TestTransforms(o);

    xiiGameObjectTest::TestInternals(o.pParent1, nullptr, 0);
    xiiGameObjectTest::TestInternals(o.pParent2, o.pParent1, 1);
    xiiGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1);
    xiiGameObjectTest::TestInternals(o.pChild21, o.pParent2, 2);

    XII_TEST_INT(o.pParent1->GetChildCount(), 2);
    auto it = o.pParent1->GetChildren();
    XII_TEST_BOOL(o.pChild11 == it);
    ++it;
    XII_TEST_BOOL(o.pParent2 == it);
    ++it;
    XII_TEST_BOOL(!it.IsValid());

    it = o.pParent2->GetChildren();
    XII_TEST_BOOL(o.pChild21 == it);
    ++it;
    XII_TEST_BOOL(!it.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Re-parenting 2")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    XII_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    o.pChild21->SetParent(xiiGameObjectHandle());
    SanityCheckWorld(world);
    // No need to update the world since re-parenting is now done immediately.
    // world.Update();

    TestTransforms(o);

    xiiGameObjectTest::TestInternals(o.pParent1, nullptr, 0);
    xiiGameObjectTest::TestInternals(o.pParent2, nullptr, 0);
    xiiGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1);
    xiiGameObjectTest::TestInternals(o.pChild21, nullptr, 0);

    auto it = o.pParent1->GetChildren();
    XII_TEST_BOOL(o.pChild11 == it);
    ++it;
    XII_TEST_BOOL(!it.IsValid());

    XII_TEST_INT(o.pParent2->GetChildCount(), 0);
    it = o.pParent2->GetChildren();
    XII_TEST_BOOL(!it.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Re-parenting 3")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    XII_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);
    SanityCheckWorld(world);
    // Here we test whether the sibling information is correctly cleared.

    o.pChild21->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) sibling.
    o.pParent2->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) and next (pParent2) sibling.
    o.pChild21->SetParent(xiiGameObjectHandle());
    SanityCheckWorld(world);
    // pChild21 has no siblings.
    o.pChild21->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) sibling again.
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Traversal")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    XII_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, false);

    {
      struct BreadthFirstTest
      {
        BreadthFirstTest() { m_uiCounter = 0; }

        xiiVisitorExecution::Enum Visit(xiiGameObject* pObject)
        {
          if (m_uiCounter < XII_ARRAY_SIZE(m_o.pObjects))
          {
            XII_TEST_BOOL(pObject == m_o.pObjects[m_uiCounter]);
          }

          ++m_uiCounter;
          return xiiVisitorExecution::Continue;
        }

        xiiUInt32        m_uiCounter;
        TestWorldObjects m_o;
      };

      BreadthFirstTest bft;
      bft.m_o = o;

      world.Traverse(xiiWorld::VisitorFunc(&BreadthFirstTest::Visit, &bft), xiiWorld::BreadthFirst);
      XII_TEST_INT(bft.m_uiCounter, XII_ARRAY_SIZE(o.pObjects));
    }

    {
      world.CreateObject(xiiGameObjectDesc());

      struct DepthFirstTest
      {
        DepthFirstTest() { m_uiCounter = 0; }

        xiiVisitorExecution::Enum Visit(xiiGameObject* pObject)
        {
          if (m_uiCounter == 0)
          {
            XII_TEST_BOOL(pObject == m_o.pParent1);
          }
          else if (m_uiCounter == 1)
          {
            XII_TEST_BOOL(pObject == m_o.pChild11);
          }
          else if (m_uiCounter == 2)
          {
            XII_TEST_BOOL(pObject == m_o.pParent2);
          }
          else if (m_uiCounter == 3)
          {
            XII_TEST_BOOL(pObject == m_o.pChild21);
          }

          ++m_uiCounter;
          if (m_uiCounter >= XII_ARRAY_SIZE(m_o.pObjects))
            return xiiVisitorExecution::Stop;

          return xiiVisitorExecution::Continue;
        }

        xiiUInt32        m_uiCounter;
        TestWorldObjects m_o;
      };

      DepthFirstTest dft;
      dft.m_o = o;

      world.Traverse(xiiWorld::VisitorFunc(&DepthFirstTest::Visit, &dft), xiiWorld::DepthFirst);
      XII_TEST_INT(dft.m_uiCounter, XII_ARRAY_SIZE(o.pObjects));
    }

    {
      XII_TEST_INT(world.GetObjectCount(), 5);
      world.DeleteObjectNow(o.pChild11->GetHandle(), false);
      XII_TEST_INT(world.GetObjectCount(), 4);

      for (auto it = world.GetObjects(); it.IsValid(); ++it)
      {
        XII_TEST_BOOL(!it->GetHandle().IsInvalidated());
      }

      const xiiWorld& constWorld = world;
      for (auto it = constWorld.GetObjects(); it.IsValid(); ++it)
      {
        XII_TEST_BOOL(!it->GetHandle().IsInvalidated());
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Multiple Worlds")
  {
    xiiWorldDesc worldDesc1("Test1");
    xiiWorld     world1(worldDesc1);
    XII_LOCK(world1.GetWriteMarker());

    xiiWorldDesc worldDesc2("Test2");
    xiiWorld     world2(worldDesc2);
    XII_LOCK(world2.GetWriteMarker());

    xiiGameObjectDesc desc;
    desc.m_sName.Assign("Obj1");

    xiiGameObjectHandle hObj1 = world1.CreateObject(desc);
    XII_TEST_BOOL(world1.IsValidObject(hObj1));

    desc.m_sName.Assign("Obj2");

    xiiGameObjectHandle hObj2 = world2.CreateObject(desc);
    XII_TEST_BOOL(world2.IsValidObject(hObj2));

    xiiGameObject* pObj1 = nullptr;
    XII_TEST_BOOL(world1.TryGetObject(hObj1, pObj1));
    XII_TEST_BOOL(pObj1 != nullptr);

    pObj1->SetGlobalKey("Obj1");
    pObj1 = nullptr;
    XII_TEST_BOOL(world1.TryGetObjectWithGlobalKey(xiiTempHashedString("Obj1"), pObj1));
    XII_TEST_BOOL(!world1.TryGetObjectWithGlobalKey(xiiTempHashedString("Obj2"), pObj1));
    XII_TEST_BOOL(pObj1 != nullptr);

    xiiGameObject* pObj2 = nullptr;
    XII_TEST_BOOL(world2.TryGetObject(hObj2, pObj2));
    XII_TEST_BOOL(pObj2 != nullptr);

    pObj2->SetGlobalKey("Obj2");
    pObj2 = nullptr;
    XII_TEST_BOOL(world2.TryGetObjectWithGlobalKey(xiiTempHashedString("Obj2"), pObj2));
    XII_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(xiiTempHashedString("Obj1"), pObj2));
    XII_TEST_BOOL(pObj2 != nullptr);

    pObj2->SetGlobalKey("Deschd");
    XII_TEST_BOOL(world2.TryGetObjectWithGlobalKey(xiiTempHashedString("Deschd"), pObj2));
    XII_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(xiiTempHashedString("Obj2"), pObj2));

    world2.DeleteObjectNow(hObj2);

    XII_TEST_BOOL(!world2.IsValidObject(hObj2));
    XII_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(xiiTempHashedString("Deschd"), pObj2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Custom coordinate system")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);

    xiiSharedPtr<CustomCoordinateSystemProvider> pProvider       = XII_DEFAULT_NEW(CustomCoordinateSystemProvider, &world);
    CustomCoordinateSystemProvider*              pProviderBackup = pProvider.Borrow();

    world.SetCoordinateSystemProvider(pProvider);
    XII_TEST_BOOL(&world.GetCoordinateSystemProvider() == pProviderBackup);

    xiiVec3 pos = xiiVec3(2, 3, 0);

    xiiCoordinateSystem coordSys;
    world.GetCoordinateSystem(pos, coordSys);

    XII_TEST_VEC3(coordSys.m_vForwardDir, (-pos).GetNormalized(), xiiMath::SmallEpsilon<float>());
    XII_TEST_VEC3(coordSys.m_vUpDir, xiiVec3(0, 0, 1), xiiMath::SmallEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Active Flag / Active State")
  {
    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    XII_LOCK(world.GetWriteMarker());

    xiiGameObjectHandle hParent;
    xiiGameObjectDesc   desc;
    xiiGameObjectHandle hObjects[10];
    xiiGameObject*      pObjects[10];

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      desc.m_hParent = hParent;
      hObjects[i]    = world.CreateObject(desc, pObjects[i]);
      hParent        = hObjects[i];

      XII_TEST_BOOL(pObjects[i]->GetActiveFlag());
      XII_TEST_BOOL(pObjects[i]->IsActive());
    }

    xiiUInt32 iTopDisabled = 1;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      XII_TEST_BOOL(pObjects[i]->GetActiveFlag() == (i != iTopDisabled));
      XII_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    pObjects[iTopDisabled]->SetActiveFlag(true);

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      XII_TEST_BOOL(pObjects[i]->GetActiveFlag() == true);
      XII_TEST_BOOL(pObjects[i]->IsActive() == true);
    }

    iTopDisabled = 5;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      XII_TEST_BOOL(pObjects[i]->GetActiveFlag() == (i != iTopDisabled));
      XII_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    iTopDisabled = 3;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      XII_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    pObjects[iTopDisabled]->SetActiveFlag(true);

    iTopDisabled = 5;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      XII_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }
  }

#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Velocity")
  {
    constexpr xiiUInt32 numObjects = 10;

    xiiWorldDesc worldDesc("Test");
    xiiWorld     world(worldDesc);
    XII_LOCK(world.GetWriteMarker());

    auto pModule = world.GetOrCreateModule<VelocityTestModule>();

    xiiGameObjectDesc objectDesc;
    objectDesc.m_bDynamic = true;

    xiiGameObjectHandle hObjects[numObjects];
    xiiGameObject*      pObjects[numObjects];
    for (xiiUInt32 i = 0; i < numObjects; ++i)
    {
      objectDesc.m_LocalPosition = xiiVec3(0, 0, 5);
      objectDesc.m_LocalRotation.SetFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(90));

      hObjects[i] = world.CreateObject(objectDesc, pObjects[i]);
    }

    pModule->m_bSetLocalPos    = true;
    pModule->m_bResetGlobalPos = false;

    world.GetClock().SetFixedTimeStep(xiiTime::MakeFromMilliseconds(100));
    world.Update();

    for (auto& pObject : pObjects)
    {
      xiiUInt32 i                      = pObject->GetHandle().GetInternalID().m_InstanceIndex;
      xiiVec3   expectedLastPos        = xiiVec3(0, 0, 5);
      xiiVec3   expectedPos            = xiiVec3(i * 10, 0, 0);
      xiiVec3   expectedLinearVelocity = xiiVec3(i * 100, 0, -50);
      XII_TEST_VEC3(pObject->GetLastGlobalTransform().m_vPosition, expectedLastPos, xiiMath::DefaultEpsilon<float>());
      XII_TEST_VEC3(pObject->GetGlobalPosition(), expectedPos, xiiMath::DefaultEpsilon<float>());
      XII_TEST_VEC3(pObject->GetLinearVelocity(), expectedLinearVelocity, xiiMath::DefaultEpsilon<float>());

      xiiVec3 expectedAngularVelocity = xiiVec3(0, 0, (xiiAngle::MakeFromDegree(i * 30) - xiiAngle::MakeFromDegree(90)).GetRadian() * 10);
      xiiVec3 angularVelocity         = pObject->GetAngularVelocity();
      XII_TEST_VEC3(angularVelocity, expectedAngularVelocity, xiiMath::DefaultEpsilon<float>());
    }

    pModule->m_bSetLocalPos    = false;
    pModule->m_bResetGlobalPos = true;

    world.Update();

    for (auto& pObject : pObjects)
    {
      xiiUInt32 i                      = pObject->GetHandle().GetInternalID().m_InstanceIndex;
      xiiVec3   expectedLastPos        = xiiVec3(i * 10, 0, 0);
      xiiVec3   expectedLinearVelocity = xiiVec3(i * -100.0f, 0, 0);
      XII_TEST_VEC3(pObject->GetLastGlobalTransform().m_vPosition, expectedLastPos, xiiMath::DefaultEpsilon<float>());
      XII_TEST_VEC3(pObject->GetGlobalPosition(), xiiVec3::MakeZero(), xiiMath::DefaultEpsilon<float>());
      XII_TEST_VEC3(pObject->GetLinearVelocity(), expectedLinearVelocity, xiiMath::DefaultEpsilon<float>());
    }
  }
#endif
}
