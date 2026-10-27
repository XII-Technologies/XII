/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalCameraPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <Texture/Image/Image.h>
#include <Texture/Utilities/TextureAssetFile.h>
XII_CREATE_SIMPLE_TEST_GROUP(SceneTransparency);
XII_CREATE_SIMPLE_TEST(SceneTransparency, RealWorldAlphaLayers)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  const auto                                 canonical = xiiLoadPipelineLibraryResource("Scene");
  xiiResourceLock<xiiRenderPipelineResource> loaded(canonical, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
    return;
  xiiRenderPipelineResourceHandle pipelines[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    auto definition = loaded->GetAuthoring()->m_Descriptor;
    for (auto& pass : definition.m_Passes)
      if (pass.m_pType && pass.m_pType->GetTypeName() == xiiStringView("xiiAmbientOcclusionPass"))
        pass.m_Properties.Insert("Enabled", false);
    const auto trans = xiiUuid::MakeStableUuidFromString("XII.Scene/Transparency");
    for (auto& pass : definition.m_Passes)
      if (pass.m_Id == trans)
      {
        auto& enable = pass.m_Overrides.ExpandAndGetRef();
        enable.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneTransparency/Transparent View"));
        enable.m_Properties.Insert("Enabled", variant == 1);
      }
    definition.m_Interface.PushBack({"UntransformedColor", false, xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenReflections"), "HDRSceneColor"});
    const auto                          source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(variant ? "Transparency.Scene.On" : "Transparency.Scene.Off", std::move(definition));
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, source.GetResourceID()});
    const auto scene = root.AddSubgraph("Canonical Scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
    for (const xiiStringView name : {"HDRSceneColor", "UntransformedColor", "SceneDepth", "ObjectId", "TransparentAccumulation", "TransparentRevealage", "TransparentAdditive", "TransparentModulation", "TransparentHistoryTrust"})
    {
      const auto sink = root.AddPass(name, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(root.Connect(scene, name, sink, "Input"));
      root.m_Interface.PushBack({name, false, root.m_Passes[sink].m_Id, "Output"});
    }
    pipelines[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(variant ? "Transparency.Root.On" : "Transparency.Root.Off", std::move(root));
  }
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  descriptor.AddSection(12, 0, 0);
  descriptor.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("Transparency.Box", std::move(descriptor));
  xiiMaterialResourceDescriptor blue;
  blue.m_BaseColor                     = xiiColor(0.05f, 0.1f, 0.8f);
  const auto                    opaque = xiiResourceManager::CreateResource<xiiMaterialResource>("Transparency.Blue", std::move(blue));
  xiiMaterialResourceDescriptor red;
  red.m_BaseColor                 = xiiColor(0.8f, 0.05f, 0.02f, 0.5f);
  red.m_AlphaMode                 = xiiMaterialAlphaMode::Blend;
  red.m_ShadingModel              = xiiMaterialShadingModel::Unlit;
  const auto          transparent = xiiResourceManager::CreateResource<xiiMaterialResource>("Transparency.Red", std::move(red));
  xiiWorldDescription worldDescription("Transparent material world");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  const auto add = [&](xiiVec3 position, xiiVec3 scale, xiiMaterialResourceHandle material, bool dynamic) {
    xiiGameObjectDescription objectDescription;
    objectDescription.m_bDynamic      = dynamic;
    objectDescription.m_LocalPosition = position;
    objectDescription.m_LocalScaling  = scale;
    xiiGameObject* object             = nullptr;
    world.CreateObject(objectDescription, object);
    xiiMeshComponent* component = nullptr;
    xiiMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    component->SetMaterialOverride(0, material);
    return component;
  };
  add({0, 0, -0.5f}, {8, 8, 0.5f}, opaque, false);
  auto* pane     = add({0, 0, 0.5f}, {2, 2, 0.01f}, transparent, true);
  auto* occluder = add({-2, 0, 2.5f}, {0.6f, 0.8f, 0.1f}, opaque, true);
  occluder->SetActiveFlag(false);
  xiiGameObject*           sky = nullptr;
  xiiGameObjectDescription skyDescription;
  world.CreateObject(skyDescription, sky);
  xiiSkyAtmosphereComponent* atmosphere = nullptr;
  xiiSkyAtmosphereComponent::CreateComponent(sky, atmosphere);
  xiiAtmosphereSettings atmosphereSettings;
  atmosphereSettings.m_vRayleighScattering *= 500;
  atmosphereSettings.m_vMieScattering *= 100;
  atmosphere->SetAtmosphereSettings(atmosphereSettings);
  atmosphere->SetActiveFlag(false);
  xiiMaterialResourceHandle materials[6];
  materials[0] = transparent;
  for (xiiUInt32 mode = 1; mode < 6; ++mode)
  {
    xiiMaterialResourceDescriptor material;
    material.m_BaseColor    = xiiColor(0.8f, 0.05f, 0.02f, mode == 5 ? 0.0f : 0.5f);
    material.m_ShadingModel = mode == 4 ? xiiMaterialShadingModel::Lit : xiiMaterialShadingModel::Unlit;
    material.m_BlendMode    = mode == 1 ? xiiMaterialBlendMode::Additive : mode == 2 ? xiiMaterialBlendMode::Modulate :
                                                                                       xiiMaterialBlendMode::Translucent;
    xiiStringBuilder name;
    name.SetFormat("Transparency.Mode.{}", mode);
    materials[mode] = xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(material));
  }
  xiiGameObjectDescription sun;
  sun.m_bDynamic           = true;
  sun.m_LocalRotation      = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), -xiiVec3::MakeAxisZ());
  xiiGameObject* sunObject = nullptr;
  world.CreateObject(sun, sunObject);
  xiiDirectionalLightComponent* light = nullptr;
  xiiDirectionalLightComponent::CreateComponent(sunObject, light);
  light->SetIntensityUnit(xiiPhotometricUnit::Lux);
  light->SetIntensity(0.1f);
  xiiGameObjectDescription localDescription;
  localDescription.m_LocalPosition = {0.75f, 0, 2};
  xiiGameObject* localObject       = nullptr;
  world.CreateObject(localDescription, localObject);
  xiiPointLightComponent* local = nullptr;
  xiiPointLightComponent::CreateComponent(localObject, local);
  local->SetIntensityUnit(xiiPhotometricUnit::Candela);
  local->SetIntensity(0.5f);
  local->SetRange(5);
  local->SetActiveFlag(false);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({0, 0, 4}, {0, 0, 0}, {0, 1, 0});
  camera.SetExposure(8);
  xiiView*                    views[2];
  xiiSharedPtr<xiiGALTexture> targets[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Transparent Scene comparison", views[variant]);
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
  xiiMaterialResourceDescriptor greenDescription;
  greenDescription.m_BaseColor    = {0.02f, 0.8f, 0.05f, 0.25f};
  greenDescription.m_AlphaMode    = xiiMaterialAlphaMode::Blend;
  greenDescription.m_ShadingModel = xiiMaterialShadingModel::Unlit;
  const auto green                = xiiResourceManager::CreateResource<xiiMaterialResource>("Transparency.Green", std::move(greenDescription));
  auto*      second               = add({0.75f, 0, 0.75f}, {2, 2, 0.01f}, green, true);
  second->SetActiveFlag(false);
  xiiMeshComponent*        replacement = nullptr;
  xiiDynamicArray<xiiVec4> layered;
  xiiDynamicArray<xiiVec4> first;
  double                   litEnergy = 0;
  for (xiiUInt32 phase = 0; phase < 29; ++phase)
  {
    if (phase >= 1 && phase <= 4)
      pane->SetMaterialOverride(0, materials[phase]);
    if (phase == 5)
      light->SetIntensity(0);
    if (phase == 6)
      pane->SetMaterialOverride(0, materials[5]);
    if (phase == 7)
      pane->SetActiveFlag(false);
    if (phase == 8)
    {
      pane->SetActiveFlag(true);
      pane->SetMaterialOverride(0, materials[0]);
      light->SetIntensity(0.1f);
    }
    if (phase == 9)
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
      {
        auto target      = targets[variant]->GetDescription();
        target.m_Size    = {193, 129};
        targets[variant] = environment.GetDevice()->CreateTexture(target);
        views[variant]->SetViewport({0, 0, 193, 129});
        views[variant]->SetRenderTargetView(targets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      }
    if (phase == 10)
      camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
    if (phase == 11)
      pane->GetOwner()->SetLocalPosition(xiiVec3(0.75f, 0, 0.5f));
    if (phase == 12)
    {
      pane->SetMaterialOverride(0, materials[4]);
      light->SetCastShadows(true);
      sunObject->SetLocalRotation(xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3(0.8f, 0, -0.6f)));
    }
    if (phase == 13)
      occluder->SetActiveFlag(true);
    if (phase == 14)
      occluder->SetActiveFlag(false);
    if (phase == 15)
    {
      light->SetCastShadows(false);
      atmosphere->SetActiveFlag(true);
    }
    if (phase == 16)
      light->SetCastShadows(true);
    if (phase == 17)
      atmosphere->SetActiveFlag(false);
    if (phase == 18)
    {
      light->SetCastShadows(false);
      light->SetIntensity(0);
    }
    if (phase == 19)
      local->SetActiveFlag(true);
    if (phase == 20)
      local->SetActiveFlag(false);
    if (phase == 21)
    {
      pane->SetMaterialOverride(0, materials[0]);
      second->SetActiveFlag(true);
    }
    if (phase == 22)
    {
      pane->SetActiveFlag(false);
      replacement = add({0.75f, 0, 0.5f}, {2, 2, 0.01f}, materials[0], true);
    }
    if (phase == 23)
      second->GetOwner()->SetLocalPosition(xiiVec3(0.75f, 0, -1));
    if (phase == 24)
      replacement->GetOwner()->SetLocalPosition(xiiVec3(0.75f, 0, -1.5f));
    if (phase == 25)
    {
      replacement->GetOwner()->SetLocalPosition(xiiVec3(0.75f, 0, 0.5f));
      second->SetActiveFlag(false);
    }

    if (phase >= 26)
    {
      xiiGeometry samplingBox;
      samplingBox.AddBox(xiiVec3(1), true);
      samplingBox.SetAllVertexTexCoord({1.1f, 0.5f});
      xiiMeshResourceDescriptor sampled;
      sampled.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(samplingBox, xiiGALPrimitiveTopology::TriangleList);
      sampled.AddSection(12, 0, 0);
      sampled.ComputeBounds();
      xiiStringBuilder name;
      name.SetFormat("Transparency.TexturedMesh.{}", phase);
      replacement->SetMesh(xiiResourceManager::CreateResource<xiiMeshResource>(name, std::move(sampled)));
      const xiiUInt8                 texels[] = {255, 255, 255, 255, 0, 0, 255, 0};
      xiiGALTextureSubResourceData   content(xiiConstByteBlobPtr(texels, sizeof(texels)), sizeof(texels), sizeof(texels));
      xiiTexture2DResourceDescriptor texture;
      texture.m_TextureDescription.m_Size        = {2, 1};
      texture.m_TextureDescription.m_uiMipLevels = 1;
      texture.m_TextureDescription.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
      texture.m_InitialContent                   = xiiMakeArrayPtr(&content, 1);
      const auto filter                          = phase == 28 ? xiiGALFilterType::Linear : xiiGALFilterType::Point;
      texture.m_SamplerDescription.m_MinFilter   = filter;
      texture.m_SamplerDescription.m_MagFilter   = filter;
      texture.m_SamplerDescription.m_MipFilter   = filter;
      texture.m_SamplerDescription.m_AddressU    = phase == 27 ? xiiGALTextureAddressMode::Clamp : xiiGALTextureAddressMode::Wrap;
      xiiImageView image;
      image.ResetAndViewExternalStorage(texture.m_TextureDescription, content.m_pData);
      {
        xiiFileWriter file;
        if (!XII_TEST_RESULT(file.Open(":imgout/TransparentSampling.xiiBinTexture2D")) || !XII_TEST_RESULT(xiiTextureAssetFile::WriteImage(file, image, texture.m_SamplerDescription, xiiAssetFileHeader())))
          return;
      }
      const auto native = xiiResourceManager::LoadResource<xiiTexture2DResource>(":imgout/TransparentSampling.xiiBinTexture2D");
      if (phase > 26)
        XII_TEST_BOOL(xiiResourceManager::ReloadResource(native, true));
      {
        xiiResourceLock<xiiTexture2DResource> loadedTexture(native, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
        if (!XII_TEST_BOOL(loadedTexture.GetAcquireResult() == xiiResourceAcquireResult::Final))
          return;
      }
      xiiMaterialResourceDescriptor textured;
      textured.m_BaseColor         = {0.8f, 0.05f, 0.02f, 0.5f};
      textured.m_AlphaMode         = xiiMaterialAlphaMode::Blend;
      textured.m_ShadingModel      = xiiMaterialShadingModel::Unlit;
      textured.m_hBaseColorTexture = native;
      name.SetFormat("Transparency.TexturedMaterial.{}", phase);
      replacement->SetMaterialOverride(0, xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(textured)));
    }
    world.Update();
    environment.GetDevice()->WaitIdle();
    for (auto* view : views)
      if (!XII_TEST_RESULT_MSG(view->GetLastRenderResult(), "%s", view->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
        return;
    const auto image = [&](xiiUInt32 variant, xiiStringView name) {
      auto&       instance = views[variant]->GetRenderPipelineInstance();
      const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
      return binding ? views[variant]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
    };
    xiiDynamicArray<xiiVec4>   enabled, disabled, base, accum, added, modulation;
    xiiDynamicArray<float>     reveal, trust, depth[2];
    xiiDynamicArray<xiiUInt32> final[2], ids[2];
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "HDRSceneColor"), enabled) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(0, "HDRSceneColor"), disabled) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "UntransformedColor"), base))
      return;
    const bool produced = image(1, "TransparentAccumulation") != nullptr;
    if (produced)
    {
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "TransparentAccumulation"), accum) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "TransparentRevealage"), reveal) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "TransparentHistoryTrust"), trust) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "TransparentAdditive"), added) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "TransparentModulation"), modulation))
        return;
    }
    else
    {
      accum.SetCount(enabled.GetCount());
      added.SetCount(enabled.GetCount());
      modulation.SetCount(enabled.GetCount());
      reveal.SetCount(enabled.GetCount());
      trust.SetCount(enabled.GetCount());
      for (xiiUInt32 pixel = 0; pixel < enabled.GetCount(); ++pixel)
      {
        accum[pixel]      = xiiVec4::MakeZero();
        added[pixel]      = xiiVec4::MakeZero();
        modulation[pixel] = xiiVec4(1);
        reveal[pixel] = trust[pixel] = 1;
      }
    }
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[variant], final[variant]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "SceneDepth"), depth[variant]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "ObjectId"), ids[variant]))
        return;
    XII_TEST_BOOL(disabled == base && depth[0] == depth[1] && ids[0] == ids[1]);
    XII_TEST_BOOL(image(0, "TransparentAccumulation") == nullptr);
    XII_TEST_BOOL(produced == (phase != 7));
    xiiUInt32 covered = 0, changed = 0;
    double    energy = 0;
    for (xiiUInt32 pixel = 0; pixel < enabled.GetCount(); ++pixel)
    {
      XII_TEST_BOOL(enabled[pixel].IsValid());
      if (trust[pixel] < 0.99f)
      {
        ++covered;
        XII_TEST_FLOAT(trust[pixel], phase == 21 || phase == 22 ? 0.375f : phase == 28 ? 0.65f :
                                                                                         0.5f,
                       phase == 28 ? 0.001f : 0.00001f);
        XII_TEST_FLOAT(reveal[pixel], phase == 1 || phase == 2 ? 1.0f : trust[pixel], 0.00001f);
      }
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
      {
        const float color    = channel == 0 ? 0.8f : channel == 1 ? 0.05f :
                                                                    0.02f;
        const float source   = accum[pixel].w > 0.000001f ? accum[pixel].GetData()[channel] / accum[pixel].w : 0;
        const float expected = ((accum[pixel].w > 0.000001f ? source * (1 - reveal[pixel]) + base[pixel].GetData()[channel] * reveal[pixel] : base[pixel].GetData()[channel]) + added[pixel].GetData()[channel]) * modulation[pixel].GetData()[channel];
        XII_TEST_FLOAT(enabled[pixel].GetData()[channel], expected, 0.0001f);
        if (trust[pixel] < 0.99f)
        {
          if (phase == 1)
          {
            XII_TEST_FLOAT(accum[pixel].w, 0, 0);
            XII_TEST_FLOAT(added[pixel].GetData()[channel], color * 0.5f, 0.00001f);
          }
          else if (phase == 2)
          {
            XII_TEST_FLOAT(accum[pixel].w, 0, 0);
            XII_TEST_FLOAT(modulation[pixel].GetData()[channel], 0.5f + color * 0.5f, 0.00001f);
          }
          else if ((phase != 4 && phase != 5 && phase < 12) || phase >= 26)
            XII_TEST_FLOAT(source, color * (phase == 28 && channel < 2 ? 0.7f : 1.0f), phase == 28 ? 0.002f : 0.0001f);
        }
      }
      if (trust[pixel] < 0.99f && accum[pixel].w > 0)
        energy += accum[pixel].x / accum[pixel].w;
      changed += final[0][pixel] != final[1][pixel];
    }
    if (phase == 15 || phase == 16)
      XII_TEST_BOOL(energy < litEnergy * 0.999);
    if (phase == 17)
      XII_TEST_DOUBLE(energy, litEnergy, 0.001);
    if (phase == 18)
      litEnergy = energy;
    if (phase == 19)
      XII_TEST_BOOL(energy > litEnergy * 2);
    if (phase == 20)
      XII_TEST_DOUBLE(energy, litEnergy, 0.001);
    if (phase == 12)
      litEnergy = energy;
    if (phase == 13)
      XII_TEST_BOOL(energy < litEnergy * 0.99);
    if (phase == 14)
      XII_TEST_DOUBLE(energy, litEnergy, 0.001);
    if (phase == 21)
      layered = enabled;
    if (phase == 22)
    {
      XII_TEST_INT(enabled.GetCount(), layered.GetCount());
      for (xiiUInt32 p = 0; p < enabled.GetCount(); ++p)
      {
        for (xiiUInt32 c = 0; c < 4; ++c)
        {
          XII_TEST_FLOAT(enabled[p].GetData()[c], layered[p].GetData()[c], 0.00001f);
        }
      }
    }
    if (phase == 0)
      first = enabled;
    if (phase == 4)
      litEnergy = energy;
    if (phase == 5)
      XII_TEST_BOOL(energy > 0 && energy < litEnergy * 0.5);
    if (phase == 6 || phase == 7 || phase == 24 || phase == 27)
      XII_TEST_BOOL(covered == 0 && changed == 0 && enabled == base);
    else
      XII_TEST_BOOL(covered > 100 && changed > 100);
    if (phase == 8)
      XII_TEST_BOOL(enabled == first);
    xiiTestFramework::Output(xiiTestOutput::Details, "Transparent Scene phase %u: %u covered, %u final pixels, red energy %.3f.", phase, covered, changed, energy);

    if (phase == 0)
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
      {
        xiiImage capture;
        capture.ResetAndAlloc(targets[variant]->GetDescription());
        xiiMemoryUtils::Copy(capture.GetPixelPointer<xiiUInt32>(), final[variant].GetData(), final[variant].GetCount());
        XII_TEST_RESULT(capture.SaveTo(xiiStringBuilder(":imgout/SceneTransparency-", variant ? "Enabled" : "Disabled", ".png")));
      }
  }
}

