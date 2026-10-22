/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeFrameData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>

class xiiDDGIProbeTestRetain : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDDGIProbeTestRetain, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Irradiance{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureInputPin  m_Distance{xiiGALResourceFormat::RG32Float, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_States{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin m_IrradianceOut{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_DistanceOut{xiiGALResourceFormat::RG32Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineBufferOutputPin  m_StatesOut{false, xiiGALBindFlags::ShaderResource, xiiRenderPipelinePinAccess::Read};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphTextureHandle m_Irradiance, m_Distance;
      xiiRenderGraphBufferHandle  m_States;
    };
    const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Irradiance=builder.ReadTexture(context.GetInputTexture(m_Irradiance),xiiGALResourceStateFlags::ShaderResource); builder.ExportTexture(data.m_Irradiance,xiiGALResourceStateFlags::ShaderResource);
      data.m_Distance=builder.ReadTexture(context.GetInputTexture(m_Distance),xiiGALResourceStateFlags::ShaderResource); builder.ExportTexture(data.m_Distance,xiiGALResourceStateFlags::ShaderResource);
      data.m_States=builder.ReadBuffer(context.GetInputBuffer(m_States),xiiGALResourceStateFlags::ShaderResource); builder.ExportBuffer(data.m_States,xiiGALResourceStateFlags::ShaderResource); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_IrradianceOut, pass.first->m_Irradiance));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_DistanceOut, pass.first->m_Distance));
    return context.SetOutput(m_StatesOut, pass.first->m_States);
  }
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDDGIProbeTestRetain,1,xiiRTTIDefaultAllocator<xiiDDGIProbeTestRetain>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Irradiance",m_Irradiance)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("Distance",m_Distance)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("ProbeStates",m_States)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("IrradianceOut",m_IrradianceOut)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("DistanceOut",m_DistanceOut)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("ProbeStatesOut",m_StatesOut)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
namespace
{
  class DDGIProbeMemoryLoader : public xiiResourceLoaderFromMemory
  {
  public:
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> m_Closed;
    void                                                  CloseDataStream(const xiiResource* resource, const xiiResourceLoadData& data) override
    {
      xiiResourceLoaderFromMemory::CloseDataStream(resource, data);
      m_Closed->m_Content.RaiseSignal();
    }
  };
  bool ReloadProbeDefinition(const xiiRenderPipelineResourceHandle& resource, xiiRenderPipelineResourceDescriptor definition)
  {
    for (auto& pass : definition.m_Passes)
      if (pass.m_sType == "xiiDDGIEnvironmentSeedPass")
        pass.m_Properties.Insert("AsyncCompute", true);
    const auto                          closed = xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>>(XII_DEFAULT_NEW(xiiRefCountedContainer<xiiThreadSignal>));
    xiiUniquePtr<DDGIProbeMemoryLoader> loader = XII_DEFAULT_NEW(DDGIProbeMemoryLoader);
    loader->m_Closed                           = closed;
    xiiMemoryStreamWriter stream(&loader->m_CustomData);
    stream << xiiString("DDGIEnvironmentProbes.xiiBinRenderPipeline");
    xiiAssetFileHeader header;
    if (!XII_TEST_RESULT(header.Write(stream)))
      return false;
    stream << xiiUInt8(3);
    if (!XII_TEST_RESULT(definition.Serialize(stream)))
      return false;
    xiiResourceManager::UpdateResourceWithCustomLoader(resource, std::move(loader));
    return XII_TEST_BOOL(closed->m_Content.WaitForSignal(xiiTime::MakeFromSeconds(10)) == xiiThreadSignal::WaitResult::Signaled);
  }
  bool ReadProbeAtlas(xiiGALDevice& device, xiiGALTexture* source, xiiDynamicArray<xiiVec4>& values)
  {
    xiiGALTextureReadback readback(&device);
    auto                  commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    const auto enqueued = readback.Enqueue(commands, {.m_pTexture = source});
    commands->End();
    if (!XII_TEST_RESULT(enqueued))
      return false;
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    readback.WaitForNextCompleted();
    const auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(static_cast<bool>(capture)))
      return false;
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    if (!XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
      return false;
    values.SetCount(capture.m_uiWidth * capture.m_uiHeight);
    for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
    {
      const auto* row = xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt8*>(mapped.m_pData), y * mapped.m_uiStride);
      for (xiiUInt32 x = 0; x < capture.m_uiWidth; ++x)
        if (source->GetDescription().m_Format == xiiGALResourceFormat::RGBA16Float)
        {
          const auto* p                     = reinterpret_cast<const xiiFloat16*>(row) + x * 4;
          values[y * capture.m_uiWidth + x] = xiiVec4(float(p[0]), float(p[1]), float(p[2]), float(p[3]));
        }
        else
        {
          const auto* p                     = reinterpret_cast<const float*>(row) + x * 2;
          values[y * capture.m_uiWidth + x] = xiiVec4(p[0], p[1], 0, 0);
        }
    }
    XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
    commands->End();
    return true;
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeDDGIProbes);
XII_CREATE_SIMPLE_TEST(NativeDDGIProbes, RepeatedViewsBudgetsHistoryReloadAndQueuedRetirement)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource("DDGIEnvironmentProbes");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto authored = loaded->GetAuthoring();
  xiiUuid    scheduleId;
  for (const auto& pass : authored->m_Descriptor.m_Passes)
    if (pass.m_sType == "xiiDDGIProbeSchedulePass")
      scheduleId = pass.m_Id;
  if (!XII_TEST_BOOL(scheduleId.IsValid()))
    return;
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authored->m_Descriptor.m_Id, asset.GetResourceID()});
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto node       = root.AddSubgraph(i ? "Second probe field" : "First probe field", authored->m_Descriptor.m_Id);
    auto&      properties = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    properties.m_Path.PushBack(scheduleId);
    properties.m_Properties.Insert("ProbeCountX", i ? 2U : 4U);
    properties.m_Properties.Insert("ProbeCountY", 2U);
    properties.m_Properties.Insert("ProbeCountZ", i ? 2U : 4U);
    properties.m_Properties.Insert("Spacing", i ? 3.0f : 2.0f);
    properties.m_Properties.Insert("UpdateBudget", 8U);
    properties.m_Properties.Insert("Hysteresis", 0.5f);
    properties.m_Properties.Insert("UseViewCamera", false);
    properties.m_Properties.Insert("CameraPosition", i ? xiiVec3(100) : xiiVec3::MakeZero());
    root.m_Interface.PushBack({i ? "BEnvironment" : "AEnvironment", true, root.m_Passes[node].m_Id, "EnvironmentLight"});
    root.m_Interface.PushBack({i ? "BSchedule" : "ASchedule", false, root.m_Passes[node].m_Id, "Schedule"});
    const auto retain = root.AddPass(i ? "Retain second field" : "Retain first field", "xiiDDGIProbeTestRetain");
    for (const xiiStringView pin : {"Irradiance", "Distance", "ProbeStates"})
    {
      XII_TEST_RESULT(root.Connect(node, pin, retain, pin));
      root.m_Interface.PushBack({xiiStringBuilder(i ? "B" : "A", pin), false, root.m_Passes[retain].m_Id, xiiStringBuilder(pin, "Out")});
    }
  }
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Repeated independent environment probes", std::move(root));
  xiiRenderGraphTestContext contexts[2];
  xiiRenderPipelineInstance instances[2];
  xiiDynamicArray<xiiVec4>  expectedIrradiance[2][2], expectedDistance[2][2];
  for (xiiUInt32 v = 0; v < 2; ++v)
    if (!XII_TEST_RESULT(contexts[v].Initialize("Independent DDGI views")) || !XII_TEST_RESULT(instances[v].Synchronize(resource)))
      return;
  XII_TEST_BOOL(instances[0].GetPipeline() == instances[1].GetPipeline());
  XII_TEST_BOOL(instances[0].GetPipeline()->GetRuntimeFeatures().IsNoFlagSet() && instances[0].GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
  for (xiiUInt32 phase = 0; phase < 17; ++phase)
  {
    if (phase == 7)
      for (auto& instance : instances)
        instance.InvalidateTemporalHistory();
    if (phase == 8)
    {
      const auto previous = instances[0].GetPipeline();
      if (!ReloadProbeDefinition(asset, authored->m_Descriptor))
        return;
      for (auto& instance : instances)
      {
        bool changed = false;
        if (!XII_TEST_RESULT(instance.Synchronize(resource, &changed)))
          return;
        XII_TEST_BOOL(changed && instance.GetPipeline() != previous);
      }
      XII_TEST_BOOL(instances[0].GetPipeline() == instances[1].GetPipeline());
    }
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiSharedPtr<xiiGALTexture> physical[2][2];
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
          xiiEnvironmentLightConstants m_Value;
          xiiRenderGraphBufferHandle   m_Buffer;
        };
        xiiEnvironmentLightConstants value{};
        value.EnvironmentDirectionAndIlluminance = xiiVec4(0, 0, -1, float(phase + 4 + view));
        value.EnvironmentColorAndAngularRadius   = field ? xiiVec4(0.5f, 1, 2, 0) : xiiVec4(1, 0.5f, 0.25f, 0);
        value.EnvironmentAmbientRadiance         = field ? xiiVec4(4, 2, 1, 0) : xiiVec4(1, 2, 3, 0);
        targets[field]                           = value.EnvironmentAmbientRadiance.GetAsVec3() + value.EnvironmentColorAndAngularRadius.GetAsVec3() * value.EnvironmentDirectionAndIlluminance.w * 0.25f;
        const auto source                        = graph.AddPass<Source>(field ? "Second explicit environment" : "First explicit environment", xiiGALCommandQueueFlags::Graphics, [&](Source& data, xiiRenderGraphBuilder& builder) { data.m_Value=value; xiiGALBufferCreationDescription description; description.m_uiSize=sizeof(value); description.m_BindFlags=xiiGALBindFlags::UniformBuffer; description.m_Usage=xiiGALResourceUsage::Dynamic; description.m_CPUAccessFlags=xiiGALCPUAccessFlag::Write; data.m_Buffer=builder.WriteBuffer(field ? "Second environment" : "First environment",description,xiiGALResourceStateFlags::ConstantBuffer); }, [](const Source& data, xiiRenderGraphPassContext& execute) { { xiiGALMapHelper<xiiEnvironmentLightConstants> mapped(execute.GetCommandList(),execute.GetBuffer(data.m_Buffer),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=data.m_Value; } xiiGALStateTransitionDescription transition; transition.m_pResource=execute.GetBuffer(data.m_Buffer); transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState; execute.GetCommandList().TransitionResourceStates(xiiMakeArrayPtr(&transition,1)); });
        inputs.PushBack(xiiRenderPipelineInputBinding(graph, instances[view].GetPipeline()->FindInterface(field ? "BEnvironment" : "AEnvironment", true), source.first->m_Buffer));
      }
      xiiVariantDictionary parameters;
      if (phase >= 4)
        parameters.Insert("DDGICameraPosition", phase == 6 ? xiiVec3(10000) : xiiVec3(2.1f, 0, 0));
      const auto built = instances[view].Build(nullptr, graph, contexts[view].GetBlackboard(), parameters, inputs);
      if (!XII_TEST_RESULT_MSG(built, "%s", instances[view].GetLastBuildError().GetStartPointer()))
        return;
      xiiSharedPtr<const xiiDDGIProbeFrameData> snapshots[2];
      xiiRenderGraphTextureHandle               irradiance[2], distance[2];
      xiiRenderGraphBufferHandle                states[2];
      for (xiiUInt32 field = 0; field < 2; ++field)
      {
        const auto binding = [&](xiiStringView suffix) {
          return instances[view].GetInterfaceBinding(instances[view].GetPipeline()->FindInterface(xiiStringBuilder(field ? "B" : "A", suffix), false));
        };
        snapshots[field]     = binding("Schedule")->m_pFrameData.Downcast<const xiiDDGIProbeFrameData>();
        const auto& snapshot = *snapshots[field];
        irradiance[field]    = binding("Irradiance")->m_hTexture;
        distance[field]      = binding("Distance")->m_hTexture;
        states[field]        = binding("ProbeStates")->m_hBuffer;
        auto& expected       = expectedIrradiance[view][field];
        auto& moments        = expectedDistance[view][field];
        if (!snapshot.m_bHistoryValid)
        {
          expected.SetCount(snapshot.m_AtlasSize.width * snapshot.m_AtlasSize.height);
          moments.SetCount(expected.GetCount());
          xiiMemoryUtils::ZeroFill(expected.GetData(), expected.GetCount());
          xiiMemoryUtils::ZeroFill(moments.GetData(), moments.GetCount());
        }
        if (phase == 0 || phase == 7 || phase == 8)
          XII_TEST_BOOL(!snapshot.m_bHistoryValid);
        if (phase > 0 && phase < 7)
          XII_TEST_BOOL(snapshot.m_bHistoryValid);
        for (const auto& update : snapshot.m_Updates)
        {
          const auto index   = update.m_vMetadata.x;
          const auto weight  = update.m_vPositionAndHistoryWeight.w;
          expected[index]    = xiiVec4(targets[field] * (1 - weight) + expected[index].GetAsVec3() * weight, 1);
          const auto maximum = snapshot.m_Constants.SpacingHysteresisDistance.z;
          moments[index]     = xiiVec4(maximum, maximum * maximum, 0, 0);
        }
      }
      if (phase == 16)
        instances[view].Clear();
      graph.EndSetup();
      xiiStringBuilder error;
      const auto       compiled = graph.Compile({}, &error);
      if (!XII_TEST_RESULT_MSG(compiled, "%s", error.GetData()))
        return;
      if (!XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache)))
        return;
      if (phase == 0)
        XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
      if (phase == 0)
        xiiTestFramework::Output(xiiTestOutput::Details, "DDGI view %u: %u compiled operations, %u queue submissions, %u barriers.", view, graph.GetStatistics().m_uiCompiledPassCount, graph.GetStatistics().m_uiQueueSubmissionCount, graph.GetStatistics().m_uiTotalBarrierCount);
      for (xiiUInt32 field = 0; field < 2; ++field)
      {
        physical[view][field] = graph.GetExportedTexture(irradiance[field]);
        if (phase > 8 && phase < 16)
          continue; // Eight consecutive frames submit without explicit host idle waits.
        xiiDynamicArray<xiiVec4> readIrradiance, readDistance;
        if (!ReadProbeAtlas(*environment.GetDevice(), physical[view][field].Borrow(), readIrradiance) || !ReadProbeAtlas(*environment.GetDevice(), graph.GetExportedTexture(distance[field]).Borrow(), readDistance))
          return;
        XII_TEST_INT(readIrradiance.GetCount(), expectedIrradiance[view][field].GetCount());
        for (xiiUInt32 i = 0; i < readIrradiance.GetCount(); ++i)
        {
          XII_TEST_BOOL(readIrradiance[i].IsEqual(expectedIrradiance[view][field][i], 0.02f));
          XII_TEST_BOOL(readDistance[i].IsEqual(expectedDistance[view][field][i], 0.0001f));
        }
        for (xiiUInt32 i = 0; i < snapshots[field]->m_Probes.GetCount(); ++i)
        {
          xiiGpuDDGIProbeState record;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(states[field]).Borrow(), i * sizeof(record), record))
            return;
          auto expected = snapshots[field]->m_Probes[i];
          for (const auto& update : snapshots[field]->m_Updates)
            if (update.m_vMetadata.x == i)
            {
              expected.m_vCellAndFlags.w          = (expected.m_vCellAndFlags.w | xiiDDGIProbeFlags::Valid) & ~xiiDDGIProbeFlags::NeedsUpdate;
              expected.m_vPositionAndLastUpdate.w = float(frame.GetFrameIndex());
            }
          XII_TEST_BOOL(record.m_vCellAndFlags == expected.m_vCellAndFlags && record.m_vPositionAndLastUpdate == expected.m_vPositionAndLastUpdate);
        }
      }
      XII_TEST_BOOL(physical[view][0] != physical[view][1]);
    }
    XII_TEST_BOOL(physical[0][0] != physical[1][0] && physical[0][1] != physical[1][1]);
    frame.End();
  }
}

