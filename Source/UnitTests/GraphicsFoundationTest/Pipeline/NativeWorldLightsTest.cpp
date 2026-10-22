/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterViewPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/WorldLightsUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/BufferExportPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/CounterResetPass.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/DiscAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Components/Lights/RectangleAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/SphereAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/SpotLightComponent.h>
#include <GraphicsCore/Components/Lights/TubeAreaLightComponent.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/IESProfileResource.h>
#include <GraphicsCore/Lighting/WorldLightData.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
namespace
{
  class NativeLightProfileLoader : public xiiResourceLoaderFromMemory
  {
  public:
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> m_Closed;
    void                                                  CloseDataStream(const xiiResource* resource, const xiiResourceLoadData& data) override
    {
      xiiResourceLoaderFromMemory::CloseDataStream(resource, data);
      m_Closed->m_Content.RaiseSignal();
    }
  };
  bool ReloadLightProfile(const xiiIESProfileResourceHandle& profile, bool changed)
  {
    const auto                             closed = xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>>(XII_DEFAULT_NEW(xiiRefCountedContainer<xiiThreadSignal>));
    xiiUniquePtr<NativeLightProfileLoader> loader = XII_DEFAULT_NEW(NativeLightProfileLoader);
    loader->m_Closed                              = closed;
    xiiMemoryStreamWriter stream(&loader->m_CustomData);
    stream << xiiString("NativeWorldLights.ies");
    const xiiString source = changed ? "IESNA:LM-63-2002\nTILT=NONE\n1 1000 1 3 1 1 2 0 0 0\n1 1 10\n0 90 180\n0\n100 100 0\n" : "IESNA:LM-63-2002\nTILT=NONE\n1 1000 1 3 1 1 2 0 0 0\n1 1 10\n0 90 180\n0\n50 100 50\n";
    if (!XII_TEST_RESULT(stream.WriteBytes(source.GetData(), source.GetElementCount())))
      return false;
    xiiResourceManager::UpdateResourceWithCustomLoader(profile, std::move(loader));
    {
      xiiResourceLock<xiiIESProfileResource> loaded(profile, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
        return false;
    }
    return XII_TEST_BOOL(closed->m_Content.WaitForSignal(xiiTime::MakeFromSeconds(10)) == xiiThreadSignal::WaitResult::Signaled);
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeWorldLights);
XII_CREATE_SIMPLE_TEST(NativeWorldLights, SharedViewsIncrementalUploadAndComponentLifetime)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource("WorldLightsUpload");
  xiiResourceLock<xiiRenderPipelineResource> source(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(source.IsValid() && source->GetAuthoring() && source->GetPipeline()))
    return;
  const auto                          authored = source->GetAuthoring();
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authored->m_Descriptor.m_Id, asset.GetResourceID()});
  const auto upload = root.AddSubgraph("Shared world light producer", authored->m_Descriptor.m_Id);
  for (const xiiStringView name : {"Lights", "IESProfiles", "Counts", "Candidates"})
  {
    const auto retain = root.AddPass(xiiStringBuilder("Retain ", name), xiiGetStaticRTTI<xiiBufferExportPass>());
    XII_TEST_RESULT(root.Connect(upload, name, retain, "Input"));
    root.m_Interface.PushBack({name, false, root.m_Passes[retain].m_Id, "Output"});
  }
  const auto                                 clusterAsset = xiiLoadPipelineLibraryResource("ClusterLightLists");
  xiiResourceLock<xiiRenderPipelineResource> clusterDefinition(clusterAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(clusterDefinition.IsValid() && clusterDefinition->GetAuthoring()))
    return;
  const auto clusters = clusterDefinition->GetAuthoring();
  root.m_References.PushBack({clusters->m_Descriptor.m_Id, clusterAsset.GetResourceID()});
  const auto clustering = root.AddSubgraph("View selected light lists", clusters->m_Descriptor.m_Id);
  XII_TEST_RESULT(root.Connect(upload, "Lights", clustering, "Lights"));
  XII_TEST_RESULT(root.Connect(upload, "Candidates", clustering, "Candidates"));
  for (const auto& node : clusters->m_Descriptor.m_Passes)
    if (node.m_pType == xiiGetStaticRTTI<xiiLightClusterViewPass>())
    {
      auto& override = root.m_Passes[clustering].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(node.m_Id);
      override.m_Properties.Insert("DepthSlices", 1U);
      override.m_Properties.Insert("MaxLightsPerCluster", 8U);
    }
  for (const xiiStringView name : {"LightGrid", "LightIndices", "Overflow", "PackedIndices", "SamplingTotals"})
  {
    const auto retain = root.AddPass(xiiStringBuilder("Retain ", name), xiiGetStaticRTTI<xiiBufferExportPass>());
    XII_TEST_RESULT(root.Connect(clustering, name, retain, "Input"));
    root.m_Interface.PushBack({name, false, root.m_Passes[retain].m_Id, "Output"});
  }
  const auto pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Native world light extraction", std::move(root));
  const auto profile  = xiiResourceManager::LoadResource<xiiIESProfileResource>("NativeWorldLights.ies");
  if (!ReloadLightProfile(profile, false))
    return;
  xiiWorldDescription description("Authoritative light extraction");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView* views[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    module->CreateView(i ? "Filtered light view" : "All light view", views[i]);
    views[i]->SetCamera(&camera);
    views[i]->SetViewport({0, 0, 32, 32});
    views[i]->SetRenderPipelineResource(pipeline);
  }
  const auto& hidden = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Native.WorldLights.Hidden");
  views[1]->m_ExcludeTags.Set(hidden);
  xiiGameObject*           parent = nullptr;
  xiiGameObjectDescription objectDescription;
  objectDescription.m_bDynamic      = true;
  objectDescription.m_LocalPosition = xiiVec3(1, 2, 3);
  const auto parentHandle           = world.CreateObject(objectDescription, parent);
  objectDescription.m_hParent       = parentHandle;
  objectDescription.m_LocalPosition = xiiVec3(2, 0, 0);
  xiiComponentHandle  handles[7];
  xiiGameObjectHandle objectHandles[7];
  const auto          create = [&]<typename T>(xiiUInt32 index) -> T* {
    xiiGameObject* object   = nullptr;
    auto           authored = objectDescription;
    if (index != 1)
    {
      authored.m_hParent.Invalidate();
      authored.m_bDynamic      = false;
      authored.m_LocalPosition = xiiVec3(3, 2, 3);
    }
    objectHandles[index] = world.CreateObject(authored, object);
    T* component         = nullptr;
    handles[index]       = T::CreateComponent(object, component);
    component->SetCastShadows(true);
    component->SetIntensityUnit(index == 0 ? xiiPhotometricUnit::Lux : index <= 2 ? xiiPhotometricUnit::Candela :
                                                                                    xiiPhotometricUnit::Nit);
    component->SetIntensity(100);
    return component;
  };
  create.operator()<xiiDirectionalLightComponent>(0);
  auto* point = create.operator()<xiiPointLightComponent>(1);
  point->SetRange(10);
  point->SetIESProfile(profile);
  auto* spot = create.operator()<xiiSpotLightComponent>(2);
  spot->SetRange(10);
  spot->SetIESProfile(profile);
  create.operator()<xiiRectangleAreaLightComponent>(3);
  create.operator()<xiiDiscAreaLightComponent>(4);
  create.operator()<xiiSphereAreaLightComponent>(5);
  create.operator()<xiiTubeAreaLightComponent>(6);
  xiiGameObject* pointObject = nullptr;
  if (!XII_TEST_BOOL(world.TryGetObject(objectHandles[1], pointObject)))
    return;
  xiiTagSet tags;
  tags.Set(hidden);
  pointObject->SetTags(tags);
  xiiUInt32 originalIdentity = 0, reenabledIdentity = 0, previousProfileRevision = 0;
  for (xiiUInt32 phase = 0; phase < 10; ++phase)
  {
    if (phase == 9 && !ReloadLightProfile(profile, true))
      return;
    if (phase == 3)
      pointObject->SetLocalPosition(xiiVec3(5, 0, 0));
    if (phase == 4)
      point->SetActiveFlag(false);
    if (phase == 5)
      point->SetActiveFlag(true);
    if (phase == 6)
    {
      const auto removed = handles[1];
      point->GetOwningManager()->DeleteComponent(removed);
      handles[1] = xiiPointLightComponent::CreateComponent(pointObject, point);
      point->SetCastShadows(true);
      point->SetIntensityUnit(xiiPhotometricUnit::Candela);
      point->SetIntensity(100);
      point->SetRange(10);
      point->SetIESProfile(profile);
      xiiComponentHandle resolved;
      XII_TEST_BOOL(module->GetWorldLightData()->Resolve(reenabledIdentity, resolved).Failed());
    }
    if (phase == 7 || phase == 8)
      for (const auto handle : handles)
      {
        xiiComponent* component = nullptr;
        if (world.TryGetComponent(handle, component))
          component->SetActiveFlag(phase == 8);
      }
    world.Update();
    environment.GetDevice()->WaitIdle();
    const auto* lights = module->GetWorldLightData();
    if (!XII_TEST_BOOL(lights != nullptr))
      return;
    const xiiUInt32 active = phase == 7 ? 0 : phase == 4 ? 6 :
                                                           7;
    XII_TEST_INT(lights->GetLights().GetCount(), active);
    if (phase != 7)
      for (xiiUInt32 i = 0; i < 7; ++i)
      {
        if (i == 1 && phase == 4)
          continue;
        const auto index = lights->GetIndex(handles[i]);
        if (!XII_TEST_BOOL(index != xiiInvalidIndex))
          return;
        const auto& record = lights->GetLights()[index];
        XII_TEST_INT(record.m_Metadata.z, i);
        XII_TEST_INT(record.m_Metadata.y, index);
        XII_TEST_FLOAT(record.m_ColorAndIntensity.w, 100.0f, 0.0001f);
        XII_TEST_FLOAT(record.m_OrientationRightAndIES.w, i == 1 || i == 2 ? 1.0f : 0.0f, 0.0001f);
        XII_TEST_BOOL(record.m_BoundsCenterAndRadius.IsValid());
      }
    XII_TEST_BOOL(module->GetGpuInstanceData() == nullptr && !xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    XII_TEST_BOOL(module->GetExtractionRequirements() == xiiRenderExtractionCategory::Lights);
    if (phase != 4 && phase != 7)
    {
      const auto index = lights->GetIndex(handles[1]);
      if (!XII_TEST_BOOL(index != xiiInvalidIndex))
        return;
      const auto& record = lights->GetLights()[index];
      XII_TEST_FLOAT(record.m_PositionAndInvRange.x, phase >= 3 ? 6.0f : 3.0f, 0.0001f);
      XII_TEST_FLOAT(record.m_PositionAndInvRange.y, 2.0f, 0.0001f);
      XII_TEST_FLOAT(record.m_ColorAndIntensity.w, 100.0f, 0.0001f);
      XII_TEST_FLOAT(record.m_OrientationRightAndIES.w, 1.0f, 0.0001f);
      xiiComponentHandle resolved;
      XII_TEST_RESULT(lights->Resolve(record.m_Metadata.x, resolved));
      XII_TEST_BOOL(resolved == handles[1]);
      if (phase == 0)
        originalIdentity = record.m_Metadata.x;
      if (phase == 8)
        previousProfileRevision = record.m_Metadata.w;
      if (phase == 9)
        XII_TEST_BOOL(record.m_Metadata.w != previousProfileRevision);
      if (phase <= 3)
        XII_TEST_INT(record.m_Metadata.x, originalIdentity);
      if (phase == 5)
      {
        reenabledIdentity = record.m_Metadata.x;
        XII_TEST_BOOL(reenabledIdentity != originalIdentity);
        XII_TEST_BOOL(lights->Resolve(originalIdentity, resolved).Failed());
      }
      if (phase == 6)
        XII_TEST_BOOL(record.m_Metadata.x != reenabledIdentity);
    }
    xiiSharedPtr<xiiGALBuffer> recordBuffers[2], profileBuffers[2];
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      const auto& execution = views[view]->GetRenderPipelineInstance();
      const auto* graph     = views[view]->GetRenderGraph();
      const auto  buffer    = [&](xiiStringView name) {
        const auto* binding = execution.GetInterfaceBinding(execution.GetPipeline()->FindInterface(name, false));
        return graph->GetExportedBuffer(binding->m_hBuffer);
      };
      recordBuffers[view]  = buffer("Lights");
      profileBuffers[view] = buffer("IESProfiles");
      xiiVec4U32 counts;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer("Counts").Borrow(), 0, counts))
        return;
      XII_TEST_INT(counts.x, active);
      XII_TEST_INT(counts.y, active ? 1 : 0);
      XII_TEST_INT(counts.z, active ? 1 : 0);
      const auto candidates = buffer("Candidates");
      const auto selected   = phase == 7 ? 0U : view == 1 || phase == 4 ? 6U :
                                                                          7U;
      XII_TEST_INT(candidates->GetDescription().m_uiSize, xiiMath::Max(selected, 1U) * sizeof(xiiUInt32));
      struct ClusterGrid
      {
        XII_DECLARE_POD_TYPE();
        xiiVec2U32 m_Cells[4];
      } grid;
      struct ClusterIndices
      {
        XII_DECLARE_POD_TYPE();
        xiiUInt32 m_Values[32];
      } indices;
      xiiUInt32 overflow = 1;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer("LightGrid").Borrow(), 0, grid) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer("LightIndices").Borrow(), 0, indices) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer("Overflow").Borrow(), 0, overflow))
        return;
      XII_TEST_INT(overflow, 0);
      bool       pointAssigned = false;
      const auto pointIndex    = lights->GetIndex(handles[1]);
      for (const auto& cell : grid.m_Cells)
      {
        if (!XII_TEST_BOOL(cell.y <= 8 && cell.x <= 24))
          return;
        for (xiiUInt32 i = 0; i < cell.y; ++i)
        {
          const auto index = indices.m_Values[cell.x + i];
          XII_TEST_BOOL(index < active);
          if (index == pointIndex)
            pointAssigned = true;
          if (view == 1)
            XII_TEST_BOOL(index != pointIndex);
        }
        if (!selected)
          XII_TEST_INT(cell.y, 0);
      }
      XII_TEST_BOOL(pointAssigned == (view == 0 && phase != 4 && phase != 7));
      for (xiiUInt32 i = 0; i < xiiMath::Max(selected, 1U); ++i)
      {
        xiiUInt32 candidate = 0;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), candidates.Borrow(), i * sizeof(candidate), candidate))
          return;
        if (!selected)
        {
          XII_TEST_INT(candidate, xiiInvalidIndex);
          continue;
        }
        XII_TEST_BOOL(candidate < active);
        if (view == 1)
          XII_TEST_BOOL(candidate != lights->GetIndex(handles[1]));
        xiiGpuLightData record;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), recordBuffers[view].Borrow(), candidate * sizeof(record), record))
          return;
        XII_TEST_BOOL(xiiMemoryUtils::IsEqual(&record, &lights->GetLights()[candidate], 1));
      }
      float sample = 0;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), profileBuffers[view].Borrow(), 0, sample))
        return;
      XII_TEST_FLOAT(sample, !active ? 0.0f : phase == 9 ? 1.0f :
                                                           0.5f,
                     0.00001f);
    }
    XII_TEST_BOOL(recordBuffers[0] == recordBuffers[1] && profileBuffers[0] == profileBuffers[1]);
    const auto& stats = lights->GetUploadStatistics();
    if (phase == 2)
      XII_TEST_INT(stats.m_uiBytesUploaded, 0);
    if (phase == 3)
    {
      XII_TEST_INT(stats.m_uiBytesUploaded, sizeof(xiiGpuLightData));
      XII_TEST_INT(stats.m_uiRanges, 1);
      XII_TEST_INT(stats.m_uiUploadPasses, 1);
    }
    xiiTestFramework::Output(xiiTestOutput::Details, "World light phase %u: %u records, %llu shared uploaded bytes in %u ranges.", phase, active, stats.m_uiBytesUploaded, stats.m_uiRanges);
  }
  // Submit both cameras over a burst without host waits; slots must obey shared frame completion.
  for (xiiUInt32 frame = 0; frame < 8; ++frame)
  {
    pointObject->SetLocalPosition(xiiVec3(10.0f + float(frame), 0, 0));
    world.Update();
  }
  environment.GetDevice()->WaitIdle();
  {
    const auto*     lights    = module->GetWorldLightData();
    const auto      index     = lights->GetIndex(handles[1]);
    const auto&     execution = views[0]->GetRenderPipelineInstance();
    const auto*     binding   = execution.GetInterfaceBinding(execution.GetPipeline()->FindInterface("Lights", false));
    xiiGpuLightData record;
    if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), views[0]->GetRenderGraph()->GetExportedBuffer(binding->m_hBuffer).Borrow(), index * sizeof(record), record))
      return;
    XII_TEST_FLOAT(record.m_PositionAndInvRange.x, 18.0f, 0.0001f);
    XII_TEST_INT(record.m_Metadata.x, lights->GetLights()[index].m_Metadata.x);
  }
  // A specialized pipeline with no light demand releases derived allocations and dispatches no light extraction.
  xiiRenderPipelineResourceDescriptor minimal;
  const auto                          counter = minimal.AddPass("Diagnostic counter", xiiGetStaticRTTI<xiiCounterResetPass>());
  const auto                          retain  = minimal.AddPass("Retain diagnostic counter", xiiGetStaticRTTI<xiiBufferExportPass>());
  XII_TEST_RESULT(minimal.Connect(counter, "Counter", retain, "Input"));
  const auto noLights = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("No light demand", std::move(minimal));
  for (auto* view : views)
    view->SetRenderPipelineResource(noLights);
  world.Update();
  environment.GetDevice()->WaitIdle();
  XII_TEST_BOOL(module->GetWorldLightData() == nullptr);
  XII_TEST_BOOL(module->GetExtractionRequirements().IsNoFlagSet());
}

