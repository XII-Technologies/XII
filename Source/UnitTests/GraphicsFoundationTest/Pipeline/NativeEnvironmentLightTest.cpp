/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/IESProfileResource.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>

/// Test readback root with an explicit uniform-buffer contract.
class xiiEnvironmentTestRetain : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEnvironmentTestRetain, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin  m_Input{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferOutputPin m_Output{false, xiiGALBindFlags::UniformBuffer, xiiRenderPipelinePinAccess::Read};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    const auto input = context.GetInputBuffer(m_Input);
    const auto pass  = context.m_Graph.AddPass<xiiRenderGraphBufferHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) { data = builder.ReadBuffer(input, xiiGALResourceStateFlags::ConstantBuffer); builder.ExportBuffer(data, xiiGALResourceStateFlags::ConstantBuffer); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return context.SetOutput(m_Output, *pass.first);
  }
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEnvironmentTestRetain, 1, xiiRTTIDefaultAllocator<xiiEnvironmentTestRetain>)
{
  XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Input", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("Output", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly), } XII_END_PROPERTIES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_CREATE_SIMPLE_TEST_GROUP(NativeEnvironmentLight);
XII_CREATE_SIMPLE_TEST(NativeEnvironmentLight, RepeatedAuthoredEmittersRetireBeforeRecording)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource("EnvironmentLightConstants");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
    return;
  const auto                          authored = loaded->GetAuthoring();
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authored->m_Descriptor.m_Id, asset.GetResourceID()});
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto node       = root.AddSubgraph(i ? "Second environment" : "First environment", authored->m_Descriptor.m_Id);
    auto&      properties = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    properties.m_Path.PushBack(authored->m_Descriptor.m_Passes[0].m_Id);
    properties.m_Properties.Insert("Illuminance", i ? 250.0f : 100.0f);
    properties.m_Properties.Insert("Direction", i ? xiiVec3(0, 2, 0) : xiiVec3(0, 0, -3));
    properties.m_Properties.Insert("Color", i ? xiiVec3(0.1f, 0.5f, 1) : xiiVec3(1, 0.5f, 0.1f));
    properties.m_Properties.Insert("AmbientRadiance", i ? xiiVec3(0.6f, 0.4f, 0.2f) : xiiVec3(0.2f, 0.4f, 0.6f));
    const auto retain = root.AddPass(i ? "Retain second environment" : "Retain first environment", "xiiEnvironmentTestRetain");
    XII_TEST_RESULT(root.Connect(node, "EnvironmentLight", retain, "Input"));
    root.m_Interface.PushBack({i ? "Second" : "First", false, root.m_Passes[retain].m_Id, "Output"});
  }
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Independent environment snapshots", std::move(root));
  xiiRenderGraphTestContext context;
  xiiRenderPipelineInstance instance;
  if (!XII_TEST_RESULT(context.Initialize("Independent illumination")) || !XII_TEST_RESULT(instance.Synchronize(resource)))
    return;
  XII_TEST_BOOL(instance.GetPipeline()->GetRuntimeFeatures().IsNoFlagSet() && instance.GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = context.GetGraph();
  auto& cache = context.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame());
  graph.BeginSetup(frame.GetFrameIndex());
  if (!XII_TEST_RESULT(instance.Build(nullptr, graph, context.GetBlackboard())))
    return;
  const auto first  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("First", false))->m_hBuffer;
  const auto second = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Second", false))->m_hBuffer;
  instance.Clear();
  graph.EndSetup();
  if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
    return;
  frame.End();
  environment.GetDevice()->WaitIdle();
  auto firstBuffer = graph.GetExportedBuffer(first), secondBuffer = graph.GetExportedBuffer(second);
  XII_TEST_BOOL(firstBuffer != secondBuffer);
  xiiEnvironmentLightConstants a, b;
  if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), firstBuffer.Borrow(), 0, a) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), secondBuffer.Borrow(), 0, b))
    return;
  XII_TEST_BOOL(a.EnvironmentDirectionAndIlluminance == xiiVec4(0, 0, -1, 100));
  XII_TEST_BOOL(b.EnvironmentDirectionAndIlluminance == xiiVec4(0, 1, 0, 250));
  XII_TEST_BOOL(a.EnvironmentColorAndAngularRadius == xiiVec4(1, 0.5f, 0.1f, 0));
  XII_TEST_BOOL(b.EnvironmentColorAndAngularRadius == xiiVec4(0.1f, 0.5f, 1, 0));
  XII_TEST_BOOL(a.EnvironmentAmbientRadiance == xiiVec4(0.2f, 0.4f, 0.6f, 0));
  XII_TEST_BOOL(b.EnvironmentAmbientRadiance == xiiVec4(0.6f, 0.4f, 0.2f, 0));
}

