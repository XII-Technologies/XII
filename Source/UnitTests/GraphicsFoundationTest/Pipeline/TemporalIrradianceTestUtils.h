/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsFoundationTest/Pipeline/SceneIrradianceTestUtils.h>

namespace xiiTemporalIrradianceTest
{
  inline void Run(bool editor)
  {
    xiiGPUTestingEnvironment environment;
    if (!XII_TEST_RESULT(environment.Initialize(false, true)))
      return;
    xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
    xiiStartup::StartupHighLevelSystems();
    XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
    if (!xiiRayTracingResources::IsHardwareRayTracingSupported())
      return;
    const auto scene = xiiLoadPipelineLibraryResource("Scene"), gi = xiiLoadPipelineLibraryResource("SceneScreenSpaceIrradiance");
    xiiUuid    gate, ray, filter;
    {
      xiiResourceLock<xiiRenderPipelineResource> loaded(gi, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      const auto&                                definition = loaded->GetAuthoring()->m_Descriptor;
      ray                                                   = xiiFindPipelineTestPass(definition, "xiiRayTracedIrradiancePass");
      filter                                                = xiiFindPipelineTestPass(definition, "xiiIrradianceDenoisePass");
      for (const auto& edge : definition.m_Connections)
        if (definition.m_Passes[edge.m_uiTarget].m_Id == ray && edge.m_sInput == "Depth")
          gate = definition.m_Passes[edge.m_uiSource].m_Id;
    }
    xiiRenderPipelineResourceHandle resources[2];
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
    {
      xiiResourceLock<xiiRenderPipelineResource> loaded(scene, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      auto                                       definition = loaded->GetAuthoring()->m_Descriptor;
      for (auto& pass : definition.m_Passes)
      {
        const auto type = pass.m_pType ? pass.m_pType->GetTypeName() : xiiStringView();
        if (type == "xiiTemporalCameraPass" || type == "xiiTemporalAntiAliasingPass" || type == "xiiAmbientOcclusionPass")
          pass.m_Properties.Insert("Enabled", false);
        if (pass.m_Id == xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenIrradiance"))
        {
          auto& enable = pass.m_Overrides.ExpandAndGetRef();
          enable.m_Path.PushBack(gate);
          enable.m_Properties.Insert("Enabled", variant == 1);
          auto& budget = pass.m_Overrides.ExpandAndGetRef();
          budget.m_Path.PushBack(ray);
          budget.m_Properties.Insert("SampleCount", 4U);
          budget.m_Properties.Insert("MaximumHits", 1U);
          auto& spatial = pass.m_Overrides.ExpandAndGetRef();
          spatial.m_Path.PushBack(filter);
          spatial.m_Properties.Insert("SpatialWeight", 0.0f);
        }
      }
      xiiStringBuilder name;
      name.SetFormat("TemporalGI.Scene.{}", variant);
      auto source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
      if (editor)
      {
        xiiResourceLock<xiiRenderPipelineResource> e(xiiLoadPipelineLibraryResource("Editor"), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
        auto                                       d       = e->GetAuthoring()->m_Descriptor;
        const auto                                 sceneId = loaded->GetAuthoring()->m_Descriptor.m_Id;
        xiiUuid                                    occurrence;
        for (const auto& pass : d.m_Passes)
          if (pass.m_Definition == sceneId)
            occurrence = pass.m_Id;
        for (auto& ref : d.m_References)
          if (ref.m_Id == sceneId)
            ref.m_sResource = source.GetResourceID();
        for (const xiiStringView pin : {"RawScreenIrradiance", "ScreenIrradiance", "IrradianceHistory", "Albedo", "HDRSceneColor"})
          d.m_Interface.PushBack({pin, false, occurrence, pin});
        name.Append(".Editor");
        source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(d));
      }
      xiiResourceLock<xiiRenderPipelineResource> rootSource(source, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      xiiRenderPipelineResourceDescriptor        root;
      root.m_References.PushBack({rootSource->GetAuthoring()->m_Descriptor.m_Id, source.GetResourceID()});
      const auto node = root.AddSubgraph("GI temporal", rootSource->GetAuthoring()->m_Descriptor.m_Id);
      for (const xiiStringView pin : {"RawScreenIrradiance", "ScreenIrradiance", "IrradianceHistory", "Albedo", "HDRSceneColor"})
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
    xiiMeshResourceDescriptor mesh;
    mesh.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
    mesh.AddSection(12, 0, 0);
    mesh.ComputeBounds();
    const auto                shape = xiiResourceManager::CreateResource<xiiMeshResource>("TemporalGI.Enclosure", std::move(mesh));
    xiiMaterialResourceHandle materials[3];
    for (xiiUInt32 index = 0; index < 3; ++index)
    {
      xiiMaterialResourceDescriptor m;
      m.m_BaseColor     = xiiColor(0.5f, 0.5f, 0.5f);
      m.m_EmissiveColor = index == 2 ? xiiColor::Black : xiiColor::White;
      m.m_fRoughness    = index == 1 ? 0.1f : 0.8f;
      m.m_FeatureFlags.Add(xiiMaterialFeatureFlags::TwoSided);
      xiiStringBuilder name;
      name.SetFormat("TemporalGI.Material.{}", index);
      materials[index] = xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(m));
    }
    xiiWorldDescription wd("Temporal irradiance");
    xiiWorld            world(wd);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    xiiGameObject* object = nullptr;
    world.CreateObject({}, object);
    xiiMeshComponent* component = nullptr;
    xiiMeshComponent::CreateComponent(object, component);
    component->SetMesh(shape);
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    camera.LookAt({0, 0, 0}, {1, 0, 0}, {0, 0, 1});
    xiiView*                    views[2];
    xiiSharedPtr<xiiGALTexture> targets[2];
    const auto                  resize = [&](xiiSizeU32 size) {
      for (xiiUInt32 index = 0; index < 2; ++index)
      {
        xiiGALTextureCreationDescription d;
        d.m_Type       = xiiGALResourceDimension::Texture2D;
        d.m_Size       = size;
        d.m_Format     = xiiGALResourceFormat::RGBA8UNormalized;
        d.m_BindFlags  = xiiGALBindFlags::RenderTarget;
        targets[index] = environment.GetDevice()->CreateTexture(d);
        views[index]->SetViewport({0, 0, float(size.width), float(size.height)});
        views[index]->SetRenderTargetView(targets[index]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      }
    };
    for (xiiUInt32 index = 0; index < 2; ++index)
    {
      world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Temporal GI acceptance", views[index]);
      views[index]->SetCamera(&camera);
      views[index]->SetRenderPipelineResource(resources[index]);
    }
    resize({17, 13});
    xiiDynamicArray<xiiVec4> previousRaw;
    for (xiiUInt32 phase = 0; phase < 7; ++phase)
    {
      component->SetMaterialOverride(0, materials[phase == 2 ? 1 : phase == 4 ? 2 :
                                                                                0]);
      if (phase == 3)
        resize({9, 7});
      if (phase == 6)
        views[1]->InvalidateTemporalHistory();
      world.Update();
      environment.GetDevice()->WaitIdle();
      const auto image = [&](xiiUInt32 variant, xiiStringView pin) {
        auto&       instance = views[variant]->GetRenderPipelineInstance();
        const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(pin, false));
        return binding && binding->m_hTexture.IsValid() ? views[variant]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
      };
      for (auto* view : views)
        if (!XII_TEST_RESULT_MSG(view->GetLastRenderResult(), "%s", view->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
          return;
      xiiDynamicArray<xiiVec4>   raw, filtered, hdr, baseline;
      xiiDynamicArray<xiiUInt32> albedo;
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "RawScreenIrradiance"), raw) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "ScreenIrradiance"), filtered) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "HDRSceneColor"), hdr) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(0, "HDRSceneColor"), baseline) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "Albedo"), albedo))
        return;
      XII_TEST_BOOL(!image(0, "IrradianceHistory"));
      if (phase == 0)
        XII_TEST_BOOL(!image(1, "IrradianceHistory"));
      if (phase == 1)
      {
        XII_TEST_BOOL(image(1, "IrradianceHistory") != nullptr);
        XII_TEST_BOOL(raw != previousRaw);
        XII_TEST_BOOL(filtered != raw);
      }
      if (phase == 0 || phase == 2 || phase == 3 || phase == 6)
        XII_TEST_BOOL(filtered == raw);
      for (xiiUInt32 pixel = 0; pixel < raw.GetCount(); ++pixel)
      {
        XII_TEST_BOOL(filtered[pixel].IsValid());
        if (phase == 4)
          XII_TEST_FLOAT(filtered[pixel].GetAsVec3().GetLength(), 0, 1e-6f);
        const float reflectance = float(albedo[pixel] & 255) / 255;
        XII_TEST_FLOAT(hdr[pixel].x - baseline[pixel].x, filtered[pixel].x * reflectance / xiiMath::Pi<float>(), 2e-4f);
      }
      previousRaw = raw;
    }
  }
} // namespace xiiTemporalIrradianceTest
