/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Lighting/RayTracingScene.h>

XII_CREATE_SIMPLE_TEST(Lighting, RayTracingScene)
{
  xiiRayTracingSceneDescription settings;
  settings.m_uiMaxGeometries  = 4U;
  settings.m_uiMaxInstances   = 8U;
  settings.m_uiFramesInFlight = 2U;
  XII_TEST_BOOL(xiiRayTracingSceneManager::Configure(settings).Succeeded());
  XII_TEST_BOOL(xiiRayTracingSceneManager::IsInitialized());

  xiiRayTracingGeometryDescription geometryDescription;
  geometryDescription.m_hMeshBuffer           = xiiResourceManager::LoadResource<xiiMeshBufferResource>("UnitTests/RayTracingScene/Mesh");
  const xiiRayTracingGeometryHandle hGeometry = xiiRayTracingSceneManager::RegisterGeometry(geometryDescription);
  XII_TEST_BOOL(hGeometry.IsValid());
  XII_TEST_BOOL(xiiRayTracingSceneManager::IsValid(hGeometry));

  xiiRayTracingInstanceDescription instanceDescription;
  instanceDescription.m_hGeometry             = hGeometry;
  instanceDescription.m_uiStableObjectId      = 42U;
  const xiiRayTracingInstanceHandle hInstance = xiiRayTracingSceneManager::CreateInstance(instanceDescription);
  XII_TEST_BOOL(hInstance.IsValid());
  XII_TEST_BOOL(xiiRayTracingSceneManager::IsValid(hInstance));

  xiiRayTracingSceneStats stats = xiiRayTracingSceneManager::GetStats();
  XII_TEST_INT(stats.m_uiGeometryCount, 1U);
  XII_TEST_INT(stats.m_uiInstanceCount, 1U);
  XII_TEST_INT(stats.m_uiPendingBLASCount, 1U);

  instanceDescription.m_Transform.SetTranslationVector(xiiVec3(1.0f, 2.0f, 3.0f));
  XII_TEST_BOOL(xiiRayTracingSceneManager::UpdateInstance(hInstance, instanceDescription));

  // Removing a BLAS source atomically invalidates all dependent TLAS instances.
  xiiRayTracingSceneManager::UnregisterGeometry(hGeometry);
  XII_TEST_BOOL(!xiiRayTracingSceneManager::IsValid(hGeometry));
  XII_TEST_BOOL(!xiiRayTracingSceneManager::IsValid(hInstance));
  stats = xiiRayTracingSceneManager::GetStats();
  XII_TEST_INT(stats.m_uiGeometryCount, 0U);
  XII_TEST_INT(stats.m_uiInstanceCount, 0U);

  XII_TEST_BOOL(xiiRayTracingSceneManager::Configure(xiiRayTracingSceneDescription()).Succeeded());
}
