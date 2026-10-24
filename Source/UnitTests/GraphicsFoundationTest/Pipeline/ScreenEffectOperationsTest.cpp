/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>

#include <Core/ResourceManager/ResourceManager.h>

#include <Core/World/World.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Composition/RadianceAddPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/DepthSensorPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/RgbSensorPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>

// Synthetic shader inputs, independent of world geometry, lights or their residency services.


XII_CREATE_SIMPLE_TEST(PipelineResources, ReusableScreenEffectsWithoutSceneServices)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiRenderPipelineResourceDescriptor        leaf;
  const auto                                 source = leaf.AddPass("Synthetic inputs", xiiGetStaticRTTI<xiiScreenEffectTestSource>());
  const auto                                 camera = leaf.AddPass("Camera", xiiGetStaticRTTI<xiiViewConstantsPass>());
  xiiVariantDictionary                       properties;
  const auto                                 diffuseAsset = xiiCreateScreenDiffuseLightingTestResource(), refractionAsset = xiiCreateScreenRefractionTestResource();
  xiiResourceLock<xiiRenderPipelineResource> diffuse(diffuseAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), refractionDefinition(refractionAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(diffuse.IsValid() && refractionDefinition.IsValid() && diffuse->GetAuthoring() && refractionDefinition->GetAuthoring()))
    return;
  leaf.m_References.PushBack({diffuse->GetAuthoring()->m_Descriptor.m_Id, diffuseAsset.GetResourceID()});
  leaf.m_References.PushBack({refractionDefinition->GetAuthoring()->m_Descriptor.m_Id, refractionAsset.GetResourceID()});
  const auto trace         = leaf.AddSubgraph("Authored screen diffuse", diffuse->GetAuthoring()->m_Descriptor.m_Id);
  const auto response      = xiiFindPipelineTestPass(diffuse->GetAuthoring()->m_Descriptor, "xiiDiffuseSurfaceLightingPass");
  auto&      responseScale = leaf.m_Passes[trace].m_Overrides.ExpandAndGetRef();
  responseScale.m_Path.PushBack(response);
  responseScale.m_Properties.Insert("Scale", 0.25f);
  const auto         composite  = leaf.AddPass("Add diffuse radiance", xiiGetStaticRTTI<xiiRadianceAddPass>());
  const auto         refraction = leaf.AddSubgraph("Authored screen refraction", refractionDefinition->GetAuthoring()->m_Descriptor.m_Id);
  xiiDepthSensorPass sensor;
  sensor.m_Calibration.m_NoiseModel    = xiiSensorNoiseModel::None;
  sensor.m_Calibration.m_uiResolutionX = sensor.m_Calibration.m_uiResolutionY = 8;
  XII_TEST_RESULT(sensor.GetProperties(properties));
  const auto output = leaf.AddPass("Calibrated output", xiiGetStaticRTTI<xiiDepthSensorPass>(), properties);
  for (const auto operation : {trace, refraction})
  {
    XII_TEST_RESULT(leaf.Connect(source, "Depth", operation, "Depth"));
    XII_TEST_RESULT(leaf.Connect(source, "Attributes", operation, "Normal"));
    XII_TEST_RESULT(leaf.Connect(source, "Attributes", operation, "Material"));
    XII_TEST_RESULT(leaf.Connect(source, "Attributes", operation, "Emissive"));
    XII_TEST_RESULT(leaf.Connect(camera, "PerFrameCamera", operation, "Camera"));
  }
  XII_TEST_RESULT(leaf.Connect(source, "Attributes", trace, "Albedo"));
  XII_TEST_RESULT(leaf.Connect(source, "Color", trace, "Radiance"));
  XII_TEST_RESULT(leaf.Connect(source, "Color", composite, "A"));
  XII_TEST_RESULT(leaf.Connect(trace, "Lighting", composite, "B"));
  XII_TEST_RESULT(leaf.Connect(composite, "Radiance", refraction, "Radiance"));
  XII_TEST_RESULT(leaf.Connect(composite, "Radiance", refraction, "BackgroundRadiance"));
  XII_TEST_RESULT(leaf.Connect(source, "Depth", output, "SceneDepth"));
  XII_TEST_RESULT(leaf.Connect(camera, "PerFrameCamera", output, "PerFrameCamera"));
  // RGB legitimately consumes scene radiance; depth does not. Keep the screen
  // operations live through the appropriate independent consumer.
  xiiRgbSensorPass rgb;
  rgb.m_Calibration.m_uiResolutionX = rgb.m_Calibration.m_uiResolutionY = 8;
  rgb.m_Calibration.m_NoiseModel                                        = xiiSensorNoiseModel::None;
  XII_TEST_RESULT(rgb.GetProperties(properties));
  const auto colorOutput = leaf.AddPass("Calibrated RGB output", xiiGetStaticRTTI<xiiRgbSensorPass>(), properties);
  XII_TEST_RESULT(leaf.Connect(source, "Depth", colorOutput, "SceneDepth"));
  XII_TEST_RESULT(leaf.Connect(camera, "PerFrameCamera", colorOutput, "PerFrameCamera"));
  XII_TEST_RESULT(leaf.Connect(refraction, "Radiance", colorOutput, "Radiance"));
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.m_References = leaf.m_References;
  root.AddSubgraph("First sensor", definition.m_Id);
  root.AddSubgraph("Wide sensor", definition.m_Id);
  const auto wide     = root.m_Passes[1].m_Id;
  auto&      override = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  override.m_Path.PushBack(leaf.m_Passes[output].m_Id);
  sensor.m_Calibration.m_uiResolutionX = 12;
  XII_TEST_RESULT(sensor.GetProperties(override.m_Properties));
  auto& disabledGI = root.m_Passes[0].m_Overrides.ExpandAndGetRef();
  disabledGI.m_Path.PushBack(leaf.m_Passes[trace].m_Id);
  disabledGI.m_Path.PushBack(response);
  disabledGI.m_Properties.Insert("Scale", 0.0f);
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Independent screen operations", std::move(root));
  xiiWorldDescription worldDescription("Screen operations only");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera viewCamera;
  viewCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView*   view       = nullptr;
  const auto viewHandle = module->CreateView("No geometry or lighting extraction", view);
  view->SetCamera(&viewCamera);
  view->SetViewport({0, 0, 8, 8});
  view->SetRenderPipelineResource(resource);
  xiiSharedPtr<xiiGALTexture> previous[2];
  for (xiiUInt32 frame = 0; frame < 3; ++frame)
  {
    if (frame == 2)
      view->InvalidateTemporalHistory();
    world.Update();
    environment.GetDevice()->WaitIdle();
    const auto& instance = view->GetRenderPipelineInstance();
    const auto* compiled = instance.GetPipeline();
    if (!XII_TEST_BOOL(compiled != nullptr))
      return;
    XII_TEST_BOOL(compiled->GetExtractionRequirements().IsNoFlagSet());
    XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet());
    xiiUInt32 inspected = 0;
    for (xiiUInt32 index = 0; index < compiled->GetDescriptor().m_Passes.GetCount(); ++index)
    {
      if (!xiiDynamicCast<const xiiDepthSensorPass*>(instance.GetPass(index)))
        continue;
      const auto  branch  = compiled->GetProvenance()[index].m_InstancePath[0] == wide ? 1U : 0U;
      const auto* binding = instance.GetOutputBinding(index, "SensorOutput");
      if (!XII_TEST_BOOL(binding != nullptr))
        return;
      auto texture = view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture);
      if (!XII_TEST_BOOL(texture != nullptr))
        return;
      ++inspected;
      XII_TEST_INT(texture->GetDescription().m_Size.width, branch ? 12 : 8);
      if (frame)
        XII_TEST_BOOL(texture == previous[branch]);
      previous[branch] = texture;
      xiiGALTextureReadback readback(environment.GetDevice());
      auto                  commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      commands->Begin();
      const auto enqueued = readback.Enqueue(commands, {.m_pTexture = texture});
      commands->End();
      if (!XII_TEST_RESULT(enqueued))
        return;
      auto* queue = environment.GetDevice()->GetCommandQueue();
      queue->WaitForFenceValue(queue->Submit(commands));
      readback.WaitForNextCompleted();
      const auto capture = readback.GetCompleted();
      if (!XII_TEST_BOOL(static_cast<bool>(capture)))
        return;
      commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      commands->Begin();
      xiiGALMappedTextureSubresource mapped;
      if (XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
      {
        for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
        {
          const auto* row = xiiMemoryUtils::AddByteOffset(static_cast<const float*>(mapped.m_pData), y * mapped.m_uiStride);
          for (xiiUInt32 x = 0; x < capture.m_uiWidth * 4; ++x)
            XII_TEST_FLOAT(row[x], 0.0f, 0.00001f);
        }
        XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
      }
      commands->End();
      readback.RecycleStagingTexture(capture.m_pStagingTexture);
    }
    XII_TEST_INT(inspected, 2);
    XII_TEST_BOOL(previous[0] != previous[1]);
  }
  module->DestroyView(viewHandle);
}
