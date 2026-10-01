/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/VolumetricMedium.h>

XII_CREATE_SIMPLE_TEST_GROUP(VolumetricMedium);

XII_CREATE_SIMPLE_TEST(VolumetricMedium, StreamingHierarchy)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Generation checked registration")
  {
    XII_TEST_BOOL(xiiVolumetricMediumManager::IsSubsystemInitialized());

    xiiVolumetricMediumDescription description;
    description.m_vCenter      = xiiVec3(4.0f, 2.0f, 1.0f);
    description.m_vHalfExtents = xiiVec3(2.0f);
    description.m_vScattering  = xiiVec3(0.2f, 0.1f, 0.05f);
    description.m_vAbsorption  = xiiVec3(0.03f);
    description.m_iPriority    = 5;

    const xiiVolumetricMediumHandle handle = xiiVolumetricMediumManager::RegisterMedium(description);
    XII_TEST_BOOL(handle.IsValid());
    XII_TEST_BOOL(xiiVolumetricMediumManager::IsValid(handle));

    xiiGpuVolumetricMediumArray gpuMedia;
    xiiVolumetricMediumManager::GatherGpuMedia(xiiVec3::MakeZero(), gpuMedia);
    XII_TEST_INT(gpuMedia.GetCount(), 1);
    XII_TEST_FLOAT(gpuMedia[0].m_vScatteringAndPriority.x, 0.2f, 0.0f);

    xiiVolumetricMediumManager::UnregisterMedium(handle);
    XII_TEST_BOOL(!xiiVolumetricMediumManager::IsValid(handle));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Streaming cells preserve stable handles")
  {
    xiiVolumetricMediumDescription description;
    description.m_vCenter      = xiiVec3(8.0f, 8.0f, 8.0f);
    description.m_vHalfExtents = xiiVec3(1.0f);
    const xiiVolumetricMediumHandle handle = xiiVolumetricMediumManager::RegisterMedium(description);
    XII_TEST_BOOL(handle.IsValid());

    const xiiVec3I32 cell = xiiVec3I32::MakeZero();
    xiiVolumetricMediumManager::SetCellResident(cell, false);
    XII_TEST_BOOL(!xiiVolumetricMediumManager::IsCellResident(cell));

    xiiGpuVolumetricMediumArray gpuMedia;
    xiiVolumetricMediumManager::GatherGpuMedia(xiiVec3::MakeZero(), gpuMedia);
    XII_TEST_BOOL(gpuMedia.IsEmpty());
    XII_TEST_BOOL(xiiVolumetricMediumManager::IsValid(handle));

    xiiVolumetricMediumManager::SetCellResident(cell, true);
    xiiVolumetricMediumManager::GatherGpuMedia(xiiVec3::MakeZero(), gpuMedia);
    XII_TEST_INT(gpuMedia.GetCount(), 1);

    xiiVec3I32 unpacked;
    xiiVolumetricMediumManager::UnpackCellKey(xiiVolumetricMediumManager::PackCellKey(xiiVec3I32(-17, 23, 9)), unpacked);
    XII_TEST_BOOL(unpacked == xiiVec3I32(-17, 23, 9));

    xiiVolumetricMediumManager::UnregisterMedium(handle);
  }
}
