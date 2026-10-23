/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>

#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Exposure/AutoExposurePass.h>
#include <GraphicsCore/Pipeline/Passes/Exposure/LuminanceHistogramPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/BloomPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/UpscalePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>

class xiiOutputOperationSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOutputOperationSource, xiiRenderPipelinePass);

public:
  void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Color", xiiRenderPipelinePin::Texture, false, false, xiiGALResourceFormat::RGBA16Float});
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphTextureHandle m_Color;
    };
    auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription description;
      description.m_Type = xiiGALResourceDimension::Texture2D;
      description.m_Size = {8, 8};
      description.m_Format = xiiGALResourceFormat::RGBA16Float;
      description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      data.m_Color = builder.WriteTexture("Operation source", description, xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execution) { execution.GetCommandList().ClearRenderTargetView(execution.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(4, 4, 4, 1)); });
    return context.SetOutput("Color", emitted.first->m_Color);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOutputOperationSource, 1, xiiRTTIDefaultAllocator<xiiOutputOperationSource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiOutputOperationExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOutputOperationExport, xiiRenderPipelinePass);

public:
  xiiRenderGraphTextureHandle m_Outputs[2];
  void                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Left", xiiRenderPipelinePin::Texture, true});
    pins.PushBack({"Right", xiiRenderPipelinePin::Texture, true});
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphTextureHandle m_Inputs[2];
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Inputs[0] = builder.ReadTexture(context.GetInputTexture("Left"), xiiGALResourceStateFlags::CopySource);
      data.m_Inputs[1] = builder.ReadTexture(context.GetInputTexture("Right"), xiiGALResourceStateFlags::CopySource);
      for (const auto input : data.m_Inputs) builder.ExportTexture(input, xiiGALResourceStateFlags::CopySource); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    m_Outputs[0]       = emitted.first->m_Inputs[0];
    m_Outputs[1]       = emitted.first->m_Inputs[1];
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOutputOperationExport, 1, xiiRTTIDefaultAllocator<xiiOutputOperationExport>)
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiExposureOperationReadback : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExposureOperationReadback, xiiRenderPipelinePass);

public:
  xiiRenderGraphBufferHandle m_Exposure, m_Readback;
  void                       GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override { pins.PushBack({"Exposure", xiiRenderPipelinePin::Buffer, true}); }
  xiiResult                  Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphBufferHandle m_Source, m_Target;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
        data.m_Source = builder.ReadBuffer(context.GetInputBuffer("Exposure"), xiiGALResourceStateFlags::CopySource);
        xiiGALBufferCreationDescription description;
        description.m_uiSize = sizeof(float) * 2;
        description.m_Usage = xiiGALResourceUsage::Staging;
        description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
        data.m_Target = builder.WriteBuffer("Exposure readback", description, xiiGALResourceStateFlags::CopyDestination);
        builder.ExportBuffer(data.m_Source, xiiGALResourceStateFlags::CopySource);
        builder.ExportBuffer(data.m_Target, xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execution) { execution.GetCommandList().CopyBuffer(execution.GetBuffer(data.m_Source), execution.GetBuffer(data.m_Target)); }, true);
    m_Exposure         = emitted.first->m_Source;
    m_Readback         = emitted.first->m_Target;
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExposureOperationReadback, 1, xiiRTTIDefaultAllocator<xiiExposureOperationReadback>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(PipelineResources, ReusableExposureOperations)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor leaf;
  leaf.AddPass("HDR", xiiGetStaticRTTI<xiiOutputOperationSource>());
  leaf.AddPass("Meter", xiiGetStaticRTTI<xiiLuminanceHistogramPass>());
  xiiAutoExposurePass authored;
  authored.m_Settings.m_Mode = xiiExposureMode::Manual;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(authored.GetProperties(properties));
  leaf.AddPass("Adapt", xiiGetStaticRTTI<xiiAutoExposurePass>(), properties);
  leaf.AddPass("Read exposure", xiiGetStaticRTTI<xiiExposureOperationReadback>());
  XII_TEST_RESULT(leaf.Connect(0, "Color", 1, "HDRSceneColor"));
  XII_TEST_RESULT(leaf.Connect(1, "LuminanceHistogram", 2, "LuminanceHistogram"));
  XII_TEST_RESULT(leaf.Connect(2, "CurrentExposure", 3, "Exposure"));
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("Neutral", definition.m_Id);
  root.AddSubgraph("Brighter", definition.m_Id);
  const xiiUuid brighter = root.m_Passes[1].m_Id;
  auto&         override = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  override.m_Path.PushBack(leaf.m_Passes[2].m_Id);
  authored.m_Settings.m_fExposureCompensation = 2.0f;
  XII_TEST_RESULT(authored.GetProperties(override.m_Properties));
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Reusable exposure", std::move(root));
  xiiWorldDescription worldDescription("Exposure occurrences");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView*   view   = nullptr;
  const auto handle = module->CreateView("Exposure only", view);
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 8, 8});
  view->SetRenderPipelineResource(resource);
  xiiSharedPtr<xiiGALBuffer> histories[2];
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
      const auto* readback = xiiDynamicCast<const xiiExposureOperationReadback*>(instance.GetPass(index));
      if (!readback)
        continue;
      ++inspected;
      const auto branch   = compiled->GetProvenance()[index].m_InstancePath[0] == brighter ? 1U : 0U;
      auto       exposure = view->GetRenderGraph()->GetExportedBuffer(readback->m_Exposure);
      if (frame == 0)
        histories[branch] = exposure;
      XII_TEST_BOOL(exposure && exposure == histories[branch]);
      auto staging = view->GetRenderGraph()->GetExportedBuffer(readback->m_Readback);
      if (!XII_TEST_BOOL(staging != nullptr))
        return;
      auto commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      commands->Begin();
      void* mapped = nullptr;
      if (XII_TEST_RESULT(commands->MapBuffer(staging.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, mapped)))
      {
        XII_TEST_FLOAT(static_cast<const float*>(mapped)[0], branch ? 4.0f : 1.0f, 0.001f);
        XII_TEST_RESULT(commands->UnmapBuffer(staging.Borrow(), xiiGALMapType::Read));
      }
      commands->End();
    }
    XII_TEST_INT(inspected, 2);
    XII_TEST_BOOL(histories[0] && histories[1] && histories[0] != histories[1]);
  }
  module->DestroyView(handle);
}

