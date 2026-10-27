/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/DirectionalShadowVisibilityPass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowAtlasPass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowCascadePass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowCascadeViewPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Texture/Image/Image.h>

namespace
{
  class ShadowReloadLoader : public xiiResourceLoaderFromMemory
  {
  public:
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> m_Closed;
    void                                                  CloseDataStream(const xiiResource* resource, const xiiResourceLoadData& data) override
    {
      xiiResourceLoaderFromMemory::CloseDataStream(resource, data);
      m_Closed->m_Content.RaiseSignal();
    }
  };

  bool ReloadShadowLayout(const xiiRenderPipelineResourceHandle& resource)
  {
    xiiRenderPipelineResourceDescriptor descriptor;
    {
      xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
        return false;
      descriptor = loaded->GetAuthoring()->m_Descriptor;
    }
    for (auto& pass : descriptor.m_Passes)
      if (pass.m_pType == xiiGetStaticRTTI<xiiShadowCascadePass>())
      {
        auto settings = pass.m_Properties.GetValue("Settings")->Get<xiiVariantDictionary>();
        settings.Insert("CascadeCount", 1U);
        settings.Insert("ShadowMapResolution", 512U);
        settings.Insert("MaximumShadowDistance", 30.0f);
        pass.m_Properties.Insert("Settings", settings);
      }
    const auto                       closed = xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>>(XII_DEFAULT_NEW(xiiRefCountedContainer<xiiThreadSignal>));
    xiiUniquePtr<ShadowReloadLoader> loader = XII_DEFAULT_NEW(ShadowReloadLoader);
    loader->m_Closed                        = closed;
    xiiMemoryStreamWriter stream(&loader->m_CustomData);
    stream << xiiString(resource.GetResourceID());
    xiiAssetFileHeader header;
    if (!XII_TEST_RESULT(header.Write(stream)))
      return false;
    stream << xiiUInt8(3);
    if (!XII_TEST_RESULT(descriptor.Serialize(stream)))
      return false;
    xiiResourceManager::UpdateResourceWithCustomLoader(resource, std::move(loader));
    return XII_TEST_BOOL(closed->m_Content.WaitForSignal(xiiTime::MakeFromSeconds(10)) == xiiThreadSignal::WaitResult::Signaled);
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(SceneDirectionalShadows);
static void TestCanonicalDirectionalShadows(bool extendedFormats)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, extendedFormats)))
    return;
  if (!extendedFormats)
    XII_TEST_BOOL(environment.GetDevice()->GetFeatures().m_TextureUAVExtendedFormats == xiiGALDeviceFeatureState::Disabled);
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 canonical      = xiiLoadPipelineLibraryResource("Scene");
  const auto                                 shadowResource = xiiLoadPipelineLibraryResource("SceneDirectionalShadows");
  const auto                                 scene          = xiiMakeReloadablePipelineTestResource(canonical, "SceneShadow.Reloadable");
  xiiResourceLock<xiiRenderPipelineResource> loaded(scene, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
    return;
  const auto                      shadowNode  = xiiUuid::MakeStableUuidFromString("XII.Scene/DirectionalShadows");
  const auto                      cascadeNode = xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneDirectionalShadows/World Directional Cascades");
  xiiRenderPipelineResourceHandle pipelines[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, scene.GetResourceID()});
    const auto node = root.AddSubgraph("Canonical Scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
    if (!extendedFormats)
    {
      auto& override = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(xiiFindPipelineTestPass(loaded->GetAuthoring()->m_Descriptor, "xiiAmbientOcclusionPass"));
      override.m_Properties.Insert("Enabled", false);
    }
    if (variant)
    {
      auto& override = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(shadowNode);
      override.m_Path.PushBack(cascadeNode);
      override.m_Properties.Insert("Enabled", false);
    }
    for (const xiiStringView name : {"ShadowAtlas", "ShadowVisibility", "AmbientLighting", "DirectLighting", "HDRSceneColor", "SceneDepth", "ObjectId"})
    {
      const auto capture = root.AddPass(name, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(root.Connect(node, name, capture, "Input"));
      root.m_Interface.PushBack({name, false, root.m_Passes[capture].m_Id, "Output"});
    }
    pipelines[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(variant ? "SceneShadow.Disabled" : "SceneShadow.Enabled", std::move(root));
  }
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescriptor;
  meshDescriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  meshDescriptor.AddSection(12, 0, 0);
  meshDescriptor.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("SceneShadow.Box", std::move(meshDescriptor));
  xiiMaterialResourceDescriptor materialDescriptor;
  materialDescriptor.m_BaseColor         = xiiColor(0.65f, 0.45f, 0.25f);
  const auto                    material = xiiResourceManager::CreateResource<xiiMaterialResource>("SceneShadow.Material", std::move(materialDescriptor));
  xiiMaterialResourceDescriptor nonCaster;
  nonCaster.m_BaseColor = xiiColor(0.65f, 0.45f, 0.25f);
  nonCaster.m_FeatureFlags.Remove(xiiMaterialFeatureFlags::CastsShadows);
  const auto noCast = xiiResourceManager::CreateResource<xiiMaterialResource>("SceneShadow.NoCast", std::move(nonCaster));
  struct World
  {
    xiiUniquePtr<xiiWorld>      m_World;
    xiiCamera                   m_Camera;
    xiiView*                    m_Views[2];
    xiiSharedPtr<xiiGALTexture> m_Targets[2];
    xiiGameObjectHandle         m_Caster;
    xiiComponentHandle          m_Mesh, m_Light;
    xiiGameObjectHandle         m_Sun;
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
  const auto addCaster = [&](World& world, xiiVec3 position) {
    xiiGameObjectDescription description;
    description.m_bDynamic            = true;
    description.m_LocalPosition       = position;
    xiiGameObject* object             = nullptr;
    world.m_Caster                    = world.m_World->CreateObject(description, object);
    xiiStaticMeshComponent* component = nullptr;
    world.m_Mesh                      = xiiStaticMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    component->SetMaterialOverride(0, material);
  };
  for (xiiUInt32 index = 0; index < 2; ++index)
  {
    auto&               world = worlds[index];
    xiiWorldDescription description("Shadow independent world");
    world.m_World = XII_DEFAULT_NEW(xiiWorld, description);
    XII_LOCK(world.m_World->GetWriteMarker());
    world.m_World->SetWorldSimulationEnabled(false);
    auto*                    module = world.m_World->GetOrCreateModule<xiiRenderWorldModule>();
    xiiGameObjectDescription floor;
    floor.m_LocalPosition = {0, 0, -0.1f};
    floor.m_LocalScaling  = {12, 12, 0.2f};
    xiiGameObject* object = nullptr;
    world.m_World->CreateObject(floor, object);
    xiiStaticMeshComponent* component = nullptr;
    xiiStaticMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    component->SetMaterialOverride(0, material);
    addCaster(world, {index ? 0.7f : 0, 0, 0.5f});
    xiiGameObjectDescription sun;
    sun.m_bDynamic                      = true;
    sun.m_LocalRotation                 = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3(-0.8f, -0.6f, -1).GetNormalized());
    world.m_Sun                         = world.m_World->CreateObject(sun, object);
    xiiDirectionalLightComponent* light = nullptr;
    world.m_Light                       = xiiDirectionalLightComponent::CreateComponent(object, light);
    light->SetIntensityUnit(xiiPhotometricUnit::Lux);
    light->SetIntensity(0.1f);
    light->SetCastShadows(true);
    world.m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    world.m_Camera.LookAt({-4, -5, 3}, {0, 0, 0.2f}, {0, 0, 1});
    world.m_Camera.SetExposure(8);
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      module->CreateView("Shadow enabled/disabled comparison", world.m_Views[view]);
      world.m_Views[view]->SetCamera(&world.m_Camera);
      world.m_Views[view]->SetRenderPipelineResource(pipelines[view]);
      xiiDisplayOutputSettings display;
      display.m_Exposure.m_Mode = xiiExposureMode::Manual;
      display.m_fBloomStrength  = 0;
      XII_TEST_RESULT(world.m_Views[view]->SetDisplayOutputSettings(display));
    }
    resize(world, 129, 97);
  }
  xiiDynamicArray<float> previousMasks[2];
  for (xiiUInt32 phase = 0; phase < 13; ++phase)
  {
    if (phase == 5 && !ReloadShadowLayout(shadowResource))
      return;
    if (phase == 7)
      xiiMakeReloadablePipelineTestResource(canonical, scene.GetResourceID());
    if (phase == 12)
      worlds[0].m_World.Clear();
    xiiSharedPtr<xiiGALTexture> masks[2];
    for (xiiUInt32 index = 0; index < 2; ++index)
    {
      auto& world = worlds[index];
      if (!world.m_World)
        continue;
      XII_LOCK(world.m_World->GetWriteMarker());
      if (phase == 3)
        for (auto* view : world.m_Views)
          view->InvalidateTemporalHistory();
      if (phase == 4)
        resize(world, 97, 73);
      if (phase == 6 && index == 0)
        world.m_World->DeleteObjectNow(world.m_Caster);
      if (phase == 8)
      {
        if (index)
          world.m_World->DeleteObjectNow(world.m_Caster);
        addCaster(world, {0, -3, 3});
        world.m_Camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 30);
        world.m_Camera.LookAt({0, 0, 10}, {0, 0, 0}, {0, 1, 0});
        xiiGameObject* sun = nullptr;
        if (!XII_TEST_BOOL(world.m_World->TryGetObject(world.m_Sun, sun)))
          return;
        sun->SetLocalRotation(xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3(0, 1, -1).GetNormalized()));
      }
      if (phase == 9 && index == 0)
      {
        xiiStaticMeshComponent* caster = nullptr;
        if (!XII_TEST_BOOL(world.m_World->TryGetComponent(world.m_Mesh, caster)))
          return;
        caster->SetMaterialOverride(0, noCast);
      }
      if (phase == 10 || (phase == 11 && index == 1))
      {
        xiiDirectionalLightComponent* light = nullptr;
        if (!XII_TEST_BOOL(world.m_World->TryGetComponent(world.m_Light, light)))
          return;
        light->SetCastShadows(phase != 10);
      }
      world.m_World->Update();
      environment.GetDevice()->WaitIdle();
      for (auto* view : world.m_Views)
        if (!XII_TEST_RESULT_MSG(view->GetLastRenderResult(), "%s", view->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
          return;
      if (phase < 2)
        continue;
      const auto image = [&](xiiUInt32 view, xiiStringView name) {
        const auto& instance = world.m_Views[view]->GetRenderPipelineInstance();
        const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
        return world.m_Views[view]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture);
      };
      const bool active = phase < 10 || (phase >= 11 && index == 1);
      masks[index]      = image(0, "ShadowVisibility");
      XII_TEST_BOOL(image(1, "ShadowVisibility") == nullptr && image(1, "ShadowAtlas") == nullptr);
      XII_TEST_BOOL((masks[index] != nullptr) == active);
      xiiDynamicArray<float> mask;
      if (active && !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), masks[index], mask))
        return;
      if (phase == 3)
        XII_TEST_BOOL(mask == previousMasks[index]);
      previousMasks[index] = mask;
      if (active)
      {
        const auto atlas = image(0, "ShadowAtlas");
        if (!XII_TEST_BOOL(atlas != nullptr))
          return;
        XII_TEST_INT(atlas->GetDescription().m_Size.width, phase >= 5 ? 512 : 2048);
        XII_TEST_INT(atlas->GetDescription().m_Size.height, phase >= 5 ? 512 : 2048);
        if (phase == 2 || phase == 8)
        {
          xiiDynamicArray<float> atlasPixels;
          if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), atlas, atlasPixels))
            return;
          bool      valid    = true;
          xiiUInt32 occupied = 0;
          for (const float depth : atlasPixels)
          {
            valid &= xiiMath::IsFinite(depth) && depth >= 0 && depth <= 1;
            occupied += depth > 0;
          }
          XII_TEST_BOOL(valid && occupied > 8);
        }
      }
      xiiDynamicArray<xiiVec4>   ambient[2], direct[2], hdr[2];
      xiiDynamicArray<float>     depth[2];
      xiiDynamicArray<xiiUInt32> output[2], ids[2];
      for (xiiUInt32 view = 0; view < 2; ++view)
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "AmbientLighting"), ambient[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "DirectLighting"), direct[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "HDRSceneColor"), hdr[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "SceneDepth"), depth[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "ObjectId"), ids[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), world.m_Targets[view], output[view]))
          return;
      XII_TEST_BOOL(depth[0] == depth[1] && ids[0] == ids[1] && ambient[0] == ambient[1]);
      if (phase >= 8)
        for (const auto identity : ids[0])
          if (identity)
          {
            xiiGpuInstanceSource source;
            if (!XII_TEST_RESULT(world.m_Views[0]->GetGpuInstanceData()->Resolve(identity, source)))
              return;
            XII_TEST_BOOL(source.m_hOwnerObject != world.m_Caster);
          }
      xiiUInt32 shadowed = 0, changed = 0;
      for (xiiUInt32 pixel = 0; pixel < depth[0].GetCount(); ++pixel)
      {
        const float visibility = active ? mask[pixel] : 1;
        XII_TEST_BOOL(xiiMath::IsFinite(visibility) && visibility >= 0 && visibility <= 1.000001f);
        shadowed += visibility < 0.95f;
        changed += output[0][pixel] != output[1][pixel];
        for (xiiUInt32 channel = 0; channel < 3; ++channel)
        {
          // Directional evaluation rounds to RGBA16Float before HDR composition.
          XII_TEST_FLOAT(direct[0][pixel].GetData()[channel], direct[1][pixel].GetData()[channel] * visibility, 0.00002f);
          XII_TEST_FLOAT(hdr[1][pixel].GetData()[channel] - hdr[0][pixel].GetData()[channel], direct[1][pixel].GetData()[channel] - direct[0][pixel].GetData()[channel], 0.000003f);
        }
        if (depth[0][pixel] == 0)
          XII_TEST_FLOAT(visibility, 1, 0);
      }
      xiiTestFramework::Output(xiiTestOutput::Details, "Shadow world %u phase %u: %u shadowed and %u final pixels changed.", index, phase, shadowed, changed);
      if (!active || (index == 0 && ((phase >= 6 && phase < 8) || phase == 9)))
      {
        XII_TEST_INT(shadowed, 0);
        XII_TEST_INT(changed, 0);
      }
      else
      {
        XII_TEST_BOOL(shadowed > 8);
        XII_TEST_BOOL(changed > 8);
      }
      if (phase == 2 && index == 0)
      {
        auto header = world.m_Targets[0]->GetDescription();
        for (xiiUInt32 panel = 0; panel < 3; ++panel)
        {
          xiiImage capture;
          capture.ResetAndAlloc(header);
          auto pixels = capture.GetPixelPointer<xiiUInt32>();
          for (xiiUInt32 pixel = 0; pixel < output[0].GetCount(); ++pixel)
            pixels[pixel] = panel < 2 ? output[panel][pixel] : 0xFF000000U | (xiiUInt32(xiiMath::Clamp(mask[pixel], 0.0f, 1.0f) * 255) * 0x00010101U);
          xiiStringBuilder path;
          path.SetFormat(":imgout/SceneShadow-{}.png", panel == 0 ? "Enabled" : panel == 1 ? "Disabled" :
                                                                                             "Mask");
          XII_TEST_RESULT(capture.SaveTo(path));
        }
      }
      const auto& enabledStats  = world.m_Views[0]->GetRenderGraph()->GetStatistics();
      const auto& disabledStats = world.m_Views[1]->GetRenderGraph()->GetStatistics();
      if (active)
        XII_TEST_BOOL(enabledStats.m_uiCompiledPassCount > disabledStats.m_uiCompiledPassCount + 8);
      else
      {
        // The first view may also upload the world's changed instance/light slices.
        // Check shadow work directly rather than treating those shared uploads as shadows.
        const auto& instance = world.m_Views[0]->GetRenderPipelineInstance();
        for (const auto& operation : instance.GetPipeline()->GetDescriptor().m_Passes)
          if (operation.m_pType == xiiGetStaticRTTI<xiiShadowAtlasPass>() || operation.m_pType == xiiGetStaticRTTI<xiiShadowCascadeViewPass>() || operation.m_pType == xiiGetStaticRTTI<xiiDirectionalShadowVisibilityPass>() || operation.m_pType == xiiGetStaticRTTI<xiiShadowCascadePass>())
            for (const auto& pass : world.m_Views[0]->GetRenderGraph()->GetCompiledPasses())
              if (pass.m_sName.GetView() == operation.m_sName)
                XII_TEST_BOOL(pass.m_bIsCulled);
      }
    }
    if (phase >= 2 && phase < 10)
      XII_TEST_BOOL(masks[0] != masks[1]);
  }

  // Production operations retain copied frame data and GAL objects after the
  // view, world and mutable pipeline instances have been destroyed.
  xiiRenderGraphTestContext retained;
  if (!XII_TEST_RESULT(retained.Initialize("Retained canonical directional shadows")))
    return;
  xiiRenderPipelineInstance recording;
  if (!XII_TEST_RESULT(recording.Synchronize(pipelines[0])))
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
    XII_LOCK(worlds[1].m_World->GetWriteMarker());
    if (!XII_TEST_RESULT_MSG(recording.Build(worlds[1].m_Views[0], graph, retained.GetBlackboard()), "%s", recording.GetLastBuildError().GetStartPointer()))
      return;
  }
  const auto mask = recording.GetInterfaceBinding(recording.GetPipeline()->FindInterface("ShadowVisibility", false))->m_hTexture;
  graph.EndSetup();
  recording.Clear();
  worlds[1].m_World.Clear();
  xiiStringBuilder error;
  if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &retained.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
    return;
  environment.GetDevice()->WaitIdle();
  xiiDynamicArray<float> pixels;
  if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), graph.GetExportedTexture(mask), pixels))
    return;
  xiiUInt32 shadowed = 0;
  for (const auto value : pixels)
    shadowed += value < 0.95f;
  XII_TEST_BOOL(shadowed > 8);
}

