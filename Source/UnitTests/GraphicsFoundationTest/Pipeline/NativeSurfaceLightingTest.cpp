/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/IESProfileResource.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
class xiiNativeSurfaceInputs : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativeSurfaceInputs, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureOutputPin m_Depth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin m_Normal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin m_Albedo{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin m_Material{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin m_Emissive{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin m_Visibility{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin  m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto* parameter = context.m_Parameters.GetValue("SurfaceTestPhase");
    const auto  phase     = parameter ? parameter->Get<xiiUInt32>() : 0U;
    struct Data
    {
      xiiRenderGraphTextureHandle m_Depth, m_Normal, m_Albedo, m_Material, m_Emissive, m_Visibility;
      xiiRenderGraphBufferHandle  m_Camera;
      xiiUInt32                   m_Phase;
    };
    const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Phase=phase;
      const auto texture=[&](xiiStringView name,xiiGALResourceFormat::Enum format){xiiGALTextureCreationDescription d; d.m_Type=xiiGALResourceDimension::Texture2D; d.m_Size={1,1}; d.m_Format=format; d.m_BindFlags=xiiGALBindFlags::ShaderResource|xiiGALBindFlags::RenderTarget; return builder.WriteTexture(name,d,xiiGALResourceStateFlags::RenderTarget);};
      data.m_Depth=texture("Surface Depth",xiiGALResourceFormat::R32Float); data.m_Normal=texture("Surface Normal",xiiGALResourceFormat::RG16Float); data.m_Albedo=texture("Surface Albedo",xiiGALResourceFormat::RGBA16Float); data.m_Material=texture("Surface Material",xiiGALResourceFormat::RGBA16Float); data.m_Emissive=texture("Surface Emissive",xiiGALResourceFormat::RGBA16Float); data.m_Visibility=texture("Visibility",xiiGALResourceFormat::R32Float);
      xiiGALBufferCreationDescription d; d.m_uiSize=sizeof(xiiFrameCameraConstants); d.m_BindFlags=xiiGALBindFlags::UniformBuffer; d.m_Usage=xiiGALResourceUsage::Dynamic; d.m_CPUAccessFlags=xiiGALCPUAccessFlag::Write; data.m_Camera=builder.WriteBuffer("Exact Camera",d,xiiGALResourceStateFlags::ConstantBuffer); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands=execute.GetCommandList();
      const auto clear=[&](xiiRenderGraphTextureHandle handle,xiiColor color){auto* texture=execute.GetTexture(handle); const auto state=texture->GetResourceState(); commands.ClearRenderTargetView(texture->GetDefaultView(xiiGALTextureViewType::RenderTarget),color); xiiGALStateTransitionDescription t; t.m_pResource=texture; t.m_NewState=state; t.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState; commands.TransitionResourceStates(xiiMakeArrayPtr(&t,1));};
      clear(data.m_Depth,{data.m_Phase==6 ? 0.0f:0.5f,0,0,0}); clear(data.m_Normal,{0.5f,0.5f,0,0}); clear(data.m_Albedo,{0.5f,0.25f,1,1}); clear(data.m_Material,{1,0,0,data.m_Phase==7 ? 64.0f/255:0}); clear(data.m_Emissive,{0,0,0,data.m_Phase==7 ? 0.5f:0}); clear(data.m_Visibility,{data.m_Phase==8 ? 0.25f:1.0f,0,0,0});
      { xiiGALMapHelper<xiiFrameCameraConstants> mapped(commands,execute.GetBuffer(data.m_Camera),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=xiiFrameCameraConstants(); mapped->m_CameraPositionAndNearPlane=xiiVec4(0,0,2,0.1f); }
      xiiGALStateTransitionDescription t; t.m_pResource=execute.GetBuffer(data.m_Camera); t.m_NewState=xiiGALResourceStateFlags::ConstantBuffer; t.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState; commands.TransitionResourceStates(xiiMakeArrayPtr(&t,1)); });
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Depth, pass.first->m_Depth));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Normal, pass.first->m_Normal));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Albedo, pass.first->m_Albedo));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Material, pass.first->m_Material));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Emissive, pass.first->m_Emissive));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Visibility, pass.first->m_Visibility));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Camera, pass.first->m_Camera));
    return XII_SUCCESS;
  }
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativeSurfaceInputs,1,xiiRTTIDefaultAllocator<xiiNativeSurfaceInputs>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Normal",m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Albedo",m_Albedo)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Material",m_Material)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Emissive",m_Emissive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Visibility",m_Visibility)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
class xiiNativeSurfaceRetain : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativeSurfaceRetain, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto pass = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {data=builder.ReadTexture(context.GetInputTexture(m_Input),xiiGALResourceStateFlags::ShaderResource);builder.ExportTexture(data,xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return context.SetOutput(m_Output, *pass.first);
  }
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativeSurfaceRetain,1,xiiRTTIDefaultAllocator<xiiNativeSurfaceRetain>)
{
  XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Input",m_Input)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("Output",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly), } XII_END_PROPERTIES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
namespace
{
  void CheckSurfacePixel(xiiGALDevice& device, xiiGALTexture* source, xiiVec3 expected)
  {
    xiiGALTextureReadback readback(&device);
    auto                  commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    const auto enqueued = readback.Enqueue(commands, {.m_pTexture = source});
    commands->End();
    if (!XII_TEST_RESULT(enqueued))
      return;
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    readback.WaitForNextCompleted();
    const auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(static_cast<bool>(capture)))
      return;
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    if (XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
    {
      const auto* pixel = static_cast<const xiiFloat16*>(mapped.m_pData);
      XII_TEST_FLOAT(float(pixel[0]), expected.x, 0.003f);
      XII_TEST_FLOAT(float(pixel[1]), expected.y, 0.003f);
      XII_TEST_FLOAT(float(pixel[2]), expected.z, 0.003f);
      XII_TEST_FLOAT(float(pixel[3]), 1, 0);
      XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
    }
    commands->End();
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeSurfaceLighting);
XII_CREATE_SIMPLE_TEST(NativeSurfaceLighting, WorldViewsPhotometryTransmissionVisibilityAndOverflow)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor root;
  const auto                          reference = [&](xiiStringView name, bool registerReference = true) {
    const auto                                 asset = xiiLoadPipelineLibraryResource(name);
    xiiResourceLock<xiiRenderPipelineResource> source(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(source.IsValid() && source->GetAuthoring()))
      return xiiSharedPtr<const xiiRenderPipelineAuthoring>();
    const auto authored = source->GetAuthoring();
    if (registerReference)
      root.m_References.PushBack({authored->m_Descriptor.m_Id, asset.GetResourceID()});
    return authored;
  };
  const auto surfaceDefinition = reference("WorldSurfaceLighting"), clusterDefinition = reference("ClusterLightLists", false), directionalDefinition = reference("DirectionalSurfaceLighting");
  if (!surfaceDefinition || !clusterDefinition || !directionalDefinition)
    return;
  const auto inputs  = root.AddPass("Exact single pixel surface", "xiiNativeSurfaceInputs");
  const auto surface = root.AddSubgraph("Authored world surface lighting", surfaceDefinition->m_Descriptor.m_Id);
  xiiUuid    clusterNode, layoutNode;
  for (const auto& node : surfaceDefinition->m_Descriptor.m_Passes)
    if (node.m_Definition == clusterDefinition->m_Descriptor.m_Id)
      clusterNode = node.m_Id;
  for (const auto& node : clusterDefinition->m_Descriptor.m_Passes)
    if (node.m_sType == "xiiLightClusterViewPass")
      layoutNode = node.m_Id;
  if (!XII_TEST_BOOL(clusterNode.IsValid() && layoutNode.IsValid()))
    return;
  auto& layout = root.m_Passes[surface].m_Overrides.ExpandAndGetRef();
  layout.m_Path.PushBack(clusterNode);
  layout.m_Path.PushBack(layoutNode);
  layout.m_Properties.Insert("UseViewCamera", false);
  layout.m_Properties.Insert("Width", 1U);
  layout.m_Properties.Insert("Height", 1U);
  layout.m_Properties.Insert("TileSize", 1U);
  layout.m_Properties.Insert("DepthSlices", 1U);
  layout.m_Properties.Insert("NearDistance", 0.1f);
  layout.m_Properties.Insert("FarDistance", 5.0f);
  layout.m_Properties.Insert("MaxLightsPerCluster", 1U);
  for (const xiiStringView pin : {"Depth", "Normal", "Albedo", "Material", "Emissive", "Camera", "Visibility"})
    XII_TEST_RESULT(root.Connect(inputs, pin, surface, pin));
  const auto expose = [&](xiiUInt32 operation, xiiStringView pin, xiiStringView name) {
    const auto retain = root.AddPass(xiiStringBuilder("Retain ", name), "xiiNativeSurfaceRetain");
    XII_TEST_RESULT(root.Connect(operation, pin, retain, "Input"));
    root.m_Interface.PushBack({name, false, root.m_Passes[retain].m_Id, "Output"});
  };
  expose(surface, "DirectionalLighting", "SunA");
  expose(surface, "LocalLighting", "Local");
  const auto overflow = root.AddPass("Retain overflow", "xiiBufferExportPass");
  XII_TEST_RESULT(root.Connect(surface, "Overflow", overflow, "Input"));
  root.m_Interface.PushBack({"Overflow", false, root.m_Passes[overflow].m_Id, "Output"});
  const auto secondSun = root.AddSubgraph("Second sun evaluator", directionalDefinition->m_Descriptor.m_Id);
  auto&      scale     = root.m_Passes[secondSun].m_Overrides.ExpandAndGetRef();
  scale.m_Path.PushBack(directionalDefinition->m_Descriptor.m_Passes[0].m_Id);
  scale.m_Properties.Insert("Scale", 2.0f);
  for (const xiiStringView pin : {"Depth", "Normal", "Albedo", "Material", "Emissive", "Camera"})
    XII_TEST_RESULT(root.Connect(inputs, pin, secondSun, pin));
  XII_TEST_RESULT(root.Connect(surface, "EnvironmentLight", secondSun, "EnvironmentLight"));
  expose(secondSun, "Lighting", "SunB");
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Native surface lighting from world components", std::move(root));
  xiiWorldDescription description("Photometric world");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt(xiiVec3(0, 0, 2), xiiVec3::MakeZero(), xiiVec3::MakeAxisY());
  xiiView* views[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    module->CreateView(i ? "Filtered local lights" : "All local lights", views[i]);
    views[i]->SetCamera(&camera);
    views[i]->SetViewport({0, 0, 1, 1});
    views[i]->SetRenderPipelineResource(resource);
  }
  const auto& hidden = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Native.SurfaceLighting.Hidden");
  views[1]->m_ExcludeTags.Set(hidden);
  xiiTagSet tags;
  tags.Set(hidden);
  xiiGameObjectDescription objectDescription;
  objectDescription.m_bDynamic      = true;
  objectDescription.m_LocalPosition = xiiVec3(0, 0, 1.5f);
  xiiGameObject* pointObject        = nullptr;
  const auto     pointHandle        = world.CreateObject(objectDescription, pointObject);
  XII_IGNORE_UNUSED(pointHandle);
  pointObject->SetTags(tags);
  xiiPointLightComponent* point = nullptr;
  xiiPointLightComponent::CreateComponent(pointObject, point);
  point->SetRange(10);
  point->SetIntensityUnit(xiiPhotometricUnit::Candela);
  point->SetIntensity(1);
  xiiGameObject* sunObject          = nullptr;
  objectDescription.m_LocalRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisY(), xiiAngle::MakeFromDegree(90));
  const auto sunHandle              = world.CreateObject(objectDescription, sunObject);
  XII_IGNORE_UNUSED(sunHandle);
  xiiDirectionalLightComponent* directional = nullptr;
  xiiDirectionalLightComponent::CreateComponent(sunObject, directional);
  directional->SetIntensityUnit(xiiPhotometricUnit::Lux);
  directional->SetIntensity(2);
  xiiIESProfileResourceDescriptor profile;
  profile.m_fMaximumCandela = 1;
  profile.m_NormalizedCandela.SetCount(2048);
  for (auto& v : profile.m_NormalizedCandela)
    v = 0.5f;
  const auto              ies    = xiiResourceManager::CreateResource<xiiIESProfileResource>("Half intensity surface profile", std::move(profile));
  xiiPointLightComponent* second = nullptr;
  for (xiiUInt32 phase = 0; phase < 10; ++phase)
  {
    if (phase == 1)
      point->SetIntensity(2);
    if (phase == 2)
      point->SetIESProfile(ies);
    if (phase == 3)
      point->SetActiveFlag(false);
    if (phase == 4)
      point->SetActiveFlag(true);
    if (phase == 5)
    {
      xiiPointLightComponent::CreateComponent(pointObject, second);
      second->SetRange(10);
      second->SetIntensityUnit(xiiPhotometricUnit::Candela);
      second->SetIntensity(3);
    }
    if (phase == 9)
    {
      point->SetActiveFlag(false);
      second->SetActiveFlag(false);
      directional->SetActiveFlag(false);
    }
    for (auto* view : views)
      view->SetPipelineParameter("SurfaceTestPhase", phase);
    world.Update();
    environment.GetDevice()->WaitIdle();
    XII_TEST_BOOL(module->GetExtractionRequirements() == xiiRenderExtractionCategory::Lights && module->GetGpuInstanceData() == nullptr && !xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    const auto    color = xiiEvaluateLightColor(xiiColorLinearUB(255, 255, 255), point->GetTemperature());
    const xiiVec3 albedo(0.5f, 0.25f, 1);
    const auto    response     = xiiVec3(albedo.x * color.r, albedo.y * color.g, albedo.z * color.b) / xiiMath::Pi<float>();
    const auto    transmission = phase == 7 ? 0.5f : 1.0f;
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      const auto& instance = views[view]->GetRenderPipelineInstance();
      const auto* graph    = views[view]->GetRenderGraph();
      if (!XII_TEST_BOOL(instance.GetPipeline() != nullptr && graph != nullptr))
        return;
      const auto  texture  = [&](xiiStringView name) {
        const auto* binding = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
        return binding ? graph->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
      };
      auto a = texture("SunA"), b = texture("SunB"), localImage = texture("Local");
      if (!XII_TEST_BOOL(a && b && localImage && a != b))
        return;
      const bool empty = phase == 6 || phase == 9;
      CheckSurfacePixel(*environment.GetDevice(), a.Borrow(), empty ? xiiVec3::MakeZero() : response * (2 * transmission * (phase == 8 ? 0.25f : 1.0f)));
      CheckSurfacePixel(*environment.GetDevice(), b.Borrow(), empty ? xiiVec3::MakeZero() : response * (4 * transmission));
      // The component caps authored range by its photometric influence threshold.
      // At unit distance the shared finite-emitter attenuation is this range window.
      const auto window = [](const xiiPointLightComponent& light) {
        const float range = xiiMath::Max(light.GetEffectiveRange(), 0.001f);
        return xiiMath::Square(xiiMath::Saturate(1.0f - 1.0f / (range * range)));
      };
      const float primary = phase == 3 ? 0.0f : (phase == 0 ? 1.0f : 2.0f) * (phase >= 2 ? 0.5f : 1.0f) * window(*point);
      const float additional = phase >= 5 && phase < 9 ? 3.0f * window(*second) : 0.0f;
      CheckSurfacePixel(*environment.GetDevice(), localImage.Borrow(), empty || view == 1 ? xiiVec3::MakeZero() : response * ((primary + additional) * transmission));
      const auto* overflow = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Overflow", false));
      xiiUInt32   flag     = 0;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph->GetExportedBuffer(overflow->m_hBuffer).Borrow(), 0, flag))
        return;
      XII_TEST_INT(flag, phase >= 5 && phase < 9 && view == 0 ? 1 : 0);
    }
  }
  // Queued recording retains the independent evaluator resources after instances and views retire.
  xiiRenderGraphContext     queued;
  xiiRenderPipelineInstance detached;
  if (!XII_TEST_RESULT(queued.Initialize("Retired native surface recording")) || !XII_TEST_RESULT(detached.Synchronize(resource)))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = queued.GetGraph();
  auto& cache = queued.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame());
  graph.BeginSetup(frame.GetFrameIndex());
  xiiVariantDictionary parameters;
  parameters.Insert("SurfaceTestPhase", 9U);
  if (!XII_TEST_RESULT(detached.Build(views[0], graph, queued.GetBlackboard(), parameters)))
    return;
  const auto first = detached.GetInterfaceBinding(detached.GetPipeline()->FindInterface("SunA", false))->m_hTexture;
  const auto local = detached.GetInterfaceBinding(detached.GetPipeline()->FindInterface("Local", false))->m_hTexture;
  detached.Clear();
  for (auto* view : views)
    module->DestroyView(view->GetHandle());
  graph.EndSetup();
  if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &queued.GetBlackboard(), &cache)))
    return;
  frame.End();
  environment.GetDevice()->WaitIdle();
  CheckSurfacePixel(*environment.GetDevice(), graph.GetExportedTexture(first).Borrow(), xiiVec3::MakeZero());
  CheckSurfacePixel(*environment.GetDevice(), graph.GetExportedTexture(local).Borrow(), xiiVec3::MakeZero());
}
