/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionTemporalPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneAmbientOcclusion);
XII_CREATE_SIMPLE_TEST(SceneAmbientOcclusion, CanonicalGeometryLightingDisableResizeReloadAndWorldIsolation)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  const auto                                 canonical = xiiLoadPipelineLibraryResource("Scene");
  const auto                                 scene     = xiiMakeReloadablePipelineTestResource(canonical, "SceneAO.ReloadableScene");
  xiiResourceLock<xiiRenderPipelineResource> loaded(scene, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto                      ao         = xiiFindPipelineTestPass(loaded->GetAuthoring()->m_Descriptor, "xiiAmbientOcclusionPass");
  const auto*                     exportType = xiiGetStaticRTTI<xiiOpaqueTestExport>();
  xiiRenderPipelineResourceHandle pipelines[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    xiiRenderPipelineResourceDescriptor descriptor;
    descriptor.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, scene.GetResourceID()});
    const auto node = descriptor.AddSubgraph("Canonical Scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
    if (variant)
    {
      auto& override = descriptor.m_Passes[node].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(ao);
      override.m_Properties.Insert("Enabled", false);
    }
    for (const xiiStringView name : {"AmbientOcclusion", "AmbientLighting", "DirectLighting", "HDRSceneColor", "SceneDepth"})
    {
      const auto capture = descriptor.AddPass(name, exportType);
      XII_TEST_RESULT(descriptor.Connect(node, name, capture, "Input"));
      descriptor.m_Interface.PushBack({name, false, descriptor.m_Passes[capture].m_Id, "Output"});
    }
    pipelines[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(variant ? "SceneAO.Disabled" : "SceneAO.Enabled", std::move(descriptor));
  }
  // No ray scene or mesh shader is required by this authored screen-space path.
  XII_TEST_BOOL(!loaded->GetPipeline()->GetDescriptor().m_Settings.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::RayTracing));

  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescriptor;
  meshDescriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  meshDescriptor.AddSection(12, 0, 0);
  meshDescriptor.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("SceneAO.Box", std::move(meshDescriptor));
  xiiMaterialResourceDescriptor materialDescriptor;
  materialDescriptor.m_BaseColor = xiiColor(0.65f, 0.45f, 0.25f);
  const auto material            = xiiResourceManager::CreateResource<xiiMaterialResource>("SceneAO.Diffuse", std::move(materialDescriptor));
  struct World
  {
    xiiUniquePtr<xiiWorld>      m_World;
    xiiRenderWorldModule*       m_Module = nullptr;
    xiiCamera                   m_Camera;
    xiiView*                    m_Views[2];
    xiiSharedPtr<xiiGALTexture> m_Targets[2];
    xiiGameObjectHandle         m_Occluder;
  } worlds[2];
  const auto resize = [&](World& world, xiiUInt32 width, xiiUInt32 height) {
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      xiiGALTextureCreationDescription target;
      target.m_Type         = xiiGALResourceDimension::Texture2D;
      target.m_Size         = {width, height};
      target.m_Format       = xiiGALResourceFormat::RGBA8UNormalized;
      target.m_BindFlags    = xiiGALBindFlags::RenderTarget;
      world.m_Targets[view] = environment.GetDevice()->CreateTexture(target);
      world.m_Views[view]->SetViewport({0, 0, float(width), float(height)});
      world.m_Views[view]->SetRenderTargetView(world.m_Targets[view]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    }
  };
  for (xiiUInt32 index = 0; index < 2; ++index)
  {
    auto&               world = worlds[index];
    xiiWorldDescription worldDescription("AO independent world");
    world.m_World = XII_DEFAULT_NEW(xiiWorld, worldDescription);
    XII_LOCK(world.m_World->GetWriteMarker());
    world.m_World->SetWorldSimulationEnabled(false);
    world.m_Module = world.m_World->GetOrCreateModule<xiiRenderWorldModule>();
    for (xiiUInt32 objectIndex = 0; objectIndex < 2; ++objectIndex)
    {
      xiiGameObjectDescription description;
      description.m_LocalPosition = objectIndex ? xiiVec3(index ? 0.5f : 0, 0, 0.5f) : xiiVec3(0, 0, -0.1f);
      description.m_LocalScaling  = objectIndex ? xiiVec3(1) : xiiVec3(8, 8, 0.2f);
      xiiGameObject* object       = nullptr;
      const auto     handle       = world.m_World->CreateObject(description, object);
      if (objectIndex)
        world.m_Occluder = handle;
      xiiStaticMeshComponent* component = nullptr;
      xiiStaticMeshComponent::CreateComponent(object, component);
      component->SetMesh(mesh);
      component->SetMaterialOverride(0, material);
    }
    xiiGameObject* lightObject = nullptr;
    world.m_World->CreateObject({}, lightObject);
    xiiDirectionalLightComponent* light = nullptr;
    xiiDirectionalLightComponent::CreateComponent(lightObject, light);
    light->SetIntensityUnit(xiiPhotometricUnit::Lux);
    light->SetIntensity(0.1f);
    world.m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    world.m_Camera.LookAt({-4, -5, 3}, {0, 0, 0.2f}, {0, 0, 1});
    world.m_Camera.SetExposure(8);
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      world.m_Module->CreateView("AO enabled/disabled comparison", world.m_Views[view]);
      world.m_Views[view]->SetCamera(&world.m_Camera);
      world.m_Views[view]->SetRenderPipelineResource(pipelines[view]);
      xiiDisplayOutputSettings display;
      display.m_Exposure.m_Mode = xiiExposureMode::Manual;
      display.m_fBloomStrength  = 0;
      XII_TEST_RESULT(world.m_Views[view]->SetDisplayOutputSettings(display));
    }
    resize(world, 129, 97);
  }
  xiiDynamicArray<xiiUInt8>   previousMasks[2];
  xiiSharedPtr<xiiGALTexture> masks[2];
  for (xiiUInt32 phase = 0; phase < 8; ++phase)
  {
    if (phase == 5)
      xiiMakeReloadablePipelineTestResource(canonical, scene.GetResourceID());
    for (xiiUInt32 index = 0; index < 2; ++index)
    {
      auto& world = worlds[index];
      XII_LOCK(world.m_World->GetWriteMarker());
      if (phase == 3)
        for (auto* view : world.m_Views)
          view->InvalidateTemporalHistory();
      if (phase == 4)
        resize(world, 97, 73);
      if (phase == 6 && index == 0)
        world.m_World->DeleteObjectNow(world.m_Occluder);
      world.m_World->Update();
      environment.GetDevice()->WaitIdle();
      for (auto* view : world.m_Views)
        if (!XII_TEST_RESULT(view->GetLastRenderResult()))
          return;
      if (phase < 2)
        continue;
      const auto image = [&](xiiUInt32 view, xiiStringView name) {
        const auto& instance = world.m_Views[view]->GetRenderPipelineInstance();
        const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
        return world.m_Views[view]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture);
      };
      xiiDynamicArray<xiiUInt8> mask;
      masks[index] = image(0, "AmbientOcclusion");
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), masks[index], mask))
        return;
      XII_TEST_BOOL(image(1, "AmbientOcclusion") == nullptr);
      if (phase == 3)
        XII_TEST_BOOL(mask == previousMasks[index]);
      previousMasks[index] = mask;
      xiiDynamicArray<xiiVec4>   ambient[2], direct[2], hdr[2];
      xiiDynamicArray<float>     depth[2];
      xiiDynamicArray<xiiUInt32> output[2];
      for (xiiUInt32 view = 0; view < 2; ++view)
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "AmbientLighting"), ambient[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "DirectLighting"), direct[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "HDRSceneColor"), hdr[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "SceneDepth"), depth[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), world.m_Targets[view], output[view]))
          return;
      if (phase == 2 && index == 0)
      {
        const auto                       size = world.m_Targets[0]->GetDescription().m_Size;
        xiiGALTextureCreationDescription header;
        header.m_Type   = xiiGALResourceDimension::Texture2D;
        header.m_Size   = size;
        header.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
        for (xiiUInt32 panel = 0; panel < 3; ++panel)
        {
          xiiImage capture;
          capture.ResetAndAlloc(header);
          auto pixels = capture.GetPixelPointer<xiiUInt32>();
          for (xiiUInt32 pixel = 0; pixel < mask.GetCount(); ++pixel)
            pixels[pixel] = panel < 2 ? output[panel][pixel] : 0xFF000000U | (xiiUInt32(mask[pixel]) * 0x00010101U);
          xiiStringBuilder path;
          path.SetFormat(":imgout/SceneAO-{}.png", panel == 0 ? "Enabled" : panel == 1 ? "Disabled" :
                                                                                         "Mask");
          XII_TEST_RESULT(capture.SaveTo(path));
        }
      }
      xiiUInt32 darkened = 0, visible = 0, affectedOutput = 0;
      xiiUInt8  minimum = 255;
      for (xiiUInt32 pixel = 0; pixel < mask.GetCount(); ++pixel)
      {
        XII_TEST_FLOAT(depth[0][pixel], depth[1][pixel], 0);
        const float visibility = float(mask[pixel]) / 255;
        for (xiiUInt32 channel = 0; channel < 3; ++channel)
        {
          XII_TEST_FLOAT(direct[0][pixel].GetData()[channel], direct[1][pixel].GetData()[channel], 0.000001f);
          XII_TEST_FLOAT(ambient[0][pixel].GetData()[channel], ambient[1][pixel].GetData()[channel] * visibility, 0.000001f);
          XII_TEST_FLOAT(hdr[1][pixel].GetData()[channel] - hdr[0][pixel].GetData()[channel], ambient[1][pixel].GetData()[channel] - ambient[0][pixel].GetData()[channel], 0.000001f);
        }
        if (depth[0][pixel] > 0)
        {
          ++visible;
          minimum = xiiMath::Min(minimum, mask[pixel]);
          darkened += mask[pixel] < 250;
          affectedOutput += output[0][pixel] != output[1][pixel];
        }
        else
          XII_TEST_INT(mask[pixel], 255);
      }
      xiiTestFramework::Output(xiiTestOutput::Details, "AO world %u phase %u: %u visible, %u occluded, minimum %u, %u final pixels changed.", index, phase, visible, darkened, minimum, affectedOutput);
      XII_TEST_BOOL(visible > 100);
      if (phase >= 6 && index == 0)
      {
        XII_TEST_INT(darkened, 0);
        XII_TEST_INT(affectedOutput, 0);
      }
      else
      {
        XII_TEST_BOOL(darkened > 8 && minimum < 230);
        XII_TEST_BOOL(affectedOutput > 8);
      }
      const auto& enabledStats  = world.m_Views[0]->GetRenderGraph()->GetStatistics();
      const auto& disabledStats = world.m_Views[1]->GetRenderGraph()->GetStatistics();
      XII_TEST_BOOL(enabledStats.m_uiCompiledPassCount >= disabledStats.m_uiCompiledPassCount + 2);
    }
    if (phase >= 2)
      XII_TEST_BOOL(masks[0] != masks[1]);
  }
}

