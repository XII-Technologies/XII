/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>

XII_CREATE_SIMPLE_TEST(PipelineResources, GeometryCoverageScaling)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

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
  const auto before                                    = xiiGeometryResidencyManager::GetStats();
  const auto start                                     = xiiTime::Now();
  for (xiiUInt32 frame = 0; frame < 16; ++frame)
    for (xiiUInt32 section = 0; section < 512; ++section)
    {
      XII_TEST_RESULT(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(handle, section * 32 * 12 + 1, 32 * 12 - 2, xiiMakeArrayPtr(counts)));
      XII_TEST_INT(counts[0], 32 * meshlets.GetCount());
    }
  xiiTestFramework::Output(xiiTestOutput::Details, "Coverage: %u meshlets, 512 ranges, 16 updates: %.3f ms.", copies * meshlets.GetCount(), (xiiTime::Now() - start).GetMilliseconds());
  const auto after = xiiGeometryResidencyManager::GetStats();
  XII_TEST_INT(after.m_uiCoverageIndexBuilds - before.m_uiCoverageIndexBuilds, 1);
  XII_TEST_INT(after.m_uiCoverageMeshlets - before.m_uiCoverageMeshlets, copies * meshlets.GetCount());
}

XII_CREATE_SIMPLE_TEST(PipelineResources, CompiledGeometryScaling)
{
  const auto path = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-geometryModel");
  if (path.IsEmpty())
  {
    xiiTestFramework::Output(xiiTestOutput::Details, "Supply -geometryModel <absolute compiled mesh> for the large-asset CPU benchmark.");
    return;
  }
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path.GetFileDirectory(), "GeometryBenchmark", "GeometryBenchmark"));
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("GeometryBenchmark"));
  xiiMeshResourceDescriptor descriptor;
  {
    xiiFileReader reader;
    if (!XII_TEST_RESULT(reader.Open(xiiStringBuilder(":GeometryBenchmark/", path.GetFileNameAndExtension()))))
      return;
    xiiAssetFileHeader header;
    if (!XII_TEST_RESULT(header.Read(reader)) || !XII_TEST_RESULT(descriptor.Load(reader)))
      return;
  }
  // Geometry timing excludes texture/material IO and asset import. Preserve the
  // compiled LOD/section topology while using already prepared native buffers.
  for (xiiUInt32 i = 0; i < descriptor.GetMaterials().GetCount(); ++i)
    descriptor.SetMaterial(i, "");
  const auto                       mesh = xiiResourceManager::CreateResource<xiiMeshResource>("Compiled geometry benchmark", std::move(descriptor));
  xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    return;
  const auto handle = xiiGeometryResidencyManager::AcquireMeshGeometry(mesh);
  XII_SCOPE_EXIT(xiiGeometryResidencyManager::ReleaseMeshGeometry(handle, 100));
  xiiResourceLock<xiiMeshBufferResource> buffer(loaded->GetMeshBuffer(), xiiResourceAcquireMode::PointerOnly);
  const auto                             sections    = loaded->GetSections();
  xiiUInt64                              checksum    = 0;
  const auto                             scanStart   = xiiTime::Now();
  const auto                             sampleCount = xiiMath::Min(128U, sections.GetCount());
  for (xiiUInt32 i = 0; i < sampleCount; ++i)
    for (const auto& meshlet : buffer->GetMeshlets())
      if (meshlet.m_uiFirstPrimitive < xiiUInt64(sections[i].m_uiFirstPrimitive) + sections[i].m_uiPrimitiveCount && xiiUInt64(meshlet.m_uiFirstPrimitive) + meshlet.m_uiPrimitiveCount > sections[i].m_uiFirstPrimitive)
        ++checksum;
  const auto scanTime        = xiiTime::Now() - scanStart;
  const auto indexStart      = xiiTime::Now();
  xiiUInt64  indexedChecksum = 0;
  for (xiiUInt32 i = 0; i < sampleCount; ++i)
  {
    xiiUInt32 counts[xiiGpuGeometryRecord::s_uiMaxLods] = {};
    XII_TEST_RESULT(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(handle, sections[i].m_uiFirstPrimitive, sections[i].m_uiPrimitiveCount, xiiMakeArrayPtr(counts)));
    for (const auto count : counts)
      indexedChecksum += count;
  }
  XII_TEST_INT(checksum, indexedChecksum);
  xiiTestFramework::Output(xiiTestOutput::Details, "Compiled geometry: %u vertices, %u triangles, %u meshlets, %u sections. %u sampled ranges: scan %.3f ms, indexed including preparation %.3f ms.", buffer->GetVertexCount(), buffer->GetPrimitiveCount(), buffer->GetMeshletCount(), sections.GetCount(), sampleCount, scanTime.GetMilliseconds(), (xiiTime::Now() - indexStart).GetMilliseconds());

  xiiWorldDescription worldDescription("Compiled geometry synchronization benchmark");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*                    renderer = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiGameObject*           object   = nullptr;
  xiiGameObjectDescription objectDescription;
  objectDescription.m_bDynamic = true;
  world.CreateObject(objectDescription, object);
  xiiMeshComponent* component = nullptr;
  xiiMeshComponent::CreateComponent(object, component);
  component->SetMesh(mesh);
  world.Update();
  xiiExtractedRenderData  extracted;
  xiiMsgExtractRenderData request;
  request.m_pExtractedRenderData = &extracted;
  component->SendMessage(request);
  extracted.SortAndBatches();
  const auto cold = xiiTime::Now();
  XII_TEST_RESULT(renderer->UpdateGpuInstances(extracted, 1));
  const auto coldTime = xiiTime::Now() - cold;
  const auto queries  = xiiGeometryResidencyManager::GetStats().m_uiCoverageQueries;
  const auto warm     = xiiTime::Now();
  for (xiiUInt32 frame = 2; frame <= 21; ++frame)
    XII_TEST_RESULT(renderer->UpdateGpuInstances(extracted, frame));
  XII_TEST_BOOL(renderer->GetGpuInstanceData()->GetUploadRanges().IsEmpty());
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiCoverageQueries, queries);
  xiiTestFramework::Output(xiiTestOutput::Details, "Compiled geometry GPU scene: %u draw instances, cold %.3f ms; stationary %.3f ms/update (20 updates).", renderer->GetGpuInstanceData()->GetInstanceCount(), coldTime.GetMilliseconds(), (xiiTime::Now() - warm).GetMilliseconds() / 20.0);
}

