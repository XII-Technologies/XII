/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Scene/SceneDatabase.h>
#include <GraphicsCore/Scene/SceneSpatialHierarchy.h>

XII_CREATE_SIMPLE_TEST_GROUP(Scene);

XII_CREATE_SIMPLE_TEST(Scene, DataOrientedScene)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Hierarchy transforms and previous frame state")
  {
    xiiSceneDatabase scene;

    xiiSceneObjectDesc parentDesc;
    parentDesc.m_LocalTransform       = xiiMat4::MakeTranslation(xiiVec3(10.0f, 0.0f, 0.0f));
    parentDesc.m_LocalBounds          = xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), xiiVec3(0.5f), 0.9f);
    const xiiSceneObjectHandle parent = scene.CreateObject(parentDesc);

    xiiSceneObjectDesc childDesc;
    childDesc.m_hParent              = parent;
    childDesc.m_LocalTransform       = xiiMat4::MakeTranslation(xiiVec3(0.0f, 2.0f, 0.0f));
    childDesc.m_LocalBounds          = xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), xiiVec3(0.25f), 0.45f);
    childDesc.m_uiGeometryIndex      = 7U;
    childDesc.m_uiMaterialIndex      = 11U;
    const xiiSceneObjectHandle child = scene.CreateObject(childDesc);

    XII_TEST_BOOL(parent.IsValid());
    XII_TEST_BOOL(child.IsValid());
    XII_TEST_BOOL(!scene.SetParent(parent, child));

    scene.CommitFrame(1U);
    XII_TEST_VEC3(scene.GetGlobalTransform(child).GetTranslationVector(), xiiVec3(10.0f, 2.0f, 0.0f), 0.0001f);
    XII_TEST_INT(scene.GetGpuInstances().GetCount(), 2U);

    XII_TEST_BOOL(scene.SetLocalTransform(parent, xiiMat4::MakeTranslation(xiiVec3(20.0f, 0.0f, 0.0f))));
    scene.CommitFrame(2U);
    XII_TEST_VEC3(scene.GetGlobalTransform(child).GetTranslationVector(), xiiVec3(20.0f, 2.0f, 0.0f), 0.0001f);

    const xiiGpuSceneInstance* pChildInstance = nullptr;
    for (const xiiGpuSceneInstance& instance : scene.GetGpuInstances())
    {
      if (instance.m_uiObjectIndex == child.m_uiIndex)
      {
        pChildInstance = &instance;
        break;
      }
    }
    XII_TEST_BOOL(pChildInstance != nullptr);
    if (pChildInstance != nullptr)
    {
      XII_TEST_VEC3(pChildInstance->m_PreviousGlobalTransform.GetTranslationVector(), xiiVec3(10.0f, 2.0f, 0.0f), 0.0001f);
      XII_TEST_INT(pChildInstance->m_uiGeometryIndex, 7U);
      XII_TEST_INT(pChildInstance->m_uiMaterialIndex, 11U);
    }

    XII_TEST_BOOL(scene.DestroyObject(parent));
    XII_TEST_BOOL(scene.IsAlive(child));
    scene.CommitFrame(3U);
    XII_TEST_VEC3(scene.GetGlobalTransform(child).GetTranslationVector(), xiiVec3(20.0f, 2.0f, 0.0f), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Balanced ordered spatial hierarchy")
  {
    constexpr xiiUInt32      uiObjectCount = 4096U;
    xiiSceneSpatialHierarchy hierarchy(0.05f);
    hierarchy.Reserve(uiObjectCount);

    xiiDynamicArray<xiiSceneObjectHandle> handles;
    handles.SetCount(uiObjectCount);
    for (xiiUInt32 i = 0U; i < uiObjectCount; ++i)
    {
      handles[i].m_uiIndex        = i;
      handles[i].m_uiGeneration   = 1U;
      const xiiBoundingBox bounds = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3(static_cast<float>(i), 0.0f, 0.0f), xiiVec3(0.25f));
      XII_TEST_BOOL(hierarchy.Insert(handles[i], bounds, 1U, xiiSceneObjectFlags::Enabled));
    }

    xiiSceneSpatialStats stats = hierarchy.GetStats();
    XII_TEST_INT(stats.m_uiLeafCount, uiObjectCount);
    XII_TEST_INT(stats.m_uiNodeCount, uiObjectCount * 2U - 1U);
    XII_TEST_BOOL(stats.m_uiTreeHeight < 32U);
    XII_TEST_BOOL(stats.m_uiRotations > 0U);

    xiiSceneSpatialQuery query;
    query.m_uiVisibilityMask = 1U;
    query.m_RequiredFlags    = xiiSceneObjectFlags::Enabled;
    xiiDynamicArray<xiiSceneObjectHandle> results;
    hierarchy.QueryBox(xiiBoundingBox::MakeFromMinMax(xiiVec3(99.5f, -1.0f, -1.0f), xiiVec3(199.5f, 1.0f, 1.0f)), query, results);
    XII_TEST_INT(results.GetCount(), 100U);

    for (xiiUInt32 i = 0U; i < uiObjectCount; i += 2U)
      XII_TEST_BOOL(hierarchy.Remove(handles[i]));

    results.Clear();
    hierarchy.QueryBox(xiiBoundingBox::MakeFromMinMax(xiiVec3(-1.0f), xiiVec3(static_cast<float>(uiObjectCount) + 1.0f)), query, results);
    XII_TEST_INT(results.GetCount(), uiObjectCount / 2U);
    stats = hierarchy.GetStats();
    XII_TEST_BOOL(stats.m_uiTreeHeight < 32U);
  }
}
