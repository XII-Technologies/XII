/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereApplyPass.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereMultiScatterPass.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereTransmittancePass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/EnvironmentLightConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
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
  class AtmosphereReloadLoader : public xiiResourceLoaderFromMemory
  {
  public:
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> m_Closed;
    void                                                  CloseDataStream(const xiiResource* resource, const xiiResourceLoadData& data) override
    {
      xiiResourceLoaderFromMemory::CloseDataStream(resource, data);
      m_Closed->m_Content.RaiseSignal();
    }
  };

  bool ReloadAtmosphereLookups(const xiiRenderPipelineResourceHandle& resource)
  {
    xiiRenderPipelineResourceDescriptor descriptor;
    {
      xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
        return false;
      descriptor = loaded->GetAuthoring()->m_Descriptor;
    }
    for (auto& pass : descriptor.m_Passes)
    {
      if (pass.m_pType == xiiGetStaticRTTI<xiiAtmosphereTransmittancePass>())
      {
        pass.m_Properties.Insert("Width", 65U);
        pass.m_Properties.Insert("Height", 17U);
      }
      if (pass.m_pType == xiiGetStaticRTTI<xiiAtmosphereMultiScatterPass>())
      {
        pass.m_Properties.Insert("Width", 17U);
        pass.m_Properties.Insert("Height", 9U);
      }
    }
    const auto                           closed = xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>>(XII_DEFAULT_NEW(xiiRefCountedContainer<xiiThreadSignal>));
    xiiUniquePtr<AtmosphereReloadLoader> loader = XII_DEFAULT_NEW(AtmosphereReloadLoader);
    loader->m_Closed                            = closed;
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

XII_CREATE_SIMPLE_TEST_GROUP(SceneAtmosphere);
XII_CREATE_SIMPLE_TEST(SceneAtmosphere, CanonicalSkyAerialPerspectiveOptionalWorldsAndReload)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                          scene      = xiiLoadPipelineLibraryResource("Scene");
  const auto                          atmosphere = xiiLoadPipelineLibraryResource("SceneAtmosphere");
  xiiRenderPipelineResourceDescriptor sceneDefinition, atmosphereDefinition;
  {
    xiiResourceLock<xiiRenderPipelineResource> a(scene, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    xiiResourceLock<xiiRenderPipelineResource> b(atmosphere, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(a.IsValid() && b.IsValid() && a->GetAuthoring() && b->GetAuthoring()))
      return;
    sceneDefinition      = a->GetAuthoring()->m_Descriptor;
    atmosphereDefinition = b->GetAuthoring()->m_Descriptor;
  }
  xiiUuid atmosphereOccurrence;
  for (const auto& pass : sceneDefinition.m_Passes)
    if (pass.m_Definition == atmosphereDefinition.m_Id)
      atmosphereOccurrence = pass.m_Id;
  if (!XII_TEST_BOOL(atmosphereOccurrence.IsValid()))
    return;
  xiiRenderPipelineResourceHandle pipelines[2];
  for (xiiUInt32 disabled = 0; disabled < 2; ++disabled)
  {
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({sceneDefinition.m_Id, scene.GetResourceID()});
    const auto node = root.AddSubgraph("Production Scene", sceneDefinition.m_Id);
    if (disabled)
    {
      auto& override = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(atmosphereOccurrence);
      override.m_Path.PushBack(xiiFindPipelineTestPass(atmosphereDefinition, "xiiAtmosphereConstantsPass"));
      override.m_Properties.Insert("Enabled", false);
    }
    for (const xiiStringView name : {"HDRSceneColor", "SceneDepth", "ObjectId"})
    {
      const auto capture = root.AddPass(name, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(root.Connect(node, name, capture, "Input"));
      root.m_Interface.PushBack({name, false, root.m_Passes[capture].m_Id, "Output"});
    }
    pipelines[disabled] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(disabled ? "SceneAtmosphere.Disabled" : "SceneAtmosphere.Enabled", std::move(root));
  }
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescriptor;
  meshDescriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  meshDescriptor.AddSection(12, 0, 0);
  meshDescriptor.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("SceneAtmosphere.DistantBox", std::move(meshDescriptor));
  xiiMaterialResourceDescriptor materialDescriptor;
  materialDescriptor.m_BaseColor = xiiColor(0.7f, 0.2f, 0.1f);
  const auto material            = xiiResourceManager::CreateResource<xiiMaterialResource>("SceneAtmosphere.Material", std::move(materialDescriptor));
  struct Scene
  {
    xiiUniquePtr<xiiWorld>      m_World;
    xiiRenderWorldModule*       m_Module = nullptr;
    xiiCamera                   m_Camera;
    xiiView*                    m_Views[2];
    xiiSharedPtr<xiiGALTexture> m_Targets[2];
    xiiGameObjectHandle         m_AtmosphereObject;
    xiiGameObjectHandle         m_SunObject;
    xiiComponentHandle          m_Atmosphere;
    xiiAtmosphereSettings       m_Settings;
  } worlds[2];
  const auto addAtmosphere = [&](Scene& world) {
    xiiGameObject* object                = nullptr;
    world.m_AtmosphereObject             = world.m_World->CreateObject({}, object);
    xiiSkyAtmosphereComponent* component = nullptr;
    world.m_Atmosphere                   = xiiSkyAtmosphereComponent::CreateComponent(object, component);
    component->SetAtmosphereSettings(world.m_Settings);
  };
  const auto resize = [&](Scene& world, xiiUInt32 width, xiiUInt32 height) {
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
    xiiWorldDescription description("Atmosphere production world");
    world.m_World = XII_DEFAULT_NEW(xiiWorld, description);
    XII_LOCK(world.m_World->GetWriteMarker());
    world.m_World->SetWorldSimulationEnabled(false);
    world.m_Module = world.m_World->GetOrCreateModule<xiiRenderWorldModule>();
    xiiGameObjectDescription box;
    box.m_LocalPosition   = {1000, 0, 0};
    box.m_LocalScaling    = xiiVec3(200);
    xiiGameObject* object = nullptr;
    world.m_World->CreateObject(box, object);
    xiiStaticMeshComponent* component = nullptr;
    xiiStaticMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    component->SetMaterialOverride(0, material);
    xiiGameObjectDescription sun;
    sun.m_bDynamic                      = true;
    sun.m_LocalRotation                 = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), -xiiVec3::MakeAxisZ());
    world.m_SunObject                   = world.m_World->CreateObject(sun, object);
    xiiDirectionalLightComponent* light = nullptr;
    xiiDirectionalLightComponent::CreateComponent(object, light);
    light->SetIntensityUnit(xiiPhotometricUnit::Lux);
    light->SetIntensity(4);
    world.m_Settings.m_vRayleighScattering *= index ? 0.5f : 1;
    world.m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 5000);
    world.m_Camera.LookAt({-10, 0, 3}, {1000, 0, 3}, {0, 0, 1});
    world.m_Camera.SetExposure(2);
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      world.m_Module->CreateView("Atmosphere Scene/Editor comparison", world.m_Views[view]);
      world.m_Views[view]->SetCamera(&world.m_Camera);
      world.m_Views[view]->SetRenderPipelineResource(pipelines[view]);
      xiiDisplayOutputSettings display;
      display.m_Exposure.m_Mode = xiiExposureMode::Manual;
      display.m_fBloomStrength  = 0;
      XII_TEST_RESULT(world.m_Views[view]->SetDisplayOutputSettings(display));
    }
    resize(world, 65, 49);
  }
  xiiDynamicArray<xiiUInt32> previousOutput[2];
  for (xiiUInt32 phase = 0; phase < 12; ++phase)
  {
    if (phase == 6 && !ReloadAtmosphereLookups(atmosphere))
      return;
    if (phase == 8)
      worlds[0].m_World.Clear();
    xiiVec4 sky[2] = {};
    for (xiiUInt32 index = 0; index < 2; ++index)
    {
      auto& world = worlds[index];
      if (!world.m_World)
        continue;
      XII_LOCK(world.m_World->GetWriteMarker());
      if (phase == 3 || (phase == 11 && index == 1))
        addAtmosphere(world);
      if (phase == 4)
        for (auto* view : world.m_Views)
          view->InvalidateTemporalHistory();
      if (phase == 5)
      {
        resize(world, 97, 65);
        if (index == 0)
        {
          world.m_Settings.m_vRayleighScattering *= 1.5f;
          xiiSkyAtmosphereComponent* component = nullptr;
          if (!XII_TEST_BOOL(world.m_World->TryGetComponent(world.m_Atmosphere, component)))
            return;
          component->SetAtmosphereSettings(world.m_Settings);
        }
      }
      if ((phase == 7 && index == 0) || (phase == 10 && index == 1))
        world.m_World->DeleteObjectNow(world.m_AtmosphereObject);
      if (phase == 9)
      {
        world.m_Camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 200, 0.1f, 5000);
        xiiGameObject* sun = nullptr;
        if (!XII_TEST_BOOL(world.m_World->TryGetObject(world.m_SunObject, sun)))
          return;
        sun->SetLocalRotation(xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3(-0.6f, -0.5f, -0.7f).GetNormalized()));
      }
      world.m_World->Update();
      environment.GetDevice()->WaitIdle();
      for (auto* view : world.m_Views)
        if (!XII_TEST_RESULT(view->GetLastRenderResult()))
          return;
      if (phase < 2)
        continue;
      const bool active = phase >= 3 && !(index == 0 && phase >= 7) && phase != 10;
      const auto image  = [&](xiiUInt32 view, xiiStringView name) {
        const auto& instance = world.m_Views[view]->GetRenderPipelineInstance();
        const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
        return world.m_Views[view]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture);
      };
      xiiDynamicArray<xiiVec4>   hdr[2];
      xiiDynamicArray<float>     depth[2];
      xiiDynamicArray<xiiUInt32> output[2], objectId[2];
      for (xiiUInt32 view = 0; view < 2; ++view)
      {
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), world.m_Targets[view], output[view]))
          return;
        if (view < 2 && (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "HDRSceneColor"), hdr[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "SceneDepth"), depth[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "ObjectId"), objectId[view])))
          return;
        const auto& instance  = world.m_Views[view]->GetRenderPipelineInstance();
        const auto* compiled  = instance.GetPipeline();
        xiiUInt32   inspected = 0;
        for (xiiUInt32 pass = 0; pass < compiled->GetDescriptor().m_Passes.GetCount(); ++pass)
          if (xiiDynamicCast<const xiiAtmosphereTransmittancePass*>(instance.GetPass(pass)))
          {
            const auto* binding     = instance.GetOutputBinding(pass, "Transmittance");
            const auto* description = world.m_Views[view]->GetRenderGraph()->GetTextureDescription(binding->m_hTexture);
            const bool  expected    = active && view != 1;
            XII_TEST_BOOL((description != nullptr) == expected);
            if (description)
            {
              XII_TEST_BOOL(description->m_Format == xiiGALResourceFormat::RGBA32Float);
              XII_TEST_INT(description->m_Size.width, phase >= 6 ? 65 : 256);
              XII_TEST_INT(description->m_Size.height, phase >= 6 ? 17 : 64);
            }
            ++inspected;
          }
        XII_TEST_INT(inspected, 1);
      }
      XII_TEST_BOOL(depth[0] == depth[1] && objectId[0] == objectId[1]);
      if (phase == 4)
        XII_TEST_BOOL(output[0] == previousOutput[index]);
      previousOutput[index] = output[0];
      xiiUInt32 skyChanged = 0, geometryChanged = 0, finalChanged = 0, geometryPixels = 0;
      for (xiiUInt32 pixel = 0; pixel < hdr[0].GetCount(); ++pixel)
      {
        XII_TEST_BOOL(hdr[0][pixel].IsValid() && hdr[0][pixel].x >= 0 && hdr[0][pixel].y >= 0 && hdr[0][pixel].z >= 0);
        const bool changed = !hdr[0][pixel].IsEqual(hdr[1][pixel], 0.000001f);
        if (objectId[0][pixel])
        {
          ++geometryPixels;
          geometryChanged += changed;
        }
        else
          skyChanged += changed;
        finalChanged += output[0][pixel] != output[1][pixel];
      }
      sky[index] = hdr[0][0];
      xiiTestFramework::Output(xiiTestOutput::Details, "Atmosphere world %u phase %u: %u geometry, %u sky and %u geometry HDR changes, %u final pixel changes.", index, phase, geometryPixels, skyChanged, geometryChanged, finalChanged);
      XII_TEST_BOOL(geometryPixels > 10);
      if (active)
      {
        XII_TEST_BOOL(skyChanged > 100 && geometryChanged > 10 && finalChanged > 100);
        XII_TEST_BOOL(sky[index].x + sky[index].y + sky[index].z > 0.001f);
        if (phase == 9 || phase == 11)
        {
          const auto width = world.m_Targets[0]->GetDescription().m_Size.width;
          XII_TEST_BOOL(hdr[0][0].IsEqual(hdr[0][width - 1], 0.000001f));
        }
      }
      else
      {
        XII_TEST_BOOL(hdr[0] == hdr[1]);
        XII_TEST_BOOL(output[0] == output[1]);
        XII_TEST_INT(finalChanged, 0);
      }
      if (phase == 3 && index == 0)
        for (xiiUInt32 variant = 0; variant < 2; ++variant)
        {
          xiiGALTextureCreationDescription description;
          description.m_Type   = xiiGALResourceDimension::Texture2D;
          description.m_Size   = world.m_Targets[variant]->GetDescription().m_Size;
          description.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
          xiiImage capture;
          capture.ResetAndAlloc(description);
          xiiMemoryUtils::Copy(capture.GetPixelPointer<xiiUInt32>(), output[variant].GetData(), output[variant].GetCount());
          XII_TEST_RESULT(capture.SaveTo(variant ? ":imgout/SceneAtmosphere-Disabled.png" : ":imgout/SceneAtmosphere-Enabled.png"));
        }
    }
    if (phase >= 3 && phase < 7)
      XII_TEST_BOOL(!sky[0].IsEqual(sky[1], 0.0001f));
  }
}