XII_CREATE_SIMPLE_TEST(PipelineResources, GeometryAdmissionAndUploadOwnership)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiGeometryResidencyDescription configuration;
  configuration.m_uiMaxGeometries = 8;
  configuration.m_uiMaxMeshlets   = 2;
  XII_TEST_RESULT(xiiGeometryResidencyManager::Configure(configuration));
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshBufferResourceDescriptor descriptor;
  descriptor.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, true);
  if (!XII_TEST_INT(descriptor.m_Meshlets.GetCount(), 1))
    return;
  const auto             buffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Admission arena cube", std::move(descriptor));
  xiiGeometryDescription description;
  description.m_Lods.ExpandAndGetRef().m_hMeshBuffer = buffer;
  xiiGeometryHandle handles[3];
  for (auto& handle : handles)
  {
    handle = xiiGeometryResidencyManager::RegisterGeometry(description);
    xiiGeometryResidencyManager::RequestResidency(handle, 0, 1);
  }
  XII_SCOPE_EXIT(for (const auto handle : handles) xiiGeometryResidencyManager::UnregisterGeometry(handle, 3));
  xiiRenderGraphTestContext stale;
  XII_TEST_RESULT(stale.Initialize("Pre-admission geometry snapshot"));
  stale.GetResourceCache().BeginFrame(1, 0);
  stale.GetGraph().BeginSetup(1);
  const auto staleUpload = xiiGeometryResidencyManager::AddUploadPass(stale.GetGraph(), 1);
  XII_TEST_BOOL(!staleUpload.m_hGeometryMetadata.IsInvalidated());
  stale.GetGraph().EndSetup();
  xiiGeometryResidencyManager::ProcessStreaming(1, 0, configuration.m_uiUploadBudgetPerFrameBytes);
  auto stats = xiiGeometryResidencyManager::GetStats();
  XII_TEST_INT(stats.m_uiResidentGeometryCount, 2);
  XII_TEST_INT(stats.m_uiArenaDeferred, 1);
  XII_TEST_INT(stats.m_uiPreparedMeshlets, 2);
  xiiGeometryResidencyManager::ProcessStreaming(1, 0, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiPreparedMeshlets, 2);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiArenaMeshlets, 2);

  xiiRenderGraphTestContext graphs[3];
  for (auto& context : graphs)
  {
    XII_TEST_RESULT(context.Initialize("Shared pending geometry uploads"));
    context.GetResourceCache().BeginFrame(1, 0);
    context.GetGraph().BeginSetup(1);
    const auto uploaded = xiiGeometryResidencyManager::AddUploadPass(context.GetGraph(), 1);
    XII_TEST_BOOL(!uploaded.m_hGeometryMetadata.IsInvalidated());
    context.GetGraph().EndSetup();
  }
  // The first graph is abandoned until after another graph records the same batch.
  // Setup never consumes the only payload, and retained graphs cannot upload it twice.
  for (xiiUInt32 index : {1U, 2U, 0U})
  {
    auto& context = graphs[index];
    XII_TEST_RESULT(context.GetGraph().Compile());
    XII_TEST_RESULT(context.GetGraph().Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &context.GetResourceCache()));
    context.GetResourceCache().EndFrame();
    environment.GetDevice()->WaitIdle();
    const auto bytes = xiiGeometryResidencyManager::GetStats().m_uiUploadedBytes;
    XII_TEST_INT(bytes, 3 * sizeof(xiiGpuGeometryRecord) + 2 * sizeof(xiiMeshlet));
  }
  // A delayed pre-admission graph must not overwrite the newer published record.
  XII_TEST_RESULT(stale.GetGraph().Compile());
  XII_TEST_RESULT(stale.GetGraph().Execute(environment.GetDevice(), nullptr, &stale.GetBlackboard(), &stale.GetResourceCache()));
  stale.GetResourceCache().EndFrame();
  environment.GetDevice()->WaitIdle();
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiUploadedBytes, 3 * sizeof(xiiGpuGeometryRecord) + 2 * sizeof(xiiMeshlet));
  xiiGeometryResidencyManager::UnregisterGeometry(handles[0], 1);
  xiiGeometryResidencyManager::ProcessStreaming(2, 0, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiPreparedMeshlets, 0);
  xiiGeometryResidencyManager::ProcessStreaming(2, 1, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_BOOL(!xiiGeometryResidencyManager::IsValid(handles[0]));
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiPreparedMeshlets, 1);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, 2);
  description.m_bPinned = true;
  const auto pinned     = xiiGeometryResidencyManager::RegisterGeometry(description);
  XII_SCOPE_EXIT(xiiGeometryResidencyManager::UnregisterGeometry(pinned, 6));
  xiiGeometryResidencyManager::RequestResidency(pinned, 0, 3);
  xiiGeometryResidencyManager::Touch(handles[1], 3);
  xiiGeometryResidencyManager::ProcessStreaming(3, 2, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_BOOL(xiiGeometryResidencyManager::GetState(pinned) == xiiGeometryResidencyState::Resident);
  XII_TEST_BOOL(xiiGeometryResidencyManager::GetState(handles[2]) == xiiGeometryResidencyState::Unloaded);
  // Arena pressure evicts completed cold geometry even below the byte budget.
  // Pinned and current-frame geometry remain protected from immediate churn.
  xiiGeometryResidencyManager::RequestResidency(handles[2], 0, 4);
  xiiGeometryResidencyManager::ProcessStreaming(4, 3, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_BOOL(xiiGeometryResidencyManager::GetState(handles[1]) == xiiGeometryResidencyState::Unloaded);
  XII_TEST_BOOL(xiiGeometryResidencyManager::GetState(handles[2]) == xiiGeometryResidencyState::Resident);
  xiiGeometryResidencyManager::RequestResidency(handles[1], 0, 5);
  xiiGeometryResidencyManager::Touch(handles[2], 5);
  xiiGeometryResidencyManager::ProcessStreaming(5, 4, configuration.m_uiUploadBudgetPerFrameBytes);
  XII_TEST_BOOL(xiiGeometryResidencyManager::GetState(pinned) == xiiGeometryResidencyState::Resident);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiArenaDeferred, 1);
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiPreparedMeshlets, 0);
}

