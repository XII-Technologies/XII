#include <GameEngineTest/GameEngineTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Utilities/DataStructures/DynamicOctree.h>

XII_CREATE_SIMPLE_TEST_GROUP(DataStructures);

namespace DynamicOctreeTestDetail
{
  static xiiInt32  g_iSearchInstance = 0;
  static bool      g_bFoundSearched  = false;
  static xiiUInt32 g_iReturned       = 0;

  static bool ObjectFound(void* pPassThrough, xiiDynamicTreeObjectConst object)
  {
    XII_TEST_BOOL(pPassThrough == nullptr);

    ++g_iReturned;

    if (object.Value().m_iObjectInstance == g_iSearchInstance)
      g_bFoundSearched = true;

    // let it give us all the objects in range and count how many that are
    return true;
  }
} // namespace DynamicOctreeTestDetail

XII_CREATE_SIMPLE_TEST(DataStructures, DynamicOctree)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CreateTree / GetBoundingBox")
  {
    xiiDynamicOctree o;
    o.CreateTree(xiiVec3(100, 200, 300), xiiVec3(300, 400, 500), 1.0f);

    const xiiBoundingBox& bb = o.GetBoundingBox();

    XII_TEST_VEC3(bb.GetCenter(), xiiVec3(100, 200, 300), 0.01f);
    XII_TEST_VEC3(bb.GetHalfExtents(), xiiVec3(500), 0.01f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert Inside / Outside")
  {
    const xiiVec3 c(100, 200, 300);
    const float   e = 50;

    xiiDynamicOctree o;
    o.CreateTree(c, xiiVec3(e), 1.0f);
    xiiInt32 iInstance = 0;

    for (float z = -e - 99; z < e + 100; z += 10.0f)
    {
      for (float y = -e - 99; y < e + 100; y += 10.0f)
      {
        for (float x = -e - 99; x < e + 100; x += 10.0f)
        {
          const bool bInside = (z > -e) && (z < e) && (y > -e) && (y < e) && (x > -e) && (x < e);

          XII_TEST_BOOL(o.InsertObject(c + xiiVec3(x, y, z), xiiVec3(1.0f), 0, iInstance, nullptr, true) == (bInside ? XII_SUCCESS : XII_FAILURE));
          XII_TEST_BOOL(o.InsertObject(c + xiiVec3(x, y, z), xiiVec3(1.0f), 0, iInstance, nullptr, false) == XII_SUCCESS);

          ++iInstance;
        }
      }
    }
  }

  struct TestObject
  {
    xiiVec3              m_vPos;
    xiiVec3              m_vExtents;
    xiiDynamicTreeObject m_hObject;
  };

  xiiDeque<TestObject> Objects;

  {
    TestObject to;


    to.m_vPos.Set(-90, 50, 0);
    to.m_vExtents.Set(2.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(-90, 50, 0);
    to.m_vExtents.Set(2.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(-90, 50, 80);
    to.m_vExtents.Set(2.0f, 4.0f, 10.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(0, 0, -50);
    to.m_vExtents.Set(20.0f, 4.0f, 10.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(10, 10, 10);
    to.m_vExtents.Set(50.0f, 2.0f, 1.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(50, -20, 10);
    to.m_vExtents.Set(1.0f, 2.0f, 1.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(50, -20, 10);
    to.m_vExtents.Set(1.0f, 2.0f, 1.0f);

    Objects.PushBack(to);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindObjectsInRange(Point)")
  {
    xiiDynamicOctree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents * 0.9f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents * 0.9f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindObjectsInRange(Radius)")
  {
    xiiDynamicOctree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      // point inside object

      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents * 0.9f, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents * 0.9f, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      // point outside object

      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents + xiiVec3(2, 0, 0), 2.5f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents - xiiVec3(0, 2, 0), 2.5f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveObject(handle)")
  {
    xiiDynamicOctree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      o.RemoveObject(Objects[i].m_hObject);

      // one less in the tree
      XII_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == false);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveObject(index)")
  {
    xiiDynamicOctree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      o.RemoveObject(i, i + 1);

      // one less in the tree
      XII_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == false);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveObjectsOfType")
  {
    xiiDynamicOctree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i + 1;

      o.RemoveObjectsOfType(i);

      // one less in the tree
      XII_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicOctreeTestDetail::g_iReturned      = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == false);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    o.RemoveObjectsOfType(0);

    XII_TEST_BOOL(o.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveAllObjects")
  {
    xiiDynamicOctree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    o.RemoveAllObjects();
    XII_TEST_BOOL(o.IsEmpty());
    XII_TEST_INT(o.GetCount(), 0);
  }
}