static void TestTransparentTemporalWorlds(bool independentBlend)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false, independentBlend)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 canonical = xiiLoadPipelineLibraryResource("Scene");
  xiiResourceLock<xiiRenderPipelineResource> loaded(canonical, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
    return;
  auto definition = loaded->GetAuthoring()->m_Descriptor;
  for (auto& pass : definition.m_Passes)
  {
    if (pass.m_pType && pass.m_pType->GetTypeName() == xiiStringView("xiiAmbientOcclusionPass"))
      pass.m_Properties.Insert("Enabled", false);
    if (pass.m_pType && pass.m_pType->GetTypeName() == xiiStringView("xiiTemporalCameraPass"))
      pass.m_Properties.Insert("Enabled", true);
  }
  const auto                      taa        = xiiFindPipelineTestPass(definition, "xiiTemporalAntiAliasingPass");
  const auto                      trans      = xiiUuid::MakeStableUuidFromString("XII.Scene/Transparency");
  const auto                      source     = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Transparency.TemporalSource", std::move(definition));
  const auto                      reloadable = xiiMakeReloadablePipelineTestResource(source, "Transparency.ReloadableTemporal");
  xiiRenderPipelineResourceHandle pipelines[3];
  for (xiiUInt32 variant = 0; variant < 3; ++variant)
  {
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, reloadable.GetResourceID()});
    const auto node = root.AddSubgraph("Production Scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
    if (variant == 0)
    {
      auto& raw = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      raw.m_Path.PushBack(taa);
      raw.m_Properties.Insert("BaseBlendAlpha", 1.0f);
    }
    if (variant == 2)
    {
      auto& off = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      off.m_Path.PushBack(trans);
      off.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneTransparency/Transparent View"));
      off.m_Properties.Insert("Enabled", false);
    }
    for (const xiiStringView pin : {"HDRSceneColor", "TransparentHistoryTrust", "TransparentAccumulation", "SceneDepth", "ObjectId"})
    {
      const auto sink = root.AddPass(pin, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(root.Connect(node, pin, sink, "Input"));
      root.m_Interface.PushBack({pin, false, root.m_Passes[sink].m_Id, "Output"});
    }
    xiiStringBuilder name;
    name.SetFormat("Transparency.TemporalRoot.{}", variant);
    pipelines[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(root));
  }
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  descriptor.AddSection(12, 0, 0);
  descriptor.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("Transparency.TemporalMesh", std::move(descriptor));
  xiiMaterialResourceDescriptor backgroundDescription;
  backgroundDescription.m_BaseColor = {0.1f, 0.4f, 0.2f};
  const auto background             = xiiResourceManager::CreateResource<xiiMaterialResource>("Transparency.TemporalBackground", std::move(backgroundDescription));
  struct Scene
  {
    xiiUniquePtr<xiiWorld>      world;
    xiiCamera                   camera;
    xiiView*                    views[2];
    xiiMeshComponent*           pane = nullptr;
    xiiSharedPtr<xiiGALTexture> targets[2];
    xiiDynamicArray<float>      previousTrust;
    xiiSizeU32                  size = {129, 97};
  } scenes[2];
  const auto resize = [&](Scene& scene, xiiSizeU32 size) {
    scene.size = size;
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      xiiGALTextureCreationDescription target;
      target.m_Type      = xiiGALResourceDimension::Texture2D;
      target.m_Size      = size;
      target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
      target.m_BindFlags = xiiGALBindFlags::RenderTarget;
      scene.targets[v]   = environment.GetDevice()->CreateTexture(target);
      scene.views[v]->SetViewport({0, 0, float(size.width), float(size.height)});
      scene.views[v]->SetRenderTargetView(scene.targets[v]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    }
  };
  for (xiiUInt32 w = 0; w < 2; ++w)
  {
    auto&               scene = scenes[w];
    xiiWorldDescription worldDescription("Independent transparent temporal world");
    scene.world = XII_DEFAULT_NEW(xiiWorld, worldDescription);
    XII_LOCK(scene.world->GetWriteMarker());
    scene.world->SetWorldSimulationEnabled(false);
    xiiMaterialResourceDescriptor material;
    material.m_BaseColor    = w ? xiiColor(0.02f, 0.05f, 0.8f, 0.5f) : xiiColor(0.8f, 0.05f, 0.02f, 0.5f);
    material.m_ShadingModel = xiiMaterialShadingModel::Unlit;
    material.m_AlphaMode    = xiiMaterialAlphaMode::Blend;
    const auto paneMaterial = xiiResourceManager::CreateResource<xiiMaterialResource>(w ? "Transparency.TemporalBlue" : "Transparency.TemporalRed", std::move(material));
    for (xiiUInt32 objectIndex = 0; objectIndex < 2; ++objectIndex)
    {
      xiiGameObjectDescription objectDescription;
      objectDescription.m_bDynamic      = true;
      objectDescription.m_LocalPosition = {0, 0, objectIndex ? 0.5f : -0.5f};
      objectDescription.m_LocalScaling  = objectIndex ? xiiVec3(2, 2, 0.01f) : xiiVec3(8, 8, 0.5f);
      xiiGameObject* object             = nullptr;
      scene.world->CreateObject(objectDescription, object);
      xiiMeshComponent* component = nullptr;
      xiiMeshComponent::CreateComponent(object, component);
      component->SetMesh(mesh);
      component->SetMaterialOverride(0, objectIndex ? paneMaterial : background);
      if (objectIndex)
        scene.pane = component;
    }
    scene.camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    scene.camera.LookAt({0, 0, 4}, {0, 0, 0}, {0, 1, 0});
    scene.camera.SetExposure(8);
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      scene.world->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Independent transparent temporal view", scene.views[v]);
      scene.views[v]->SetCamera(&scene.camera);
      scene.views[v]->SetRenderPipelineResource(pipelines[v]);
      xiiDisplayOutputSettings display;
      display.m_Exposure.m_Mode = xiiExposureMode::Manual;
      display.m_fBloomStrength  = 0;
      XII_TEST_RESULT(scene.views[v]->SetDisplayOutputSettings(display));
    }
    resize(scene, scene.size);
  }
  for (xiiUInt32 phase = 0; phase < (independentBlend ? 10U : 2U); ++phase)
  {
    if (phase == 7)
      xiiMakeReloadablePipelineTestResource(source, reloadable.GetResourceID());
    for (xiiUInt32 w = 0; w < 2; ++w)
    {
      auto& scene = scenes[w];
      XII_LOCK(scene.world->GetWriteMarker());
      if (phase == 2)
        scene.pane->GetOwner()->SetLocalPosition(xiiVec3(0.7f, 0, 0.5f));
      if (phase == 3)
        scene.pane->SetActiveFlag(false);
      if (phase == 4)
        scene.pane->SetActiveFlag(true);
      if (phase == 5)
        for (auto* view : scene.views)
          view->InvalidateTemporalHistory();
      if (phase == 6)
        resize(scene, {193, 129});
      if (phase == 8)
      {
        scene.camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
        for (auto* view : scene.views)
          view->InvalidateTemporalHistory();
      }
      if (phase == 9)
        scene.camera.LookAt({0.1f, 0, 4}, {0.1f, 0, 0}, {0, 1, 0});
      if (!independentBlend)
      {
        for (auto* view : scene.views)
          view->SetRenderPipelineResource(pipelines[phase ? 2 : 1]);
        if (phase == 0)
          scene.pane->SetActiveFlag(false);
        else
          scene.pane->SetActiveFlag(true);
      }
      scene.world->Update();
      environment.GetDevice()->WaitIdle();
      for (auto* view : scene.views)
        if (!XII_TEST_RESULT_MSG(view->GetLastRenderResult(), "%s", view->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
          return;
      const auto image = [&](xiiUInt32 v, xiiStringView pin) {
        auto&       instance = scene.views[v]->GetRenderPipelineInstance();
        const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(pin, false));
        return binding ? scene.views[v]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
      };
      xiiDynamicArray<xiiVec4> raw, resolved, accum;
      xiiDynamicArray<float>   trust;
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(0, "HDRSceneColor"), raw) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "HDRSceneColor"), resolved))
        return;
      const auto mask = image(1, "TransparentHistoryTrust");
      if (mask)
      {
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), mask, trust) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "TransparentAccumulation"), accum))
          return;
      }
      else
      {
        trust.SetCount(raw.GetCount());
        for (auto& value : trust)
          value = 1;
      }
      XII_TEST_BOOL(bool(mask) == (independentBlend && phase != 3));
      xiiUInt32 covered = 0, disappeared = 0;
      for (xiiUInt32 p = 0; p < resolved.GetCount(); ++p)
      {
        XII_TEST_BOOL(resolved[p].IsValid());
        if (trust[p] < 0.99f)
        {
          ++covered;
          XII_TEST_BOOL(raw[p] == resolved[p]);
          XII_TEST_BOOL(w ? accum[p].z > accum[p].x * 10 : accum[p].x > accum[p].z * 10);
        }
        if (phase == 0 || phase == 5 || phase == 6 || phase == 7 || phase == 8)
          XII_TEST_BOOL(raw[p] == resolved[p]);
        if (phase == 3 && p > scene.size.width && p + scene.size.width + 1 < scene.previousTrust.GetCount() && p % scene.size.width > 0 && p % scene.size.width + 1 < scene.size.width)
        {
          bool interior = true;
          for (xiiInt32 y = -1; y <= 1; ++y)
            for (xiiInt32 x = -1; x <= 1; ++x)
              interior &= scene.previousTrust[p + y * xiiInt32(scene.size.width) + x] < 0.99f;
          if (interior)
          {
            ++disappeared;
            XII_TEST_BOOL(raw[p] == resolved[p]);
          }
        }
      }
      if (independentBlend)
      {
        if (phase == 3)
          XII_TEST_BOOL(disappeared > 100 && covered == 0);
        else
          XII_TEST_BOOL(covered > 100);
      }
      scene.previousTrust = trust;
      xiiTestFramework::Output(xiiTestOutput::Details, "Transparent temporal world %u phase %u: %u current, %u disappeared pixels verified.", w, phase, covered, disappeared);
    }
  }
  if (!independentBlend)
  {
    auto& scene = scenes[0];
    XII_LOCK(scene.world->GetWriteMarker());
    scene.views[0]->SetRenderPipelineResource(pipelines[1]);
    xiiTestLogInterface   expected;
    xiiTestLogSystemScope expectedScope(&expected);
    expected.ExpectMessage("Transparent layers require independent blending", xiiLogMsgType::ErrorMsg, 1);
    expected.ExpectMessage("Failed to execute render graph for view", xiiLogMsgType::ErrorMsg, 1);
    scene.world->Update();
    XII_TEST_BOOL(scene.views[0]->GetLastRenderResult().Failed());
    return;
  }
  xiiRenderGraphTestContext retained;
  if (!XII_TEST_RESULT(retained.Initialize("Retained transparent world")))
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
    XII_LOCK(scenes[1].world->GetWriteMarker());
    if (!XII_TEST_RESULT(recording.Build(scenes[1].views[1], graph, retained.GetBlackboard())))
      return;
  }
  const auto output = recording.GetInterfaceBinding(recording.GetPipeline()->FindInterface("TransparentAccumulation", false))->m_hTexture;
  graph.EndSetup();
  recording.Clear();
  scenes[1].world.Clear();
  xiiStringBuilder error;
  if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &retained.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
    return;
  xiiDynamicArray<xiiVec4> pixels;
  if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), graph.GetExportedTexture(output), pixels))
    return;
  xiiUInt32 blue = 0;
  for (const auto& pixel : pixels)
  {
    XII_TEST_BOOL(pixel.IsValid());
    blue += pixel.w > 0.01f && pixel.z > pixel.x * 10;
  }
  XII_TEST_BOOL(blue > 100);
}
XII_CREATE_SIMPLE_TEST(SceneTransparency, TemporalWorldsViewsReloadAndRetainedRecording) { TestTransparentTemporalWorlds(true); }
XII_CREATE_SIMPLE_TEST(SceneTransparency, ExplicitIndependentBlendFallback) { TestTransparentTemporalWorlds(false); }