XII_CREATE_SIMPLE_TEST(PipelineResources, CompiledMeshletPreparation)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, false);
  descriptor.AddSection(12, 0, 0);
  descriptor.ComputeBounds();
  XII_TEST_RESULT(descriptor.BuildMeshlets());
  auto& compiled = descriptor.GetMeshBufferDescriptor();
  compiled.m_Meshlets[0].m_Bounds.m_fRadius += 1.0f;
  const float authoredRadius                                            = compiled.m_Meshlets[0].m_Bounds.m_fRadius;
  auto        invalid                                                   = descriptor;
  invalid.GetMeshBufferDescriptor().m_Meshlets[0].m_uiVertexRemapOffset = xiiMath::MaxValue<xiiUInt32>();
  const auto                       mesh                                 = xiiResourceManager::CreateResource<xiiMeshResource>("Retained compiled meshlets", std::move(descriptor));
  xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::PointerOnly);
  XII_TEST_BOOL(loaded->GetLoadingState() == xiiResourceState::Loaded);
  xiiResourceLock<xiiMeshBufferResource> buffer(loaded->GetMeshBuffer(), xiiResourceAcquireMode::PointerOnly);
  XII_TEST_FLOAT(buffer->GetMeshlets()[0].m_Bounds.m_fRadius, authoredRadius, 0);
  const auto                       bad = xiiResourceManager::CreateResource<xiiMeshResource>("Invalid compiled meshlets", std::move(invalid));
  xiiResourceLock<xiiMeshResource> rejected(bad, xiiResourceAcquireMode::PointerOnly);
  XII_TEST_BOOL(rejected->GetLoadingState() == xiiResourceState::LoadedResourceMissing);
}

