/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Scene/SceneDatabaseManager.h>
#include <GraphicsCore/Scene/SceneSpatialHierarchy.h>

XII_CREATE_SIMPLE_TEST_GROUP(Scene);

XII_CREATE_SIMPLE_TEST(Scene, DataOrientedScene)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LOD controls preserve identity and reject invalid mutations")
  {
    xiiSceneDatabaseContext context;
    XII_TEST_RESULT(context.Initialize());
    auto&              scene = context.GetDatabase();
    xiiSceneObjectDesc description;
    description.m_uiForcedLod = 3;
    description.m_uiDrawLod   = 2;
    description.m_fLodBias    = 1.5f;
    const auto object         = scene.CreateObject(description);
    scene.CommitFrame(1);
    const auto id = scene.GetGpuInstances()[0].m_uiObjectId;
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiForcedLod, 3);
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiDrawLod, 2);
    XII_TEST_FLOAT(scene.GetGpuInstances()[0].m_fLodBias, 1.5f, 0);
    const auto revision = scene.GetRevision();
    XII_TEST_RESULT(scene.SetLodSelection(object, 3, 1.5f, 2));
    XII_TEST_INT(scene.GetRevision(), revision);
    XII_TEST_BOOL(scene.SetLodSelection({}, 0, 0).Failed());
    XII_TEST_BOOL(scene.SetLodSelection(object, 8, 0).Failed());
    XII_TEST_BOOL(scene.SetLodSelection(object, 0, 0, 8).Failed());
    XII_TEST_BOOL(scene.SetLodSelection(object, 0, xiiMath::NaN<float>()).Failed());
    XII_TEST_BOOL(scene.SetLodSelection(object, 0, xiiMath::Infinity<float>()).Failed());
    XII_TEST_INT(scene.GetRevision(), revision);
    description.m_uiDrawLod = 8;
    XII_TEST_BOOL(scene.CreateObject(description).IsInvalidated());
    description.m_uiDrawLod   = 2;
    description.m_uiForcedLod = 8;
    XII_TEST_BOOL(scene.CreateObject(description).IsInvalidated());
    description.m_uiForcedLod = 0;
    description.m_fLodBias    = xiiMath::NaN<float>();
    XII_TEST_BOOL(scene.CreateObject(description).IsInvalidated());
    XII_TEST_INT(scene.GetObjectCount(), 1);
    XII_TEST_RESULT(scene.SetLodSelection(object, xiiInvalidIndex, -2));
    scene.CommitFrame(2);
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiObjectId, id);
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiForcedLod, xiiInvalidIndex);
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiDrawLod, xiiInvalidIndex);
    XII_TEST_FLOAT(scene.GetGpuInstances()[0].m_fLodBias, -2, 0);
    XII_TEST_BOOL(!scene.GetUploadRanges().IsEmpty());
    XII_TEST_BOOL(scene.DestroyObject(object));
    XII_TEST_BOOL(scene.SetLodSelection(object, 0, 0).Failed());
    scene.Clear();
    const auto replacement = scene.CreateObject(xiiSceneObjectDesc{});
    scene.CommitFrame(3);
    XII_TEST_BOOL(!replacement.IsInvalidated());
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiForcedLod, xiiInvalidIndex);
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiDrawLod, xiiInvalidIndex);
    XII_TEST_FLOAT(scene.GetGpuInstances()[0].m_fLodBias, 0, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LOD groups follow dense compaction and never reuse a destroyed representative")
  {
    xiiSceneDatabaseContext context;
    XII_TEST_RESULT(context.Initialize());
    auto&      scene          = context.GetDatabase();
    const auto unrelated      = scene.CreateObject({});
    const auto member         = scene.CreateObject({});
    const auto representative = scene.CreateObject({}); // Deliberately follows the member.
    XII_TEST_RESULT(scene.SetLodGroup(member, representative));
    scene.CommitFrame(1);
    XII_TEST_INT(scene.GetGpuInstances()[1].m_uiLodSelectionIndex, 2);
    const auto revision = scene.GetRevision();
    XII_TEST_RESULT(scene.SetLodGroup(member, representative));
    XII_TEST_INT(scene.GetRevision(), revision);
    XII_TEST_BOOL(scene.SetLodGroup({}, representative).Failed());
    XII_TEST_BOOL(scene.DestroyObject(unrelated));
    scene.CommitFrame(2);
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiLodSelectionIndex, 1);
    scene.CommitFrame(3);
    XII_TEST_BOOL(scene.GetUploadRanges().IsEmpty());
    XII_TEST_BOOL(scene.DestroyObject(representative));
    scene.CommitFrame(4);
    XII_TEST_INT(scene.GetGpuInstances()[0].m_uiLodSelectionIndex, xiiInvalidIndex);
    const auto replacement = scene.CreateObject({});
    XII_TEST_BOOL(replacement != representative);
    XII_TEST_BOOL(scene.SetLodGroup(member, representative).Failed());
    auto gpuIndex = [&](xiiSceneObjectHandle handle) {
      const auto instances = scene.GetGpuInstances();
      for (xiiUInt32 i = 0; i < instances.GetCount(); ++i)
        if (instances[i].m_uiObjectId == handle.GetInternalID().m_Data + 1U)
          return i;
      return xiiInvalidIndex;
    };
    scene.CommitFrame(5);
    XII_TEST_INT(scene.GetGpuInstances()[gpuIndex(member)].m_uiLodSelectionIndex, xiiInvalidIndex);
    XII_TEST_RESULT(scene.SetLodGroup(member, replacement));
    scene.CommitFrame(6);
    XII_TEST_INT(scene.GetGpuInstances()[gpuIndex(member)].m_uiLodSelectionIndex, gpuIndex(replacement));
    XII_TEST_RESULT(scene.SetLodGroup(member, member));
    scene.CommitFrame(7);
    XII_TEST_INT(scene.GetGpuInstances()[gpuIndex(member)].m_uiLodSelectionIndex, xiiInvalidIndex);
  }

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
