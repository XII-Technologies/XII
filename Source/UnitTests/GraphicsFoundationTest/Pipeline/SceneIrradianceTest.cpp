/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/Composition/RadianceAddPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/ScreenIrradianceTracePass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/TextureForwardPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneIrradiance);
XII_CREATE_SIMPLE_TEST(SceneIrradiance, GeometryTransportsEmissionToDiffuseReceivers)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                      scene = xiiLoadPipelineLibraryResource("Scene");
  xiiRenderPipelineResourceHandle pipelines[2], sources[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    xiiResourceLock<xiiRenderPipelineResource> canonical(scene, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(canonical.IsValid() && canonical->GetAuthoring()))
      return;
    auto definition = canonical->GetAuthoring()->m_Descriptor;
    for (auto& pass : definition.m_Passes)
    {
      const auto type = pass.m_pType ? pass.m_pType->GetTypeName() : xiiStringView();
      if (type == "xiiAmbientOcclusionPass" || type == "xiiTemporalCameraPass" || type == "xiiTemporalAntiAliasingPass")
        pass.m_Properties.Insert("Enabled", false);
      if (pass.m_Id == xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenIrradiance"))
      {
        auto& enabled = pass.m_Overrides.ExpandAndGetRef();
        enabled.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneScreenSpaceIrradiance/Bounce Depth"));
        enabled.m_Properties.Insert("Enabled", variant != 0);
      }
    }
    xiiStringBuilder name;
    name.SetFormat("Irradiance.Scene.{}", variant);
    const auto                                 source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
    xiiResourceLock<xiiRenderPipelineResource> loaded(source, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetPipeline()))
      return;
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, source.GetResourceID()});
    const auto node = root.AddSubgraph("Scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
    for (const xiiStringView pin : {"HDRSceneColor", "AmbientLighting", "Albedo", "Material", "SceneDepth", "ObjectId", "ScreenIrradiance"})
    {
      const auto sink = root.AddPass(pin, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(root.Connect(node, pin, sink, "Input"));
      root.m_Interface.PushBack({pin, false, root.m_Passes[sink].m_Id, "Output"});
    }
    name.Append(".Root");
    sources[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(root));
    name.Append(".Reloadable");
    pipelines[variant] = xiiMakeReloadablePipelineTestResource(sources[variant], name);
  }
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  geometry.ComputeTangents();
  xiiMeshResourceDescriptor meshDescription;
  meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  meshDescription.AddSection(12, 0, 0);
  meshDescription.ComputeBounds();
  const auto mesh     = xiiResourceManager::CreateResource<xiiMeshResource>("Irradiance.Box", std::move(meshDescription));
  const auto material = [](xiiStringView name, xiiColor color, xiiColor emission) {
    xiiMaterialResourceDescriptor result;
    result.m_BaseColor       = color;
    result.m_EmissiveColor   = emission;
    result.m_fSheenRoughness = 1;
    return xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(result));
  };
  const auto          floorMaterial = material("Irradiance.Floor", xiiColor(0.8f, 0.8f, 0.8f), xiiColor::Black);
  const auto          red           = material("Irradiance.Red", xiiColor::Black, {4, 0.04f, 0.02f});
  const auto          blue          = material("Irradiance.Blue", xiiColor::Black, {0.02f, 0.04f, 4});
  const auto          black         = material("Irradiance.Black", xiiColor::Black, xiiColor::Black);
  xiiWorldDescription worldDescription("Screen diffuse transport world");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  const auto add = [&](xiiVec3 position, xiiVec3 scale, const xiiMaterialResourceHandle& surface) {
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
  auto* floor   = add({0, 0, -0.05f}, {5, 5, 0.1f}, floorMaterial);
  auto* emitter = add({0, 1, 1}, {5, 0.1f, 2}, red);
  auto* blocker = add({0, 0.5f, 0.5f}, {2, 0.1f, 1}, black);
  blocker->SetActiveFlag(false);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({0, -3, 2.5f}, {0, 0.4f, 0.3f}, {0, 0, 1});
  camera.SetExposure(8);
  xiiView*                    views[2];
  xiiSharedPtr<xiiGALTexture> targets[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Diffuse bounce comparison", views[variant]);
    views[variant]->SetCamera(&camera);
    views[variant]->SetRenderPipelineResource(pipelines[variant]);
    views[variant]->SetViewport({0, 0, 129, 97});
    xiiGALTextureCreationDescription target;
    target.m_Type      = xiiGALResourceDimension::Texture2D;
    target.m_Size      = {129, 97};
    target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    target.m_BindFlags = xiiGALBindFlags::RenderTarget;
    targets[variant]   = environment.GetDevice()->CreateTexture(target);
    views[variant]->SetRenderTargetView(targets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    xiiDisplayOutputSettings display;
    display.m_Exposure.m_Mode = xiiExposureMode::Manual;
    display.m_fBloomStrength  = 0;
    XII_TEST_RESULT(views[variant]->SetDisplayOutputSettings(display));
  }
  double                         initialEnergy = 0;
  xiiDynamicArray<xiiVec4>       initial;
  const xiiColorGammaUB          normalTexel(1, 128, 128, 255);
  xiiGALTextureSubResourceData   normalData(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(&normalTexel), sizeof(normalTexel)), sizeof(normalTexel), sizeof(normalTexel));
  xiiTexture2DResourceDescriptor normalDescriptor;
  normalDescriptor.m_TextureDescription.m_Type   = xiiGALResourceDimension::Texture2D;
  normalDescriptor.m_TextureDescription.m_Size   = {1, 1};
  normalDescriptor.m_TextureDescription.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
  normalDescriptor.m_InitialContent              = xiiMakeArrayPtr(&normalData, 1);
  const auto normalMap                           = xiiResourceManager::CreateResource<xiiTexture2DResource>("Irradiance.ReceiverNormal", std::move(normalDescriptor));
  for (xiiUInt32 phase = 0; phase < 20; ++phase)
  {
    floor->SetMaterialOverride(0, floorMaterial);
    if (phase == 11 || phase == 12 || phase == 15 || phase == 16 || phase == 18)
    {
      xiiMaterialResourceDescriptor receiver;
      receiver.m_BaseColor       = {0.8f, 0.8f, 0.8f};
      receiver.m_fSheenRoughness = 1;
      receiver.m_fMetallic       = phase == 11 ? 1.0f : 0.0f;
      receiver.m_ShadingModel    = phase == 12 ? xiiMaterialShadingModel::Unlit : xiiMaterialShadingModel::Lit;
      if (phase == 15)
        receiver.m_FeatureFlags.Remove(xiiMaterialFeatureFlags::ReceivesLighting);
      receiver.m_fTransmission = phase == 16 ? 0.5f : 0.0f;
      if (phase == 18)
        receiver.m_hNormalTexture = normalMap;
      xiiStringBuilder name;
      name.SetFormat("Irradiance.Receiver.{}", phase);
      floor->SetMaterialOverride(0, xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(receiver)));
    }
    emitter->SetMaterialOverride(0, phase == 1 ? black : phase == 3 ? blue :
                                                                      red);
    blocker->SetActiveFlag(phase == 4);
    emitter->SetActiveFlag(phase != 5);
    if (phase == 6)
      camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
    if (phase == 8)
      camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    if (phase == 9)
      xiiMakeReloadablePipelineTestResource(sources[1], pipelines[1].GetResourceID());
    if (phase == 10)
      for (auto* view : views)
        view->InvalidateTemporalHistory();
    if (phase == 17)
    {
      xiiResourceLock<xiiRenderPipelineResource> original(sources[1], xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      auto                                       descriptor = original->GetAuthoring()->m_Descriptor;
      auto&                                      setting    = descriptor.m_Passes[0].m_Overrides.ExpandAndGetRef();
      setting.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenIrradiance"));
      setting.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneScreenSpaceIrradiance/Trace Diffuse Bounce"));
      setting.m_Properties.Insert("AsyncCompute", true);
      const auto replacement = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Irradiance.Async", std::move(descriptor));
      xiiMakeReloadablePipelineTestResource(replacement, pipelines[1].GetResourceID());
    }
    if (phase == 7 || phase == 8 || phase == 13 || phase == 14)
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
      {
        auto target      = targets[variant]->GetDescription();
        target.m_Size    = phase == 7 ? xiiSizeU32(193, 129) : phase == 13 ? xiiSizeU32(1, 1) :
                                                                             xiiSizeU32(129, 97);
        targets[variant] = environment.GetDevice()->CreateTexture(target);
        views[variant]->SetViewport({0, 0, float(target.m_Size.width), float(target.m_Size.height)});
        views[variant]->SetRenderTargetView(targets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      }
    world.Update();
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiVec4>   hdr[2], irradiance;
    xiiDynamicArray<float>     depth[2];
    xiiDynamicArray<xiiUInt32> ids[2], final[2], albedo, materialPixels;
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
    {
      if (!XII_TEST_RESULT_MSG(views[variant]->GetLastRenderResult(), "%s", views[variant]->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
        return;
      const auto image = [&](xiiStringView pin) {
        auto&       instance = views[variant]->GetRenderPipelineInstance();
        const auto* b        = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(pin, false));
        return views[variant]->GetRenderGraph()->GetExportedTexture(b->m_hTexture);
      };
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("HDRSceneColor"), hdr[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("SceneDepth"), depth[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("ObjectId"), ids[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[variant], final[variant]))
        return;
      if (variant == 1)
      {
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("ScreenIrradiance"), irradiance) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("Albedo"), albedo) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("Material"), materialPixels))
          return;
      }
      else
        XII_TEST_BOOL(image("ScreenIrradiance") == nullptr);
    }
    XII_TEST_BOOL(depth[0] == depth[1] && ids[0] == ids[1]);
    xiiUInt32 hits = 0, changed = 0;
    double    energy = 0;
    for (xiiUInt32 pixel = 0; pixel < hdr[1].GetCount(); ++pixel)
    {
      XII_TEST_BOOL(hdr[1][pixel].IsValid() && irradiance[pixel].IsValid());
      changed += final[0][pixel] != final[1][pixel];
      if ((albedo[pixel] & 255) < 128)
        continue;
      const auto bounce = irradiance[pixel].GetAsVec3();
      hits += bounce.GetLengthSquared() > 0;
      energy += phase == 3 ? bounce.z : bounce.x;
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
      {
        const float reflectance = float((albedo[pixel] >> (8 * channel)) & 255) / 255;
        const float metallic    = float((materialPixels[pixel] >> 8) & 255) / 255;
        const float response    = (materialPixels[pixel] >> 24) & 1 ? 0.0f : (1 - metallic) * (phase == 16 ? 0.5f : 1.0f);
        XII_TEST_FLOAT(hdr[1][pixel].GetData()[channel] - hdr[0][pixel].GetData()[channel], bounce.GetData()[channel] * reflectance * response / xiiMath::Pi<float>(), 2e-5f);
      }
    }
    xiiTestFramework::Output(xiiTestOutput::Details, "Diffuse transport phase %u: %u floor hits, %u final pixels changed, energy %.5f", phase, hits, changed, energy);
    if (phase == 0)
    {
      initialEnergy = energy;
      initial       = hdr[1];
    }
    if (phase == 1 || phase == 5 || phase == 13 || phase == 18)
      XII_TEST_BOOL(hits == 0 && final[0] == final[1]);
    else if (phase == 11 || phase == 12 || phase == 15)
      XII_TEST_BOOL(hits > 8 && final[0] == final[1]);
    else
      XII_TEST_BOOL(hits > 8 && changed > 8 && energy > 0.1);
    if (phase == 2 || phase == 8 || phase == 9 || phase == 10 || phase == 14 || phase == 17 || phase == 19)
      XII_TEST_BOOL(initial == hdr[1]);
    if (phase == 18)
      XII_TEST_BOOL(energy < initialEnergy * 0.9);
    if (phase == 4)
      XII_TEST_BOOL(energy < initialEnergy * 0.95);
    if (phase == 0)
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
      {
        xiiImage capture;
        capture.ResetAndAlloc(targets[variant]->GetDescription());
        xiiMemoryUtils::Copy(capture.GetPixelPointer<xiiUInt32>(), final[variant].GetData(), final[variant].GetCount());
        XII_TEST_RESULT(capture.SaveTo(xiiStringBuilder(":imgout/SceneIrradiance-", variant ? "Enabled" : "Disabled", ".png")));
      }
  }

  const auto image = [](xiiView* view, xiiStringView pin) {
    auto&       instance = view->GetRenderPipelineInstance();
    const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(pin, false));
    return view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture);
  };
  xiiDynamicArray<xiiVec4> primary;
  if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(views[1], "ScreenIrradiance"), primary))
    return;
  xiiRenderPipelineResourceHandle            temporal[2], temporalSources[2];
  xiiResourceLock<xiiRenderPipelineResource> canonical(scene, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  const auto                                 cameraPass = xiiFindPipelineTestPass(canonical->GetAuthoring()->m_Descriptor, "xiiTemporalCameraPass");
  const auto                                 taaPass    = xiiFindPipelineTestPass(canonical->GetAuthoring()->m_Descriptor, "xiiTemporalAntiAliasingPass");
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    xiiResourceLock<xiiRenderPipelineResource> root(sources[1], xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    auto                                       definition    = root->GetAuthoring()->m_Descriptor;
    auto&                                      cameraSetting = definition.m_Passes[0].m_Overrides.ExpandAndGetRef();
    cameraSetting.m_Path.PushBack(cameraPass);
    cameraSetting.m_Properties.Insert("Enabled", true);
    auto& taaSetting = definition.m_Passes[0].m_Overrides.ExpandAndGetRef();
    taaSetting.m_Path.PushBack(taaPass);
    taaSetting.m_Properties.Insert("Enabled", true);
    taaSetting.m_Properties.Insert("BaseBlendAlpha", variant ? 0.08f : 1.0f);
    xiiStringBuilder name;
    name.SetFormat("Irradiance.Temporal.{}", variant);
    temporalSources[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
    name.Append(".Reloadable");
    temporal[variant] = xiiMakeReloadablePipelineTestResource(temporalSources[variant], name);
  }
  xiiCamera foreignCamera;
  foreignCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  foreignCamera.LookAt({0, -3, 2.5f}, {0, 0.4f, 0.3f}, {0, 0, 1});
  foreignCamera.SetExposure(8);
  xiiWorldDescription         foreignDescription("Independent blue irradiance world");
  xiiUniquePtr<xiiWorld>      foreign = XII_DEFAULT_NEW(xiiWorld, foreignDescription);
  xiiView*                    foreignViews[2];
  xiiMeshComponent*           foreignEmitter = nullptr;
  xiiSharedPtr<xiiGALTexture> foreignTargets[2];
  {
    XII_LOCK(foreign->GetWriteMarker());
    foreign->SetWorldSimulationEnabled(false);
    for (xiiUInt32 object = 0; object < 2; ++object)
    {
      xiiGameObjectDescription description;
      description.m_bDynamic      = true;
      description.m_LocalPosition = object ? xiiVec3(0, 1, 1) : xiiVec3(0, 0, -0.05f);
      description.m_LocalScaling  = object ? xiiVec3(5, 0.1f, 2) : xiiVec3(5, 5, 0.1f);
      xiiGameObject* owner        = nullptr;
      foreign->CreateObject(description, owner);
      xiiMeshComponent* component = nullptr;
      xiiMeshComponent::CreateComponent(owner, component);
      component->SetMesh(mesh);
      component->SetMaterialOverride(0, object ? blue : floorMaterial);
      if (object)
        foreignEmitter = component;
    }
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
    {
      foreign->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Independent temporal bounce", foreignViews[variant]);
      foreignViews[variant]->SetCamera(&foreignCamera);
      foreignViews[variant]->SetRenderPipelineResource(temporal[variant]);
      foreignViews[variant]->SetViewport({0, 0, 129, 97});
      foreignTargets[variant] = environment.GetDevice()->CreateTexture(targets[variant]->GetDescription());
      foreignViews[variant]->SetRenderTargetView(foreignTargets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    }
  }
  xiiDynamicArray<xiiVec4> previousIrradiance;
  xiiUInt32                disappeared = 0;
  for (xiiUInt32 phase = 0; phase < 8; ++phase)
  {
    {
      XII_LOCK(foreign->GetWriteMarker());
      if (phase == 2 || phase == 6)
        for (auto* view : foreignViews)
          view->InvalidateTemporalHistory();
      if (phase == 4)
        for (xiiUInt32 variant = 0; variant < 2; ++variant)
        {
          auto target             = foreignTargets[variant]->GetDescription();
          target.m_Size           = {97, 73};
          foreignTargets[variant] = environment.GetDevice()->CreateTexture(target);
          foreignViews[variant]->SetViewport({0, 0, 97, 73});
          foreignViews[variant]->SetRenderTargetView(foreignTargets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
        }
      if (phase == 5)
        for (xiiUInt32 variant = 0; variant < 2; ++variant)
          xiiMakeReloadablePipelineTestResource(temporalSources[variant], temporal[variant].GetResourceID());
      if (phase == 6)
        foreignCamera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
      if (phase == 7)
        foreignEmitter->SetActiveFlag(false);
      foreign->Update();
    }
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiVec4> current, resolved, signal;
    for (auto* view : foreignViews)
      if (!XII_TEST_RESULT_MSG(view->GetLastRenderResult(), "%s", view->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
        return;
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(foreignViews[0], "HDRSceneColor"), current) ||
        !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(foreignViews[1], "HDRSceneColor"), resolved) ||
        !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(foreignViews[1], "ScreenIrradiance"), signal))
      return;
    XII_TEST_BOOL(image(foreignViews[0], "ScreenIrradiance") != image(foreignViews[1], "ScreenIrradiance"));
    xiiUInt32 active = 0;
    for (xiiUInt32 pixel = 0; pixel < resolved.GetCount(); ++pixel)
    {
      XII_TEST_BOOL(resolved[pixel].IsValid() && signal[pixel].IsValid());
      if (signal[pixel].z > 0.01f && signal[pixel].z > signal[pixel].x * 10)
      {
        ++active;
        XII_TEST_BOOL(resolved[pixel].IsEqual(current[pixel], 1e-5f));
      }
      if (phase == 0 || phase == 2 || phase == 4 || phase == 5 || phase == 6)
        XII_TEST_BOOL(resolved[pixel].IsEqual(current[pixel], 1e-5f));
      if (phase == 7)
      {
        const auto width = foreignTargets[1]->GetDescription().m_Size.width;
        if (pixel >= width + 1 && pixel + width + 1 < resolved.GetCount() && pixel % width > 0 && pixel % width + 1 < width)
        {
          bool interior = true;
          for (xiiInt32 y = -1; y <= 1; ++y)
            for (xiiInt32 x = -1; x <= 1; ++x)
              interior &= previousIrradiance[pixel + y * width + x].z > 0.01f;
          if (interior)
          {
            ++disappeared;
            XII_TEST_BOOL(resolved[pixel].IsEqual(current[pixel], 1e-5f));
          }
        }
      }
    }
    XII_TEST_BOOL(phase == 7 ? active == 0 : active > 8);
    previousIrradiance = signal;
    world.Update();
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiVec4> interleaved;
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(views[1], "ScreenIrradiance"), interleaved))
      return;
    XII_TEST_BOOL(interleaved == primary);
    xiiTestFramework::Output(xiiTestOutput::Details, "Independent GI phase %u: %u current pixels; %u disappeared verified", phase, active, disappeared);
  }
  XII_TEST_BOOL(disappeared > 8);
  {
    XII_LOCK(foreign->GetWriteMarker());
    foreignEmitter->SetActiveFlag(true);
    foreign->Update();
  }
  {
    xiiRenderGraphTestContext retained;
    if (!XII_TEST_RESULT(retained.Initialize("Retained screen irradiance")))
      return;
    xiiRenderPipelineInstance recording;
    if (!XII_TEST_RESULT(recording.Synchronize(temporal[1])))
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
      if (!XII_TEST_RESULT(recording.Build(foreignViews[1], graph, retained.GetBlackboard())))
        return;
    }
    const auto output = recording.GetInterfaceBinding(recording.GetPipeline()->FindInterface("ScreenIrradiance", false))->m_hTexture;
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
      blueHits += pixel.z > pixel.x * 10 && pixel.z > 0.01f;
    }
    XII_TEST_BOOL(blueHits > 8);
  }

  xiiRenderPipelineResourceDescriptor missingRadiance;
  XII_TEST_RESULT(xiiLoadPipelineTestDescriptor(missingRadiance, pipelines[1]));
  xiiUInt32 removed = 0;
  for (xiiUInt32 index = missingRadiance.m_Connections.GetCount(); index-- > 0;)
  {
    const auto& connection = missingRadiance.m_Connections[index];
    if (connection.m_sInput == "Radiance" && missingRadiance.m_Passes[connection.m_uiTarget].m_pType->GetTypeName() == "xiiScreenIrradianceTracePass")
    {
      XII_TEST_RESULT(missingRadiance.Disconnect(index));
      ++removed;
    }
  }
  XII_TEST_INT(removed, 1);
  auto activeMissingRadiance = missingRadiance;
  views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Irradiance.MissingRadiance", std::move(activeMissingRadiance)));
  {
    xiiTestLogInterface   expected;
    xiiTestLogSystemScope expectedScope(&expected);
    expected.ExpectMessage("ScreenIrradianceTrace requires matching depth, normal and radiance", xiiLogMsgType::ErrorMsg, 1);
    expected.ExpectMessage("Failed to execute render graph for view", xiiLogMsgType::ErrorMsg, 1);
    world.Update();
    XII_TEST_BOOL(views[0]->GetLastRenderResult().Failed());
    XII_TEST_RESULT(views[1]->GetLastRenderResult());
  }
  // Disabling the authored depth gate must bypass this incomplete workload.
  for (auto& pass : missingRadiance.m_Passes)
    if (pass.m_pType == xiiGetStaticRTTI<xiiTextureForwardPass>())
      pass.m_Properties.Insert("Enabled", false);
  views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Irradiance.DisabledMissingRadiance", std::move(missingRadiance)));
  world.Update();
  XII_TEST_RESULT(views[0]->GetLastRenderResult());
}
