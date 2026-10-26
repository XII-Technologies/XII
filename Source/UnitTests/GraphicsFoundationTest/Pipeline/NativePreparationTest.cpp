/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/LightingManager.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/BRDFLutPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

class xiiNativePreparationExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativePreparationExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::RG16Float};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::RG16Float, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto emitted = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
        data = builder.ReadTexture(context.GetInputTexture(m_Input), xiiGALResourceStateFlags::ShaderResource);
        builder.ExportTexture(data, xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return context.SetOutput(m_Output, *emitted.first);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativePreparationExport, 1, xiiRTTIDefaultAllocator<xiiNativePreparationExport>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Input", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Output", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST_GROUP(NativePreparation);
XII_CREATE_SIMPLE_TEST(NativePreparation, SharedImmutableBRDFLookup)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  XII_TEST_BOOL(!xiiLightingManager::GetBRDFLUT());
  xiiRenderPipelineResourceDescriptor leaf;
  leaf.AddPass("Integrate GGX", xiiGetStaticRTTI<xiiBRDFLutPass>());
  leaf.AddPass("Read lookup on graphics", xiiGetStaticRTTI<xiiNativePreparationExport>());
  XII_TEST_RESULT(leaf.Connect(0, "BRDFLut", 1, "Input"));
  const auto         exportId = leaf.m_Passes[1].m_Id;
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First", definition.m_Id);
  root.AddSubgraph("Second", definition.m_Id);
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Native BRDF lookup", std::move(root));
  xiiWorldDescription description("BRDF consumers");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView* views[2];
  for (auto& view : views)
  {
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("BRDF consumer", view);
    view->SetCamera(&camera);
    view->SetViewport({0, 0, 8, 8});
    view->SetRenderPipelineResource(resource);
  }
  xiiSharedPtr<xiiGALTexture> lookup;
  for (xiiUInt32 frame = 0; frame < 3; ++frame)
  {
    world.Update();
    environment.GetDevice()->WaitIdle();
    XII_TEST_BOOL(!xiiLightingManager::IsBRDFLUTGenerationPending());
    XII_TEST_BOOL(xiiLightingManager::GetBRDFLUTReadyFence()->GetCompletedValue() >= 1);
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    for (auto* view : views)
    {
      const auto& instance = view->GetRenderPipelineInstance();
      const auto* compiled = instance.GetPipeline();
      if (!XII_TEST_BOOL(compiled != nullptr))
        return;
      XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet() && compiled->GetExtractionRequirements().IsNoFlagSet());
      xiiUInt32 exports = 0;
      for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
      {
        if (compiled->GetProvenance()[i].m_Element != exportId)
          continue;
        const auto* binding = instance.GetOutputBinding(i, "Output");
        auto        texture = binding ? view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
        if (!XII_TEST_BOOL(texture != nullptr))
          return;
        if (!lookup)
          lookup = texture;
        XII_TEST_BOOL(texture == lookup);
        ++exports;
      }
      XII_TEST_INT(exports, 2);
    }
  }
  xiiGALTextureReadback readback(environment.GetDevice());
  auto                  commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  commands->Begin();
  const auto result = readback.Enqueue(commands, {.m_pTexture = lookup.Borrow()});
  commands->End();
  if (!XII_TEST_RESULT(result))
    return;
  auto* queue = environment.GetDevice()->GetCommandQueue();
  queue->WaitForFenceValue(queue->Submit(commands));
  readback.WaitForNextCompleted();
  const auto capture = readback.GetCompleted();
  if (!XII_TEST_BOOL(static_cast<bool>(capture)))
    return;
  XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
  commands->Begin();
  xiiGALMappedTextureSubresource mapped;
  if (XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
  {
    const auto* row = static_cast<const xiiFloat16*>(mapped.m_pData);
    // At near-zero roughness the split-sum integral reduces to Schlick's two terms.
    for (xiiUInt32 x : {32U, 128U, 224U})
    {
      const float ndotv   = (float(x) + 0.5f) / 256.0f;
      const float fresnel = xiiMath::Pow(1.0f - ndotv, 5.0f);
      XII_TEST_FLOAT(float(row[x * 2]), 1.0f - fresnel, 0.003f);
      XII_TEST_FLOAT(float(row[x * 2 + 1]), fresnel, 0.003f);
    }
    XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
  }
  commands->End();
}


