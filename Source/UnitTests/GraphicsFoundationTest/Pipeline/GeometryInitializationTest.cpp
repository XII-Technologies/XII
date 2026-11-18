/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Streaming/GpuResidencyManager.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>

namespace
{
  xiiUInt64 GetOccupiedGpuBytes(xiiGALDevice& device)
  {
    xiiHybridArray<xiiGALMemoryHeapStatistics, 16> heaps;
    device.GetMemoryHeapStatistics(heaps);
    xiiUInt64 uiBytes = 0U;
    for (const auto& heap : heaps)
      uiBytes += heap.m_uiAllocatedBytes;
    return uiBytes;
  }

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
        if (xiiGpuResidencyManager::EnsureInitialized().Failed())
        {
          m_bSucceeded = false;
          break;
        }
        auto metadata = xiiGpuResidencyManager::GetMetadataBuffer();
        auto meshlets = xiiGpuResidencyManager::GetMeshletMetadataBuffer();
        m_bSucceeded &= xiiGpuResidencyManager::IsInitialized() && metadata && meshlets;
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
  XII_TEST_BOOL(!xiiGpuResidencyManager::IsInitialized());
  xiiGpuResidencyDescription description;
  description.m_uiMaxGeometries = 64;
  description.m_uiMaxMeshlets   = 64;
  XII_TEST_RESULT(xiiGpuResidencyManager::Configure(description));
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
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiGeometryCount, 0);
}

