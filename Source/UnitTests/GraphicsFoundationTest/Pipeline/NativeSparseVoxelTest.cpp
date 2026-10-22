/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelEnvironmentSeedPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelSchedulePass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/BufferExportPass.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelFrameData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
namespace
{
  class SparseMemoryLoader : public xiiResourceLoaderFromMemory
  {
  public:
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> m_Closed;
    void                                                  CloseDataStream(const xiiResource* resource, const xiiResourceLoadData& data) override
    {
      xiiResourceLoaderFromMemory::CloseDataStream(resource, data);
      m_Closed->m_Content.RaiseSignal();
    }
  };
  bool ReloadSparseDefinition(const xiiRenderPipelineResourceHandle& resource, xiiRenderPipelineResourceDescriptor definition)
  {
    for (auto& pass : definition.m_Passes)
      if (pass.m_pType == xiiSparseVoxelEnvironmentSeedPass().GetDynamicRTTI())
        pass.m_Properties.Insert("AsyncCompute", true);
    const auto                       closed = xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>>(XII_DEFAULT_NEW(xiiRefCountedContainer<xiiThreadSignal>));
    xiiUniquePtr<SparseMemoryLoader> loader = XII_DEFAULT_NEW(SparseMemoryLoader);
    loader->m_Closed                        = closed;
    xiiMemoryStreamWriter stream(&loader->m_CustomData);
    stream << xiiString("SparseVoxelEnvironment.xiiBinRenderPipeline");
    xiiAssetFileHeader header;
    if (!XII_TEST_RESULT(header.Write(stream)))
      return false;
    stream << xiiUInt8(3);
    if (!XII_TEST_RESULT(definition.Serialize(stream)))
      return false;
    xiiResourceManager::UpdateResourceWithCustomLoader(resource, std::move(loader));
    return XII_TEST_BOOL(closed->m_Content.WaitForSignal(xiiTime::MakeFromSeconds(10)) == xiiThreadSignal::WaitResult::Signaled);
  }
  bool ReadSparsePool(xiiGALDevice& device, xiiGALBuffer* source, xiiDynamicArray<xiiVec4>& values)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiSize         = source->GetDescription().m_uiSize;
    description.m_Usage          = xiiGALResourceUsage::Staging;
    description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
    auto staging                 = device.CreateBuffer(description);
    auto commands                = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (!XII_TEST_BOOL(staging && commands))
      return false;
    commands->Begin();
    const auto previous = source->GetResourceState();
    commands->CopyBuffer(source, staging.Borrow());
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = source;
    transition.m_NewState        = previous;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    commands->End();
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    commands->Begin();
    void* mapped = nullptr;
    if (!XII_TEST_RESULT(commands->MapBuffer(staging.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, mapped)) || !XII_TEST_BOOL(mapped))
      return false;
    values.SetCount(xiiUInt32(description.m_uiSize / sizeof(xiiVec4)));
    xiiMemoryUtils::Copy(values.GetData(), static_cast<const xiiVec4*>(mapped), values.GetCount());
    XII_TEST_RESULT(commands->UnmapBuffer(staging.Borrow(), xiiGALMapType::Read));
    commands->End();
    return true;
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeSparseVoxel);
XII_CREATE_SIMPLE_TEST(NativeSparseVoxel, IndependentResidencyPagesBudgetsRefreshHistoryAndReload)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource("SparseVoxelEnvironment");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto authored = loaded->GetAuthoring();
  xiiUuid    scheduleId;
  for (const auto& pass : authored->m_Descriptor.m_Passes)
    if (pass.m_pType == xiiSparseVoxelSchedulePass().GetDynamicRTTI())
      scheduleId = pass.m_Id;
  if (!XII_TEST_BOOL(scheduleId.IsValid()))
    return;
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authored->m_Descriptor.m_Id, asset.GetResourceID()});
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto node       = root.AddSubgraph(i ? "Far field B" : "Far field A", authored->m_Descriptor.m_Id);
    auto&      properties = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    properties.m_Path.PushBack(scheduleId);
    properties.m_Properties.Insert("ClipmapLevels", i ? 1U : 2U);
    properties.m_Properties.Insert("BrickGridResolution", 2U);
    properties.m_Properties.Insert("BrickVoxelResolution", 2U);
    properties.m_Properties.Insert("ResidentBricks", i ? 8U : 16U);
    properties.m_Properties.Insert("UpdateBudget", 4U);
    properties.m_Properties.Insert("RefreshInterval", 6U);
    properties.m_Properties.Insert("BaseVoxelSize", i ? 1.0f : 0.5f);
    properties.m_Properties.Insert("Hysteresis", 0.5f);
    properties.m_Properties.Insert("UseViewCamera", false);
    properties.m_Properties.Insert("CameraPosition", i ? xiiVec3(-100) : xiiVec3::MakeZero());
    root.m_Interface.PushBack({i ? "BEnvironment" : "AEnvironment", true, root.m_Passes[node].m_Id, "EnvironmentLight"});
    root.m_Interface.PushBack({i ? "BSchedule" : "ASchedule", false, root.m_Passes[node].m_Id, "Schedule"});
    for (const xiiStringView pin : {"RadiancePool", "PageTable", "LevelData"})
    {
      const auto retain = root.AddPass(xiiStringBuilder(i ? "B Retain " : "A Retain ", pin), xiiBufferExportPass().GetDynamicRTTI());
      XII_TEST_RESULT(root.Connect(node, pin, retain, "Input"));
      root.m_Interface.PushBack({xiiStringBuilder(i ? "B" : "A", pin), false, root.m_Passes[retain].m_Id, "Output"});
    }
  }
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Independent sparse clipmap instances", std::move(root));
  xiiRenderGraphTestContext contexts[2];
  xiiRenderPipelineInstance instances[2];
  xiiDynamicArray<xiiVec4>  expected[2][2];
  for (xiiUInt32 v = 0; v < 2; ++v)
    if (!XII_TEST_RESULT(contexts[v].Initialize("Sparse field view")) || !XII_TEST_RESULT(instances[v].Synchronize(resource)))
      return;
  XII_TEST_BOOL(instances[0].GetPipeline() == instances[1].GetPipeline());
  XII_TEST_BOOL(instances[0].GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && instances[0].GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
  bool idleObserved = false, reuseObserved = false;
  for (xiiUInt32 phase = 0; phase < 18; ++phase)
  {
    if (phase == 8)
      for (auto& instance : instances)
        instance.InvalidateTemporalHistory();
    if (phase == 9)
    {
      const auto* before = instances[0].GetPipeline();
      if (!ReloadSparseDefinition(asset, authored->m_Descriptor))
        return;
      for (auto& instance : instances)
      {
        bool changed = false;
        if (!XII_TEST_RESULT(instance.Synchronize(resource, &changed)))
          return;
        XII_TEST_BOOL(changed && instance.GetPipeline() != before);
      }
    }
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiSharedPtr<xiiGALBuffer> physical[2][2];
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      auto& graph = contexts[view].GetGraph();
      auto& cache = contexts[view].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      xiiHybridArray<xiiRenderPipelineInputBinding, 2> inputs;
      xiiVec3                                          targets[2];
      for (xiiUInt32 field = 0; field < 2; ++field)
      {
        struct Source
        {
          xiiRenderGraphBufferHandle   m_Buffer;
          xiiEnvironmentLightConstants m_Value;
        };
        xiiEnvironmentLightConstants value{};
        value.EnvironmentDirectionAndIlluminance = xiiVec4(0, 0, -1, float(10 + phase + view));
        value.EnvironmentColorAndAngularRadius   = field ? xiiVec4(1, 0.5f, 0.25f, 0) : xiiVec4(0.25f, 0.5f, 1, 0);
        value.EnvironmentAmbientRadiance         = field ? xiiVec4(4, 2, 1, 0) : xiiVec4(1, 2, 4, 0);
        targets[field]                           = value.EnvironmentAmbientRadiance.GetAsVec3() + value.EnvironmentColorAndAngularRadius.GetAsVec3() * value.EnvironmentDirectionAndIlluminance.w * 0.08f;
        const auto source                        = graph.AddPass<Source>(field ? "B environment" : "A environment", xiiGALCommandQueueFlags::Graphics, [&](Source& data, xiiRenderGraphBuilder& builder) { data.m_Value=value; xiiGALBufferCreationDescription d; d.m_uiSize=sizeof(value); d.m_BindFlags=xiiGALBindFlags::UniformBuffer; d.m_Usage=xiiGALResourceUsage::Dynamic; d.m_CPUAccessFlags=xiiGALCPUAccessFlag::Write; data.m_Buffer=builder.WriteBuffer(field ? "Environment B":"Environment A",d,xiiGALResourceStateFlags::ConstantBuffer); }, [](const Source& data, xiiRenderGraphPassContext& execute) { auto& commands=execute.GetCommandList(); { xiiGALMapHelper<xiiEnvironmentLightConstants> mapped(commands,execute.GetBuffer(data.m_Buffer),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=data.m_Value; } xiiGALStateTransitionDescription transition; transition.m_pResource=execute.GetBuffer(data.m_Buffer); transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState; commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1)); });
        inputs.PushBack(xiiRenderPipelineInputBinding(graph, instances[view].GetPipeline()->FindInterface(field ? "BEnvironment" : "AEnvironment", true), source.first->m_Buffer));
      }
      xiiVariantDictionary parameters;
      if (phase >= 5)
        parameters.Insert("SparseVoxelCameraPosition", phase == 7 ? xiiVec3(10000) : xiiVec3(1.1f, 0, 0));
      const auto built = instances[view].Build(nullptr, graph, contexts[view].GetBlackboard(), parameters, inputs);
      if (!XII_TEST_RESULT_MSG(built, "%s", instances[view].GetLastBuildError().GetStartPointer()))
        return;
      xiiSharedPtr<const xiiSparseVoxelFrameData> snapshots[2];
      xiiRenderGraphBufferHandle                  pool[2], pages[2], levels[2];
      xiiDynamicArray<xiiUInt32>                  expectedPages[2];
      for (xiiUInt32 field = 0; field < 2; ++field)
      {
        const auto binding = [&](xiiStringView suffix) {
          return instances[view].GetInterfaceBinding(instances[view].GetPipeline()->FindInterface(xiiStringBuilder(field ? "B" : "A", suffix), false));
        };
        snapshots[field]     = binding("Schedule")->m_pFrameData.Downcast<const xiiSparseVoxelFrameData>();
        const auto& snapshot = *snapshots[field];
        pool[field]          = binding("RadiancePool")->m_hBuffer;
        pages[field]         = binding("PageTable")->m_hBuffer;
        levels[field]        = binding("LevelData")->m_hBuffer;
        if (!snapshot.m_bHistoryValid)
        {
          expected[view][field].SetCount(snapshot.m_PoolCount);
          xiiMemoryUtils::ZeroFill(expected[view][field].GetData(), snapshot.m_PoolCount);
        }
        if (phase == 0 || phase == 8 || phase == 9)
          XII_TEST_BOOL(!snapshot.m_bHistoryValid);
        else
          XII_TEST_BOOL(snapshot.m_bHistoryValid);
        expectedPages[field] = snapshot.m_Pages;
        idleObserved |= snapshot.m_Updates.IsEmpty();
        for (const auto& update : snapshot.m_Updates)
        {
          float      weight = 0;
          const auto bits   = update.m_vPhysicalLevelAndKey.w;
          xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&weight), reinterpret_cast<const xiiUInt8*>(&bits), sizeof(weight));
          const auto first = update.m_vPhysicalLevelAndKey.x * snapshot.m_Constants.PoolLayout.x;
          reuseObserved |= weight == 0 && expected[view][field][first].w > 0 && snapshot.m_bHistoryValid;
          for (xiiUInt32 i = 0; i < snapshot.m_Constants.PoolLayout.x; ++i)
            expected[view][field][first + i] = xiiVec4(targets[field] * (1 - weight) + expected[view][field][first + i].GetAsVec3() * weight, 1);
          xiiUInt32  level;
          xiiVec3I32 cell;
          xiiSparseVoxelRadianceContext::UnpackBrickKey(xiiUInt64(update.m_vPhysicalLevelAndKey.y) | (xiiUInt64(update.m_vPhysicalLevelAndKey.z) << 32U), level, cell);
          const auto minimum                                                                                                        = snapshot.m_Levels[level].m_vMinimumCellAndPageOffset;
          const auto local                                                                                                          = cell - xiiVec3I32(minimum.x, minimum.y, minimum.z);
          const auto grid                                                                                                           = snapshot.m_Constants.ClipmapAndUpdateCounts.y;
          expectedPages[field][xiiUInt32(minimum.w) + (xiiUInt32(local.z) * grid + xiiUInt32(local.y)) * grid + xiiUInt32(local.x)] = update.m_vPhysicalLevelAndKey.x;
        }
      }
      if (phase == 17)
        instances[view].Clear();
      graph.EndSetup();
      xiiStringBuilder error;
      const auto       compiled = graph.Compile({}, &error);
      if (!XII_TEST_RESULT_MSG(compiled, "%s", error.GetData()))
        return;
      if (!XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache)))
        return;
      if (phase == 0)
      {
        XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
        xiiTestFramework::Output(xiiTestOutput::Details, "Sparse view %u: %u operations, %u submissions, %u barriers.", view, graph.GetStatistics().m_uiCompiledPassCount, graph.GetStatistics().m_uiQueueSubmissionCount, graph.GetStatistics().m_uiTotalBarrierCount);
      }
      for (xiiUInt32 field = 0; field < 2; ++field)
      {
        physical[view][field] = graph.GetExportedBuffer(pool[field]);
        if (phase >= 10 && phase < 17)
          continue;
        xiiDynamicArray<xiiVec4> values;
        if (!ReadSparsePool(*environment.GetDevice(), physical[view][field].Borrow(), values))
          return;
        XII_TEST_INT(values.GetCount(), expected[view][field].GetCount());
        for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
          XII_TEST_BOOL(values[i].IsEqual(expected[view][field][i], 0.0001f));
        for (xiiUInt32 i = 0; i < expectedPages[field].GetCount(); ++i)
        {
          xiiUInt32 value;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(pages[field]).Borrow(), i * sizeof(value), value))
            return;
          XII_TEST_INT(value, expectedPages[field][i]);
        }
        for (xiiUInt32 i = 0; i < snapshots[field]->m_Levels.GetCount(); ++i)
        {
          xiiGpuSparseVoxelLevel value;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(levels[field]).Borrow(), i * sizeof(value), value))
            return;
          XII_TEST_BOOL(value.m_vMinimumCellAndPageOffset == snapshots[field]->m_Levels[i].m_vMinimumCellAndPageOffset && value.m_vVoxelAndBrickSize == snapshots[field]->m_Levels[i].m_vVoxelAndBrickSize);
        }
      }
      XII_TEST_BOOL(physical[view][0] != physical[view][1]);
    }
    XII_TEST_BOOL(physical[0][0] != physical[1][0] && physical[0][1] != physical[1][1]);
    frame.End();
  }
  XII_TEST_BOOL(idleObserved && reuseObserved);
}

