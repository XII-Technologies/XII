/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GraphicsCore/Components/Fog/VolumetricMediumComponent.h>
#include <GraphicsCore/Lighting/VolumetricMedium.h>

XII_CREATE_SIMPLE_TEST_GROUP(VolumetricMedium);

XII_CREATE_SIMPLE_TEST(VolumetricMedium, StreamingHierarchy)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Generation checked registration")
  {
    XII_TEST_BOOL(xiiVolumetricMediumManager::IsSubsystemInitialized());

    xiiVolumetricMediumDescription description;
    description.m_vCenter      = xiiVec3(4.0f, 2.0f, 1.0f);
    description.m_qRotation    = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0.0f, 1.0f, 0.0f), xiiAngle::MakeFromDegree(30.0f));
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
    XII_TEST_FLOAT(gpuMedia[0].m_vRotation.w, description.m_qRotation.w, 0.0f);

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "World component owns subsystem registration")
  {
    xiiWorldDescription worldDescription("Volumetric Medium Test");
    xiiWorld            world(worldDescription);
    XII_LOCK(world.GetWriteMarker());

    xiiGameObjectDescription objectDescription;
    objectDescription.m_bDynamic      = true;
    objectDescription.m_LocalPosition = xiiVec3(3.0f, 4.0f, 5.0f);
    objectDescription.m_LocalRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0.0f, 0.0f, 1.0f), xiiAngle::MakeFromDegree(45.0f));

    xiiGameObject* pObject = nullptr;
    world.CreateObject(objectDescription, pObject);
    xiiVolumetricMediumComponent* pComponent = nullptr;
    xiiVolumetricMediumComponent::CreateComponent(pObject, pComponent);
    pComponent->SetHalfExtents(xiiVec3(2.0f, 1.0f, 0.5f));
    world.Update();

    XII_TEST_INT(xiiVolumetricMediumManager::GetStats().m_uiRegisteredMedia, 1);
    xiiGpuVolumetricMediumArray gpuMedia;
    xiiVolumetricMediumManager::GatherGpuMedia(objectDescription.m_LocalPosition, gpuMedia);
    XII_TEST_INT(gpuMedia.GetCount(), 1);
    XII_TEST_VEC3(gpuMedia[0].m_vCenterAndShape.GetAsVec3(), objectDescription.m_LocalPosition, 0.0f);

    const xiiVec3 vMovedPosition(70.0f, 4.0f, 5.0f);
    pObject->SetGlobalPosition(vMovedPosition);
    world.Update();
    xiiVolumetricMediumManager::GatherGpuMedia(vMovedPosition, gpuMedia);
    XII_TEST_INT(gpuMedia.GetCount(), 1);
    XII_TEST_VEC3(gpuMedia[0].m_vCenterAndShape.GetAsVec3(), vMovedPosition, 0.0f);

    pObject->SetActiveFlag(false);
    world.Update();
    XII_TEST_INT(xiiVolumetricMediumManager::GetStats().m_uiRegisteredMedia, 0);
  }
}