XII_CREATE_SIMPLE_TEST(PipelineResources, OversizedGeometryResidency)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiGpuResidencyDescription configuration;
  configuration.m_uiMaxGeometries = 64;
  configuration.m_uiMaxMeshlets   = 64;
  if (!XII_TEST_RESULT(xiiGpuResidencyManager::Configure(configuration)) || !XII_TEST_RESULT(xiiGpuResidencyManager::EnsureInitialized()))
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
    handle = xiiGpuResidencyManager::RegisterGeometry(geometry);
    XII_TEST_BOOL(!handle.IsInvalidated());
    xiiGpuResidencyManager::RequestGeometry(handle, 0);
  }
  XII_SCOPE_EXIT(for (const auto handle : handles) xiiGpuResidencyManager::UnregisterGeometry(handle, 0));
  // A zero budget pauses streaming. Positive budgets must eventually admit an
  // indivisible record even when it exceeds the whole frame allowance.
  xiiGpuResidencyManager::ProcessStreaming(0, 0, 0);
  xiiGpuResidencyManager::ProcessStreaming(0, 0, bytes * 10);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentGeometryCount, 0);
  for (xiiUInt32 frame = 1; frame <= 3; ++frame)
  {
    xiiGpuResidencyManager::ProcessStreaming(frame, 0, 1);
    XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentGeometryCount, frame);
    XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentBytes, uiArenaBytes * frame);
    const auto stats = xiiGpuResidencyManager::GetStats();
    XII_TEST_INT(stats.m_uiAdmittedBytes, bytes + (frame == 1U ? uiTopologyBytes : 0U));
    XII_TEST_INT(stats.m_uiResidentTopologyBytes, uiTopologyBytes);
    XII_TEST_INT(stats.m_uiRemainingUploadBytes, 0);
    // A second world/view in the same device frame cannot grant another allowance.
    xiiGpuResidencyManager::ProcessStreaming(frame, 0, bytes * 10);
    XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentGeometryCount, frame);
    XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiPreparedMeshlets, stats.m_uiPreparedMeshlets);
  }
  for (const auto handle : handles)
  {
    XII_TEST_INT(xiiGpuResidencyManager::GetState(handle), xiiGeometryResidencyState::Resident);
    const auto* record = xiiGpuResidencyManager::GetGpuRecord(handle);
    if (XII_TEST_BOOL(record != nullptr))
      XII_TEST_INT(record->m_uiAvailableLodMask, 1);
  }

  // An ordinary record consumes the allowance: another record must wait when
  // only part of its cost remains, then load with an exact-fit allowance.
  xiiGeometryHandle ordinary[2];
  for (auto& handle : ordinary)
  {
    handle = xiiGpuResidencyManager::RegisterGeometry(geometry);
    xiiGpuResidencyManager::RequestGeometry(handle, 4);
  }
  XII_SCOPE_EXIT(for (const auto handle : ordinary) xiiGpuResidencyManager::UnregisterGeometry(handle, 0));
  xiiGpuResidencyManager::ProcessStreaming(4, 0, bytes + 1);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentGeometryCount, 4);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentBytes, uiArenaBytes * 4);
  const auto prepared = xiiGpuResidencyManager::GetStats().m_uiPreparedMeshlets;
  xiiGpuResidencyManager::ProcessStreaming(4, 0, bytes * 10);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentGeometryCount, 4);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiPreparedMeshlets, prepared);
  xiiGpuResidencyManager::ProcessStreaming(5, 0, bytes);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentGeometryCount, 5);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentBytes, uiArenaBytes * 5);
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
  xiiGpuResidencyDescription configuration;
  configuration.m_uiMaxGeometries = 16U;
  configuration.m_uiMaxMeshlets   = 2U * descriptor.m_Meshlets.GetCount();
  if (!XII_TEST_RESULT(xiiGpuResidencyManager::Configure(configuration)) || !XII_TEST_RESULT(xiiGpuResidencyManager::EnsureInitialized()))
    return;

  const auto             hMesh = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Shared topology cube", std::move(descriptor));
  xiiGeometryDescription geometry;
  geometry.m_Lods.ExpandAndGetRef().m_hMeshBuffer = hMesh;
  const auto hFirst                               = xiiGpuResidencyManager::RegisterGeometry(geometry);
  const auto hSecond                              = xiiGpuResidencyManager::RegisterGeometry(geometry);
  xiiGpuResidencyManager::RequestGeometry(hFirst, 1U);
  xiiGpuResidencyManager::RequestGeometry(hSecond, 1U);
  xiiGpuResidencyManager::ProcessStreaming(1U, 0U, configuration.m_uiUploadBudgetPerFrameBytes);

  const auto* pFirst  = xiiGpuResidencyManager::GetGpuRecord(hFirst);
  const auto* pSecond = xiiGpuResidencyManager::GetGpuRecord(hSecond);
  if (!XII_TEST_BOOL(pFirst != nullptr && pSecond != nullptr))
    return;
  XII_TEST_INT(pFirst->m_uiAvailableLodMask, 1U);
  XII_TEST_INT(pSecond->m_uiAvailableLodMask, 1U);
  XII_TEST_BOOL(pFirst->m_Lods[0].m_uiMeshletTopologyBufferIndex != xiiInvalidIndex);
  XII_TEST_INT(pFirst->m_Lods[0].m_uiMeshletTopologyBufferIndex, pSecond->m_Lods[0].m_uiMeshletTopologyBufferIndex);
  XII_TEST_INT(pFirst->m_Lods[0].m_uiMeshletPrimitiveByteOffset, pSecond->m_Lods[0].m_uiMeshletPrimitiveByteOffset);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiTopologyUploadBytes, uiTopologyBytes);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentTopologyBytes, uiTopologyBytes);

  pDevice->WaitIdle();
  const xiiUInt64 uiBefore = GetOccupiedGpuBytes(*pDevice);

  xiiGpuResidencyManager::UnregisterGeometry(hFirst, 1U);
  xiiGpuResidencyManager::ProcessStreaming(2U, 1U, configuration.m_uiUploadBudgetPerFrameBytes);
  xiiGALBindlessResourceTable::Collect(1U);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentTopologyBytes, uiTopologyBytes);
  XII_TEST_BOOL(GetOccupiedGpuBytes(*pDevice) == uiBefore);

  xiiGpuResidencyManager::Touch(hSecond, 2U);
  xiiGpuResidencyManager::UnregisterGeometry(hSecond, 2U);
  xiiGpuResidencyManager::ProcessStreaming(3U, 1U, configuration.m_uiUploadBudgetPerFrameBytes);
  xiiGALBindlessResourceTable::Collect(1U);
  XII_TEST_BOOL(GetOccupiedGpuBytes(*pDevice) == uiBefore);

  xiiGpuResidencyManager::ProcessStreaming(4U, 2U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentTopologyBytes, 0U);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiRetiredTopologyBytes, uiTopologyBytes);
  XII_TEST_BOOL(GetOccupiedGpuBytes(*pDevice) == uiBefore);
  xiiGALBindlessResourceTable::Collect(2U);
  pDevice->WaitIdle();
  const xiiUInt64 uiAfter = GetOccupiedGpuBytes(*pDevice);
  if (XII_TEST_BOOL(uiBefore >= uiAfter + uiTopologyBytes))
    xiiTestFramework::Output(xiiTestOutput::Details, "Shared topology made %llu Vulkan allocation bytes reusable while the source mesh remained loaded.", static_cast<unsigned long long>(uiBefore - uiAfter));

  xiiResourceLock<xiiMeshBufferResource> mesh(hMesh, xiiResourceAcquireMode::PointerOnly);
  XII_TEST_BOOL(mesh->GetVertexBuffer() != nullptr && mesh->GetIndexBuffer() != nullptr);
  const auto hReacquired = xiiGpuResidencyManager::RegisterGeometry(geometry);
  xiiGpuResidencyManager::RequestGeometry(hReacquired, 5U);
  xiiGpuResidencyManager::ProcessStreaming(5U, 2U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_BOOL(!xiiGpuResidencyManager::IsValid(hSecond));
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiTopologyUploadBytes, uiTopologyBytes);

  xiiGALBufferCreationDescription pressureDescription;
  pressureDescription.m_uiSize    = 2U * 1024U * 1024U;
  pressureDescription.m_BindFlags = xiiGALBindFlags::VertexBuffer;
  auto pPressureBuffer            = pDevice->CreateBuffer(pressureDescription);
  if (!XII_TEST_BOOL(pPressureBuffer != nullptr))
    return;
  auto       policy         = pDevice->GetStreamingMemorySettings();
  const auto originalPolicy = policy;
  policy.m_bUseNativeBudget = false;
  policy.m_uiBudgetMiB      = 1;
  XII_TEST_RESULT(pDevice->SetStreamingMemorySettings(policy));
  pDevice->BeginFrame();
  XII_SCOPE_EXIT(pDevice->EndFrame(); pDevice->SetStreamingMemorySettings(originalPolicy).IgnoreResult());
  XII_TEST_BOOL(pDevice->IsStreamingMemoryPressured());

  xiiGpuResidencyManager::ProcessStreaming(128U, 4U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGpuResidencyManager::GetState(hReacquired), xiiGeometryResidencyState::Resident);
  xiiGpuResidencyManager::ProcessStreaming(136U, 5U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGpuResidencyManager::GetState(hReacquired), xiiGeometryResidencyState::Unloaded);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiPressureEvictions, 1U);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiRetiredTopologyBytes, uiTopologyBytes);

  xiiGpuResidencyManager::RequestGeometry(hReacquired, 137U);
  xiiGpuResidencyManager::ProcessStreaming(137U, 5U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGpuResidencyManager::GetState(hReacquired), xiiGeometryResidencyState::Resident);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiTopologyUploadBytes, uiTopologyBytes);
}

