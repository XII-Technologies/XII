/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Scene/SceneDatabaseManager.h>
#include <GraphicsCore/Scene/SceneSpatialHierarchy.h>

XII_CREATE_SIMPLE_TEST_GROUP(Scene);

XII_CREATE_SIMPLE_TEST(Scene, DataOrientedScene)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GPU identity survives compaction and rejects retired generations")
  {
    xiiSceneDatabaseContext context;
    XII_TEST_RESULT(context.Initialize());
    auto&              scene = context.GetDatabase();
    xiiSceneObjectDesc description;
    description.m_uiUserData = 0U; // Legal application data, independent of the background ID.
    auto first               = scene.CreateObject(description);
    auto second              = scene.CreateObject(description);
    scene.CommitFrame(1);
    const auto firstId  = scene.GetGpuInstances()[0].m_uiObjectId;
    const auto secondId = scene.GetGpuInstances()[1].m_uiObjectId;
    XII_TEST_BOOL(firstId != 0 && firstId != secondId);
    XII_TEST_BOOL(scene.ResolveGpuObjectId(firstId) == first);
    XII_TEST_BOOL(scene.ResolveGpuObjectId(secondId) == second);
    XII_TEST_BOOL(scene.ResolveGpuObjectId(0).IsInvalidated());
    XII_TEST_BOOL(scene.ResolveGpuObjectId(xiiInvalidIndex).IsInvalidated());
    XII_TEST_BOOL(scene.DestroyObject(first));
    scene.CommitFrame(2);
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiObjectId, secondId);
    XII_TEST_BOOL(scene.ResolveGpuObjectId(firstId).IsInvalidated());
    // Clear retains the ID table's generations even when the hot arrays are rebuilt.
    scene.Clear();
    XII_TEST_BOOL(scene.ResolveGpuObjectId(secondId).IsInvalidated());
    auto replacement = scene.CreateObject(description);
    scene.CommitFrame(3);
    XII_TEST_BOOL(replacement.GetInternalID().m_InstanceIndex == first.GetInternalID().m_InstanceIndex);
    XII_TEST_BOOL(scene.GetGpuInstances()[0].m_uiObjectId != firstId);
    XII_TEST_BOOL(scene.ResolveGpuObjectId(firstId).IsInvalidated());
    XII_TEST_BOOL(scene.ResolveGpuObjectId(scene.GetGpuInstances()[0].m_uiObjectId) == replacement);
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiUserData, 0U);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Hierarchy transforms and previous frame state")
  {
    xiiSceneDatabaseContext sceneContext;
    XII_TEST_BOOL(sceneContext.Initialize().Succeeded());
    xiiSceneDatabase& scene = sceneContext.GetDatabase();

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

    XII_TEST_BOOL(!parent.IsInvalidated());
    XII_TEST_BOOL(!child.IsInvalidated());
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
      if (scene.ResolveGpuObjectId(instance.m_uiObjectId) == child)
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
    sceneContext.Shutdown();
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
      handles[i]                  = xiiSceneObjectHandle(xiiSceneObjectId(i, 1U));
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
