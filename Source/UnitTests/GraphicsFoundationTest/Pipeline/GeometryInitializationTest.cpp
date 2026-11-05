/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsFoundation/Resources/Buffer.h>

namespace
{
  class GeometryInitializer final : public xiiThread
  {
  public:
    explicit GeometryInitializer(xiiThreadSignal& start) : xiiThread("Concurrent geometry initialization"), m_Start(start) {}
    xiiThreadSignal            m_Ready;
    bool                       m_bSucceeded = true;
    xiiSharedPtr<xiiGALBuffer> m_Metadata, m_Meshlets;

  private:
    xiiUInt32 Run() override
    {
      m_Ready.RaiseSignal();
      m_Start.WaitForSignal();
      for (xiiUInt32 iteration = 0; iteration < 32; ++iteration)
      {
        if (xiiGeometryResidencyManager::EnsureInitialized().Failed())
        {
          m_bSucceeded = false;
          break;
        }
        auto metadata = xiiGeometryResidencyManager::GetMetadataBuffer();
        auto meshlets = xiiGeometryResidencyManager::GetMeshletMetadataBuffer();
        m_bSucceeded &= xiiGeometryResidencyManager::IsInitialized() && metadata && meshlets;
        if (iteration)
          m_bSucceeded &= metadata == m_Metadata && meshlets == m_Meshlets;
        m_Metadata = metadata;
        m_Meshlets = meshlets;
      }
      return 0;
    }
    xiiThreadSignal& m_Start;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(PipelineResources, ConcurrentGeometryInitialization)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized());
  xiiGeometryResidencyDescription description;
  description.m_uiMaxGeometries = 64;
  description.m_uiMaxMeshlets   = 64;
  XII_TEST_RESULT(xiiGeometryResidencyManager::Configure(description));
  xiiThreadSignal                   start(xiiThreadSignal::Mode::ManualReset);
  xiiUniquePtr<GeometryInitializer> initializers[4];
  for (auto& initializer : initializers)
  {
    initializer = XII_DEFAULT_NEW(GeometryInitializer, start);
    initializer->Start();
  }
  for (const auto& initializer : initializers)
    XII_TEST_BOOL(initializer->m_Ready.WaitForSignal(xiiTime::MakeFromSeconds(5)) == xiiThreadSignal::WaitResult::Signaled);
  start.RaiseSignal();
  for (auto& initializer : initializers)
    initializer->Join();
  for (const auto& initializer : initializers)
  {
    XII_TEST_BOOL(initializer->m_bSucceeded);
    XII_TEST_BOOL(initializer->m_Metadata == initializers[0]->m_Metadata);
    XII_TEST_BOOL(initializer->m_Meshlets == initializers[0]->m_Meshlets);
    XII_TEST_INT(initializer->m_Metadata->GetDescription().m_uiSize, sizeof(xiiGpuGeometryRecord) * description.m_uiMaxGeometries * description.m_uiFramesInFlight);
  }
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiGeometryCount, 0);
}

XII_CREATE_SIMPLE_TEST(PipelineResources, OversizedGeometryResidency)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiGeometryResidencyDescription configuration;
  configuration.m_uiMaxGeometries = 64;
  configuration.m_uiMaxMeshlets   = 64;
  if (!XII_TEST_RESULT(xiiGeometryResidencyManager::Configure(configuration)) || !XII_TEST_RESULT(xiiGeometryResidencyManager::EnsureInitialized()))
    return;
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshBufferResourceDescriptor descriptor;
  descriptor.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, true);
  const xiiUInt64        bytes  = descriptor.GetVertexDataSize() + descriptor.GetIndexDataSize() + descriptor.m_Meshlets.GetCount() * sizeof(xiiMeshlet);
  const auto             buffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Oversized residency cube", std::move(descriptor));
  xiiGeometryDescription geometry;
  geometry.m_Lods.ExpandAndGetRef().m_hMeshBuffer = buffer;
  xiiGeometryHandle handles[3];
  for (auto& handle : handles)
  {
    handle = xiiGeometryResidencyManager::RegisterGeometry(geometry);
    XII_TEST_BOOL(!handle.IsInvalidated());
    xiiGeometryResidencyManager::RequestResidency(handle, 0, 0);
  }
  XII_SCOPE_EXIT(for (const auto handle : handles) xiiGeometryResidencyManager::UnregisterGeometry(handle, 0));
  // A zero budget pauses streaming. Positive budgets must eventually admit an
  // indivisible record even when it exceeds the whole frame allowance.
  xiiGeometryResidencyManager::ProcessStreaming(0, 0, 0);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, 0);
  for (xiiUInt32 frame = 1; frame <= 3; ++frame)
  {
    xiiGeometryResidencyManager::ProcessStreaming(frame, 0, 1);
    XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, frame);
    XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentBytes, bytes * frame);
  }
  for (const auto handle : handles)
  {
    XII_TEST_INT(xiiGeometryResidencyManager::GetState(handle), xiiGeometryResidencyState::Resident);
    const auto* record = xiiGeometryResidencyManager::GetGpuRecord(handle);
    if (XII_TEST_BOOL(record != nullptr))
      XII_TEST_INT(record->m_uiResidentLodMask, 1);
  }

  // An ordinary record consumes the allowance: another record must wait when
  // only part of its cost remains, then load with an exact-fit allowance.
  xiiGeometryHandle ordinary[2];
  for (auto& handle : ordinary)
  {
    handle = xiiGeometryResidencyManager::RegisterGeometry(geometry);
    xiiGeometryResidencyManager::RequestResidency(handle, 0, 4);
  }
  XII_SCOPE_EXIT(for (const auto handle : ordinary) xiiGeometryResidencyManager::UnregisterGeometry(handle, 0));
  xiiGeometryResidencyManager::ProcessStreaming(4, 0, bytes + 1);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, 4);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentBytes, bytes * 4);
  xiiGeometryResidencyManager::ProcessStreaming(5, 0, bytes);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, 5);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentBytes, bytes * 5);
}