XII_CREATE_SIMPLE_TEST(SceneDirectionalShadows, CanonicalCastersLightingAndIndependentWorlds)
{
  TestCanonicalDirectionalShadows(true);
}

XII_CREATE_SIMPLE_TEST(SceneDirectionalShadows, WithoutExtendedStorageFormatsOrMeshShaders)
{
  TestCanonicalDirectionalShadows(false);
}

XII_CREATE_SIMPLE_TEST(SceneDirectionalShadows, DeviceAtlasLimitAndAbsentWorkload)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor descriptor;
  const auto                          atlas = descriptor.AddPass("Directional atlas", xiiGetStaticRTTI<xiiShadowAtlasPass>());
  descriptor.m_Interface.PushBack({"Cascades", true, descriptor.m_Passes[atlas].m_Id, "CascadeSnapshot"});
  descriptor.m_Interface.PushBack({"Atlas", false, descriptor.m_Passes[atlas].m_Id, "ShadowAtlas"});
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("SceneShadow.DeviceLimit", std::move(descriptor));
  xiiRenderPipelineInstance instance;
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(instance.Synchronize(resource)) || !XII_TEST_RESULT(context.Initialize("Shadow atlas capability limits")))
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
    graph.BeginSetup(frame.GetFrameIndex());
    xiiRenderPipelineBinding binding;
    if (variant == 0)
    {
      xiiStaticArray<xiiShadowCascadeDescription, 4> cascades;
      cascades.SetCount(4);
      const auto limit     = environment.GetDevice()->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_uiMaxTexture2DDimension;
      binding.m_pFrameData = XII_DEFAULT_NEW(xiiShadowCascadeSnapshot, graph.GetSetupSerial(), cascades, limit, xiiRenderGraphBufferHandle());
    }
    const xiiRenderPipelineInputBinding supplied(graph, instance.GetPipeline()->FindInterface("Cascades", true), binding);
    const auto                          built = instance.Build(nullptr, graph, context.GetBlackboard(), {}, xiiMakeArrayPtr(&supplied, 1));
    if (variant == 0)
    {
      XII_TEST_BOOL(built.Failed());
      XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("2D image limit") != nullptr);
    }
    else
    {
      XII_TEST_RESULT(built);
      XII_TEST_BOOL(!instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Atlas", false))->m_hTexture.IsValid());
    }
    graph.EndSetup();
  }
}