XII_CREATE_SIMPLE_TEST(NativeSparseVoxel, AuthoritativeWorldAndIndependentCameraHistory)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 directionalAsset = xiiLoadPipelineLibraryResource("WorldDirectionalLight"), environmentAsset = xiiLoadPipelineLibraryResource("EnvironmentLightConstants"), probesAsset = xiiLoadPipelineLibraryResource("SparseVoxelEnvironment");
  xiiResourceLock<xiiRenderPipelineResource> directional(directionalAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), illumination(environmentAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), probes(probesAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(directional.IsValid() && illumination.IsValid() && probes.IsValid() && directional->GetAuthoring() && illumination->GetAuthoring() && probes->GetAuthoring()))
    return;
  const auto                          d = directional->GetAuthoring(), e = illumination->GetAuthoring(), p = probes->GetAuthoring();
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({d->m_Descriptor.m_Id, directionalAsset.GetResourceID()});
  root.m_References.PushBack({e->m_Descriptor.m_Id, environmentAsset.GetResourceID()});
  root.m_References.PushBack({p->m_Descriptor.m_Id, probesAsset.GetResourceID()});
  const auto sun = root.AddSubgraph("Authoritative sun", d->m_Descriptor.m_Id), constants = root.AddSubgraph("Environment illumination", e->m_Descriptor.m_Id), field = root.AddSubgraph("Camera probes", p->m_Descriptor.m_Id);
  for (const auto& pass : p->m_Descriptor.m_Passes)
    if (pass.m_pType == xiiSparseVoxelSchedulePass().GetDynamicRTTI())
    {
      auto& properties = root.m_Passes[field].m_Overrides.ExpandAndGetRef();
      properties.m_Path.PushBack(pass.m_Id);
      properties.m_Properties.Insert("ClipmapLevels", 1U);
      properties.m_Properties.Insert("BrickGridResolution", 2U);
      properties.m_Properties.Insert("BrickVoxelResolution", 2U);
      properties.m_Properties.Insert("ResidentBricks", 8U);
      properties.m_Properties.Insert("UpdateBudget", 8U);
      properties.m_Properties.Insert("RefreshInterval", 1U);
      properties.m_Properties.Insert("Hysteresis", 0.5f);
      properties.m_Properties.Insert("BaseVoxelSize", 1.0f);
    }
  XII_TEST_RESULT(root.Connect(sun, "DirectionalLight", constants, "DirectionalLight"));
  XII_TEST_RESULT(root.Connect(constants, "EnvironmentLight", field, "EnvironmentLight"));
  const auto retain = root.AddPass("Retain camera field", xiiBufferExportPass().GetDynamicRTTI());
  XII_TEST_RESULT(root.Connect(field, "RadiancePool", retain, "Input"));
  root.m_Interface.PushBack({"RadiancePool", false, root.m_Passes[retain].m_Id, "Output"});
  root.m_Interface.PushBack({"Schedule", false, root.m_Passes[field].m_Id, "Schedule"});
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("World sparse radiance cameras", std::move(root));
  xiiWorldDescription description("Authoritative sparse radiance world");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*          module       = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiGameObject* object       = nullptr;
  const auto     objectHandle = world.CreateObject({}, object);
  XII_IGNORE_UNUSED(objectHandle);
  xiiDirectionalLightComponent* light = nullptr;
  xiiDirectionalLightComponent::CreateComponent(object, light);
  light->SetIntensityUnit(xiiPhotometricUnit::Lux);
  light->SetIntensity(100);
  xiiCamera cameras[2];
  xiiView*  views[2];
  for (xiiUInt32 v = 0; v < 2; ++v)
  {
    cameras[v].SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    const xiiVec3 position(float(v * 100), 0, 0);
    cameras[v].LookAt(position, position + xiiVec3::MakeAxisX(), xiiVec3::MakeAxisZ());
    module->CreateView(v ? "Second world GI camera" : "First world GI camera", views[v]);
    views[v]->SetCamera(&cameras[v]);
    views[v]->SetViewport({0, 0, 8, 8});
    views[v]->SetRenderPipelineResource(resource);
  }
  xiiDynamicArray<xiiVec4> expected[2];
  for (xiiUInt32 phase = 0; phase < 4; ++phase)
  {
    if (phase == 2)
      light->SetIntensity(200);
    if (phase == 3)
      cameras[1].LookAt(xiiVec3(200, 0, 0), xiiVec3(201, 0, 0), xiiVec3::MakeAxisZ());
    world.Update();
    environment.GetDevice()->WaitIdle();
    XII_TEST_BOOL(module->GetExtractionRequirements() == xiiRenderExtractionCategory::Lights && views[0]->GetRuntimeFeatures().IsNoFlagSet());
    XII_TEST_BOOL(module->GetWorldLightData() == nullptr && module->GetGpuInstanceData() == nullptr && !xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    xiiSharedPtr<xiiGALBuffer> physical[2];
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      const auto& instance = views[v]->GetRenderPipelineInstance();
      const auto  binding  = [&](xiiStringView name) {
        return instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
      };
      if (!XII_TEST_BOOL(binding("Schedule") && binding("Schedule")->m_pFrameData && binding("RadiancePool") && binding("RadiancePool")->m_hBuffer.IsValid()))
        return;
      const auto snapshot = binding("Schedule")->m_pFrameData.Downcast<const xiiSparseVoxelFrameData>();
      if (!snapshot->m_bHistoryValid)
      {
        expected[v].SetCount(snapshot->m_PoolCount);
        xiiMemoryUtils::ZeroFill(expected[v].GetData(), expected[v].GetCount());
      }
      const auto color  = xiiEvaluateLightColor(xiiColorLinearUB(255, 255, 255), light->GetTemperature());
      const auto target = xiiVec3(0.03f, 0.035f, 0.04f) + xiiVec3(color.r, color.g, color.b) * (phase >= 2 ? 200.0f : 100.0f) * 0.08f;
      for (const auto& update : snapshot->m_Updates)
      {
        float      weight = 0;
        const auto bits   = update.m_vPhysicalLevelAndKey.w;
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&weight), reinterpret_cast<const xiiUInt8*>(&bits), sizeof(weight));
        const auto first = update.m_vPhysicalLevelAndKey.x * snapshot->m_Constants.PoolLayout.x;
        for (xiiUInt32 i = 0; i < snapshot->m_Constants.PoolLayout.x; ++i)
          expected[v][first + i] = xiiVec4(target * (1 - weight) + expected[v][first + i].GetAsVec3() * weight, 1);
      }
      const auto minimum = snapshot->m_Levels[0].m_vMinimumCellAndPageOffset;
      XII_TEST_INT(minimum.x, xiiInt32(cameras[v].GetPosition().x / 2) - 1);
      XII_TEST_INT(minimum.y, -1);
      XII_TEST_INT(minimum.z, -1);
      physical[v] = views[v]->GetRenderGraph()->GetExportedBuffer(binding("RadiancePool")->m_hBuffer);
      xiiDynamicArray<xiiVec4> values;
      if (!ReadSparsePool(*environment.GetDevice(), physical[v].Borrow(), values))
        return;
      XII_TEST_INT(values.GetCount(), expected[v].GetCount());
      for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
        XII_TEST_BOOL(values[i].IsEqual(expected[v][i], 0.04f));
    }
    XII_TEST_BOOL(physical[0] != physical[1]);
  }
}