XII_CREATE_SIMPLE_TEST(SceneAmbientOcclusion, RetainedRecordingsAndInvalidationBeforeExecution)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor descriptor;
  const auto                          source = descriptor.AddPass("Empty surface", xiiGetStaticRTTI<xiiScreenEffectTestSource>());
  const auto                          camera = descriptor.AddPass("View constants", xiiGetStaticRTTI<xiiViewConstantsPass>());
  xiiVariantDictionary                properties;
  properties.Insert("UseRayTracing", false);
  const auto ao     = descriptor.AddPass("Ambient visibility", xiiGetStaticRTTI<xiiAmbientOcclusionPass>(), properties);
  const auto filter = descriptor.AddPass("Temporal filter", xiiGetStaticRTTI<xiiAmbientOcclusionTemporalPass>());
  for (const auto pass : {ao, filter})
  {
    XII_TEST_RESULT(descriptor.Connect(source, "Depth", pass, "SceneDepth"));
    XII_TEST_RESULT(descriptor.Connect(camera, "PerFrameCamera", pass, "PerFrameCamera"));
    XII_TEST_RESULT(descriptor.Connect(camera, "PerFrameGlobal", pass, "PerFrameGlobal"));
  }
  XII_TEST_RESULT(descriptor.Connect(source, "Attributes", ao, "NormalRoughness"));
  XII_TEST_RESULT(descriptor.Connect(source, "Attributes", filter, "GBufferNormal"));
  XII_TEST_RESULT(descriptor.Connect(source, "Attributes", filter, "DilatedVelocityBuffer"));
  XII_TEST_RESULT(descriptor.Connect(ao, "RawAO", filter, "RawAO"));
  descriptor.m_Interface.PushBack({"Previous", false, descriptor.m_Passes[filter].m_Id, "PreviousAO"});
  descriptor.m_Interface.PushBack({"Current", false, descriptor.m_Passes[filter].m_Id, "StableAO"});
  const auto             resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("SceneAO.Retained", std::move(descriptor));
  xiiWorldDescription    worldDescription("AO recording retirement");
  xiiUniquePtr<xiiWorld> world = XII_DEFAULT_NEW(xiiWorld, worldDescription);
  xiiCamera              cameraObject;
  cameraObject.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView* views[2];
  {
    XII_LOCK(world->GetWriteMarker());
    auto* module = world->GetOrCreateModule<xiiRenderWorldModule>();
    for (auto& view : views)
    {
      module->CreateView("AO recording view", view);
      view->SetCamera(&cameraObject);
      view->SetViewport({0, 0, 8, 8});
    }
  }
  xiiRenderGraphTestContext contexts[2];
  xiiRenderPipelineInstance instances[2];
  for (xiiUInt32 index = 0; index < 2; ++index)
    if (!XII_TEST_RESULT(contexts[index].Initialize("AO retained recording")) || !XII_TEST_RESULT(instances[index].Synchronize(resource)))
      return;
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiRenderGraphTextureHandle outputs[2];
    for (xiiUInt32 index = 0; index < 2; ++index)
    {
      auto& graph = contexts[index].GetGraph();
      contexts[index].GetResourceCache().BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      const auto built = instances[index].Build(views[index], graph, contexts[index].GetBlackboard());
      if (!XII_TEST_RESULT_MSG(built, "%s", instances[index].GetLastBuildError().GetStartPointer()))
        return;
      const auto* pipeline = instances[index].GetPipeline();
      const auto  previous = instances[index].GetInterfaceBinding(pipeline->FindInterface("Previous", false))->m_hTexture;
      XII_TEST_BOOL(previous.IsValid() == (phase == 2));
      outputs[index] = instances[index].GetInterfaceBinding(pipeline->FindInterface("Current", false))->m_hTexture;
      graph.AddPass<xiiUInt32>("Capture retained AO", xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) { builder.ExportTexture(builder.ReadTexture(outputs[index], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
      if (phase == 0)
        instances[index].InvalidateTemporalHistory();
      if (phase == 2)
        instances[index].Clear();
      graph.EndSetup();
    }
    if (phase == 2)
      world.Clear();
    xiiSharedPtr<xiiGALTexture> images[2];
    for (xiiUInt32 index = 0; index < 2; ++index)
    {
      auto& graph = contexts[index].GetGraph();
      auto& cache = contexts[index].GetResourceCache();
      XII_SCOPE_EXIT(cache.EndFrame());
      if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &contexts[index].GetBlackboard(), &cache)))
        return;
      environment.GetDevice()->WaitIdle();
      images[index]  = graph.GetExportedTexture(outputs[index]);
      xiiUInt8 value = 0;
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), images[index].Borrow(), 4, 4, value))
        return;
      XII_TEST_INT(value, 255);
    }
    XII_TEST_BOOL(images[0] && images[1] && images[0] != images[1]);
    frame.End();
  }
}

