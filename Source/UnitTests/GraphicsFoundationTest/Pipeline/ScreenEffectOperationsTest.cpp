/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/ScreenSpaceGlobalIlluminationPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/SensorOutputPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

// Synthetic shader inputs, independent of world geometry, lights or their residency services.
class xiiScreenEffectTestSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenEffectTestSource, xiiRenderPipelinePass);

public:
  void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Color", xiiRenderPipelinePin::Texture, false, false, xiiGALResourceFormat::RGBA16Float});
    pins.PushBack({"Attributes", xiiRenderPipelinePin::Texture, false, false, xiiGALResourceFormat::RGBA16Float});
    pins.PushBack({"Depth", xiiRenderPipelinePin::Texture, false, false, xiiGALResourceFormat::R32Float});
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphTextureHandle m_Color, m_Attributes, m_Depth;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription description;
      description.m_Type = xiiGALResourceDimension::Texture2D;
      description.m_Size = {8, 8};
      description.m_Format = xiiGALResourceFormat::RGBA16Float;
      description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      data.m_Color = builder.WriteTexture(xiiStringBuilder(context.m_sPassName, "/Color"), description, xiiGALResourceStateFlags::CopyDestination);
      data.m_Attributes = builder.WriteTexture(xiiStringBuilder(context.m_sPassName, "/Attributes"), description, xiiGALResourceStateFlags::CopyDestination);
      description.m_Format = xiiGALResourceFormat::R32Float;
      data.m_Depth = builder.WriteTexture(xiiStringBuilder(context.m_sPassName, "/Depth"), description, xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      execute.GetCommandList().ClearRenderTargetView(execute.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(4, 4, 4, 1));
      execute.GetCommandList().ClearRenderTargetView(execute.GetTexture(data.m_Attributes)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black);
      execute.GetCommandList().ClearRenderTargetView(execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black); });
    XII_SUCCEED_OR_RETURN(context.SetOutput("Color", emitted.first->m_Color));
    XII_SUCCEED_OR_RETURN(context.SetOutput("Attributes", emitted.first->m_Attributes));
    return context.SetOutput("Depth", emitted.first->m_Depth);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenEffectTestSource, 1, xiiRTTIDefaultAllocator<xiiScreenEffectTestSource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(PipelineResources, ReusableScreenEffectsWithoutSceneServices)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiRenderPipelineResourceDescriptor  leaf;
  const auto                           source = leaf.AddPass("Synthetic inputs", "xiiScreenEffectTestSource");
  const auto                           camera = leaf.AddPass("Camera", "xiiViewConstantsPass");
  xiiScreenSpaceGlobalIlluminationPass gi;
  gi.m_fIntensity = 0.25f;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(gi.GetProperties(properties));
  const auto          trace      = leaf.AddPass("Diffuse screen rays", "xiiScreenSpaceGlobalIlluminationPass", properties);
  const auto          composite  = leaf.AddPass("Add diffuse rays", "xiiScreenSpaceGlobalIlluminationCompositePass");
  const auto          snapshot   = leaf.AddPass("Snapshot radiance", "xiiScreenSpaceRefractionSnapshotPass");
  const auto          refraction = leaf.AddPass("Refract", "xiiScreenSpaceRefractionPass");
  xiiSensorOutputPass sensor;
  sensor.m_Profile.m_Type          = xiiSensorType::DepthCamera;
  sensor.m_Profile.m_NoiseModel    = xiiSensorNoiseModel::None;
  sensor.m_Profile.m_uiResolutionX = sensor.m_Profile.m_uiResolutionY = 8;
  XII_TEST_RESULT(sensor.GetProperties(properties));
  const auto output = leaf.AddPass("Calibrated output", "xiiSensorOutputPass", properties);
  for (const auto operation : {trace, refraction, output})
  {
    XII_TEST_RESULT(leaf.Connect(source, "Depth", operation, "SceneDepth"));
    XII_TEST_RESULT(leaf.Connect(source, "Attributes", operation, "GBufferNormal"));
    XII_TEST_RESULT(leaf.Connect(camera, "PerFrameCamera", operation, "PerFrameCamera"));
  }
  for (const auto operation : {trace, output})
  {
    XII_TEST_RESULT(leaf.Connect(source, "Attributes", operation, "GBufferAlbedo"));
    XII_TEST_RESULT(leaf.Connect(camera, "PerFrameGlobal", operation, "PerFrameGlobal"));
  }
  XII_TEST_RESULT(leaf.Connect(source, "Attributes", trace, "GBufferMaterial"));
  XII_TEST_RESULT(leaf.Connect(source, "Color", trace, "HDRSceneColor"));
  XII_TEST_RESULT(leaf.Connect(trace, "SSGITerm", composite, "SSGITerm"));
  XII_TEST_RESULT(leaf.Connect(source, "Color", composite, "HDRSceneColor"));
  XII_TEST_RESULT(leaf.Connect(composite, "HDRSceneColor", snapshot, "HDRSceneColor"));
  XII_TEST_RESULT(leaf.Connect(composite, "HDRSceneColor", refraction, "HDRSceneColor"));
  XII_TEST_RESULT(leaf.Connect(snapshot, "RefractionSceneColorInput", refraction, "RefractionSceneColorInput"));
  XII_TEST_RESULT(leaf.Connect(source, "Attributes", refraction, "GBufferMaterial"));
  XII_TEST_RESULT(leaf.Connect(source, "Attributes", refraction, "GBufferEmissive"));
  XII_TEST_RESULT(leaf.Connect(refraction, "HDRSceneColor", output, "HDRSceneColor"));
  XII_TEST_RESULT(leaf.Connect(source, "Attributes", output, "DilatedVelocityBuffer"));
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First sensor", definition.m_Id);
  root.AddSubgraph("Wide sensor", definition.m_Id);
  const auto wide     = root.m_Passes[1].m_Id;
  auto&      override = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  override.m_Path.PushBack(leaf.m_Passes[output].m_Id);
  sensor.m_Profile.m_uiResolutionX = 12;
  XII_TEST_RESULT(sensor.GetProperties(override.m_Properties));
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
      if (!xiiDynamicCast<const xiiSensorOutputPass*>(instance.GetPass(index)))
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