XII_CREATE_SIMPLE_TEST(NativePreparation, RetainedBRDFRecording)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor descriptor;
  const auto                          generate = descriptor.AddPass("Retained GGX integration", xiiGetStaticRTTI<xiiBRDFLutPass>());
  const auto                          sink     = descriptor.AddPass("Read integrated GGX", xiiGetStaticRTTI<xiiNativePreparationExport>());
  XII_TEST_RESULT(descriptor.Connect(generate, "BRDFLut", sink, "Input"));
  descriptor.m_Interface.PushBack({"LUT", false, descriptor.m_Passes[sink].m_Id, "Output"});
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("BRDF.RetainedIntegration", std::move(descriptor));
  xiiRenderPipelineInstance instance;
  if (!XII_TEST_RESULT(instance.Synchronize(resource)))
    return;
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Retained GGX integration")))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = context.GetGraph();
  auto& cache = context.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
  graph.BeginSetup(frame.GetFrameIndex());
  if (!XII_TEST_RESULT(instance.Build(nullptr, graph, context.GetBlackboard())))
    return;
  const auto output = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("LUT", false))->m_hTexture;
  graph.EndSetup();
  instance.Clear();
  xiiStringBuilder error;
  if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
    return;
  struct Lookup
  {
    XII_DECLARE_POD_TYPE();
    xiiFloat16 scale, bias;
  };
  Lookup lookup;
  if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), graph.GetExportedTexture(output).Borrow(), 128, 0, lookup))
    return;
  const float fresnel = xiiMath::Pow(1.0f - 128.5f / 256, 5.0f);
  XII_TEST_FLOAT(float(lookup.scale), 1.0f - fresnel, 0.003f);
  XII_TEST_FLOAT(float(lookup.bias), fresnel, 0.003f);
}

XII_CREATE_SIMPLE_TEST(NativePreparation, GatedBRDFAndUnsupportedStorage)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  XII_TEST_BOOL(!xiiLightingManager::GetBRDFLUT());
  for (xiiUInt32 inactive = 0; inactive < 2; ++inactive)
  {
    xiiRenderPipelineResourceDescriptor descriptor;
    xiiVariantDictionary                properties;
    properties.Insert("RequireActivation", inactive != 0);
    const auto generate = descriptor.AddPass("Conditional GGX", xiiGetStaticRTTI<xiiBRDFLutPass>(), properties);
    descriptor.m_Interface.PushBack({"LUT", false, descriptor.m_Passes[generate].m_Id, "BRDFLut"});
    const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(inactive ? "BRDF.Inactive" : "BRDF.UnsupportedStorage", std::move(descriptor));
    xiiRenderPipelineInstance instance;
    if (!XII_TEST_RESULT(instance.Synchronize(resource)))
      return;
    xiiRenderGraphTestContext context;
    if (!XII_TEST_RESULT(context.Initialize("BRDF device restriction")))
      return;
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = context.GetGraph();
    graph.BeginSetup(frame.GetFrameIndex());
    const auto result = instance.Build(nullptr, graph, context.GetBlackboard());
    if (inactive)
    {
      XII_TEST_RESULT(result);
      XII_TEST_BOOL(!instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("LUT", false))->m_hTexture.IsValid());
    }
    else
    {
      XII_TEST_BOOL(result.Failed());
      XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("TextureUAVExtendedFormats") != nullptr);
    }
    graph.EndSetup();
    frame.End();
    XII_TEST_BOOL(!xiiLightingManager::GetBRDFLUT());
  }
}
