/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <GraphicsCore/Pipeline/Passes/Lighting/DirectionalSurfaceLightingPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/EnvironmentLightConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/PresentPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Output/BloomPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <Texture/Image/Image.h>

namespace
{
  constexpr xiiStringView sBloomShader = "Shaders/Pipeline/BloomChain.xiiShader";

  class MissingShaderLoader : public xiiResourceTypeLoader
  {
  public:
    xiiResourceLoadData OpenDataStream(const xiiResource*) override { return {}; }
    void                CloseDataStream(const xiiResource*, const xiiResourceLoadData&) override {}
  };

  xiiShaderResourceHandle CreateMissingShader(xiiStringView shaderPath)
  {
    // Exercise real missing-content state using the normal resource loader API,
    // without deleting shader sources or invalidating shared shader-cache files.
    xiiTestLogInterface   log;
    xiiTestLogSystemScope logScope(&log, true);
    log.ExpectMessage("Missing Resource of Type 'xiiShaderResource'", xiiLogMsgType::SeriousWarningMsg);
    const auto                          shader = xiiResourceManager::LoadResource<xiiShaderResource>(shaderPath);
    xiiUniquePtr<xiiResourceTypeLoader> loader = XII_DEFAULT_NEW(MissingShaderLoader);
    xiiResourceManager::UpdateResourceWithCustomLoader(shader, std::move(loader));
    xiiResourceLock<xiiShaderResource> loaded(shader, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    XII_TEST_BOOL(!loaded.IsValid());
    return shader;
  }

  xiiRenderGraphTextureHandle AddBloomInput(xiiRenderGraph& graph)
  {
    struct Input
    {
      xiiRenderGraphTextureHandle m_Color;
    };
    const auto emitted = graph.AddPass<Input>("Bloom input", xiiGALCommandQueueFlags::Compute, [](Input& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription description;
      description.m_Type = xiiGALResourceDimension::Texture2D;
      description.m_Size = {16, 16};
      description.m_Format = xiiGALResourceFormat::RGBA16Float;
      description.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
      data.m_Color = builder.WriteTexture(xiiRGBlackboardKeys::k_UpscaledColor, description, xiiGALResourceStateFlags::UnorderedAccess); }, [](const Input&, xiiRenderGraphPassContext&) {});
    return emitted.first->m_Color;
  }

  xiiResult RestoreShader(const xiiShaderResourceHandle& shader)
  {
    xiiUniquePtr<xiiResourceTypeLoader> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromFile);
    xiiResourceManager::UpdateResourceWithCustomLoader(shader, std::move(loader));
    xiiResourceLock<xiiShaderResource> loaded(shader, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    return loaded.IsValid() && loaded->IsShaderValid() ? XII_SUCCESS : XII_FAILURE;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(RenderOperationFailure);

XII_CREATE_SIMPLE_TEST(RenderOperationFailure, PassFailureAndRecovery)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiWorldDescription description("Shader setup failure");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Shader setup failure", view);
  view->SetViewport({0, 0, 16, 16});
  xiiBloomPass         pass;
  xiiVariantDictionary properties;
  if (!XII_TEST_RESULT(pass.Configure(properties)))
    return;

  const auto                                           shader = CreateMissingShader(sBloomShader);
  const xiiHashTable<xiiHashedString, xiiHashedString> permutations;
  for (bool allowFallback : {false, true})
  {
    XII_TEST_BOOL(!xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, permutations, allowFallback).IsValid());
  }
  xiiRenderGraph                        graph("Shader setup failure");
  xiiRenderGraphBlackboard              blackboard;
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  pass.GetPins(pins);
  XII_TEST_RESULT(pass.InitializePinBindings(pins));
  xiiDynamicArray<xiiRenderPipelineBinding> bindings;
  bindings.SetCount(pins.GetCount());
  xiiRenderPipelinePassContext context{view, graph, blackboard, properties, pins, bindings, "Shader setup failure"};
  graph.BeginSetup(1);
  bindings[context.FindPin("UpscaledColor", true)].m_hTexture = AddBloomInput(graph);
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope logScope(&log, true);
    log.ExpectMessage("Failed to create reflected compute operation pipeline", xiiLogMsgType::ErrorMsg);
    XII_TEST_BOOL(pass.Build(context).Failed());
  }
  graph.EndSetup();

  if (!XII_TEST_RESULT(RestoreShader(shader)))
    return;
  graph.BeginSetup(2);
  bindings[context.FindPin("UpscaledColor", true)].m_hTexture = AddBloomInput(graph);
  XII_TEST_RESULT(pass.Build(context));
  graph.EndSetup();
  XII_TEST_BOOL(bindings[context.FindPin("BloomTexture", false)].m_hTexture.IsValid());
}