XII_CREATE_SIMPLE_TEST(NativeWorldLights, WorldRetirementAndRecreationBeforeRecording)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor definition;
  const auto                          source = definition.AddPass("World light source", xiiGetStaticRTTI<xiiWorldLightsUploadPass>());
  for (const xiiStringView name : {"Lights", "IESProfiles", "Counts", "Candidates"})
  {
    const auto retain = definition.AddPass(xiiStringBuilder("Retain ", name), xiiGetStaticRTTI<xiiBufferExportPass>());
    XII_TEST_RESULT(definition.Connect(source, name, retain, "Input"));
    definition.m_Interface.PushBack({name, false, definition.m_Passes[retain].m_Id, "Output"});
  }
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("World light owner retirement", std::move(definition));
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Retained world light uploads")))
    return;
  xiiUInt32 retiredIdentity = 0;
  for (xiiUInt32 iteration = 0; iteration < 2; ++iteration)
  {
    xiiWorldDescription     description("Retired light world");
    xiiUniquePtr<xiiWorld>  world  = XII_DEFAULT_NEW(xiiWorld, description);
    xiiView*                view   = nullptr;
    xiiRenderWorldModule*   module = nullptr;
    xiiPointLightComponent* point  = nullptr;
    xiiCamera               camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    xiiComponentHandle handle;
    xiiUInt32          identity = 0;
    {
      XII_LOCK(world->GetWriteMarker());
      world->SetWorldSimulationEnabled(false);
      module = world->GetOrCreateModule<xiiRenderWorldModule>();
      module->CreateView("Retiring view", view);
      view->SetCamera(&camera);
      view->SetViewport({0, 0, 16, 16});
      view->SetRenderPipelineResource(resource);
      xiiGameObject*           object = nullptr;
      xiiGameObjectDescription authored;
      const auto               created = world->CreateObject(authored, object);
      XII_IGNORE_UNUSED(created);
      handle = xiiPointLightComponent::CreateComponent(object, point);
      point->SetIntensityUnit(xiiPhotometricUnit::Candela);
      point->SetIntensity(100);
      world->Update();
      environment.GetDevice()->WaitIdle();
      const auto* data = module->GetWorldLightData();
      if (!XII_TEST_BOOL(data && data->GetLights().GetCount() == 1))
        return;
      identity = data->GetLights()[0].m_Metadata.x;
      xiiComponentHandle resolved;
      if (iteration)
      {
        XII_TEST_BOOL(identity != retiredIdentity);
        XII_TEST_BOOL(data->Resolve(retiredIdentity, resolved).Failed());
      }
      point->SetIntensity(200.0f + float(iteration));
      xiiExtractedRenderData  packets;
      xiiMsgExtractRenderData request;
      request.m_pView                = view;
      request.m_pExtractedRenderData = &packets;
      point->SendMessage(request);
      packets.SortAndBatches();
      if (!XII_TEST_RESULT(module->UpdateWorldLights(packets.GetAllRenderData())))
        return;
    }
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiRenderPipelineInstance pipeline;
    if (!XII_TEST_RESULT(pipeline.Synchronize(resource)))
      return;
    {
      XII_LOCK(world->GetReadMarker());
      const auto built = pipeline.Build(view, graph, context.GetBlackboard());
      if (!XII_TEST_RESULT_MSG(built, "%s", pipeline.GetLastBuildError().GetStartPointer()))
        return;
    }
    const auto output = [&](xiiStringView name) {
      return pipeline.GetInterfaceBinding(pipeline.GetPipeline()->FindInterface(name, false))->m_hBuffer;
    };
    const auto lights = output("Lights"), counts = output("Counts"), candidates = output("Candidates");
    pipeline.Clear();
    world.Clear();
    module = nullptr;
    view   = nullptr;
    point  = nullptr;
    graph.EndSetup();
    xiiStringBuilder error;
    const auto       compiled = graph.Compile({.m_bEnableAsyncQueues = false}, &error);
    if (!XII_TEST_RESULT_MSG(compiled, "%s", error.GetData()))
      return;
    const auto executed = graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error);
    if (!XII_TEST_RESULT_MSG(executed, "%s", error.GetData()))
      return;
    frame.End();
    environment.GetDevice()->WaitIdle();
    xiiGpuLightData record;
    xiiVec4U32      metadata;
    xiiUInt32       candidate = xiiInvalidIndex;
    if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(lights).Borrow(), 0, record) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(counts).Borrow(), 0, metadata) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(candidates).Borrow(), 0, candidate))
      return;
    XII_TEST_INT(record.m_Metadata.x, identity);
    XII_TEST_INT(candidate, 0);
    XII_TEST_INT(metadata.x, 1);
    XII_TEST_FLOAT(record.m_ColorAndIntensity.w, 200.0f + float(iteration), 0.0001f);
    retiredIdentity = identity;
  }
}
