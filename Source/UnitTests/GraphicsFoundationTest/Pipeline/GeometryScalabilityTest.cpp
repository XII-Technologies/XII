/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>

XII_CREATE_SIMPLE_TEST(PipelineResources, GeometryCoverageScaling)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshBufferResourceDescriptor descriptor;
  descriptor.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, true);
  const auto                indices  = descriptor.m_IndexData;
  const auto                meshlets = descriptor.m_Meshlets;
  constexpr xiiUInt32       copies   = 16384;
  xiiDynamicArray<xiiUInt8> repeated;
  repeated.SetCountUninitialized(indices.GetCount() * copies);
  for (xiiUInt32 i = 0; i < copies; ++i)
    xiiMemoryUtils::Copy(repeated.GetData() + i * indices.GetCount(), indices.GetData(), indices.GetCount());
  descriptor.SetIndexData(repeated, descriptor.GetIndexCount() * copies, descriptor.m_IndexType);
  descriptor.m_Meshlets.SetCount(meshlets.GetCount() * copies);
  for (xiiUInt32 i = 0; i < copies; ++i)
    for (xiiUInt32 m = 0; m < meshlets.GetCount(); ++m)
    {
      auto& meshlet = descriptor.m_Meshlets[i * meshlets.GetCount() + m];
      meshlet       = meshlets[m];
      meshlet.m_uiFirstPrimitive += i * 12;
    }
  const auto             buffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Coverage stress mesh", std::move(descriptor));
  xiiGeometryDescription description;
  description.m_Lods.ExpandAndGetRef().m_hMeshBuffer = buffer;
  const auto handle                                  = xiiGeometryResidencyManager::RegisterGeometry(description);
  XII_SCOPE_EXIT(xiiGeometryResidencyManager::UnregisterGeometry(handle, 0));
  xiiUInt32  counts[xiiGpuGeometryRecord::s_uiMaxLods] = {};
  const auto start                                     = xiiTime::Now();
  for (xiiUInt32 frame = 0; frame < 16; ++frame)
    for (xiiUInt32 section = 0; section < 512; ++section)
    {
      XII_TEST_RESULT(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(handle, section * 32 * 12 + 1, 32 * 12 - 2, xiiMakeArrayPtr(counts)));
      XII_TEST_INT(counts[0], 32 * meshlets.GetCount());
    }
  xiiTestFramework::Output(xiiTestOutput::Details, "Coverage: %u meshlets, 512 ranges, 16 updates: %.3f ms.", copies * meshlets.GetCount(), (xiiTime::Now() - start).GetMilliseconds());
}
