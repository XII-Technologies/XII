/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneRefraction);
XII_CREATE_SIMPLE_TEST(SceneRefraction, DielectricSlabUsesSurfaceOpticsAndOpaqueBackground)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  const auto                                 sceneResource       = xiiLoadPipelineLibraryResource("Scene");
  const auto                                 transparentResource = xiiLoadPipelineLibraryResource("SceneTransparency");
  xiiResourceLock<xiiRenderPipelineResource> scene(sceneResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  xiiResourceLock<xiiRenderPipelineResource> transparent(transparentResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(scene.IsValid() && transparent.IsValid() && scene->GetAuthoring() && transparent->GetAuthoring()))
    return;
  const auto                      rasterId = xiiFindPipelineTestPass(transparent->GetAuthoring()->m_Descriptor, "xiiTransparentRasterPass");
  xiiRenderPipelineResourceHandle pipelines[3], sources[3];
  for (xiiUInt32 variant = 0; variant < 3; ++variant)
  {
    auto definition = scene->GetAuthoring()->m_Descriptor;
    for (auto& pass : definition.m_Passes)
    {
      const auto type = pass.m_pType ? pass.m_pType->GetTypeName() : xiiStringView();
      if (type == "xiiAmbientOcclusionPass" || type == "xiiTemporalAntiAliasingPass" || type == "xiiTemporalCameraPass")
        pass.m_Properties.Insert("Enabled", false);
      if (pass.m_Id == xiiUuid::MakeStableUuidFromString("XII.Scene/Transparency"))
      {
        auto& optics = pass.m_Overrides.ExpandAndGetRef();
        optics.m_Path.PushBack(rasterId);
        optics.m_Properties.Insert("EnableRefraction", variant != 0);
        if (variant == 2)
          optics.m_Properties.Insert("RefractionDistance", 0.1f);
      }
    }
    definition.m_Interface.PushBack({"BaseRadiance", false, xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenReflections"), "HDRSceneColor"});
    xiiStringBuilder name;
    name.SetFormat("Refraction.Scene.{}", variant);
    const auto                          source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({scene->GetAuthoring()->m_Descriptor.m_Id, source.GetResourceID()});
    const auto node = root.AddSubgraph("Canonical Scene", scene->GetAuthoring()->m_Descriptor.m_Id);
    for (const xiiStringView pin : {"HDRSceneColor", "BaseRadiance", "SceneDepth", "ObjectId", "TransparentHistoryTrust"})
    {
      const auto sink = root.AddPass(pin, xiiRTTI::FindTypeByName("xiiOpaqueTestExport"));
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
  xiiMeshResourceDescriptor description;
  description.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  description.AddSection(12, 0, 0);
  description.ComputeBounds();
  const auto          mesh = xiiResourceManager::CreateResource<xiiMeshResource>("Refraction.Box", std::move(description));
  xiiWorldDescription worldDescription("Dielectric refraction world");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  const auto material = [](xiiStringView name, xiiColor color) {
    xiiMaterialResourceDescriptor descriptor;
    descriptor.m_BaseColor    = color;
    descriptor.m_ShadingModel = xiiMaterialShadingModel::Unlit;
    return xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(descriptor));
  };
  const auto red  = material("Refraction.RedReceiver", {0.8f, 0.02f, 0.01f});
  const auto blue = material("Refraction.BlueReceiver", {0.01f, 0.02f, 0.8f});
  const auto add  = [&](xiiVec3 position, xiiVec3 scale, const xiiMaterialResourceHandle& surface) {
    xiiGameObjectDescription objectDescription;
    objectDescription.m_bDynamic      = true;
    objectDescription.m_LocalPosition = position;
    objectDescription.m_LocalScaling  = scale;
    xiiGameObject* object             = nullptr;
    world.CreateObject(objectDescription, object);
    xiiMeshComponent* component = nullptr;
    xiiMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    component->SetMaterialOverride(0, surface);
    return component;
  };
  add({-2, 0, -1}, {4, 8, 0.1f}, red);
  add({2, 0, -1}, {4, 8, 0.1f}, blue);
  auto* pane = add({0.2f, 0, 1}, {1.5f, 1.5f, 0.01f}, blue);
  pane->GetOwner()->SetLocalRotation(xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisY(), xiiAngle::MakeFromDegree(45)));
  auto* foreground = add({-0.1f, 0, 2}, {0.2f, 1, 0.1f}, red);
  foreground->SetActiveFlag(false);

  xiiGameObjectDescription skyDescription;
  xiiGameObject*           sky = nullptr;
  world.CreateObject(skyDescription, sky);
  xiiSkyAtmosphereComponent* atmosphere = nullptr;
  xiiSkyAtmosphereComponent::CreateComponent(sky, atmosphere);
  xiiAtmosphereSettings atmosphereSettings;
  atmosphereSettings.m_vRayleighScattering *= 500;
  atmosphereSettings.m_vMieScattering *= 100;
  atmosphere->SetAtmosphereSettings(atmosphereSettings);
  atmosphere->SetActiveFlag(false);
  xiiGameObjectDescription sunDescription;
  sunDescription.m_LocalRotation = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), -xiiVec3::MakeAxisZ());
  xiiGameObject* sun             = nullptr;
  world.CreateObject(sunDescription, sun);
  xiiDirectionalLightComponent* light = nullptr;
  xiiDirectionalLightComponent::CreateComponent(sun, light);
  light->SetIntensityUnit(xiiPhotometricUnit::Lux);
  light->SetIntensity(0.1f);
  light->SetCastShadows(true);
  light->SetActiveFlag(false);

  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
  camera.LookAt({0.2f, 0, 4}, {0.2f, 0, 0}, {0, 1, 0});
  camera.SetExposure(8);
  xiiView*                    views[2];
  xiiSharedPtr<xiiGALTexture> targets[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Refraction comparison", views[variant]);
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
  const auto texture = [](xiiStringView name, xiiColorGammaUB texel) {
    xiiGALTextureSubResourceData   pixels(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(&texel), sizeof(texel)), sizeof(texel), sizeof(texel));
    xiiTexture2DResourceDescriptor descriptor;
    descriptor.m_TextureDescription.m_Type      = xiiGALResourceDimension::Texture2D;
    descriptor.m_TextureDescription.m_Size      = {1, 1};
    descriptor.m_TextureDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    descriptor.m_InitialContent                 = xiiMakeArrayPtr(&pixels, 1);
    descriptor.m_SamplerDescription.m_MinFilter = xiiGALFilterType::Point;
    descriptor.m_SamplerDescription.m_MagFilter = xiiGALFilterType::Point;
    return xiiResourceManager::CreateResource<xiiTexture2DResource>(name, std::move(descriptor));
  };
  const auto               normalMap       = texture("Refraction.NormalMap", xiiColorGammaUB(128, 218, 218, 255));
  const auto               transmissionMap = texture("Refraction.TransmissionMap", xiiColorGammaUB(128, 255, 255, 255));
  xiiDynamicArray<xiiVec4> initial[2];
  for (xiiUInt32 phase = 0; phase < 24; ++phase)
  {
    xiiMaterialResourceDescriptor glass;
    glass.m_BaseColor          = xiiColor::White;
    glass.m_AlphaMode          = xiiMaterialAlphaMode::Blend;
    glass.m_ShadingModel       = xiiMaterialShadingModel::Unlit;
    glass.m_fTransmission      = phase == 3 ? 0.0f : 1.0f;
    glass.m_fThickness         = phase == 2 ? 0.0f : 0.8f;
    glass.m_fIndexOfRefraction = phase == 1 ? 1.0f : 1.5f;
    if (phase == 19 || phase == 20)
      glass.m_fIndexOfRefraction = 1;
    if (phase == 21 || phase == 22)
      glass.m_ShadingModel = xiiMaterialShadingModel::Lit;
    if (phase == 8)
      glass.m_hNormalTexture = normalMap;
    if (phase == 9)
      glass.m_hTransmissionTexture = transmissionMap;
    if (phase == 13)
      glass.m_fThickness = 1000;
    if (phase == 14)
      glass.m_BaseColor = {0.2f, 0.8f, 0.4f, 1};
    xiiStringBuilder name;
    name.SetFormat("Refraction.Glass.{}", phase);
    pane->SetMaterialOverride(0, xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(glass)));
    pane->SetActiveFlag(phase != 4);
    if (phase == 6 || phase == 16 || phase == 17)
    {
      const xiiSizeU32 size = phase == 6 ? xiiSizeU32(193, 129) : phase == 16 ? xiiSizeU32(1, 1) :
                                                                                xiiSizeU32(129, 97);
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
      {
        auto target      = targets[variant]->GetDescription();
        target.m_Size    = size;
        targets[variant] = environment.GetDevice()->CreateTexture(target);
        views[variant]->SetViewport({0, 0, float(size.width), float(size.height)});
        views[variant]->SetRenderTargetView(targets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      }
    }
    if (phase == 7)
      camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    if (phase == 8)
      camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
    if (phase == 11)
      foreground->SetActiveFlag(true);
    if (phase == 12)
      foreground->SetActiveFlag(false);
    if (phase == 15)
      views[1]->SetRenderPipelineResource(pipelines[2]);
    if (phase == 16)
      views[1]->SetRenderPipelineResource(pipelines[1]);
    if (phase == 18)
      xiiMakeReloadablePipelineTestResource(sources[1], pipelines[1].GetResourceID());
    atmosphere->SetActiveFlag(phase == 19 || phase == 21);
    light->SetActiveFlag(phase >= 20 && phase <= 22);
    world.Update();
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiVec4>   hdr[2], base;
    xiiDynamicArray<float>     depth[2];
    xiiDynamicArray<xiiUInt32> ids[2], final[2];
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
    {
      if (!XII_TEST_RESULT_MSG(views[variant]->GetLastRenderResult(), "%s", views[variant]->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
        return;
      const auto image = [&](xiiStringView pin) {
        auto&       instance = views[variant]->GetRenderPipelineInstance();
        const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(pin, false));
        return binding ? views[variant]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
      };
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("HDRSceneColor"), hdr[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("SceneDepth"), depth[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("ObjectId"), ids[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[variant], final[variant]))
        return;
      if (variant == 1 && !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("BaseRadiance"), base))
        return;
    }
    XII_TEST_BOOL(depth[0] == depth[1] && ids[0] == ids[1]);
    const auto middle = hdr[1].GetCount() / 2;
    const auto center = hdr[1][middle];
    xiiTestFramework::Output(xiiTestOutput::Details, "Refraction phase %u center: %.6f %.6f %.6f, base %.6f %.6f %.6f", phase, center.x, center.y, center.z, base[middle].x, base[middle].y, base[middle].z);
    for (const auto& pixel : hdr[1])
      XII_TEST_BOOL(pixel.IsValid());
    xiiUInt32 changed = 0;
    for (xiiUInt32 pixel = 0; pixel < final[0].GetCount(); ++pixel)
      changed += final[0][pixel] != final[1][pixel];
    xiiTestFramework::Output(xiiTestOutput::Details, "Refraction phase %u: %u final pixels changed", phase, changed);
    if (phase == 0)
    {
      initial[0] = hdr[0];
      initial[1] = hdr[1];
    }
    if (phase == 5 || phase == 17 || phase == 18 || phase == 23)
      XII_TEST_BOOL(hdr[0] == initial[0] && hdr[1] == initial[1]);
    if (phase == 0 || phase == 6 || phase == 7 || phase == 10 || phase == 12 || phase == 17 || phase == 18)
      XII_TEST_BOOL(center.x > center.z * 10);
    if (phase == 8 || phase == 11 || phase == 13 || phase == 15)
      XII_TEST_BOOL(center.z > center.x * 10);
    if (phase == 9)
    {
      const auto  receiver     = base[middle + 24];
      const float cosine       = xiiMath::Sqrt(0.5f);
      const float transmission = (128.0f / 255.0f);
      const float fresnel      = 0.04f + 0.96f * xiiMath::Pow(1 - cosine, 5.0f);
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
        XII_TEST_FLOAT(center.GetData()[channel], 1 - transmission + receiver.GetData()[channel] * transmission * (1 - fresnel), 2e-5f);
    }
    if (phase == 0 || phase == 5)
      XII_TEST_BOOL(center.x > center.z * 10 && center.x > base[middle].x * 10);
    if (phase == 0 || phase == 2)
    {
      const float cosine       = xiiMath::Sqrt(0.5f);
      const float transmission = 1 - (0.04f + 0.96f * xiiMath::Pow(1 - cosine, 5.0f));
      const auto  receiver     = phase == 0 ? base[middle + 24] : base[middle];
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
        XII_TEST_FLOAT(center.GetData()[channel], receiver.GetData()[channel] * transmission, 2e-5f);
    }
    if (phase == 1)
      XII_TEST_BOOL(center.IsEqual(base[middle], 1e-5f));
    if (phase == 19 || phase == 20)
      XII_TEST_BOOL(center.IsEqual(base[middle], 1e-5f));
    if (phase >= 19 && phase <= 22)
      XII_TEST_BOOL(changed > 8);
    if (phase == 2)
      XII_TEST_BOOL(center.z > center.x * 10 && center.z > base[middle].z * 0.9f);
    if (phase == 3 || phase == 4)
      XII_TEST_BOOL(hdr[0] == hdr[1] && final[0] == final[1]);
    if (phase == 0)
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
      {
        xiiImage capture;
        capture.ResetAndAlloc(targets[variant]->GetDescription());
        xiiMemoryUtils::Copy(capture.GetPixelPointer<xiiUInt32>(), final[variant].GetData(), final[variant].GetCount());
        XII_TEST_RESULT(capture.SaveTo(xiiStringBuilder(":imgout/SceneRefraction-", variant ? "Enabled" : "Disabled", ".png")));
      }
  }

  xiiRenderPipelineResourceDescriptor missingBackground;
  XII_TEST_RESULT(xiiLoadPipelineTestDescriptor(missingBackground, pipelines[1]));
  for (xiiUInt32 index = missingBackground.m_Connections.GetCount(); index-- > 0;)
  {
    const auto& connection = missingBackground.m_Connections[index];
    if (connection.m_sInput == "BackgroundRadiance" && missingBackground.m_Passes[connection.m_uiTarget].m_pType->GetTypeName() == "xiiTransparentRasterPass")
      XII_TEST_RESULT(missingBackground.Disconnect(index));
  }
  const auto malformed = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Refraction.MissingBackground", std::move(missingBackground));
  views[0]->SetRenderPipelineResource(malformed);
  {
    xiiTestLogInterface   expected;
    xiiTestLogSystemScope expectedScope(&expected);
    expected.ExpectMessage("Active refraction requires matching immutable HDR background", xiiLogMsgType::ErrorMsg, 1);
    expected.ExpectMessage("Failed to execute render graph for view", xiiLogMsgType::ErrorMsg, 1);
    world.Update();
    XII_TEST_BOOL(views[0]->GetLastRenderResult().Failed());
  }
  pane->SetActiveFlag(false);
  world.Update();
  XII_TEST_RESULT(views[0]->GetLastRenderResult());
  pane->SetActiveFlag(true);
  views[0]->SetRenderPipelineResource(pipelines[1]);
  views[1]->SetRenderPipelineResource(pipelines[0]);
  xiiMaterialResourceDescriptor invalidOptics;
  invalidOptics.m_AlphaMode          = xiiMaterialAlphaMode::Blend;
  invalidOptics.m_fTransmission      = 1;
  invalidOptics.m_fIndexOfRefraction = 0.5f;
  pane->SetMaterialOverride(0, xiiResourceManager::CreateResource<xiiMaterialResource>("Refraction.InvalidOptics", std::move(invalidOptics)));
  {
    xiiTestLogInterface   expected;
    xiiTestLogSystemScope expectedScope(&expected);
    expected.ExpectMessage("Refraction materials require finite transmission", xiiLogMsgType::ErrorMsg, 1);
    expected.ExpectMessage("Failed to execute render graph for view", xiiLogMsgType::ErrorMsg, 1);
    world.Update();
    XII_TEST_BOOL(views[0]->GetLastRenderResult().Failed());
    XII_TEST_RESULT(views[1]->GetLastRenderResult());
  }
}
