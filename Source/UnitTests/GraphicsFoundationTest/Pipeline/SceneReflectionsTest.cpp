/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneReflections);
static void TestSceneReflections(bool extendedFormats)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, extendedFormats)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 canonical = xiiLoadPipelineLibraryResource("Scene");
  xiiResourceLock<xiiRenderPipelineResource> loaded(canonical, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
    return;
  const auto                      workload   = xiiLoadPipelineLibraryResource("SceneScreenSpaceReflections");
  const auto                      reloadable = xiiMakeReloadablePipelineTestResource(workload, "Reflection.ReloadableWorkload");
  xiiRenderPipelineResourceHandle pipelines[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    auto definition = loaded->GetAuthoring()->m_Descriptor;
    for (auto& reference : definition.m_References)
      if (reference.m_Id == xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneScreenSpaceReflections"))
        reference.m_sResource = reloadable.GetResourceID();
    if (!extendedFormats)
      for (auto& pass : definition.m_Passes)
        if (pass.m_pType && pass.m_pType->GetTypeName() == xiiStringView("xiiAmbientOcclusionPass"))
          pass.m_Properties.Insert("Enabled", false);
    definition.m_Interface.PushBack({"UnreflectedColor", false, xiiUuid::MakeStableUuidFromString("XII.Scene/Atmosphere"), "HDRSceneColor"});
    for (auto& pass : definition.m_Passes)
      if (pass.m_Id == xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenReflections"))
      {
        auto& enable = pass.m_Overrides.ExpandAndGetRef();
        enable.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneScreenSpaceReflections/Reflection Depth"));
        enable.m_Properties.Insert("Enabled", variant == 1);
      }
    const auto                          source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(variant ? "Reflection.Scene.On" : "Reflection.Scene.Off", std::move(definition));
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, source.GetResourceID()});
    const auto scene = root.AddSubgraph("Canonical Scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
    for (const xiiStringView name : {"HDRSceneColor", "UnreflectedColor", "ScreenReflections", "ReflectionLighting", "SceneDepth", "ObjectId"})
    {
      const auto output = root.AddPass(name, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(root.Connect(scene, name, output, "Input"));
      root.m_Interface.PushBack({name, false, root.m_Passes[output].m_Id, "Output"});
    }
    pipelines[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(variant ? "Reflection.Root.On" : "Reflection.Root.Off", std::move(root));
  }
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescriptor;
  meshDescriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  meshDescriptor.AddSection(12, 0, 0);
  meshDescriptor.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("Reflection.Box", std::move(meshDescriptor));
  xiiMaterialResourceDescriptor red;
  red.m_BaseColor                           = xiiColor(0.8f, 0.02f, 0.01f);
  red.m_EmissiveColor                       = xiiColor(4, 0, 0);
  const auto                    redMaterial = xiiResourceManager::CreateResource<xiiMaterialResource>("Reflection.Red", std::move(red));
  xiiMaterialResourceDescriptor mirror;
  mirror.m_BaseColor                = xiiColor(0.8f, 0.8f, 0.8f);
  mirror.m_fMetallic                = 1;
  mirror.m_fRoughness               = 0.05f;
  const auto          floorMaterial = xiiResourceManager::CreateResource<xiiMaterialResource>("Reflection.Mirror", std::move(mirror));
  xiiWorldDescription worldDescription("Reflection world");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  const auto addMesh = [&](xiiVec3 position, xiiVec3 scale, const xiiMaterialResourceHandle& surface) {
    xiiGameObjectDescription description;
    description.m_bDynamic      = true;
    description.m_LocalPosition = position;
    description.m_LocalScaling  = scale;
    xiiGameObject* object       = nullptr;
    world.CreateObject(description, object);
    xiiMeshComponent* component = nullptr;
    xiiMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    component->SetMaterialOverride(0, surface);
    return component;
  };
  auto*                    floor = addMesh({0, 0, -0.1f}, {12, 12, 0.2f}, floorMaterial);
  auto*                    box   = addMesh({0, 0, 0.75f}, {0.8f, 0.8f, 1.5f}, redMaterial);
  xiiGameObjectDescription sun;
  sun.m_LocalRotation      = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3(-0.8f, -0.6f, -1).GetNormalized());
  xiiGameObject* sunObject = nullptr;
  world.CreateObject(sun, sunObject);
  xiiDirectionalLightComponent* light = nullptr;
  xiiDirectionalLightComponent::CreateComponent(sunObject, light);
  light->SetIntensityUnit(xiiPhotometricUnit::Lux);
  light->SetIntensity(0.1f);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({-4, -5, 3}, {0, 0, 0.3f}, {0, 0, 1});
  camera.SetExposure(8);
  auto*                       module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView*                    views[2];
  xiiSharedPtr<xiiGALTexture> targets[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    module->CreateView("Reflection comparison", views[variant]);
    views[variant]->SetCamera(&camera);
    views[variant]->SetRenderPipelineResource(pipelines[variant]);
    xiiGALTextureCreationDescription target;
    target.m_Type      = xiiGALResourceDimension::Texture2D;
    target.m_Size      = {257, 193};
    target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    target.m_BindFlags = xiiGALBindFlags::RenderTarget;
    targets[variant]   = environment.GetDevice()->CreateTexture(target);
    views[variant]->SetViewport({0, 0, 257, 193});
    views[variant]->SetRenderTargetView(targets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    xiiDisplayOutputSettings display;
    display.m_Exposure.m_Mode = xiiExposureMode::Manual;
    display.m_fBloomStrength  = 0;
    XII_TEST_RESULT(views[variant]->SetDisplayOutputSettings(display));
  }
  xiiMaterialResourceDescriptor blue;
  blue.m_BaseColor                           = xiiColor(0.01f, 0.02f, 0.8f);
  blue.m_EmissiveColor                       = xiiColor(0, 0, 4);
  const auto                    blueMaterial = xiiResourceManager::CreateResource<xiiMaterialResource>("Reflection.Blue", std::move(blue));
  xiiMaterialResourceDescriptor rough;
  rough.m_BaseColor                           = xiiColor(0.8f, 0.8f, 0.8f);
  rough.m_fMetallic                           = 1;
  rough.m_fRoughness                          = 0.9f;
  const auto                    roughMaterial = xiiResourceManager::CreateResource<xiiMaterialResource>("Reflection.Rough", std::move(rough));
  xiiMaterialResourceDescriptor dielectric;
  dielectric.m_BaseColor                         = xiiColor(0.8f, 0.8f, 0.8f);
  dielectric.m_fRoughness                        = 0.05f;
  const auto                  dielectricMaterial = xiiResourceManager::CreateResource<xiiMaterialResource>("Reflection.Dielectric", std::move(dielectric));
  xiiDynamicArray<xiiVec4>    baseline;
  double                      mirrorEnergy = 0, stationaryCentroid = 0;
  xiiWorldDescription         foreignDescription("Independent reflection world");
  xiiUniquePtr<xiiWorld>      foreign     = XII_DEFAULT_NEW(xiiWorld, foreignDescription);
  xiiView*                    foreignView = nullptr;
  xiiSharedPtr<xiiGALTexture> foreignTarget;
  {
    XII_LOCK(foreign->GetWriteMarker());
    foreign->SetWorldSimulationEnabled(false);
    for (xiiUInt32 index = 0; index < 2; ++index)
    {
      xiiGameObjectDescription description;
      description.m_LocalPosition = index ? xiiVec3(0, 0, 0.75f) : xiiVec3(0, 0, -0.1f);
      description.m_LocalScaling  = index ? xiiVec3(0.8f, 0.8f, 1.5f) : xiiVec3(12, 12, 0.2f);
      xiiGameObject* object       = nullptr;
      foreign->CreateObject(description, object);
      xiiStaticMeshComponent* component = nullptr;
      xiiStaticMeshComponent::CreateComponent(object, component);
      component->SetMesh(mesh);
      component->SetMaterialOverride(0, index ? blueMaterial : floorMaterial);
    }
    xiiGameObject* object = nullptr;
    foreign->CreateObject(sun, object);
    xiiDirectionalLightComponent* foreignSun = nullptr;
    xiiDirectionalLightComponent::CreateComponent(object, foreignSun);
    foreignSun->SetIntensityUnit(xiiPhotometricUnit::Lux);
    foreignSun->SetIntensity(0.1f);
    foreign->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Independent reflection view", foreignView);
    foreignView->SetCamera(&camera);
    foreignView->SetRenderPipelineResource(pipelines[1]);
    foreignTarget = environment.GetDevice()->CreateTexture(targets[0]->GetDescription());
    foreignView->SetViewport({0, 0, 257, 193});
    foreignView->SetRenderTargetView(foreignTarget->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }
  for (xiiUInt32 phase = 0; phase < 11; ++phase)
  {
    if (phase == 1)
      box->SetMaterialOverride(0, blueMaterial);
    if (phase == 2)
    {
      box->SetMaterialOverride(0, redMaterial);
      floor->SetMaterialOverride(0, roughMaterial);
    }
    if (phase == 3)
      floor->SetMaterialOverride(0, dielectricMaterial);
    if (phase == 4)
    {
      floor->SetMaterialOverride(0, floorMaterial);
      box->SetActiveFlag(false);
    }
    if (phase == 5)
    {
      box->SetActiveFlag(true);
      xiiMakeReloadablePipelineTestResource(workload, reloadable.GetResourceID());
    }
    if (phase == 6)
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
      {
        auto target      = targets[variant]->GetDescription();
        target.m_Size    = {193, 129};
        targets[variant] = environment.GetDevice()->CreateTexture(target);
        views[variant]->SetViewport({0, 0, 193, 129});
        views[variant]->SetRenderTargetView(targets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      }
    if (phase == 7)
      camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 5, 0.1f, 100);
    if (phase == 9)
      box->GetOwner()->SetLocalPosition(xiiVec3(1.5f, 0, 0.75f));
    if (phase == 10)
      camera.LookAt({-3.5f, -5, 3}, {0.5f, 0, 0.3f}, {0, 0, 1});
    if (phase == 8)
    {
      xiiResourceLock<xiiRenderPipelineResource> original(workload, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      auto                                       changed = original->GetAuthoring()->m_Descriptor;
      for (auto& pass : changed.m_Passes)
        if (pass.m_pType && pass.m_pType->GetTypeName() == xiiStringView("xiiScreenSpaceReflectionsPass"))
          pass.m_Properties.Insert("MaxRoughness", 0.0f);
      const auto replacement = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Reflection.ReloadedRoughness", std::move(changed));
      xiiMakeReloadablePipelineTestResource(replacement, reloadable.GetResourceID());
      for (auto* view : views)
        view->InvalidateTemporalHistory();
    }
    if (phase == 9)
      xiiMakeReloadablePipelineTestResource(workload, reloadable.GetResourceID());
    {
      xiiTestLogInterface   log;
      xiiTestLogSystemScope scope(&log, true);
      if (!extendedFormats)
      {
        log.ExpectMessage("Failed to build render pipeline", xiiLogMsgType::ErrorMsg);
        log.ExpectMessage("Failed to execute render graph", xiiLogMsgType::ErrorMsg);
      }
      world.Update();
      environment.GetDevice()->WaitIdle();
    }
    if (!extendedFormats)
    {
      XII_TEST_RESULT(views[0]->GetLastRenderResult());
      XII_TEST_BOOL(views[1]->GetLastRenderResult().Failed());
      XII_TEST_BOOL(views[1]->GetRenderPipelineInstance().GetLastBuildError().FindSubString("TextureUAVExtendedFormats") != nullptr);
      xiiDynamicArray<xiiUInt32> pixels;
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[0], pixels))
        return;
      xiiUInt32 lit = 0;
      for (auto pixel : pixels)
        lit += (pixel & 0xFFFFFF) != 0;
      XII_TEST_BOOL(lit > 100);
      return;
    }
    {
      XII_LOCK(foreign->GetWriteMarker());
      foreign->Update();
    }
    environment.GetDevice()->WaitIdle();
    if (!XII_TEST_RESULT_MSG(foreignView->GetLastRenderResult(), "%s", foreignView->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
      return;
    auto&                    foreignInstance = foreignView->GetRenderPipelineInstance();
    const auto*              foreignBinding  = foreignInstance.GetInterfaceBinding(foreignInstance.GetPipeline()->FindInterface("ScreenReflections", false));
    xiiDynamicArray<xiiVec4> foreignPixels;
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), foreignView->GetRenderGraph()->GetExportedTexture(foreignBinding->m_hTexture), foreignPixels))
      return;
    xiiUInt32 foreignBlue = 0;
    for (const auto& pixel : foreignPixels)
      foreignBlue += pixel.w > 0.001f && pixel.z > pixel.x * 2 + 0.01f;
    XII_TEST_BOOL(phase == 8 ? foreignBlue == 0 : foreignBlue > 8);
    for (auto* view : views)
      if (!XII_TEST_RESULT_MSG(view->GetLastRenderResult(), "%s", view->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
        return;
    const auto image = [&](xiiUInt32 variant, xiiStringView name) {
      auto*       view     = views[variant];
      auto&       instance = view->GetRenderPipelineInstance();
      const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
      return binding ? view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
    };
    xiiDynamicArray<xiiVec4>   raw, weighted, on, off, base;
    xiiDynamicArray<xiiUInt32> final[2], ids[2];
    xiiDynamicArray<float>     depth[2];
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "ScreenReflections"), raw) ||
        !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "ReflectionLighting"), weighted) ||
        !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "HDRSceneColor"), on) ||
        !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(0, "HDRSceneColor"), off) ||
        !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "UnreflectedColor"), base))
      return;
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[variant], final[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "SceneDepth"), depth[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "ObjectId"), ids[variant]))
        return;
    XII_TEST_BOOL(depth[0] == depth[1] && ids[0] == ids[1]);
    XII_TEST_BOOL(off == base);
    XII_TEST_BOOL(image(0, "ScreenReflections") == nullptr && image(0, "ReflectionLighting") == nullptr);
    xiiUInt32 hits = 0, redHits = 0, blueHits = 0, changed = 0, floorHits = 0;
    double    floorEnergy = 0, redPosition = 0;
    for (xiiUInt32 pixel = 0; pixel < raw.GetCount(); ++pixel)
    {
      XII_TEST_BOOL(raw[pixel].IsValid() && on[pixel].IsValid());
      XII_TEST_BOOL(raw[pixel].w >= 0 && raw[pixel].w <= 1);
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
      {
        XII_TEST_FLOAT(weighted[pixel].GetData()[channel], raw[pixel].GetData()[channel] * raw[pixel].w, 0.0001f);
        XII_TEST_FLOAT(on[pixel].GetData()[channel], base[pixel].GetData()[channel] + weighted[pixel].GetData()[channel], 0.0001f);
      }
      hits += raw[pixel].w > 0.001f;
      redHits += raw[pixel].w > 0.001f && raw[pixel].x > raw[pixel].y * 2 + 0.01f;
      if (raw[pixel].w > 0.001f && raw[pixel].x > raw[pixel].y * 2 + 0.01f)
        redPosition += pixel % targets[1]->GetDescription().m_Size.width;
      blueHits += raw[pixel].w > 0.001f && raw[pixel].z > raw[pixel].y * 2 + 0.01f;
      changed += final[0][pixel] != final[1][pixel];
      if (depth[1][pixel] > 0)
      {
        xiiGpuInstanceSource source;
        if (!XII_TEST_RESULT(views[1]->GetGpuInstanceData()->Resolve(ids[1][pixel], source)))
          return;
        if (source.m_hOwnerObject == floor->GetOwner()->GetHandle())
        {
          floorHits += raw[pixel].w > 0.001f;
          floorEnergy += weighted[pixel].x + weighted[pixel].y + weighted[pixel].z;
          if (phase == 2)
            XII_TEST_BOOL(raw[pixel] == xiiVec4::MakeZero());
        }
      }
    }
    xiiTestFramework::Output(xiiTestOutput::Details, "Screen reflections phase %u: %u hits (%u floor), %u red, %u blue, %u final pixels, floor energy %.3f.", phase, hits, floorHits, redHits, blueHits, changed, floorEnergy);
    if (phase == 0)
    {
      baseline     = raw;
      mirrorEnergy = floorEnergy;
      XII_TEST_BOOL(floorHits > 16 && redHits > 8 && changed > 8);
    }
    if (phase == 1)
      XII_TEST_BOOL(blueHits > 8 && redHits == 0);
    if (phase == 2)
      XII_TEST_BOOL(floorHits == 0 && floorEnergy == 0);
    if (phase == 3)
      XII_TEST_BOOL(floorEnergy > 0 && floorEnergy < mirrorEnergy * 0.5);
    if (phase == 4)
      XII_TEST_BOOL(hits == 0 && changed == 0);
    if (phase == 5)
      XII_TEST_BOOL(raw == baseline);
    if (phase == 7)
      stationaryCentroid = redPosition / redHits;
    if (phase == 9)
      XII_TEST_BOOL(xiiMath::Abs(redPosition / redHits - stationaryCentroid) > 5);
    if (phase == 8)
      XII_TEST_BOOL(hits == 0 && changed == 0);
    if (phase >= 6 && phase != 8)
      XII_TEST_BOOL(floorHits > 8 && redHits > 8 && changed > 8);
    if (phase == 0)
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
      {
        xiiImage capture;
        capture.ResetAndAlloc(targets[variant]->GetDescription());
        xiiMemoryUtils::Copy(capture.GetPixelPointer<xiiUInt32>(), final[variant].GetData(), final[variant].GetCount());
        XII_TEST_RESULT(capture.SaveTo(xiiStringBuilder(":imgout/SceneReflections-", variant ? "Enabled" : "Disabled", ".png")));
      }
  }
  xiiRenderGraphTestContext retained;
  if (!XII_TEST_RESULT(retained.Initialize("Retained reflection workload")))
    return;
  xiiRenderPipelineInstance recording;
  if (!XII_TEST_RESULT(recording.Synchronize(pipelines[1])))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = retained.GetGraph();
  auto& cache = retained.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
  graph.BeginSetup(frame.GetFrameIndex());
  {
    XII_LOCK(foreign->GetWriteMarker());
    if (!XII_TEST_RESULT(recording.Build(foreignView, graph, retained.GetBlackboard())))
      return;
  }
  const auto output = recording.GetInterfaceBinding(recording.GetPipeline()->FindInterface("ScreenReflections", false))->m_hTexture;
  graph.EndSetup();
  recording.Clear();
  foreign.Clear();
  xiiStringBuilder error;
  if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &retained.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
    return;
  xiiDynamicArray<xiiVec4> retired;
  if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), graph.GetExportedTexture(output), retired))
    return;
  xiiUInt32 blueHits = 0;
  for (const auto& pixel : retired)
  {
    XII_TEST_BOOL(pixel.IsValid());
    blueHits += pixel.w > 0.001f && pixel.z > pixel.x * 2 + 0.01f;
  }
  XII_TEST_BOOL(blueHits > 8);
}
XII_CREATE_SIMPLE_TEST(SceneReflections, WorldSurfaceOutput) { TestSceneReflections(true); }
XII_CREATE_SIMPLE_TEST(SceneReflections, UnsupportedStorageHasExplicitAuthoredFallback) { TestSceneReflections(false); }