XII_CREATE_SIMPLE_TEST(PipelineResources, ProceduralMeshletDrawRanges)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  for (xiiUInt32 sections : {1U, 2U})
  {
    xiiMeshResourceDescriptor descriptor;
    descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList);
    for (xiiUInt32 section = 0; section < sections; ++section)
      descriptor.AddSection(12 / sections, section * (12 / sections), section);
    descriptor.ComputeBounds();
    xiiStringBuilder name;
    name.SetFormat("Procedural meshlet ranges {}", sections);
    const auto                       mesh = xiiResourceManager::CreateResource<xiiMeshResource>(name, std::move(descriptor));
    xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::PointerOnly);
    if (!XII_TEST_BOOL(loaded->GetLoadingState() == xiiResourceState::Loaded))
      return;
    const auto lods = loaded->GetLODs();
    XII_TEST_INT(lods.GetCount(), 1);
    XII_TEST_BOOL(lods[0].m_uiMeshletCount >= sections);
    xiiResourceLock<xiiMeshBufferResource> buffer(loaded->GetMeshBuffer(), xiiResourceAcquireMode::PointerOnly);
    for (const auto& section : loaded->GetSections())
    {
      XII_TEST_BOOL(section.m_uiMeshletCount > 0);
      xiiUInt32 primitives = 0;
      for (const auto& meshlet : buffer->GetMeshlets().GetSubArray(section.m_uiFirstMeshlet, section.m_uiMeshletCount))
      {
        XII_TEST_BOOL(meshlet.m_uiFirstPrimitive >= section.m_uiFirstPrimitive);
        XII_TEST_BOOL(meshlet.m_uiFirstPrimitive + meshlet.m_uiPrimitiveCount <= section.m_uiFirstPrimitive + section.m_uiPrimitiveCount);
        primitives += meshlet.m_uiPrimitiveCount;
      }
      XII_TEST_INT(primitives, section.m_uiPrimitiveCount);
    }
  }
}