XII_CREATE_SIMPLE_TEST(SceneAmbientOcclusion, UnsupportedStorageFormatFailsAndAuthoredDisableSucceeds)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  XII_TEST_BOOL(environment.GetDevice()->GetFeatures().m_TextureUAVExtendedFormats == xiiGALDeviceFeatureState::Disabled);
  xiiRenderPipelineResourceDescriptor definition;
  const auto                          source = definition.AddPass("Surface inputs", xiiGetStaticRTTI<xiiScreenEffectTestSource>());
  const auto                          camera = definition.AddPass("Camera", xiiGetStaticRTTI<xiiViewConstantsPass>());
  xiiVariantDictionary                properties;
  properties.Insert("UseRayTracing", false);
  const auto ao     = definition.AddPass("Ambient visibility", xiiGetStaticRTTI<xiiAmbientOcclusionPass>(), properties);
  const auto filter = definition.AddPass("Spatial filter", xiiGetStaticRTTI<xiiAmbientOcclusionTemporalPass>());
  definition.m_Passes[filter].m_Properties.Insert("UseHistory", false);
  for (const auto node : {ao, filter})
  {
    XII_TEST_RESULT(definition.Connect(source, "Depth", node, "SceneDepth"));
    XII_TEST_RESULT(definition.Connect(camera, "PerFrameCamera", node, "PerFrameCamera"));
    XII_TEST_RESULT(definition.Connect(camera, "PerFrameGlobal", node, "PerFrameGlobal"));
  }
  XII_TEST_RESULT(definition.Connect(source, "Attributes", ao, "NormalRoughness"));
  XII_TEST_RESULT(definition.Connect(source, "Attributes", filter, "GBufferNormal"));
  XII_TEST_RESULT(definition.Connect(ao, "RawAO", filter, "RawAO"));
  definition.m_Interface.PushBack({"AO", false, definition.m_Passes[filter].m_Id, "StableAO"});
  const auto enabled = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("SceneAO.UnsupportedEnabled", xiiRenderPipelineResourceDescriptor(definition));
  definition.m_Passes[ao].m_Properties.Insert("Enabled", false);
  const auto          disabled = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("SceneAO.UnsupportedDisabled", std::move(definition));
  xiiWorldDescription description("AO unsupported storage");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera cameraObject;
  cameraObject.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView* view = nullptr;
  module->CreateView("AO unsupported view", view);
  view->SetCamera(&cameraObject);
  view->SetViewport({0, 0, 8, 8});
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("AO capability check")))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = context.GetGraph();
  auto& cache = context.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    xiiRenderPipelineInstance instance;
    if (!XII_TEST_RESULT(instance.Synchronize(variant ? disabled : enabled)))
      return;
    graph.BeginSetup(frame.GetFrameIndex());
    const auto built = instance.Build(view, graph, context.GetBlackboard());
    if (variant == 0)
    {
      XII_TEST_BOOL(built.Failed());
      XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("TextureUAVExtendedFormats") != nullptr);
    }
    else
    {
      XII_TEST_RESULT(built);
      const auto* binding = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("AO", false));
      XII_TEST_BOOL(binding && !binding->m_hTexture.IsValid());
    }
    graph.EndSetup();
  }
}