XII_CREATE_SIMPLE_TEST(PipelineResources, IndependentLodTopology)
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

  constexpr xiiUInt32 uiFinePrimitiveCount = 32U * 12U;
  xiiGeometry         geometry;
  for (xiiUInt32 i = 0U; i < 33U; ++i)
  {
    xiiGeometry::GeoOptions options;
    options.m_Transform = xiiMat4::MakeTranslation(xiiVec3(static_cast<float>(i) * 2.0f, 0, 0));
    geometry.AddBox(xiiVec3(1), true, options);
  }
  xiiMeshBufferResourceDescriptor descriptor;
  descriptor.AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  const xiiMeshletBuildRange buildRanges[] = {
    {.m_uiPrimitiveCount = uiFinePrimitiveCount},
    {.m_uiFirstPrimitive = uiFinePrimitiveCount, .m_uiPrimitiveCount = 12U, .m_uiLodIndex = 1U},
  };
  if (!XII_TEST_RESULT(descriptor.BuildMeshlets(xiiMakeArrayPtr(buildRanges))) || !XII_TEST_BOOL(descriptor.m_Meshlets.GetCount() > 2U))
    return;

  // Keep valid offsets while making the fine range nonmonotonic.
  xiiMath::Swap(descriptor.m_Meshlets[0], descriptor.m_Meshlets[1]);
  const xiiUInt32  uiCoarseFirst = descriptor.m_Meshlets.GetCount() - 1U;
  const xiiMeshlet coarse        = descriptor.m_Meshlets[uiCoarseFirst];
  if (!XII_TEST_BOOL(coarse.m_uiLodIndex == 1U && coarse.m_uiVertexRemapOffset > 0U && coarse.m_uiPrimitiveIndexOffset > 0U))
    return;
  const xiiUInt64 uiCoarseBytes = coarse.m_uiVertexCount * sizeof(xiiUInt32) + xiiMath::RoundUp(xiiUInt32(coarse.m_uiPrimitiveCount) * 3U, 4U);
  const xiiUInt64 uiFineBytes   = coarse.m_uiVertexRemapOffset * sizeof(xiiUInt32) + xiiMath::RoundUp(coarse.m_uiPrimitiveIndexOffset, 4U);

  xiiGpuResidencyDescription configuration;
  configuration.m_uiMaxGeometries = 16U;
  configuration.m_uiMaxMeshlets   = 2U * descriptor.m_Meshlets.GetCount();
  if (!XII_TEST_RESULT(xiiGpuResidencyManager::Configure(configuration)) || !XII_TEST_RESULT(xiiGpuResidencyManager::EnsureInitialized()))
    return;

  const auto             hMesh = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Independent topology LODs", std::move(descriptor));
  xiiGeometryDescription description;
  description.m_Lods.PushBack({.m_hMeshBuffer = hMesh, .m_uiMeshletCount = uiCoarseFirst});
  description.m_Lods.PushBack({.m_hMeshBuffer = hMesh, .m_uiFirstMeshlet = uiCoarseFirst, .m_uiMeshletCount = 1U});
  const auto             hGeometry = xiiGpuResidencyManager::RegisterGeometry(description);
  xiiGeometryDescription peer;
  peer.m_bPinned = true;
  peer.m_Lods.PushBack({.m_hMeshBuffer = hMesh, .m_uiFirstMeshlet = uiCoarseFirst});
  const auto hPeer = xiiGpuResidencyManager::RegisterGeometry(peer);
  xiiGpuResidencyManager::RequestGeometry(hGeometry, 1U);
  xiiGpuResidencyManager::RequestGeometry(hPeer, 1U);
  xiiGpuResidencyManager::ProcessStreaming(1U, 0U, configuration.m_uiUploadBudgetPerFrameBytes);
  const auto* pRecord = xiiGpuResidencyManager::GetGpuRecord(hGeometry);
  const auto* pPeer   = xiiGpuResidencyManager::GetGpuRecord(hPeer);
  if (!XII_TEST_BOOL(pRecord != nullptr && pPeer != nullptr))
    return;
  XII_TEST_INT(pRecord->m_uiAvailableLodMask, 2U);
  XII_TEST_INT(pPeer->m_uiAvailableLodMask, 1U);
  XII_TEST_INT(pRecord->m_Lods[1].m_uiMeshletTopologyBufferIndex, pPeer->m_Lods[0].m_uiMeshletTopologyBufferIndex);
  XII_TEST_INT(pRecord->m_Lods[1].m_uiMeshletPrimitiveByteOffset, coarse.m_uiVertexCount * sizeof(xiiUInt32));
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiTopologyUploadBytes, uiCoarseBytes);

  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Rebased coarse meshlet")))
    return;
  context.GetResourceCache().BeginFrame(1U, 0U);
  context.GetGraph().BeginSetup(1U);
  const auto upload = xiiGpuResidencyManager::DeclareUploadPass(context.GetGraph());
  XII_TEST_BOOL(!upload.m_hMeshletMetadata.IsInvalidated());
  context.GetGraph().EndSetup();
  if (!XII_TEST_RESULT(context.GetGraph().Compile()) ||
      !XII_TEST_RESULT(context.GetGraph().Execute(pDevice, nullptr, &context.GetBlackboard(), &context.GetResourceCache())))
    return;
  context.GetResourceCache().EndFrame();
  xiiMeshlet uploaded;
  if (XII_TEST_BOOL(xiiGpuTestReadback::ReadBufferRecord(*pDevice, xiiGpuResidencyManager::GetMeshletMetadataBuffer().Borrow(),
                                                         pRecord->m_Lods[1].m_uiMeshletMetadataOffset * sizeof(xiiMeshlet), uploaded)))
  {
    XII_TEST_INT(uploaded.m_uiVertexRemapOffset, 0U);
    XII_TEST_INT(uploaded.m_uiPrimitiveIndexOffset, 0U);
    XII_TEST_INT(uploaded.m_uiFirstPrimitive, uiFinePrimitiveCount);
  }

  xiiGALBufferCreationDescription pressure;
  pressure.m_uiSize    = 2U * 1024U * 1024U;
  pressure.m_BindFlags = xiiGALBindFlags::VertexBuffer;
  auto pPressure       = pDevice->CreateBuffer(pressure);
  if (!XII_TEST_BOOL(pPressure != nullptr))
    return;
  auto       policy         = pDevice->GetStreamingMemorySettings();
  const auto originalPolicy = policy;
  XII_SCOPE_EXIT(pDevice->SetStreamingMemorySettings(originalPolicy).IgnoreResult());

  // A required allocation after BeginFrame can consume headroom before the next pressure sample.
  xiiHybridArray<xiiGALMemoryHeapStatistics, 8> heaps;
  pDevice->GetMemoryHeapStatistics(heaps);
  xiiGALMemoryHeapStatistics primary;
  for (const auto& heap : heaps)
    if (heap.m_bDeviceLocal && heap.m_uiCapacityBytes > primary.m_uiCapacityBytes)
      primary = heap;
  constexpr xiiUInt64 uiMiB   = 1024U * 1024U;
  const xiiUInt64     uiUsage = xiiMath::Max(primary.m_uiUsageBytes, primary.m_uiCommittedBytes) - primary.m_uiCommittedBytes + primary.m_uiAllocatedBytes;
  policy.m_bUseNativeBudget   = false;
  policy.m_uiBudgetMiB        = static_cast<xiiUInt32>(uiUsage / uiMiB) + 16;
  XII_TEST_RESULT(pDevice->SetStreamingMemorySettings(policy));
  pDevice->BeginFrame();
  XII_TEST_BOOL(!pDevice->IsStreamingMemoryPressured());
  auto lateRequiredDescription     = pressure;
  lateRequiredDescription.m_uiSize = 17U * uiMiB;
  auto pLateRequired               = pDevice->CreateBuffer(lateRequiredDescription);
  XII_TEST_BOOL(pLateRequired != nullptr);
  XII_TEST_BOOL(!pDevice->IsStreamingMemoryPressured());
  const auto statsBefore = xiiGpuResidencyManager::GetStats();
  xiiGpuResidencyManager::RequestGeometry(hGeometry, 2U);
  xiiGpuResidencyManager::ProcessStreaming(2U, 1U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(pRecord->m_uiAvailableLodMask, 2U);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiMemoryDeferred, 1U);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentBytes, statsBefore.m_uiResidentBytes);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiTopologyUploadBytes, 0U);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiPreparedMeshlets, 0U);
  pLateRequired.Clear();
  pDevice->EndFrame();
  pDevice->WaitIdle();

  policy.m_bUseNativeBudget = false;
  policy.m_uiBudgetMiB      = 1;
  XII_TEST_RESULT(pDevice->SetStreamingMemorySettings(policy));
  pDevice->BeginFrame();
  XII_TEST_BOOL(pDevice->IsStreamingMemoryPressured());
  xiiGpuResidencyManager::ProcessStreaming(3U, 2U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(pRecord->m_uiAvailableLodMask, 2U);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiMemoryDeferred, 1U);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiTopologyUploadBytes, 0U);
  pDevice->EndFrame();

  policy.m_bUseNativeBudget = true;
  XII_TEST_RESULT(pDevice->SetStreamingMemorySettings(policy));
  pDevice->BeginFrame();
  xiiGpuResidencyManager::ProcessStreaming(4U, 3U, configuration.m_uiUploadBudgetPerFrameBytes);
  pDevice->EndFrame();
  XII_TEST_INT(pRecord->m_uiAvailableLodMask, 3U);
  XII_TEST_INT(pRecord->m_Lods[0].m_uiVertexBufferIndex, pRecord->m_Lods[1].m_uiVertexBufferIndex);
  XII_TEST_INT(pRecord->m_Lods[0].m_uiIndexBufferIndex, pRecord->m_Lods[1].m_uiIndexBufferIndex);
  XII_TEST_BOOL(pRecord->m_Lods[0].m_uiMeshletTopologyBufferIndex != pRecord->m_Lods[1].m_uiMeshletTopologyBufferIndex);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiTopologyUploadBytes, uiFineBytes);

  pDevice->WaitIdle();
  const xiiUInt64 uiBefore      = GetOccupiedGpuBytes(*pDevice);
  const xiiUInt64 uiArenaBefore = xiiGpuResidencyManager::GetStats().m_uiResidentBytes;
  const xiiUInt32 uiGeneration  = pRecord->m_uiGeneration;
  policy.m_bUseNativeBudget     = false;
  policy.m_uiBudgetMiB          = 1;
  XII_TEST_RESULT(pDevice->SetStreamingMemorySettings(policy));
  pDevice->BeginFrame();
  XII_SCOPE_EXIT(pDevice->EndFrame());
  XII_TEST_BOOL(pDevice->IsStreamingMemoryPressured());

  // Merge demand across views. A coarse view does not make a recent fine view cold.
  XII_TEST_BOOL(xiiGpuResidencyManager::ReportLodDemand(hGeometry, 0U, 119U, 3U));
  XII_TEST_BOOL(xiiGpuResidencyManager::ReportLodDemand(hGeometry, 1U, 127U, 1U));
  xiiGpuResidencyManager::ProcessStreaming(128U, 126U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(pRecord->m_uiAvailableLodMask, 3U);

  XII_TEST_BOOL(xiiGpuResidencyManager::ReportLodDemand(hGeometry, 1U, 247U, 1U));
  xiiGpuResidencyManager::NoteIncompleteLodFeedback(247U);
  xiiGpuResidencyManager::ProcessStreaming(248U, 246U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(pRecord->m_uiAvailableLodMask, 3U);

  XII_TEST_BOOL(xiiGpuResidencyManager::ReportLodDemand(hGeometry, 1U, 367U, 1U));
  xiiGpuResidencyManager::RequestInstanceResidency(hGeometry, 0U, 367U, false);
  xiiGpuResidencyManager::ProcessStreaming(368U, 366U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(pRecord->m_uiAvailableLodMask, 2U);
  XII_TEST_INT(pRecord->m_uiGeneration, uiGeneration);
  XII_TEST_INT(pRecord->m_Lods[0].m_uiMeshletTopologyBufferIndex, xiiInvalidIndex);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiFineLodEvictions, 1U);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentBytes, uiArenaBefore);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentTopologyBytes, uiCoarseBytes + uiFineBytes);
  XII_TEST_BOOL(!xiiGpuResidencyManager::ReportLodDemand(hGeometry, 0U, 367U, 3U));

  xiiGpuResidencyManager::ProcessStreaming(369U, 367U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiRetiredTopologyBytes, uiFineBytes);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentBytes, uiArenaBefore - uiCoarseFirst * sizeof(xiiMeshlet));
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiResidentTopologyBytes, uiCoarseBytes);
  XII_TEST_INT(pRecord->m_uiAvailableLodMask, 2U);
  XII_TEST_INT(pPeer->m_uiAvailableLodMask, 1U);
  xiiGALBindlessResourceTable::Collect(367U);
  pDevice->WaitIdle();
  const xiiUInt64 uiAfter = GetOccupiedGpuBytes(*pDevice);
  if (XII_TEST_BOOL(uiBefore >= uiAfter + uiFineBytes))
    xiiTestFramework::Output(xiiTestOutput::Details, "Independent topology uploaded %llu coarse bytes and %llu fine bytes. Active-record eviction made %llu Vulkan allocation bytes reusable.",
                             static_cast<unsigned long long>(uiCoarseBytes), static_cast<unsigned long long>(uiFineBytes), static_cast<unsigned long long>(uiBefore - uiAfter));

  // Ordinary renewed demand waits under pressure. An exact forced draw can restore its LOD.
  XII_TEST_BOOL(xiiGpuResidencyManager::ReportLodDemand(hGeometry, 0U, 369U, 3U));
  xiiGpuResidencyManager::ProcessStreaming(370U, 368U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(pRecord->m_uiAvailableLodMask, 2U);
  xiiGpuResidencyManager::RequestInstanceResidency(hGeometry, 0U, 370U, true);
  xiiGpuResidencyManager::ProcessStreaming(370U, 368U, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(pRecord->m_uiAvailableLodMask, 3U);
  XII_TEST_INT(xiiGpuResidencyManager::GetStats().m_uiTopologyUploadBytes, uiFineBytes);
}
