/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/Pipeline/RayEmissiveLightingTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/SceneIrradianceTestUtils.h>

XII_CREATE_SIMPLE_TEST(SceneIrradiance, RayEnclosureEnergyMultibounceCutoutAndEmptyScene)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!xiiRayTracingResources::IsHardwareRayTracingSupported())
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Ray enclosure requires hardware ray tracing.");
    return;
  }
  const auto scene   = xiiLoadPipelineLibraryResource("Scene");
  const auto library = xiiLoadPipelineLibraryResource("SceneScreenSpaceIrradiance");
  xiiUuid    gate, ray;
  {
    xiiResourceLock<xiiRenderPipelineResource> loaded(library, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
      return;
    const auto& definition = loaded->GetAuthoring()->m_Descriptor;
    ray                    = xiiFindPipelineTestPass(definition, "xiiRayTracedIrradiancePass");
    for (const auto& edge : definition.m_Connections)
      if (definition.m_Passes[edge.m_uiTarget].m_Id == ray && edge.m_sInput == "Depth")
        gate = definition.m_Passes[edge.m_uiSource].m_Id;
  }
  if (!XII_TEST_BOOL(gate.IsValid()))
    return;
  xiiRenderPipelineResourceHandle resources[4];
  const xiiUInt32                 hits[] = {0, 1, 2, 4};
  for (xiiUInt32 variant = 0; variant < 4; ++variant)
  {
    xiiResourceLock<xiiRenderPipelineResource> original(scene, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    auto                                       definition = original->GetAuthoring()->m_Descriptor;
    if (!XII_TEST_RESULT(xiiSceneIrradianceTest::UseDeterministicTransport(definition)))
      return;
    for (auto& pass : definition.m_Passes)
    {
      const auto type = pass.m_pType ? pass.m_pType->GetTypeName() : xiiStringView();
      if (type == "xiiAmbientOcclusionPass" || type == "xiiTemporalCameraPass" || type == "xiiTemporalAntiAliasingPass")
        pass.m_Properties.Insert("Enabled", false);
      if (pass.m_Id == xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenIrradiance"))
      {
        auto& enable = pass.m_Overrides.ExpandAndGetRef();
        enable.m_Path.PushBack(gate);
        enable.m_Properties.Insert("Enabled", variant != 0);
        auto& budget = pass.m_Overrides.ExpandAndGetRef();
        budget.m_Path.PushBack(ray);
        budget.m_Properties.Insert("MaximumHits", xiiMath::Max(hits[variant], 1U));
        budget.m_Properties.Insert("RayLength", 30.0f);
        budget.m_Properties.Insert("EmissiveLightSamples", 0U);
      }
    }
    xiiStringBuilder name;
    name.SetFormat("Enclosure.Scene.{}", variant);
    const auto                          source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({original->GetAuthoring()->m_Descriptor.m_Id, source.GetResourceID()});
    const auto node = root.AddSubgraph("Authored Scene", original->GetAuthoring()->m_Descriptor.m_Id);
    for (const xiiStringView pin : {"ScreenIrradiance", "HDRSceneColor", "SceneDepth", "Albedo"})
    {
      const auto sink = root.AddPass(pin, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(root.Connect(node, pin, sink, "Input"));
      root.m_Interface.PushBack({pin, false, root.m_Passes[sink].m_Id, "Output"});
    }
    name.Append(".Root");
    resources[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(root));
  }
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(10), true);
  geometry.ComputeTangents();
  xiiMeshResourceDescriptor meshDescription;
  meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  meshDescription.AddSection(12, 0, 0);
  meshDescription.ComputeBounds();
  const auto                mesh = xiiResourceManager::CreateResource<xiiMeshResource>("GI.EmissiveEnclosure", std::move(meshDescription));
  xiiMaterialResourceHandle materials[4];
  for (xiiUInt32 phase = 0; phase < 4; ++phase)
  {
    xiiMaterialResourceDescriptor material;
    material.m_BaseColor     = phase == 1 ? xiiColor::Black : xiiColor(0.5f, 0.5f, 0.5f);
    material.m_EmissiveColor = phase == 2 ? xiiColor::Black : xiiColor(1, 0.5f, 0.25f);
    material.m_FeatureFlags.Add(xiiMaterialFeatureFlags::TwoSided);
    if (phase == 3)
    {
      material.m_AlphaMode   = xiiMaterialAlphaMode::Mask;
      material.m_BaseColor.a = 0;
    }
    xiiStringBuilder name;
    name.SetFormat("GI.EnclosureMaterial.{}", phase);
    materials[phase] = xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(material));
  }
  xiiWorldDescription description("Diffuse path enclosure");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObject* object = nullptr;
  world.CreateObject({}, object);
  xiiMeshComponent* component = nullptr;
  xiiMeshComponent::CreateComponent(object, component);
  component->SetMesh(mesh);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({0, 0, 0}, {1, 0, 0}, {0, 0, 1});
  xiiView*                    views[4];
  xiiSharedPtr<xiiGALTexture> targets[4];
  for (xiiUInt32 variant = 0; variant < 4; ++variant)
  {
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Diffuse path energy", views[variant]);
    views[variant]->SetCamera(&camera);
    views[variant]->SetRenderPipelineResource(resources[variant]);
    views[variant]->SetViewport({0, 0, 17, 13});
    xiiGALTextureCreationDescription target;
    target.m_Type      = xiiGALResourceDimension::Texture2D;
    target.m_Size      = {17, 13};
    target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    target.m_BindFlags = xiiGALBindFlags::RenderTarget;
    targets[variant]   = environment.GetDevice()->CreateTexture(target);
    views[variant]->SetRenderTargetView(targets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }
  xiiDynamicArray<xiiVec4> originalIrradiance[3];
  for (xiiUInt32 phase = 0; phase < 6; ++phase)
  {
    component->SetMaterialOverride(0, materials[xiiMath::Min(phase, 3U)]);
    component->SetActiveFlag(phase != 4);
    if (phase == 5)
      component->SetMaterialOverride(0, materials[0]);
    world.Update();
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiVec4>   hdr[4], irradiance[4];
    xiiDynamicArray<xiiUInt32> albedo[4];
    for (xiiUInt32 variant = 0; variant < 4; ++variant)
    {
      if (!XII_TEST_RESULT_MSG(views[variant]->GetLastRenderResult(), "%s", views[variant]->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
        return;
      const auto image = [&](xiiStringView name) {
        auto& instance = views[variant]->GetRenderPipelineInstance();
        return views[variant]->GetRenderGraph()->GetExportedTexture(instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false))->m_hTexture);
      };
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("HDRSceneColor"), hdr[variant]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("Albedo"), albedo[variant]))
        return;
      if (variant == 0)
      {
        XII_TEST_BOOL(image("ScreenIrradiance") == nullptr);
        continue;
      }
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("ScreenIrradiance"), irradiance[variant]))
        return;
      if (phase == 0)
        originalIrradiance[variant - 1] = irradiance[variant];
      if (phase == 5)
        XII_TEST_BOOL(irradiance[variant] == originalIrradiance[variant - 1]);
      for (xiiUInt32 pixel = 0; pixel < irradiance[variant].GetCount(); ++pixel)
      {
        XII_TEST_BOOL(irradiance[variant][pixel].IsValid());
        XII_TEST_FLOAT(irradiance[variant][pixel].w, phase == 3 || phase == 4 ? 0.0f : 1.0f, 1e-6f);
        for (xiiUInt32 channel = 0; channel < 3; ++channel)
        {
          const float signal   = irradiance[variant][pixel].GetData()[channel];
          const float emission = channel == 0 ? 1.0f : channel == 1 ? 0.5f :
                                                                      0.25f;
          if (phase >= 2 && phase <= 4)
            XII_TEST_FLOAT(signal, 0, 1e-6f);
          else if (phase == 1 || variant == 1)
            XII_TEST_FLOAT(signal, xiiMath::Pi<float>() * emission, 1e-4f);
          else
          {
            XII_TEST_BOOL(signal > irradiance[1][pixel].GetData()[channel] * 1.20f);
            XII_TEST_BOOL(signal <= irradiance[1][pixel].GetData()[channel] * (variant == 2 ? 1.501f : 1.876f));
            if (variant == 3)
              XII_TEST_BOOL(signal > irradiance[2][pixel].GetData()[channel] + emission * 1e-3f);
          }
          const float reflectance = float((albedo[variant][pixel] >> (channel * 8)) & 255) / 255;
          XII_TEST_FLOAT(hdr[variant][pixel].GetData()[channel] - hdr[0][pixel].GetData()[channel], signal * reflectance / xiiMath::Pi<float>(), 2e-5f);
        }
      }
    }
    xiiTestFramework::Output(xiiTestOutput::Details, "Enclosure phase %u: one/two/four-hit center irradiance %.5f / %.5f / %.5f", phase, irradiance[1][100].x, irradiance[2][100].x, irradiance[3][100].x);
  }
}

XII_CREATE_SIMPLE_TEST(SceneIrradiance, EmissiveRaySamplingEnergyVisibilityAndPanelReference)
{
  xiiRayEmissiveLightingTest::Run(false);
}
