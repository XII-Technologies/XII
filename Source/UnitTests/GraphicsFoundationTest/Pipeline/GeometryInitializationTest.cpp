/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
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
  const xiiUInt64 uiArenaBytes    = descriptor.m_Meshlets.GetCount() * sizeof(xiiMeshlet);
  const xiiUInt64 bytes           = uiArenaBytes + sizeof(xiiGpuGeometryRecord);
  const xiiUInt64 uiTopologyBytes = descriptor.m_MeshletVertexRemap.GetCount() * sizeof(xiiUInt32) +
    xiiMath::RoundUp(descriptor.m_MeshletPrimitiveIndices.GetCount(), 4U);
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
  xiiGeometryResidencyManager::ProcessStreaming(0, 0, bytes * 10);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, 0);
  for (xiiUInt32 frame = 1; frame <= 3; ++frame)
  {
    xiiGeometryResidencyManager::ProcessStreaming(frame, 0, 1);
    XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, frame);
    XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentBytes, uiArenaBytes * frame);
    const auto stats = xiiGeometryResidencyManager::GetStats();
    XII_TEST_INT(stats.m_uiAdmittedBytes, bytes + (frame == 1U ? uiTopologyBytes : 0U));
    XII_TEST_INT(stats.m_uiResidentTopologyBytes, uiTopologyBytes);
    XII_TEST_INT(stats.m_uiRemainingUploadBytes, 0);
    // A second world/view in the same device frame cannot grant another allowance.
    xiiGeometryResidencyManager::ProcessStreaming(frame, 0, bytes * 10);
    XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, frame);
    XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiPreparedMeshlets, stats.m_uiPreparedMeshlets);
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
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentBytes, uiArenaBytes * 4);
  const auto prepared = xiiGeometryResidencyManager::GetStats().m_uiPreparedMeshlets;
  xiiGeometryResidencyManager::ProcessStreaming(4, 0, bytes * 10);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, 4);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiPreparedMeshlets, prepared);
  xiiGeometryResidencyManager::ProcessStreaming(5, 0, bytes);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, 5);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentBytes, uiArenaBytes * 5);
}