XII_CREATE_SIMPLE_TEST(SceneAtmosphere, MissingLookupFailsAndAbsentAtmospherePreservesInputFormat)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiWorldDescription description("Atmosphere failure boundary");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*    module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView* view   = nullptr;
  module->CreateView("Atmosphere failure view", view);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 8, 8});
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Atmosphere failure graph")))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = context.GetGraph();
  auto& cache = context.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
  for (xiiUInt32 disabled = 0; disabled < 2; ++disabled)
  {
    xiiRenderPipelineResourceDescriptor definition;
    const auto                          source         = definition.AddPass("Surface", xiiGetStaticRTTI<xiiScreenEffectTestSource>());
    const auto                          frameConstants = definition.AddPass("Camera", xiiGetStaticRTTI<xiiViewConstantsPass>());
    const auto                          light          = definition.AddPass("Light", xiiGetStaticRTTI<xiiEnvironmentLightConstantsPass>());
    const auto                          constants      = definition.AddPass("Physical parameters", xiiGetStaticRTTI<xiiAtmosphereConstantsPass>());
    definition.m_Passes[constants].m_Properties.Insert("Enabled", disabled == 0);
    const auto apply   = definition.AddPass("Atmospheric application", xiiGetStaticRTTI<xiiAtmosphereApplyPass>());
    const auto capture = definition.AddPass("Capture", xiiGetStaticRTTI<xiiOpaqueTestExport>());
    XII_TEST_RESULT(definition.Connect(source, "Depth", apply, "Depth"));
    XII_TEST_RESULT(definition.Connect(source, "Color", apply, "Radiance"));
    XII_TEST_RESULT(definition.Connect(frameConstants, "PerFrameCamera", apply, "Camera"));
    XII_TEST_RESULT(definition.Connect(light, "EnvironmentLight", apply, "EnvironmentLight"));
    XII_TEST_RESULT(definition.Connect(constants, "AtmosphereConstants", apply, "Atmosphere"));
    XII_TEST_RESULT(definition.Connect(apply, "AtmosphereRadiance", capture, "Input"));
    definition.m_Interface.PushBack({"Result", false, definition.m_Passes[capture].m_Id, "Output"});
    const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(disabled ? "SceneAtmosphere.Absent" : "SceneAtmosphere.MissingLookup", std::move(definition));
    xiiRenderPipelineInstance instance;
    if (!XII_TEST_RESULT(instance.Synchronize(resource)))
      return;
    graph.BeginSetup(frame.GetFrameIndex());
    const auto built = instance.Build(view, graph, context.GetBlackboard());
    if (!disabled)
    {
      XII_TEST_BOOL(built.Failed());
      XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("LUTs") != nullptr);
      graph.EndSetup();
      continue;
    }
    if (!XII_TEST_RESULT_MSG(built, "%s", instance.GetLastBuildError().GetStartPointer()))
      return;
    const auto output = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Result", false))->m_hTexture;
    graph.EndSetup();
    instance.Clear();
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
      return;
    environment.GetDevice()->WaitIdle();
    const auto texture = graph.GetExportedTexture(output);
    if (!XII_TEST_BOOL(texture && texture->GetDescription().m_Format == xiiGALResourceFormat::RGBA16Float))
      return;
    struct Pixel
    {
      XII_DECLARE_POD_TYPE();
      xiiFloat16 r, g, b, a;
    } pixel;
    if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture.Borrow(), 4, 4, pixel))
      return;
    XII_TEST_FLOAT(float(pixel.r), 4, 0);
    XII_TEST_FLOAT(float(pixel.g), 4, 0);
    XII_TEST_FLOAT(float(pixel.b), 4, 0);
    XII_TEST_FLOAT(float(pixel.a), 1, 0);
  }
}
