/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/Utility/BufferExportPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/CounterResetPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneDisplay);
XII_CREATE_SIMPLE_TEST(SceneDisplay, BloomAndMeteredExposureAffectProductionOutput)
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
  auto definition = loaded->GetAuthoring()->m_Descriptor;
  for (auto& pass : definition.m_Passes)
    if (pass.m_pType && pass.m_pType->GetTypeName() == "xiiAmbientOcclusionPass")
      pass.m_Properties.Insert("Enabled", false);
  for (const xiiStringView type : {"xiiBloomPass", "xiiLuminanceHistogramPass"})
    definition.m_Interface.PushBack({type == "xiiBloomPass" ? "Bloom" : "Histogram", false, xiiFindPipelineTestPass(definition, type), type == "xiiBloomPass" ? "BloomTexture" : "LuminanceHistogram"});
  const auto                          source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Display.Scene", std::move(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, source.GetResourceID()});
  const auto scene = root.AddSubgraph("Production Scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
  for (const xiiStringView pin : {"HDRSceneColor", "ObjectId", "Bloom"})
  {
    const auto output = root.AddPass(pin, xiiGetStaticRTTI<xiiOpaqueTestExport>());
    XII_TEST_RESULT(root.Connect(scene, pin, output, "Input"));
    root.m_Interface.PushBack({pin, false, root.m_Passes[output].m_Id, "Output"});
  }
  for (const xiiStringView pin : {"CurrentExposure", "Histogram"})
  {
    const auto output = root.AddPass(pin, xiiGetStaticRTTI<xiiBufferExportPass>());
    XII_TEST_RESULT(root.Connect(scene, pin, output, "Input"));
    root.m_Interface.PushBack({pin, false, root.m_Passes[output].m_Id, "Output"});
  }
  const auto  original = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Display.Capture", std::move(root));
  const auto  pipeline = xiiMakeReloadablePipelineTestResource(original, "Display.Reloadable");
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescription;
  meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  meshDescription.AddSection(12, 0, 0);
  meshDescription.ComputeBounds();
  const auto mesh    = xiiResourceManager::CreateResource<xiiMeshResource>("Display.Box", std::move(meshDescription));
  const auto surface = [](xiiStringView name, xiiColor emission) {
    xiiMaterialResourceDescriptor material;
    material.m_BaseColor     = xiiColor::Black;
    material.m_EmissiveColor = emission;
    return xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(material));
  };
  const auto          bright = surface("Display.Bright", {32, 4, 1});
  const auto          gray   = surface("Display.Gray", {0.1f, 0.1f, 0.1f});
  xiiWorldDescription description("Production display effects");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiMeshComponent* emitter = nullptr;
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiGameObjectDescription objectDescription;
    objectDescription.m_bDynamic      = true;
    objectDescription.m_LocalPosition = i ? xiiVec3(1.5f, 0, 0) : xiiVec3::MakeZero();
    objectDescription.m_LocalScaling  = i ? xiiVec3(0.1f, 8, 8) : xiiVec3(1);
    xiiGameObject* object             = nullptr;
    world.CreateObject(objectDescription, object);
    xiiMeshComponent* component = nullptr;
    xiiMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    component->SetMaterialOverride(0, i ? gray : bright);
    if (!i)
      emitter = component;
  }
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({-4, 0, 0}, {0, 0, 0}, {0, 0, 1});
  camera.SetExposure(0.02f);
  xiiView*                    views[2];
  xiiSharedPtr<xiiGALTexture> targets[2];
  const auto                  resize = [&](xiiUInt32 width, xiiUInt32 height) {
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      xiiGALTextureCreationDescription target;
      target.m_Type      = xiiGALResourceDimension::Texture2D;
      target.m_Size      = {width, height};
      target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
      target.m_BindFlags = xiiGALBindFlags::RenderTarget;
      targets[view]      = environment.GetDevice()->CreateTexture(target);
      views[view]->SetViewport({0, 0, float(width), float(height)});
      views[view]->SetRenderTargetView(targets[view]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    }
  };
  for (auto*& view : views)
  {
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Independent display view", view);
    view->SetCamera(&camera);
    view->SetRenderPipelineResource(pipeline);
  }
  resize(129, 97);
  const auto image = [&](xiiUInt32 view, xiiStringView name) {
    const auto& instance = views[view]->GetRenderPipelineInstance();
    const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
    return binding && binding->m_hTexture.IsValid() ? views[view]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
  };
  const auto buffer = [&](xiiUInt32 view, xiiStringView name) {
    const auto& instance = views[view]->GetRenderPipelineInstance();
    const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
    return views[view]->GetRenderGraph()->GetExportedBuffer(binding->m_hBuffer);
  };
  xiiDynamicArray<xiiUInt32> initial[2];
  xiiUInt32                  initialHalo = 0;
  for (xiiUInt32 phase = 0; phase < 13; ++phase)
  {
    if (phase == 9 || phase == 11)
      resize(phase == 9 ? 193 : 129, phase == 9 ? 129 : 97);
    if (phase == 10)
      camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 3, 0.1f, 100);
    if (phase == 11)
      camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    if (phase == 12)
      xiiMakeReloadablePipelineTestResource(original, pipeline.GetResourceID());
    emitter->SetActiveFlag(phase != 7);
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      xiiDisplayOutputSettings settings;
      settings.m_Exposure.m_Mode = xiiExposureMode::Manual;
      settings.m_fBloomStrength  = view && phase != 6 ? 1.0f : 0.0f;
      settings.m_fBloomRadius    = phase == 3 ? 0.0f : 2.0f;
      settings.m_fBloomThreshold = phase == 1 ? 100000.0f : phase == 4 || phase == 5 ? 20.0f :
                                                                                       1.0f;
      settings.m_fBloomKnee      = phase == 4 ? 20.0f : phase == 5 ? 0.0f :
                                                                     0.5f;
      XII_TEST_RESULT(views[view]->SetDisplayOutputSettings(settings));
    }
    world.Update();
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiUInt32> final[2];
    xiiDynamicArray<xiiVec4>   hdr[2];
    for (xiiUInt32 view = 0; view < 2; ++view)
      if (!XII_TEST_RESULT_MSG(views[view]->GetLastRenderResult(), "%s", views[view]->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[view], final[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "HDRSceneColor"), hdr[view]))
        return;
    XII_TEST_BOOL(hdr[0] == hdr[1] && !image(0, "Bloom"));
    xiiUInt32 changed = 0, halo = 0;
    for (xiiUInt32 pixel = 0; pixel < final[0].GetCount(); ++pixel)
    {
      changed += final[0][pixel] != final[1][pixel];
      halo += final[0][pixel] != final[1][pixel] && hdr[0][pixel].x < 0.2f;
    }
    const auto bloom = image(1, "Bloom");
    XII_TEST_BOOL(bool(bloom) == (phase != 6));
    if (bloom)
    {
      xiiDynamicArray<xiiColorLinear16f> light;
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), bloom, light))
        return;
      for (const auto& pixel : light)
        XII_TEST_BOOL(pixel.ToLinearFloat().IsValid());
      // Compare final composition against the actual GPU bloom and HDR inputs,
      // including exposure before tone mapping and the presentation transfer.
      for (xiiUInt32 pixel = 0; pixel < final[1].GetCount(); ++pixel)
        for (xiiUInt32 channel = 0; channel < 3; ++channel)
        {
          const float x       = (hdr[1][pixel].GetData()[channel] + light[pixel].ToLinearFloat().GetData()[channel]) * camera.GetExposure();
          const float linear  = xiiMath::Saturate(x * (2.51f * x + 0.03f) / (x * (2.43f * x + 0.59f) + 0.14f));
          const float encoded = linear < 0.0031308f ? linear * 12.92f : 1.055f * xiiMath::Pow(linear, 1.0f / 2.4f) - 0.055f;
          XII_TEST_FLOAT(float((final[1][pixel] >> (channel * 8)) & 255) / 255, encoded, 0.005f);
        }
    }
    if (phase == 1 || phase == 5 || phase == 6 || phase == 7)
      XII_TEST_BOOL(final[0] == final[1]);
    else
      XII_TEST_BOOL(changed > 8);
    if (phase == 3)
      XII_TEST_BOOL(halo == 0);
    if (phase == 0)
    {
      initial[0]  = final[0];
      initial[1]  = final[1];
      initialHalo = halo;
      XII_TEST_BOOL(initialHalo > 8);
      for (xiiUInt32 view = 0; view < 2; ++view)
      {
        xiiImage capture;
        capture.ResetAndAlloc(targets[view]->GetDescription());
        xiiMemoryUtils::Copy(capture.GetPixelPointer<xiiUInt32>(), final[view].GetData(), final[view].GetCount());
        XII_TEST_RESULT(capture.SaveTo(xiiStringBuilder(":imgout/SceneBloom-", view ? "Enabled" : "Disabled", ".png")));
      }
    }
    if (phase == 2 || phase == 8 || phase == 11 || phase == 12)
      XII_TEST_BOOL(final[0] == initial[0] && final[1] == initial[1]);
    xiiTestFramework::Output(xiiTestOutput::Details, "Bloom phase %u: %u final pixels, %u halo pixels", phase, changed, halo);
  }
  resize(1, 1);
  auto*      clock         = xiiClock::GetGlobalClock();
  const auto previousStep  = clock->GetFixedTimeStep();
  const bool previousPause = clock->GetPaused();
  clock->SetPaused(false);
  clock->SetFixedTimeStep(xiiTime::MakeFromSeconds(0.1));
  XII_SCOPE_EXIT(clock->SetFixedTimeStep(previousStep); clock->SetPaused(previousPause));
  for (xiiUInt32 view = 0; view < 2; ++view)
  {
    xiiDisplayOutputSettings settings;
    settings.m_Exposure.m_Mode                   = view ? xiiExposureMode::Automatic : xiiExposureMode::Manual;
    settings.m_Exposure.m_fAdaptationSpeedBright = settings.m_Exposure.m_fAdaptationSpeedDark = 0;
    settings.m_fBloomStrength                                                                 = 0;
    XII_TEST_RESULT(views[view]->SetDisplayOutputSettings(settings));
  }
  float firstExposure = 0, previousExposure = 0;
  for (xiiUInt32 phase = 0; phase < 9; ++phase)
  {
    const float luminances[] = {512, 2, 2, 128, 128, 0.1f, 0.1f, 128, 0.1f};
    const float luminance    = luminances[phase];
    if (phase == 6)
    {
      auto settings                                = views[1]->GetDisplayOutputSettings();
      settings.m_Exposure.m_fAdaptationSpeedBright = 2;
      settings.m_Exposure.m_fAdaptationSpeedDark   = 4;
      XII_TEST_RESULT(views[1]->SetDisplayOutputSettings(settings));
    }
    xiiStringBuilder name;
    name.SetFormat("Display.Metered.{}", phase);
    emitter->SetMaterialOverride(0, surface(name, {luminance, luminance, luminance}));
    if (phase == 2 || phase == 5)
      views[1]->InvalidateTemporalHistory();
    if (phase == 3)
      xiiMakeReloadablePipelineTestResource(original, pipeline.GetResourceID());
    if (phase == 4)
    {
      resize(13, 9);
      camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 0.25f, 0.1f, 100);
    }
    clock->Update();
    world.Update();
    environment.GetDevice()->WaitIdle();
    xiiVec2 exposure[2];
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      if (!XII_TEST_RESULT(views[view]->GetLastRenderResult()) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer(view, "CurrentExposure").Borrow(), 0, exposure[view]))
        return;
      XII_TEST_BOOL(exposure[view].IsValid());
      if (!view)
        XII_TEST_FLOAT(exposure[view].x, camera.GetExposure(), 1e-7f);
      xiiUInt32  pixel = 0;
      const auto size  = targets[view]->GetDescription().m_Size;
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), targets[view].Borrow(), size.width / 2, size.height / 2, pixel))
        return;
      const float x       = luminance * exposure[view].x;
      const float linear  = xiiMath::Saturate(x * (2.51f * x + 0.03f) / (x * (2.43f * x + 0.59f) + 0.14f));
      const float encoded = linear < 0.0031308f ? linear * 12.92f : 1.055f * xiiMath::Pow(linear, 1.0f / 2.4f) - 0.055f;
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
        XII_TEST_FLOAT(float((pixel >> (channel * 8)) & 255) / 255, encoded, 0.005f);
    }
    XII_TEST_BOOL(buffer(0, "CurrentExposure") != buffer(1, "CurrentExposure"));
    if (phase == 0)
      firstExposure = exposure[1].x;
    if (phase == 1)
      XII_TEST_FLOAT(exposure[1].x, firstExposure, 1e-7f);
    else if (phase < 7)
      XII_TEST_FLOAT(exposure[1].x, 1.0f / (9.6f * luminance), 1.0f / (9.6f * luminance) * 0.05f);
    else
    {
      const float target = 1.0f / (9.6f * exposure[1].y);
      const float speed  = target > previousExposure ? 2.0f : 4.0f;
      const float blend  = 1 - xiiMath::Pow(2.0f, -speed * 0.1f);
      XII_TEST_FLOAT(exposure[1].x, xiiMath::Lerp(previousExposure, target, blend), 1e-5f);
      XII_TEST_BOOL(exposure[1].x != previousExposure);
    }
    previousExposure = exposure[1].x;
    XII_TEST_FLOAT(exposure[1].y, luminance, luminance * 0.05f);
    struct Histogram
    {
      XII_DECLARE_POD_TYPE();
      xiiUInt32 m_Bins[256];
    } histogram;
    if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer(1, "Histogram").Borrow(), 0, histogram))
      return;
    xiiUInt32 count = 0;
    for (const auto bin : histogram.m_Bins)
      count += bin;
    XII_TEST_INT(count, targets[1]->GetDescription().m_Size.width * targets[1]->GetDescription().m_Size.height);
    xiiTestFramework::Output(xiiTestOutput::Details, "Metering phase %u: exposure %.8f, luminance %.4f, %u samples", phase, exposure[1].x, exposure[1].y, count);
  }
  xiiResourceLock<xiiRenderPipelineResource> authored(source, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  auto                                       missingBloom = authored->GetAuthoring()->m_Descriptor;
  const auto                                 tone         = xiiFindPipelineTestPass(missingBloom, "xiiToneMappingPass");
  for (xiiUInt32 index = missingBloom.m_Connections.GetCount(); index-- > 0;)
    if (missingBloom.m_Passes[missingBloom.m_Connections[index].m_uiTarget].m_Id == tone && missingBloom.m_Connections[index].m_sInput == "BloomTexture")
      XII_TEST_RESULT(missingBloom.Disconnect(index));
  views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Display.MissingBloom", std::move(missingBloom)));
  auto settings             = views[0]->GetDisplayOutputSettings();
  settings.m_fBloomStrength = 1;
  XII_TEST_RESULT(views[0]->SetDisplayOutputSettings(settings));
  {
    xiiTestLogInterface   expected;
    xiiTestLogSystemScope expectedScope(&expected);
    expected.ExpectMessage("Active bloom requires a readable RGBA bloom image", xiiLogMsgType::ErrorMsg, 1);
    expected.ExpectMessage("Failed to execute render graph for view", xiiLogMsgType::ErrorMsg, 1);
    world.Update();
    XII_TEST_BOOL(views[0]->GetLastRenderResult().Failed());
    XII_TEST_RESULT(views[1]->GetLastRenderResult());
  }
  settings.m_fBloomStrength = 0;
  XII_TEST_RESULT(views[0]->SetDisplayOutputSettings(settings));
  world.Update();
  XII_TEST_RESULT(views[0]->GetLastRenderResult());
  auto       badHistogram = authored->GetAuthoring()->m_Descriptor;
  const auto adapt        = xiiFindPipelineTestPass(badHistogram, "xiiAutoExposurePass");
  xiiUInt32  adaptIndex   = xiiInvalidIndex;
  for (xiiUInt32 index = badHistogram.m_Connections.GetCount(); index-- > 0;)
    if (badHistogram.m_Passes[badHistogram.m_Connections[index].m_uiTarget].m_Id == adapt && badHistogram.m_Connections[index].m_sInput == "LuminanceHistogram")
    {
      adaptIndex = badHistogram.m_Connections[index].m_uiTarget;
      XII_TEST_RESULT(badHistogram.Disconnect(index));
    }
  const auto counter = badHistogram.AddPass("Malformed four-byte histogram", xiiGetStaticRTTI<xiiCounterResetPass>());
  XII_TEST_RESULT(badHistogram.Connect(counter, "Counter", adaptIndex, "LuminanceHistogram"));
  views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Display.BadHistogram", std::move(badHistogram)));
  {
    xiiTestLogInterface   expected;
    xiiTestLogSystemScope expectedScope(&expected);
    expected.ExpectMessage("Auto exposure requires a readable structured histogram of 256 uint counters", xiiLogMsgType::ErrorMsg, 1);
    expected.ExpectMessage("Failed to execute render graph for view", xiiLogMsgType::ErrorMsg, 1);
    world.Update();
    XII_TEST_BOOL(views[0]->GetLastRenderResult().Failed());
    XII_TEST_RESULT(views[1]->GetLastRenderResult());
  }
}
