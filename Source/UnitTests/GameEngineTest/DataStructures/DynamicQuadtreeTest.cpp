/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngineTest/GameEngineTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Utilities/DataStructures/DynamicQuadtree.h>

namespace DynamicQuadtreeTestDetail
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
} // namespace DynamicQuadtreeTestDetail

XII_CREATE_SIMPLE_TEST(DataStructures, DynamicQuadtree)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CreateTree / GetBoundingBox")
  {
    xiiDynamicQuadtree o;
    o.CreateTree(xiiVec3(100, 200, 300), xiiVec3(300, 400, 500), 1.0f);

    const xiiBoundingBox& bb = o.GetBoundingBox();

    xiiVec3 c = bb.GetCenter();
    c.y       = 200.0f;

    XII_TEST_VEC3(c, xiiVec3(100, 200, 300), 0.01f);

    xiiVec3 h = bb.GetHalfExtents();
    h.y       = 500.0f;
    XII_TEST_VEC3(h, xiiVec3(500), 0.01f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert Inside / Outside")
  {
    const xiiVec3 c(100, 200, 300);
    const float   e = 50;

    xiiDynamicQuadtree o;
    o.CreateTree(c, xiiVec3(e), 1.0f);
    xiiInt32 iInstance = 0;

    for (float z = -e - 99; z < e + 100; z += 10.0f)
    {
      for (float x = -e - 99; x < e + 100; x += 10.0f)
      {
        const bool bInside = (z > -e) && (z < e) && (x > -e) && (x < e);

        XII_TEST_BOOL(o.InsertObject(c + xiiVec3(x, 0, z), xiiVec3(1.0f), 0, iInstance, nullptr, true) == (bInside ? XII_SUCCESS : XII_FAILURE));
        XII_TEST_BOOL(o.InsertObject(c + xiiVec3(x, 0, z), xiiVec3(1.0f), 0, iInstance, nullptr, false) == XII_SUCCESS);

        ++iInstance;
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
    xiiDynamicQuadtree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicQuadtreeTestDetail::g_iSearchInstance = i;

      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents * 0.9f, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents * 0.9f, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_iReturned < Objects.GetCount());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindObjectsInRange(Radius)")
  {
    xiiDynamicQuadtree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicQuadtreeTestDetail::g_iSearchInstance = i;

      // point inside object

      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents * 0.9f, 1.0f, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents * 0.9f, 1.0f, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_iReturned < Objects.GetCount());

      // point outside object

      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents + xiiVec3(2, 0, 0), 2.5f, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents - xiiVec3(0, 2, 0), 2.5f, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == true);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_iReturned < Objects.GetCount());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveObject(handle)")
  {
    xiiDynamicQuadtree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicQuadtreeTestDetail::g_iSearchInstance = i;

      o.RemoveObject(Objects[i].m_hObject);

      // one less in the tree
      XII_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == false);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveObject(index)")
  {
    xiiDynamicQuadtree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicQuadtreeTestDetail::g_iSearchInstance = i;

      o.RemoveObject(i, i + 1);

      // one less in the tree
      XII_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == false);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveObjectsOfType")
  {
    xiiDynamicQuadtree o;
    o.CreateTree(xiiVec3::MakeZero(), xiiVec3(100), 1.0f);

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      XII_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == XII_SUCCESS);

      XII_TEST_BOOL(o.IsEmpty() == false);
      XII_TEST_INT(o.GetCount(), i + 1);
    }

    for (xiiUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicQuadtreeTestDetail::g_iSearchInstance = i + 1;

      o.RemoveObjectsOfType(i);

      // one less in the tree
      XII_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicQuadtreeTestDetail::g_iReturned      = 0;
      DynamicQuadtreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicQuadtreeTestDetail::ObjectFound, nullptr);
      XII_TEST_BOOL(DynamicQuadtreeTestDetail::g_bFoundSearched == false);
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
    xiiDynamicQuadtree o;
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