XII_CREATE_SIMPLE_TEST(NativeDDGIProbes, AuthoritativeWorldAndIndependentCameraHistory)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 directionalAsset = xiiLoadPipelineLibraryResource("WorldDirectionalLight"), environmentAsset = xiiLoadPipelineLibraryResource("EnvironmentLightConstants"), probesAsset = xiiLoadPipelineLibraryResource("DDGIEnvironmentProbes");
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
    if (pass.m_sType == "xiiDDGIProbeSchedulePass")
    {
      auto& properties = root.m_Passes[field].m_Overrides.ExpandAndGetRef();
      properties.m_Path.PushBack(pass.m_Id);
      properties.m_Properties.Insert("ProbeCountX", 2U);
      properties.m_Properties.Insert("ProbeCountY", 2U);
      properties.m_Properties.Insert("ProbeCountZ", 2U);
      properties.m_Properties.Insert("UpdateBudget", 8U);
      properties.m_Properties.Insert("Hysteresis", 0.5f);
      properties.m_Properties.Insert("Spacing", 2.0f);
    }
  XII_TEST_RESULT(root.Connect(sun, "DirectionalLight", constants, "DirectionalLight"));
  XII_TEST_RESULT(root.Connect(constants, "EnvironmentLight", field, "EnvironmentLight"));
  const auto retain = root.AddPass("Retain camera field", "xiiDDGIProbeTestRetain");
  for (const xiiStringView pin : {"Irradiance", "Distance", "ProbeStates"})
  {
    XII_TEST_RESULT(root.Connect(field, pin, retain, pin));
    root.m_Interface.PushBack({pin, false, root.m_Passes[retain].m_Id, xiiStringBuilder(pin, "Out")});
  }
  root.m_Interface.PushBack({"Schedule", false, root.m_Passes[field].m_Id, "Schedule"});
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("World environment probe cameras", std::move(root));
  xiiWorldDescription description("Authoritative world GI extraction");
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
    xiiSharedPtr<xiiGALTexture> physical[2];
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      const auto& instance = views[v]->GetRenderPipelineInstance();
      const auto  binding  = [&](xiiStringView name) {
        return instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
      };
      if (!XII_TEST_BOOL(binding("Schedule") && binding("Schedule")->m_pFrameData && binding("Irradiance") && binding("Irradiance")->m_hTexture.IsValid()))
        return;
      const auto snapshot = binding("Schedule")->m_pFrameData.Downcast<const xiiDDGIProbeFrameData>();
      if (!snapshot->m_bHistoryValid)
      {
        expected[v].SetCount(9);
        xiiMemoryUtils::ZeroFill(expected[v].GetData(), expected[v].GetCount());
      }
      const auto color  = xiiEvaluateLightColor(xiiColorLinearUB(255, 255, 255), light->GetTemperature());
      const auto target = xiiVec3(0.03f, 0.035f, 0.04f) + xiiVec3(color.r, color.g, color.b) * (phase >= 2 ? 200.0f : 100.0f) * 0.25f;
      for (const auto& update : snapshot->m_Updates)
      {
        const auto index   = update.m_vMetadata.x;
        const auto weight  = update.m_vPositionAndHistoryWeight.w;
        expected[v][index] = xiiVec4(target * (1 - weight) + expected[v][index].GetAsVec3() * weight, 1);
      }
      const auto minimum = snapshot->m_Constants.MinimumCellAndAtlasWidth;
      XII_TEST_INT(minimum.x, xiiInt32(cameras[v].GetPosition().x / 2) - 1);
      XII_TEST_INT(minimum.y, -1);
      XII_TEST_INT(minimum.z, -1);
      physical[v] = views[v]->GetRenderGraph()->GetExportedTexture(binding("Irradiance")->m_hTexture);
      xiiDynamicArray<xiiVec4> values;
      if (!ReadProbeAtlas(*environment.GetDevice(), physical[v].Borrow(), values))
        return;
      XII_TEST_INT(values.GetCount(), expected[v].GetCount());
      for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
        XII_TEST_BOOL(values[i].IsEqual(expected[v][i], 0.04f));
    }
    XII_TEST_BOOL(physical[0] != physical[1]);
  }
}
