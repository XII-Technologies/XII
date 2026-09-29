/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>

XII_CREATE_SIMPLE_TEST(Lighting, SparseVoxelRadiance)
{
  xiiUInt32 level = 0U;
  xiiVec3I32 cell;
  const xiiUInt64 packedKey = xiiSparseVoxelRadianceManager::PackBrickKey(6U, xiiVec3I32(-1247, 991, -7));
  xiiSparseVoxelRadianceManager::UnpackBrickKey(packedKey, level, cell);
  XII_TEST_INT(level, 6U);
  XII_TEST_INT(cell.x, -1247);
  XII_TEST_INT(cell.y, 991);
  XII_TEST_INT(cell.z, -7);

  xiiSparseVoxelRadianceSettings settings;
  settings.m_uiClipmapLevels = 2U;
  settings.m_uiClipmapBrickResolution = 4U;
  settings.m_uiBrickVoxelResolution = 8U;
  settings.m_uiMaxResidentBricks = 128U;
  settings.m_uiBrickUpdateBudget = 128U;
  settings.m_uiRefreshIntervalFrames = 1000U;
  settings.m_fBaseVoxelSize = 0.5f;
  settings.m_fTemporalHysteresis = 0.9f;
  XII_TEST_BOOL(xiiSparseVoxelRadianceManager::Configure(settings).Succeeded());

  xiiSparseVoxelRadianceManager::BeginFrame(xiiVec3::MakeZero(), 1U);
  XII_TEST_INT(xiiSparseVoxelRadianceManager::GetFrameStats().m_uiRequiredBrickCount, 128U);
  XII_TEST_INT(xiiSparseVoxelRadianceManager::GetFrameStats().m_uiResidentBrickCount, 128U);
  XII_TEST_INT(xiiSparseVoxelRadianceManager::GetFrameStats().m_uiAllocatedBrickCount, 128U);
  XII_TEST_INT(xiiSparseVoxelRadianceManager::GetScheduledUpdates().GetCount(), 128U);

  for (const xiiSparseVoxelBrickUpdate& update : xiiSparseVoxelRadianceManager::GetScheduledUpdates())
    xiiSparseVoxelRadianceManager::CommitBrickUpdate(update.m_uiPhysicalBrick, update.m_uiPackedKey, true);

  const xiiUInt64 retainedKey = xiiSparseVoxelRadianceManager::PackBrickKey(1U, xiiVec3I32(-2, -2, -2));
  const xiiSparseVoxelBrickState* pRetainedBrick = xiiSparseVoxelRadianceManager::FindBrick(retainedKey);
  XII_TEST_BOOL(pRetainedBrick != nullptr);
  const xiiUInt32 retainedPhysicalBrick = pRetainedBrick != nullptr ? pRetainedBrick->m_uiPhysicalBrick : xiiInvalidIndex;

  // One base-level brick step exposes one 4x4 slab in level zero. The coarser
  // level has not crossed its brick boundary and retains all of its history.
  xiiSparseVoxelRadianceManager::BeginFrame(xiiVec3(4.1f, 0.0f, 0.0f), 2U);
  XII_TEST_INT(xiiSparseVoxelRadianceManager::GetFrameStats().m_uiAllocatedBrickCount, 16U);
  XII_TEST_INT(xiiSparseVoxelRadianceManager::GetFrameStats().m_uiEvictedBrickCount, 16U);
  XII_TEST_INT(xiiSparseVoxelRadianceManager::GetFrameStats().m_uiDirtyBrickCount, 16U);
  XII_TEST_INT(xiiSparseVoxelRadianceManager::GetScheduledUpdates().GetCount(), 16U);

  pRetainedBrick = xiiSparseVoxelRadianceManager::FindBrick(retainedKey);
  XII_TEST_BOOL(pRetainedBrick != nullptr);
  XII_TEST_INT(pRetainedBrick != nullptr ? pRetainedBrick->m_uiPhysicalBrick : xiiInvalidIndex, retainedPhysicalBrick);

  XII_TEST_BOOL(xiiSparseVoxelRadianceManager::Configure(xiiSparseVoxelRadianceSettings()).Succeeded());
}
