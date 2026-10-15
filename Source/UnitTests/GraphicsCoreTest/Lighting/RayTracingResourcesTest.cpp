/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

XII_CREATE_SIMPLE_TEST(Lighting, RayTracingResources)
{
  xiiRayTracingResourceLimits settings;
  settings.m_uiMaxGeometries  = 4;
  settings.m_uiMaxInstances   = 8;
  settings.m_uiFramesInFlight = 2;
  XII_TEST_RESULT(xiiRayTracingResources::Configure(settings));
  xiiRayTracingGeometryDescription geometry;
  geometry.m_hMeshBuffer = xiiResourceManager::LoadResource<xiiMeshBufferResource>("UnitTests/RayTracingResources/Mesh");
  const auto first       = xiiRayTracingResources::AcquireGeometry(geometry);
  const auto shared      = xiiRayTracingResources::AcquireGeometry(geometry);
  XII_TEST_BOOL(!first.IsInvalidated() && first == shared);
  XII_TEST_INT(xiiRayTracingResources::GetStats().m_uiGeometryCount, 1);
  xiiRayTracingResources::ReleaseGeometry(first);
  XII_TEST_BOOL(xiiRayTracingResources::IsValid(shared));
  xiiRayTracingResources::ReleaseGeometry(shared);
  XII_TEST_BOOL(!xiiRayTracingResources::IsValid(first));
  const auto replacement = xiiRayTracingResources::AcquireGeometry(geometry);
  XII_TEST_BOOL(replacement != first);
  xiiRayTracingResources::ReleaseGeometry(replacement);
  XII_TEST_INT(xiiRayTracingResources::GetStats().m_uiGeometryCount, 0);
  XII_TEST_RESULT(xiiRayTracingResources::Configure({}));
}