XII_CREATE_SIMPLE_TEST(PipelineResources, SharedTopologyRetirement)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  auto* pDevice = environment.GetDevice();
  if (pDevice->GetGraphicsDeviceType() != xiiGALGraphicsDeviceType::Vulkan)
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(pDevice->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshBufferResourceDescriptor descriptor;
  descriptor.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, true);
  const xiiUInt64 uiTopologyBytes = descriptor.m_MeshletVertexRemap.GetCount() * sizeof(xiiUInt32) +
    xiiMath::RoundUp(descriptor.m_MeshletPrimitiveIndices.GetCount(), 4U);
  xiiGeometryResidencyDescription configuration;
  configuration.m_uiMaxGeometries = 16U;
  configuration.m_uiMaxMeshlets   = 2U * descriptor.m_Meshlets.GetCount();
  if (!XII_TEST_RESULT(xiiGeometryResidencyManager::Configure(configuration)) || !XII_TEST_RESULT(xiiGeometryResidencyManager::EnsureInitialized()))
    return;

  const auto             hMesh = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Shared topology cube", std::move(descriptor));
  xiiGeometryDescription geometry;
  geometry.m_Lods.ExpandAndGetRef().m_hMeshBuffer = hMesh;
  const auto hFirst                               = xiiGeometryResidencyManager::RegisterGeometry(geometry);
  const auto hSecond                              = xiiGeometryResidencyManager::RegisterGeometry(geometry);
  xiiGeometryResidencyManager::RequestResidency(hFirst, 0U, 1U);
  xiiGeometryResidencyManager::RequestResidency(hSecond, 0U, 1U);
  xiiGeometryResidencyManager::ProcessStreaming(1U, 0U, configuration.m_uiUploadBudgetPerFrameBytes);

  const auto* pFirst  = xiiGeometryResidencyManager::GetGpuRecord(hFirst);
  const auto* pSecond = xiiGeometryResidencyManager::GetGpuRecord(hSecond);
  if (!XII_TEST_BOOL(pFirst != nullptr && pSecond != nullptr))
    return;
  XII_TEST_INT(pFirst->m_uiResidentLodMask, 1U);
  XII_TEST_INT(pSecond->m_uiResidentLodMask, 1U);
  XII_TEST_BOOL(pFirst->m_Lods[0].m_uiMeshletRemapBufferIndex != xiiInvalidIndex);
  XII_TEST_INT(pFirst->m_Lods[0].m_uiMeshletRemapBufferIndex, pSecond->m_Lods[0].m_uiMeshletRemapBufferIndex);
  XII_TEST_INT(pFirst->m_Lods[0].m_uiMeshletPrimitiveBufferIndex, pSecond->m_Lods[0].m_uiMeshletPrimitiveBufferIndex);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiTopologyUploadBytes, uiTopologyBytes);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentTopologyBytes, uiTopologyBytes);

  auto getOccupiedBytes = [pDevice]() {
    xiiHybridArray<xiiGALMemoryHeapStatistics, 16> heaps;
    pDevice->GetMemoryHeapStatistics(heaps);
    xiiUInt64 uiBytes = 0U;
    for (const auto& heap : heaps)
      uiBytes += heap.m_uiAllocatedBytes;
    return uiBytes;
  };
  pDevice->WaitIdle();
  const xiiUInt64 uiBefore = getOccupiedBytes();

  xiiGeometryResidencyManager::UnregisterGeometry(hFirst, 1U);
  xiiGeometryResidencyManager::ProcessStreaming(2U, 1U, configuration.m_uiUploadBudgetPerFrameBytes);
  xiiGALBindlessResourceTable::Collect(1U);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentTopologyBytes, uiTopologyBytes);
  XII_TEST_BOOL(getOccupiedBytes() == uiBefore);

  xiiGeometryResidencyManager::Touch(hSecond, 2U);
  xiiGeometryResidencyManager::UnregisterGeometry(hSecond, 2U);
  xiiGeometryResidencyManager::ProcessStreaming(3U, 1U, configuration.m_uiUploadBudgetPerFrameBytes);
  xiiGALBindlessResourceTable::Collect(1U);
  XII_TEST_BOOL(getOccupiedBytes() == uiBefore);

  xiiGeometryResidencyManager::ProcessStreaming(4U, 2U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentTopologyBytes, 0U);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiRetiredTopologyBytes, uiTopologyBytes);
  XII_TEST_BOOL(getOccupiedBytes() == uiBefore);
  xiiGALBindlessResourceTable::Collect(2U);
  pDevice->WaitIdle();
  const xiiUInt64 uiAfter = getOccupiedBytes();
  if (XII_TEST_BOOL(uiBefore >= uiAfter + uiTopologyBytes))
    xiiTestFramework::Output(xiiTestOutput::Details, "Shared topology made %llu Vulkan allocation bytes reusable while the source mesh remained loaded.", static_cast<unsigned long long>(uiBefore - uiAfter));

  xiiResourceLock<xiiMeshBufferResource> mesh(hMesh, xiiResourceAcquireMode::PointerOnly);
  XII_TEST_BOOL(mesh->GetVertexBuffer() != nullptr && mesh->GetIndexBuffer() != nullptr);
  const auto hReacquired = xiiGeometryResidencyManager::RegisterGeometry(geometry);
  xiiGeometryResidencyManager::RequestResidency(hReacquired, 0U, 5U);
  xiiGeometryResidencyManager::ProcessStreaming(5U, 2U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_BOOL(!xiiGeometryResidencyManager::IsValid(hSecond));
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiTopologyUploadBytes, uiTopologyBytes);

  xiiGALBufferCreationDescription pressureDescription;
  pressureDescription.m_uiSize    = 2U * 1024U * 1024U;
  pressureDescription.m_BindFlags = xiiGALBindFlags::VertexBuffer;
  auto  pPressureBuffer           = pDevice->CreateBuffer(pressureDescription);
  auto* pBudget                   = static_cast<xiiCVarInt*>(xiiCVar::FindCVarByName("Streaming.GpuBudgetMiB"));
  if (!XII_TEST_BOOL(pPressureBuffer && pBudget != nullptr))
    return;
  const xiiInt32 iPreviousBudget = pBudget->GetValue();
  *pBudget                       = 1;
  pDevice->BeginFrame();
  XII_SCOPE_EXIT(pDevice->EndFrame(); *pBudget = iPreviousBudget);
  XII_TEST_BOOL(pDevice->IsStreamingMemoryPressured());

  xiiGeometryResidencyManager::ProcessStreaming(128U, 4U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGeometryResidencyManager::GetState(hReacquired), xiiGeometryResidencyState::Resident);
  xiiGeometryResidencyManager::ProcessStreaming(136U, 5U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGeometryResidencyManager::GetState(hReacquired), xiiGeometryResidencyState::Unloaded);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiPressureEvictions, 1U);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiRetiredTopologyBytes, uiTopologyBytes);

  xiiGeometryResidencyManager::RequestResidency(hReacquired, 0U, 137U);
  xiiGeometryResidencyManager::ProcessStreaming(137U, 5U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGeometryResidencyManager::GetState(hReacquired), xiiGeometryResidencyState::Resident);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiTopologyUploadBytes, uiTopologyBytes);
}
