/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/Passes/Particles/BallisticParticlePass.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleBillboardPass.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleBirthUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/Transparency/WeightedBlendedOITResolvePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Texture/Image/Image.h>
#include <Texture/Utilities/TextureAssetFile.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneParticles);
XII_CREATE_SIMPLE_TEST(SceneParticles, BallisticBillboardsAppendWorldTransparency)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 authored = xiiLoadPipelineLibraryResource("Scene");
  xiiResourceLock<xiiRenderPipelineResource> loaded(authored, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetPipeline()))
    return;
  xiiRenderPipelineResourceHandle pipelines[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    auto definition = loaded->GetAuthoring()->m_Descriptor;
    for (auto& pass : definition.m_Passes)
    {
      const auto type = pass.m_pType ? pass.m_pType->GetTypeName() : xiiStringView();
      if (type == "xiiTemporalCameraPass" || type == "xiiTemporalAntiAliasingPass" || type == "xiiAmbientOcclusionPass")
        pass.m_Properties.Insert("Enabled", false);
      if (pass.m_Id == xiiUuid::MakeStableUuidFromString("XII.Scene/Particles"))
      {
        auto& gate = pass.m_Overrides.ExpandAndGetRef();
        gate.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneParticles/World Births"));
        gate.m_Properties.Insert("Enabled", variant == 1);
        auto& acceleration = pass.m_Overrides.ExpandAndGetRef();
        acceleration.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneParticles/Ballistic Trajectories"));
        acceleration.m_Properties.Insert("Acceleration", xiiVec3::MakeZero());
      }
    }
    definition.m_Interface.PushBack({"BaseRadiance", false, xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenReflections"), "HDRSceneColor"});
    xiiStringBuilder id;
    id.SetFormat("Particles.World.{}", variant);
    const auto                          scene = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(id, std::move(definition));
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, scene.GetResourceID()});
    const auto source = root.AddSubgraph("Canonical Scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
    for (const xiiStringView name : {"HDRSceneColor", "BaseRadiance", "SceneDepth", "ObjectId", "TransparentAccumulation", "TransparentRevealage", "TransparentAdditive", "TransparentModulation", "TransparentHistoryTrust"})
    {
      const auto sink = root.AddPass(name, xiiRTTI::FindTypeByName("xiiOpaqueTestExport"));
      XII_TEST_RESULT(root.Connect(source, name, sink, "Input"));
      root.m_Interface.PushBack({name, false, root.m_Passes[sink].m_Id, "Output"});
    }
    id.Append(".Root");
    pipelines[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(id, std::move(root));
  }
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescription;
  meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  meshDescription.AddSection(12, 0, 0);
  meshDescription.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("Particles.BackgroundMesh", std::move(meshDescription));
  xiiMaterialResourceDescriptor blue;
  blue.m_BaseColor               = xiiColor(0.05f, 0.1f, 0.8f);
  blue.m_ShadingModel            = xiiMaterialShadingModel::Unlit;
  const auto          background = xiiResourceManager::CreateResource<xiiMaterialResource>("Particles.BackgroundMaterial", std::move(blue));
  xiiWorldDescription worldDescription("Ballistic billboard world");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObjectDescription objectDescription;
  objectDescription.m_LocalPosition = {0, 0, -0.5f};
  objectDescription.m_LocalScaling  = {8, 8, 0.5f};
  xiiGameObject* object             = nullptr;
  world.CreateObject(objectDescription, object);
  xiiMeshComponent* backgroundComponent = nullptr;
  xiiMeshComponent::CreateComponent(object, backgroundComponent);
  backgroundComponent->SetMesh(mesh);
  backgroundComponent->SetMaterialOverride(0, background);
  objectDescription                 = xiiGameObjectDescription();
  objectDescription.m_bDynamic      = true;
  objectDescription.m_LocalPosition = {0, 0, 0.5f};
  world.CreateObject(objectDescription, object);
  xiiParticleSystemComponent* system = nullptr;
  xiiParticleSystemComponent::CreateComponent(object, system);
  xiiParticleSystemDescriptor settings;
  settings.m_bAlwaysVisible = true;
  settings.m_uiMaxParticles = 8;
  system->SetDescriptor(settings);
  xiiParticleEmitterComponent* emitter = nullptr;
  xiiParticleEmitterComponent::CreateComponent(object, emitter);
  xiiParticleRendererComponent* renderer = nullptr;
  xiiParticleRendererComponent::CreateComponent(object, renderer);
  const auto set = [](xiiReflectedClass* component, xiiStringView name, const xiiVariant& value) {
    const auto* p = static_cast<const xiiAbstractMemberProperty*>(component->GetDynamicRTTI()->FindPropertyByName(name));
    XII_TEST_BOOL(p != nullptr);
    if (p)
      xiiReflectionUtils::SetMemberPropertyValue(p, component, value);
  };
  set(emitter, "Mode", xiiInt64(xiiParticleEmitterMode::Burst));
  set(emitter, "BurstCount", 1U);
  set(emitter, "InitialSpeed", 0.0f);
  set(emitter, "Lifetime", 10.0f);
  set(renderer, "Size", 1.5f);
  set(renderer, "Tint", xiiColor(0.8f, 0.05f, 0.02f, 0.5f));
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({0, 0, 4}, {0, 0, 0}, {0, 1, 0});
  camera.SetExposure(8);
  xiiView*                    views[2];
  xiiSharedPtr<xiiGALTexture> targets[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Particle comparison", views[i]);
    views[i]->SetCamera(&camera);
    views[i]->SetRenderPipelineResource(pipelines[i]);
    views[i]->SetViewport({0, 0, 129, 97});
    xiiGALTextureCreationDescription target;
    target.m_Type      = xiiGALResourceDimension::Texture2D;
    target.m_Size      = {129, 97};
    target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    target.m_BindFlags = xiiGALBindFlags::RenderTarget;
    targets[i]         = environment.GetDevice()->CreateTexture(target);
    views[i]->SetRenderTargetView(targets[i]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    xiiDisplayOutputSettings display;
    display.m_Exposure.m_Mode = xiiExposureMode::Manual;
    display.m_fBloomStrength  = 0;
    XII_TEST_RESULT(views[i]->SetDisplayOutputSettings(display));
  }
  xiiTexture2DResourceHandle sampledTexture;
  xiiMaterialResourceHandle  sampledMaterial;
  xiiVec4                    initialCenter, texturedCenter;
  const auto                 writeTexture = [&](xiiUInt8 alpha) {
    const xiiUInt8                   texels[] = {255, 255, 255, alpha};
    xiiGALTextureCreationDescription description;
    description.m_Type   = xiiGALResourceDimension::Texture2D;
    description.m_Size   = {1, 1};
    description.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
    xiiImageView image;
    image.ResetAndViewExternalStorage(description, xiiConstByteBlobPtr(texels, sizeof(texels)));
    xiiGALSamplerCreationDescription sampler;
    sampler.m_MinFilter = xiiGALFilterType::Point;
    sampler.m_MagFilter = xiiGALFilterType::Point;
    sampler.m_MipFilter = xiiGALFilterType::Point;
    xiiFileWriter file;
    if (!XII_TEST_RESULT(file.Open(":imgout/ParticleSampling.xiiBinTexture2D")))
      return false;
    return XII_TEST_RESULT(xiiTextureAssetFile::WriteImage(file, image, sampler, xiiAssetFileHeader()));
  };
  for (xiiUInt32 phase = 0; phase < 25; ++phase)
  {
    if (phase == 1)
      renderer->SetActiveFlag(false);
    if (phase == 2)
      renderer->SetActiveFlag(true);
    if (phase == 3)
      object->SetLocalPosition(xiiVec3(0, 0, -1));
    if (phase == 4)
      system->SetDescriptor(settings); // A new world-space burst is now behind the geometry.
    if (phase == 5)
    {
      object->SetLocalPosition(xiiVec3(0, 0, 0.5f));
      system->SetDescriptor(settings);
    }
    if (phase == 6)
      set(renderer, "SoftParticleDistance", 2.0f);
    if (phase == 7)
      set(renderer, "SoftParticleDistance", 0.0f);
    if (phase == 8)
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        auto target   = targets[i]->GetDescription();
        target.m_Size = {193, 129};
        targets[i]    = environment.GetDevice()->CreateTexture(target);
        views[i]->SetViewport({0, 0, 193, 129});
        views[i]->SetRenderTargetView(targets[i]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      }
    if (phase == 9)
      camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
    if (phase >= 10 && phase <= 12)
    {
      if (!writeTexture(phase == 11 ? 0 : 128))
        return;
      if (phase == 10)
      {
        sampledTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(":imgout/ParticleSampling.xiiBinTexture2D");
        xiiMaterialResourceDescriptor material;
        material.m_BaseColor         = {0.8f, 0.05f, 0.02f, 0.5f};
        material.m_AlphaMode         = xiiMaterialAlphaMode::Blend;
        material.m_ShadingModel      = xiiMaterialShadingModel::Unlit;
        material.m_hBaseColorTexture = sampledTexture;
        sampledMaterial              = xiiResourceManager::CreateResource<xiiMaterialResource>("Particles.SampledMaterial", std::move(material));
        set(renderer, "Material", sampledMaterial.GetResourceID());
        set(renderer, "Tint", xiiColor::White);
      }
      else
        XII_TEST_BOOL(xiiResourceManager::ReloadResource(sampledTexture, true));
    }
    if (phase == 13)
    {
      xiiResourceLock<xiiMaterialResource> material(sampledMaterial, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      XII_TEST_RESULT(material->GetDefaultInstance()->SetParameter("BaseColor", xiiColor(0.05f, 0.8f, 0.02f, 0.25f)));
    }
    if (phase == 14)
    {
      set(renderer, "Material", xiiString());
      set(renderer, "Tint", xiiColor(0.8f, 0.05f, 0.02f, 0.5f));
    }
    if (phase == 15)
    {
      set(emitter, "Lifetime", 1.0f);
      system->SetDescriptor(settings);
      world.GetClock().SetFixedTimeStep(xiiTime::MakeFromSeconds(0.5));
    }
    if (phase == 16)
      world.SetWorldSimulationEnabled(true);
    if (phase == 18)
    {
      set(emitter, "Mode", xiiInt64(xiiParticleEmitterMode::Continuous));
      set(emitter, "SpawnRate", 2.0f);
    }
    if (phase == 19)
    {
      world.SetWorldSimulationEnabled(false);
      xiiMaterialResourceDescriptor green;
      green.m_BaseColor                     = {0.02f, 0.8f, 0.05f, 0.25f};
      green.m_AlphaMode                     = xiiMaterialAlphaMode::Blend;
      green.m_ShadingModel                  = xiiMaterialShadingModel::Unlit;
      const auto               paneMaterial = xiiResourceManager::CreateResource<xiiMaterialResource>("Particles.OverlappingMeshMaterial", std::move(green));
      xiiGameObjectDescription description;
      description.m_bDynamic      = true;
      description.m_LocalPosition = {0, 0, 0.25f};
      description.m_LocalScaling  = {2, 2, 0.01f};
      xiiGameObject* paneOwner    = nullptr;
      world.CreateObject(description, paneOwner);
      xiiMeshComponent* pane = nullptr;
      xiiMeshComponent::CreateComponent(paneOwner, pane);
      pane->SetMesh(mesh);
      pane->SetMaterialOverride(0, paneMaterial);
    }
    if (phase == 20)
      renderer->SetActiveFlag(false);
    if (phase == 21)
      renderer->SetActiveFlag(true);
    if (phase == 22 || phase == 23)
    {
      xiiMaterialResourceDescriptor layer;
      layer.m_BaseColor    = {0.8f, 0.05f, 0.02f, 0.5f};
      layer.m_BlendMode    = phase == 22 ? xiiMaterialBlendMode::Additive : xiiMaterialBlendMode::Modulate;
      layer.m_ShadingModel = xiiMaterialShadingModel::Unlit;
      const auto material  = xiiResourceManager::CreateResource<xiiMaterialResource>(phase == 22 ? "Particles.Additive" : "Particles.Modulation", std::move(layer));
      set(renderer, "Material", material.GetResourceID());
    }
    if (phase == 24)
      set(renderer, "Material", xiiString());
    world.Update();
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiVec4>   hdr[2], base, accum, added, modulation;
    xiiDynamicArray<float>     reveal, trust;
    xiiDynamicArray<float>     depth[2];
    xiiDynamicArray<xiiUInt32> ids[2], pixels[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      if (!XII_TEST_RESULT_MSG(views[i]->GetLastRenderResult(), "%s", views[i]->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
        return;
      const auto image = [&](xiiStringView name) {
        auto&       instance = views[i]->GetRenderPipelineInstance();
        const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
        return binding ? views[i]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
      };
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("HDRSceneColor"), hdr[i]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("SceneDepth"), depth[i]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("ObjectId"), ids[i]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[i], pixels[i]))
        return;
    }
    const auto diagnostic = [&](xiiStringView name) {
      auto&       instance = views[1]->GetRenderPipelineInstance();
      const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
      return binding ? views[1]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
    };
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), diagnostic("BaseRadiance"), base))
      return;
    if (const auto layers = diagnostic("TransparentAccumulation"))
    {
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), layers, accum) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), diagnostic("TransparentRevealage"), reveal) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), diagnostic("TransparentHistoryTrust"), trust) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), diagnostic("TransparentAdditive"), added) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), diagnostic("TransparentModulation"), modulation))
        return;
      for (xiiUInt32 i = 0; i < base.GetCount(); ++i)
      {
        const float coverage = 1 - reveal[i];
        for (xiiUInt32 channel = 0; channel < 3; ++channel)
        {
          const float expected = ((coverage > 0 ? accum[i].GetData()[channel] / xiiMath::Max(accum[i].w, 1e-6f) * coverage : 0) + base[i].GetData()[channel] * reveal[i] + added[i].GetData()[channel]) * modulation[i].GetData()[channel];
          XII_TEST_FLOAT(hdr[1][i].GetData()[channel], expected, 2e-5f);
        }
        if (phase == 22 || phase == 23)
          XII_TEST_BOOL(trust[i] <= reveal[i] + 1e-6f);
        else
          XII_TEST_FLOAT(trust[i], reveal[i], 1e-6f);
      }
    }
    XII_TEST_BOOL(depth[0] == depth[1] && ids[0] == ids[1]);
    xiiUInt32 changed = 0, finalChanged = 0;
    for (xiiUInt32 i = 0; i < hdr[0].GetCount(); ++i)
    {
      XII_TEST_BOOL(hdr[1][i].IsValid());
      if ((hdr[1][i] - hdr[0][i]).GetLengthSquared() > 1e-5f)
        ++changed;
      if (pixels[0][i] != pixels[1][i])
        ++finalChanged;
    }
    xiiTestFramework::Output(xiiTestOutput::Details, "Particle phase %u: %u HDR / %u presented pixels changed", phase, changed, finalChanged);
    if (phase == 19)
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
      {
        xiiImage capture;
        capture.ResetAndAlloc(targets[variant]->GetDescription());
        xiiMemoryUtils::Copy(capture.GetPixelPointer<xiiUInt32>(), pixels[variant].GetData(), pixels[variant].GetCount());
        XII_TEST_RESULT(capture.SaveTo(xiiStringBuilder(":imgout/SceneParticles-", variant ? "Enabled" : "Disabled", ".png")));
      }
    const auto center = hdr[1][hdr[1].GetCount() / 2];
    if (phase == 0)
      initialCenter = center;
    if (phase == 3 || phase == 5 || phase == 7)
      XII_TEST_BOOL(center.IsEqual(initialCenter, 1e-6f));
    if (phase == 6)
    {
      const auto centerBase = base[base.GetCount() / 2];
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
        XII_TEST_FLOAT(center.GetData()[channel] - centerBase.GetData()[channel], (initialCenter.GetData()[channel] - centerBase.GetData()[channel]) * 0.375f, 2e-4f);
    }
    if (phase == 10)
      texturedCenter = center;
    if (phase == 12)
      XII_TEST_BOOL(center.IsEqual(texturedCenter, 1e-6f));
    if (phase == 16)
      XII_TEST_FLOAT(reveal[reveal.GetCount() / 2], 0.625f, 1e-5f);
    if (phase == 18)
      XII_TEST_BOOL(changed > 500); // A real continuous birth reaches the final scene again.
    if (phase == 22 || phase == 23)
    {
      const auto middle = reveal.GetCount() / 2;
      XII_TEST_FLOAT(reveal[middle], 0.75f, 1e-5f); // Particle alpha does not enter the mesh revealage for these modes.
      XII_TEST_BOOL(trust[middle] < reveal[middle]);
      XII_TEST_BOOL(accum[middle].y > accum[middle].x * 10); // Only the green mesh contributes to weighted accumulation.
      if (phase == 22)
      {
        XII_TEST_BOOL(added[middle].x > added[middle].y * 10 && added[middle].x > 0.01f);
        XII_TEST_BOOL(modulation[middle] == xiiVec4(1));
      }
      else
      {
        XII_TEST_BOOL(added[middle] == xiiVec4::MakeZero());
        XII_TEST_BOOL(modulation[middle].x < 1 && modulation[middle].y < modulation[middle].x);
      }
    }
    if (phase == 10 || phase == 12 || phase == 13)
      XII_TEST_FLOAT(reveal[reveal.GetCount() / 2], 1 - (phase == 13 ? 0.25f : 0.5f) * (128.0f / 255.0f), 2e-5f);
    if (phase != 1 && phase != 4 && phase != 11 && phase != 17 && phase != 20)
    {
      XII_TEST_BOOL(changed > 500 && finalChanged > 500);
    }
    else
    {
      XII_TEST_INT(changed, 0);
      XII_TEST_INT(finalChanged, 0);
    }
  }
}
