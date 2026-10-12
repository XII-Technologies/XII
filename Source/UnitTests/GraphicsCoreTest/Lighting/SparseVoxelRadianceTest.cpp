/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>

XII_CREATE_SIMPLE_TEST(Lighting, SparseVoxelRadiance)
{
  xiiSparseVoxelRadianceContext context;
  xiiUInt32       level = 0U;
  xiiVec3I32      cell;
  const xiiUInt64               packedKey = context.PackBrickKey(6U, xiiVec3I32(-1247, 991, -7));
  context.UnpackBrickKey(packedKey, level, cell);
  XII_TEST_INT(level, 6U);
  XII_TEST_INT(cell.x, -1247);
  XII_TEST_INT(cell.y, 991);
  XII_TEST_INT(cell.z, -7);

  xiiSparseVoxelRadianceSettings settings;
  settings.m_uiClipmapLevels          = 2U;
  settings.m_uiClipmapBrickResolution = 4U;
  settings.m_uiBrickVoxelResolution   = 8U;
  settings.m_uiMaxResidentBricks      = 128U;
  settings.m_uiBrickUpdateBudget      = 128U;
  settings.m_uiRefreshIntervalFrames  = 1000U;
  settings.m_fBaseVoxelSize           = 0.5f;
  settings.m_fTemporalHysteresis      = 0.9f;
  XII_TEST_BOOL(context.Initialize(settings).Succeeded());

  XII_TEST_RESULT(context.BeginFrame(xiiVec3::MakeZero(), 1U));
  XII_TEST_INT(context.GetFrameStats().m_uiRequiredBrickCount, 128U);
  XII_TEST_INT(context.GetFrameStats().m_uiResidentBrickCount, 128U);
  XII_TEST_INT(context.GetFrameStats().m_uiAllocatedBrickCount, 128U);
  XII_TEST_INT(context.GetScheduledUpdates().GetCount(), 128U);

  for (const xiiSparseVoxelBrickUpdate& update : context.GetScheduledUpdates())
    context.CommitBrickUpdate(update.m_uiPhysicalBrick, update.m_uiPackedKey, true);

  const xiiUInt64                 retainedKey    = context.PackBrickKey(1U, xiiVec3I32(-2, -2, -2));
  const xiiSparseVoxelBrickState* pRetainedBrick = context.FindBrick(retainedKey);
  XII_TEST_BOOL(pRetainedBrick != nullptr);
  const xiiUInt32 retainedPhysicalBrick = pRetainedBrick != nullptr ? pRetainedBrick->m_uiPhysicalBrick : xiiInvalidIndex;

  // One base-level brick step exposes one 4x4 slab in level zero. The coarser
  // level has not crossed its brick boundary and retains all of its history.
  XII_TEST_RESULT(context.BeginFrame(xiiVec3(4.1f, 0.0f, 0.0f), 2U));
  XII_TEST_INT(context.GetFrameStats().m_uiAllocatedBrickCount, 16U);
  XII_TEST_INT(context.GetFrameStats().m_uiEvictedBrickCount, 16U);
  XII_TEST_INT(context.GetFrameStats().m_uiDirtyBrickCount, 16U);
  XII_TEST_INT(context.GetScheduledUpdates().GetCount(), 16U);

  pRetainedBrick = context.FindBrick(retainedKey);
  XII_TEST_BOOL(pRetainedBrick != nullptr);
  XII_TEST_INT(pRetainedBrick != nullptr ? pRetainedBrick->m_uiPhysicalBrick : xiiInvalidIndex, retainedPhysicalBrick);
}

XII_CREATE_SIMPLE_TEST(Lighting, IndependentSparseVoxelContexts)
{
  xiiSparseVoxelRadianceContext first, second;
  XII_TEST_BOOL(!first.IsInitialized());
  xiiSparseVoxelRadianceSettings settings;
  settings.m_uiClipmapLevels          = 1;
  settings.m_uiClipmapBrickResolution = 2;
  settings.m_uiMaxResidentBricks = settings.m_uiBrickUpdateBudget = 8;
  XII_TEST_RESULT(first.Initialize(settings));
  XII_TEST_RESULT(second.Initialize(settings));
  XII_TEST_RESULT(first.BeginFrame(xiiVec3::MakeZero(), 1));
  for (const auto& update : first.GetScheduledUpdates())
    first.CommitBrickUpdate(update.m_uiPhysicalBrick, update.m_uiPackedKey, true);
  const auto retained = first.GetBricks()[0];
  XII_TEST_RESULT(second.BeginFrame(xiiVec3(1000, 2000, -3000), 1));
  for (const auto& update : second.GetScheduledUpdates())
    second.CommitBrickUpdate(update.m_uiPhysicalBrick, update.m_uiPackedKey, true);
  XII_TEST_BOOL(first.FindBrick(retained.m_uiPackedKey) != nullptr);
  XII_TEST_BOOL(second.FindBrick(retained.m_uiPackedKey) == nullptr);
  XII_TEST_BOOL(first.GetBricks()[0].m_Flags.IsSet(xiiSparseVoxelBrickFlags::Valid));
  XII_TEST_BOOL(first.BeginFrame(xiiVec3(5000), 1).Failed());
  XII_TEST_BOOL(first.FindBrick(retained.m_uiPackedKey) != nullptr);
  auto invalid                  = settings;
  invalid.m_uiMaxResidentBricks = 0;
  XII_TEST_BOOL(first.Initialize(invalid).Failed());
  XII_TEST_BOOL(first.FindBrick(retained.m_uiPackedKey) != nullptr);
  first.InvalidateHistory();
  XII_TEST_RESULT(first.BeginFrame(xiiVec3::MakeZero(), 2));
  XII_TEST_INT(first.GetFrameStats().m_uiDirtyBrickCount, 8);
  for (const auto& update : first.GetScheduledUpdates())
    XII_TEST_FLOAT(update.m_fHistoryWeight, 0, 0);
  XII_TEST_BOOL(second.GetBricks()[0].m_Flags.IsSet(xiiSparseVoxelBrickFlags::Valid));
  first.Shutdown();
  first.Shutdown();
  XII_TEST_BOOL(!first.IsInitialized());
  XII_TEST_BOOL(first.GetBricks().IsEmpty());
  XII_TEST_BOOL(second.IsInitialized());
  XII_TEST_RESULT(first.Initialize(settings));
  XII_TEST_RESULT(first.BeginFrame(xiiVec3::MakeZero(), 1));
  XII_TEST_INT(first.GetFrameStats().m_uiAllocatedBrickCount, 8);
}