XII_CREATE_SIMPLE_TEST(PipelineResources, GeometryCoverageIdentity)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiGeometryResidencyDescription configuration;
  configuration.m_uiMaxGeometries = 8;
  configuration.m_uiMaxMeshlets   = 8;
  XII_TEST_RESULT(xiiGeometryResidencyManager::Configure(configuration));
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshBufferResourceDescriptor descriptor;
  descriptor.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, true);
  const auto                original = descriptor.m_Meshlets[0];
  const auto                indices  = descriptor.m_IndexData;
  xiiDynamicArray<xiiUInt8> repeated;
  for (xiiUInt32 i = 0; i < 4; ++i)
    repeated.PushBackRange(indices);
  descriptor.SetIndexData(repeated, descriptor.GetIndexCount() * 4, descriptor.m_IndexType);
  descriptor.m_Meshlets.SetCount(4);
  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    descriptor.m_Meshlets[i]                    = original;
    descriptor.m_Meshlets[i].m_uiFirstPrimitive = (3 - i) * 12;
  }
  const auto             buffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Unordered coverage identity", std::move(descriptor));
  xiiGeometryDescription description;
  for (xiiUInt32 lod = 0; lod < 2; ++lod)
  {
    auto& source            = description.m_Lods.ExpandAndGetRef();
    source.m_hMeshBuffer    = buffer;
    source.m_uiFirstMeshlet = lod * 2;
    source.m_uiMeshletCount = 2;
  }
  const auto handle                                    = xiiGeometryResidencyManager::RegisterGeometry(description);
  xiiUInt32  counts[xiiGpuGeometryRecord::s_uiMaxLods] = {};
  XII_TEST_RESULT(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(handle, 5, 20, xiiMakeArrayPtr(counts)));
  XII_TEST_INT(counts[0], 1);
  XII_TEST_INT(counts[1], 2);
  XII_TEST_RESULT(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(handle, 12, 12, xiiMakeArrayPtr(counts)));
  XII_TEST_INT(counts[0], 0);
  XII_TEST_INT(counts[1], 1);
  XII_TEST_RESULT(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(handle, 0, 0, xiiMakeArrayPtr(counts)));
  XII_TEST_INT(counts[0], 2);
  XII_TEST_INT(counts[1], 2);
  xiiGeometryResidencyManager::RequestResidency(handle, 0, 1);
  xiiGeometryResidencyManager::ProcessStreaming(1, 0, 1);
  const auto* record = xiiGeometryResidencyManager::GetGpuRecord(handle);
  if (!XII_TEST_BOOL(record != nullptr))
    return;
  XII_TEST_INT(record->m_uiResidentLodMask, 3);
  XII_TEST_INT(record->m_Lods[0].m_uiMeshletRangesOrdered, 1);
  XII_TEST_INT(record->m_Lods[1].m_uiMeshletRangesOrdered, 1);
  xiiGeometryResidencyManager::UnregisterGeometry(handle, 1);
  xiiGeometryResidencyManager::ProcessStreaming(2, 1, 0);
  description.m_Lods[0].m_uiMeshletCount = 1;
  const auto replacement                 = xiiGeometryResidencyManager::RegisterGeometry(description);
  XII_SCOPE_EXIT(xiiGeometryResidencyManager::UnregisterGeometry(replacement, 2));
  XII_TEST_BOOL(replacement != handle);
  XII_TEST_BOOL(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(handle, 24, 12, xiiMakeArrayPtr(counts)).Failed());
  XII_TEST_RESULT(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(replacement, 24, 12, xiiMakeArrayPtr(counts)));
  XII_TEST_INT(counts[0], 0);
}