static void TestViewShaderFailure(xiiStringView shaderPath)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiWorldDescription description("Shader failure view");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Shader failure view", view);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 16, 16});
  xiiRenderPipelineResourceDescriptor definition;
  const auto                          clear = [&](xiiStringView name, xiiGALResourceFormat::Enum format, xiiColor color) {
    xiiClearTexturePass pass;
    pass.m_OutputTexture.SetResolution({xiiRenderPipelineTextureResolutionMode::Fixed, 16, 16});
    pass.m_OutputTexture.SetFormat(format);
    pass.m_Color = color;
    xiiVariantDictionary properties;
    XII_TEST_RESULT(pass.GetProperties(properties));
    return definition.AddPass(name, xiiGetStaticRTTI<xiiClearTexturePass>(), properties);
  };
  xiiUInt32 operation = 0;
  xiiString outputPin;
  if (shaderPath == sBloomShader)
  {
    const auto source = clear("Authored HDR", xiiGALResourceFormat::RGBA16Float, {2, 1, 0.5f, 1});
    operation         = definition.AddPass("Bloom operation", xiiGetStaticRTTI<xiiBloomPass>());
    XII_TEST_RESULT(definition.Connect(source, "Texture", operation, "UpscaledColor"));
    outputPin = "BloomTexture";
  }
  else
  {
    operation                       = definition.AddPass("Directional surface operation", xiiGetStaticRTTI<xiiDirectionalSurfaceLightingPass>());
    const auto depth                = clear("Depth", xiiGALResourceFormat::R32Float, {0.5f, 0, 0, 0});
    const auto normal               = clear("Normal", xiiGALResourceFormat::RG16Float, {0.5f, 0.5f, 0, 0});
    const auto albedo               = clear("Albedo", xiiGALResourceFormat::RGBA16Float, {0.5f, 0.5f, 0.5f, 1});
    const auto material             = clear("Material", xiiGALResourceFormat::RGBA16Float, {0.5f, 0, 0.5f, 0});
    const auto emissive             = clear("Emissive", xiiGALResourceFormat::RGBA16Float, xiiColor::MakeZero());
    const auto cameraConstants      = definition.AddPass("Camera constants", xiiGetStaticRTTI<xiiViewConstantsPass>());
    const auto environmentConstants = definition.AddPass("Environment constants", xiiGetStaticRTTI<xiiEnvironmentLightConstantsPass>());
    definition.m_Passes[environmentConstants].m_Properties.Insert("Illuminance", 100.0f);
    XII_TEST_RESULT(definition.Connect(depth, "Texture", operation, "Depth"));
    XII_TEST_RESULT(definition.Connect(normal, "Texture", operation, "Normal"));
    XII_TEST_RESULT(definition.Connect(albedo, "Texture", operation, "Albedo"));
    XII_TEST_RESULT(definition.Connect(material, "Texture", operation, "Material"));
    XII_TEST_RESULT(definition.Connect(emissive, "Texture", operation, "Emissive"));
    XII_TEST_RESULT(definition.Connect(cameraConstants, "PerFrameCamera", operation, "Camera"));
    XII_TEST_RESULT(definition.Connect(environmentConstants, "EnvironmentLight", operation, "EnvironmentLight"));
    outputPin = "Lighting";
  }
  const auto present = definition.AddPass("Present result", xiiGetStaticRTTI<xiiPresentPass>());
  XII_TEST_RESULT(definition.Connect(operation, outputPin, present, "GradedColor"));
  view->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>(xiiStringBuilder("Explicit failure fixture/", shaderPath), std::move(definition)));
  xiiGALTextureCreationDescription targetDescription;
  targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  targetDescription.m_Size      = {16, 16};
  targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto target                   = environment.GetDevice()->CreateTexture(targetDescription);
  if (!XII_TEST_BOOL(target != nullptr))
    return;
  view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  const auto shader        = CreateMissingShader(shaderPath);
  const auto outputBinding = [&]() -> const xiiRenderPipelineBinding* {
    const auto& instance = view->GetRenderPipelineInstance();
    const auto* pipeline = instance.GetPipeline();
    if (!pipeline)
      return nullptr;
    const auto& passes = pipeline->GetDescriptor().m_Passes;
    for (xiiUInt32 index = 0; index < passes.GetCount(); ++index)
      if (passes[index].m_pType == xiiGetStaticRTTI<xiiPresentPass>())
        return instance.GetOutputBinding(index, "ViewOutput");
    return nullptr;
  };

  for (xiiUInt32 attempt = 0; attempt < 2; ++attempt)
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope logScope(&log, true);
    log.ExpectMessage("Failed to create reflected compute operation pipeline", xiiLogMsgType::ErrorMsg);
    log.ExpectMessage("Failed to build render pipeline", xiiLogMsgType::ErrorMsg);
    log.ExpectMessage("Failed to execute render graph for view 'Shader failure view'", xiiLogMsgType::ErrorMsg);
    world.Update();
    XII_TEST_BOOL(view->GetRenderGraph()->GetQueueSubmissions().IsEmpty());
    const auto* output = outputBinding();
    XII_TEST_BOOL(output && !output->m_hTexture.IsValid());
  }
  if (!XII_TEST_RESULT(RestoreShader(shader)))
    return;
  world.Update();
  environment.GetDevice()->WaitIdle();
  XII_TEST_BOOL(!view->GetRenderGraph()->GetQueueSubmissions().IsEmpty());
  const auto* output = outputBinding();
  XII_TEST_BOOL(output && output->m_hTexture.IsValid());
  xiiImage capture;
  if (XII_TEST_RESULT(xiiCaptureThumbnail(target, {16, 16}, capture)))
    XII_TEST_INT(capture.GetPixelPointer<xiiUInt8>()[3], 255);
}

XII_CREATE_SIMPLE_TEST(RenderOperationFailure, ViewRejectsExecutionAndRecovers)
{
  TestViewShaderFailure(sBloomShader);
}

XII_CREATE_SIMPLE_TEST(RenderOperationFailure, DirectionalLightingFailureAndRecovery)
{
  TestViewShaderFailure("Shaders/Pipeline/Passes/Lighting/DirectionalSurfaceLighting.xiiShader");
}