XII_CREATE_SIMPLE_TEST(PipelineResources, IndependentOutputOperations)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown(););
  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("HDR source", xiiGetStaticRTTI<xiiOutputOperationSource>());
  for (xiiUInt32 branch = 0; branch < 2; ++branch)
  {
    xiiVariantDictionary upscale, bloom;
    xiiUpscalePass       upscalePass;
    upscalePass.m_OutputUpscaledColor.SetResolution({xiiRenderPipelineTextureResolutionMode::Fixed, branch ? 16U : 8U, branch ? 16U : 8U});
    XII_TEST_RESULT(upscalePass.GetProperties(upscale));
    const auto   scale = descriptor.AddPass(branch ? "Large upscale" : "Small upscale", xiiGetStaticRTTI<xiiUpscalePass>(), upscale);
    xiiBloomPass bloomPass;
    if (!branch)
    {
      xiiRenderPipelinePinConfiguration* sizing = XII_DEFAULT_NEW(xiiRenderPipelinePinConfiguration);
      sizing->m_Id                              = xiiUuid::MakeUuid();
      sizing->m_Metadata.m_sName                = "BloomTexture";
      sizing->m_Metadata.m_bInput               = false;
      sizing->m_Resolution.m_Mode               = xiiRenderPipelineTextureResolutionMode::Input;
      sizing->m_Resolution.m_sInput             = "UpscaledColor";
      sizing->m_Resolution.m_fScale             = 0.5f;
      bloomPass.m_PinConfigurations.PushBack(sizing);
    }
    XII_TEST_RESULT(bloomPass.GetProperties(bloom));
    bloom.Insert("UseViewDefaults", false);
    bloom.Insert("Threshold", branch ? 100.0f : 0.0f);
    const auto effect = descriptor.AddPass(branch ? "High threshold" : "Low threshold", xiiGetStaticRTTI<xiiBloomPass>(), bloom);
    XII_TEST_RESULT(descriptor.Connect(0, "Color", scale, "TAAResolved"));
    XII_TEST_RESULT(descriptor.Connect(scale, "UpscaledColor", effect, "UpscaledColor"));
  }
  const auto sink = descriptor.AddPass("Read branches", xiiGetStaticRTTI<xiiOutputOperationExport>());
  XII_TEST_RESULT(descriptor.Connect(2, "BloomTexture", sink, "Left"));
  XII_TEST_RESULT(descriptor.Connect(4, "BloomTexture", sink, "Right"));
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Independent output algorithms", std::move(descriptor));
  xiiWorldDescription worldDescription("Output operations");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView*   view       = nullptr;
  const auto viewHandle = module->CreateView("Output branches", view);
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 16, 16});
  view->SetRenderPipelineResource(resource);
  for (xiiUInt32 frame = 0; frame < 2; ++frame)
  {
    world.Update();
    environment.GetDevice()->WaitIdle();
    const auto& instance = view->GetRenderPipelineInstance();
    const auto* exported = xiiDynamicCast<const xiiOutputOperationExport*>(instance.GetPass(sink));
    if (!XII_TEST_BOOL(exported != nullptr))
      return;
    auto* graph = view->GetRenderGraph();
    auto  left  = graph->GetExportedTexture(exported->m_Outputs[0]);
    auto  right = graph->GetExportedTexture(exported->m_Outputs[1]);
    if (!XII_TEST_BOOL(left && right && left != right))
      return;
    XII_TEST_BOOL(instance.GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
    XII_TEST_BOOL(instance.GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    for (xiiUInt32 branch = 0; branch < 2; ++branch)
    {
      const auto texture = branch ? right : left;
      // Bloom independently chooses half-input or full-input resolution.
      XII_TEST_INT(texture->GetDescription().m_Size.width, branch ? 16 : 4);
      xiiGALTextureReadback readback(environment.GetDevice());
      const auto            commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      commands->Begin();
      XII_TEST_RESULT(readback.Enqueue(commands.Borrow(), {.m_pTexture = texture.Borrow()}));
      commands->End();
      const auto queue = environment.GetDevice()->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      queue->WaitForFenceValue(queue->Submit(commands.Borrow()));
      readback.WaitForNextCompleted();
      const auto capture = readback.GetCompleted();
      if (!XII_TEST_BOOL(static_cast<bool>(capture)))
        return;
      commands->Begin();
      xiiGALMappedTextureSubresource mapped;
      xiiGALTextureMipLevelData      mip;
      if (XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, mip, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
      {
        const auto* pixels = static_cast<const xiiFloat16*>(mapped.m_pData);
        XII_TEST_FLOAT(static_cast<float>(pixels[0]), branch ? 0.0f : 4.0f, 0.02f);
        XII_TEST_FLOAT(static_cast<float>(pixels[3]), 1.0f, 0.001f);
        XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, mip));
      }
      commands->End();
      readback.RecycleStagingTexture(capture.m_pStagingTexture);
    }
  }
  module->DestroyView(viewHandle);
}