XII_CREATE_SIMPLE_TEST(NativeEnvironmentLight, WorldSelectionTagsLifecycleAndMinimalDemand)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 worldAsset = xiiLoadPipelineLibraryResource("WorldDirectionalLight"), constantsAsset = xiiLoadPipelineLibraryResource("EnvironmentLightConstants");
  xiiResourceLock<xiiRenderPipelineResource> worldSource(worldAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), constantsSource(constantsAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(worldSource.IsValid() && worldSource->GetAuthoring() && constantsSource.IsValid() && constantsSource->GetAuthoring()))
    return;
  const auto                          worldDefinition = worldSource->GetAuthoring(), constantsDefinition = constantsSource->GetAuthoring();
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({worldDefinition->m_Descriptor.m_Id, worldAsset.GetResourceID()});
  root.m_References.PushBack({constantsDefinition->m_Descriptor.m_Id, constantsAsset.GetResourceID()});
  const auto source    = root.AddSubgraph("World directional selection", worldDefinition->m_Descriptor.m_Id);
  const auto constants = root.AddSubgraph("Environment constants", constantsDefinition->m_Descriptor.m_Id);
  auto&      authored  = root.m_Passes[constants].m_Overrides.ExpandAndGetRef();
  authored.m_Path.PushBack(constantsDefinition->m_Descriptor.m_Passes[0].m_Id);
  authored.m_Properties.Insert("Illuminance", 9999.0f); // A connected world snapshot wins, including an empty world.
  authored.m_Properties.Insert("AmbientRadiance", xiiVec3(0.3f, 0.4f, 0.5f));
  const auto retain = root.AddPass("Retain environment", "xiiEnvironmentTestRetain");
  XII_TEST_RESULT(root.Connect(source, "DirectionalLight", constants, "DirectionalLight"));
  XII_TEST_RESULT(root.Connect(constants, "EnvironmentLight", retain, "Input"));
  root.m_Interface.PushBack({"EnvironmentLight", false, root.m_Passes[retain].m_Id, "Output"});
  const auto          pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("World directional environment", std::move(root));
  xiiWorldDescription description("Specialized directional environment");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView* views[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    module->CreateView(i ? "Tagged sky" : "All sky", views[i]);
    views[i]->SetCamera(&camera);
    views[i]->SetViewport({0, 0, 8, 8});
    views[i]->SetRenderPipelineResource(pipeline);
  }
  const auto& hidden = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Native.Environment.Hidden");
  views[1]->m_ExcludeTags.Set(hidden);
  xiiGameObjectDescription objectDescription;
  objectDescription.m_bDynamic = true;
  xiiGameObject* objectA       = nullptr;
  xiiGameObject* objectB       = nullptr;
  const auto     objectHandleA = world.CreateObject(objectDescription, objectA);
  const auto     objectHandleB = world.CreateObject(objectDescription, objectB);
  XII_IGNORE_UNUSED(objectHandleA);
  XII_IGNORE_UNUSED(objectHandleB);
  xiiTagSet tags;
  tags.Set(hidden);
  objectA->SetTags(tags);
  xiiDirectionalLightComponent* a       = nullptr;
  xiiDirectionalLightComponent* b       = nullptr;
  auto                          handleA = xiiDirectionalLightComponent::CreateComponent(objectA, a);
  xiiDirectionalLightComponent::CreateComponent(objectB, b);
  for (auto* light : {a, b})
  {
    light->SetIntensityUnit(xiiPhotometricUnit::Lux);
    light->SetIntensity(100);
  }
  a->SetTemperature(3000);
  b->SetTemperature(9000);
  // An unrelated point profile must never be acquired by a directional-only graph.
  xiiPointLightComponent* point = nullptr;
  xiiPointLightComponent::CreateComponent(objectB, point);
  point->SetIESProfile(xiiResourceManager::LoadResource<xiiIESProfileResource>("UnconsumedEnvironmentProfile.ies"));
  for (xiiUInt32 phase = 0; phase < 5; ++phase)
  {
    if (phase == 1)
      b->SetIntensity(200);
    if (phase == 2)
    {
      b->SetActiveFlag(false);
      objectA->SetLocalRotation(xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(90)));
    }
    if (phase == 3)
    {
      a->GetOwningManager()->DeleteComponent(handleA);
      a = nullptr;
    }
    if (phase == 4)
    {
      handleA = xiiDirectionalLightComponent::CreateComponent(objectA, a);
      a->SetIntensityUnit(xiiPhotometricUnit::Lux);
      a->SetIntensity(300);
      a->SetTemperature(5000);
    }
    world.Update();
    environment.GetDevice()->WaitIdle();
    XII_TEST_BOOL(module->GetExtractionRequirements() == xiiRenderExtractionCategory::Lights && views[0]->GetRuntimeFeatures().IsNoFlagSet());
    XII_TEST_BOOL(module->GetWorldLightData() == nullptr && module->GetGpuInstanceData() == nullptr && !xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      const auto& execution = views[view]->GetRenderPipelineInstance();
      const auto* binding   = execution.GetInterfaceBinding(execution.GetPipeline()->FindInterface("EnvironmentLight", false));
      if (!XII_TEST_BOOL(binding && binding->m_hBuffer.IsValid()))
        return;
      xiiEnvironmentLightConstants value;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), views[view]->GetRenderGraph()->GetExportedBuffer(binding->m_hBuffer).Borrow(), 0, value))
        return;
      const bool empty   = phase == 3 || (view == 1 && phase >= 2);
      const bool selectB = phase == 1 || (view == 1 && phase == 0);
      XII_TEST_FLOAT(value.EnvironmentDirectionAndIlluminance.w, empty ? 0.0f : selectB ? (phase == 1 ? 200.0f : 100.0f) :
                       phase == 4                                                       ? 300.0f :
                                                                                          100.0f,
                     0.0001f);
      XII_TEST_BOOL(value.EnvironmentAmbientRadiance == xiiVec4(0.3f, 0.4f, 0.5f, 0));
      if (!empty)
      {
        const auto color = xiiEvaluateLightColor(xiiColorLinearUB(255, 255, 255), selectB ? 9000U : phase == 4 ? 5000U :
                                                                                                                 3000U);
        XII_TEST_FLOAT(value.EnvironmentColorAndAngularRadius.x, color.r, 0.0001f);
        XII_TEST_FLOAT(value.EnvironmentColorAndAngularRadius.y, color.g, 0.0001f);
        XII_TEST_FLOAT(value.EnvironmentColorAndAngularRadius.z, color.b, 0.0001f);
        const auto direction = value.EnvironmentDirectionAndIlluminance.GetAsVec3();
        XII_TEST_BOOL(direction.IsEqual(!selectB && phase >= 2 ? xiiVec3(0, 1, 0) : xiiVec3(1, 0, 0), 0.0001f));
      }
    }
  }
}
