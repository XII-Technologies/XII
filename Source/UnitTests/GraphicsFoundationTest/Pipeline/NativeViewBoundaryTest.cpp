/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Shaders/Pipeline/Passes/Output/FinalBlitConstants.h>

XII_DEFINE_AS_POD_TYPE(xiiFinalBlitConstants);

XII_CREATE_SIMPLE_TEST_GROUP(NativeViewBoundary);
XII_CREATE_SIMPLE_TEST(NativeViewBoundary, PrivateTargetsAndRetainedOutputSnapshots)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 targetAsset = xiiLoadPipelineLibraryResource("SceneTargets");
  const auto                                 outputAsset = xiiLoadPipelineLibraryResource("ViewPresentation");
  const auto                                 exportAsset = xiiLoadPipelineLibraryResource("BufferExport");
  xiiResourceLock<xiiRenderPipelineResource> targetSource(targetAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  xiiResourceLock<xiiRenderPipelineResource> outputSource(outputAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  xiiResourceLock<xiiRenderPipelineResource> exportSource(exportAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(targetSource.IsValid() && outputSource.IsValid() && exportSource.IsValid() && targetSource->GetPipeline() && outputSource->GetPipeline() && exportSource->GetPipeline()))
    return;
  const auto targetDefinition = targetSource->GetAuthoring();
  const auto outputDefinition = outputSource->GetAuthoring();
  const auto exportDefinition = exportSource->GetAuthoring();
  xiiUuid    colorNode;
  for (const auto& pass : targetDefinition->m_Descriptor.m_Passes)
    if (pass.m_sType == "xiiClearTexturePass")
      colorNode = pass.m_Id;
  if (!XII_TEST_BOOL(colorNode.IsValid()))
    return;
  xiiWorldDescription description("Native view boundaries");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*                           module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView*                        views[2];
  xiiRenderGraphTestContext       contexts[2];
  xiiRenderPipelineInstance       targets[2], outputs[2], counters[2];
  xiiRenderPipelineResourceHandle targetRoots[2], outputRoots[2], counterRoots[2];
  xiiSharedPtr<xiiGALTexture>     textures[2];
  for (xiiUInt32 v = 0; v < 2; ++v)
  {
    module->CreateView(v ? "Blue portrait" : "Red landscape", views[v]);
    xiiGALTextureCreationDescription image;
    image.m_Type      = xiiGALResourceDimension::Texture2D;
    image.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    image.m_Size      = v ? xiiSizeU32(8, 16) : xiiSizeU32(16, 8);
    image.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    textures[v]       = environment.GetDevice()->CreateTexture(image);
    if (!XII_TEST_BOOL(textures[v] != nullptr))
      return;
    views[v]->SetRenderTargetView(textures[v]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    views[v]->SetViewport({0, 0, float(image.m_Size.width), float(image.m_Size.height)});
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({targetDefinition->m_Descriptor.m_Id, targetAsset.GetResourceID()});
    auto  instance = root.AddSubgraph("Selected targets", targetDefinition->m_Descriptor.m_Id);
    auto& color    = root.m_Passes[instance].m_Overrides.ExpandAndGetRef();
    color.m_Path.PushBack(colorNode);
    color.m_Properties.Insert("Color", v ? xiiColor(0, 0, 1) : xiiColor(1, 0, 0));
    root.m_Interface.PushBack({"Color", false, root.m_Passes[instance].m_Id, "Color"});
    root.m_Interface.PushBack({"Depth", false, root.m_Passes[instance].m_Id, "Depth"});
    targetRoots[v] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(xiiStringBuilder("View boundary targets ", v ? "B" : "A"), std::move(root));
    root           = {};
    root.m_References.PushBack({outputDefinition->m_Descriptor.m_Id, outputAsset.GetResourceID()});
    instance      = root.AddSubgraph("Selected output", outputDefinition->m_Descriptor.m_Id);
    auto& display = root.m_Passes[instance].m_Overrides.ExpandAndGetRef();
    display.m_Path.PushBack(outputDefinition->m_Descriptor.m_Passes[0].m_Id);
    display.m_Properties.Insert("PaperWhiteNits", v ? 300.0f : 100.0f);
    root.m_Interface.PushBack({"Color", true, root.m_Passes[instance].m_Id, "Color"});
    root.m_Interface.PushBack({"ViewOutput", false, root.m_Passes[instance].m_Id, "ViewOutput"});
    root.m_Interface.PushBack({"Constants", false, root.m_Passes[instance].m_Id, "Constants"});
    outputRoots[v] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(xiiStringBuilder("View boundary output ", v ? "B" : "A"), std::move(root));
    root           = {};
    root.m_References.PushBack({exportDefinition->m_Descriptor.m_Id, exportAsset.GetResourceID()});
    const auto counter = root.AddPass("Counter with an external consumer", "xiiCounterResetPass");
    root.m_Passes[counter].m_Properties.Insert("Value", 41U + v);
    instance = root.AddSubgraph("Explicit export", exportDefinition->m_Descriptor.m_Id);
    if (!XII_TEST_RESULT(root.Connect(counter, "Counter", instance, "Input")))
      return;
    root.m_Interface.PushBack({"Output", false, root.m_Passes[instance].m_Id, "Output"});
    counterRoots[v] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(xiiStringBuilder("View boundary counter ", v ? "B" : "A"), std::move(root));
    if (!XII_TEST_RESULT(contexts[v].Initialize("View boundary GPU test")))
      return;
  }
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin(3)))
      return;
    xiiRenderGraphTextureHandle colorHandles[2], depthHandles[2], outputHandles[2];
    xiiRenderGraphBufferHandle  constantHandles[2], counterHandles[2];
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      auto& graph = contexts[v].GetGraph();
      auto& cache = contexts[v].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      if (!XII_TEST_RESULT(targets[v].Synchronize(targetRoots[v])) || !XII_TEST_RESULT(outputs[v].Synchronize(outputRoots[v])) || !XII_TEST_RESULT(counters[v].Synchronize(counterRoots[v])) || !XII_TEST_RESULT(targets[v].Build(views[v], graph, contexts[v].GetBlackboard())))
        return;
      colorHandles[v] = targets[v].GetInterfaceBinding(targets[v].GetPipeline()->FindInterface("Color", false))->m_hTexture;
      depthHandles[v] = targets[v].GetInterfaceBinding(targets[v].GetPipeline()->FindInterface("Depth", false))->m_hTexture;
      const xiiRenderPipelineInputBinding color(graph, outputs[v].GetPipeline()->FindInterface("Color", true), colorHandles[v]);
      if (!XII_TEST_RESULT(outputs[v].Build(views[v], graph, contexts[v].GetBlackboard(), {}, xiiMakeArrayPtr(&color, 1))) || !XII_TEST_RESULT(counters[v].Build(nullptr, graph, contexts[v].GetBlackboard())))
        return;
      outputHandles[v]   = outputs[v].GetInterfaceBinding(outputs[v].GetPipeline()->FindInterface("ViewOutput", false))->m_hTexture;
      constantHandles[v] = outputs[v].GetInterfaceBinding(outputs[v].GetPipeline()->FindInterface("Constants", false))->m_hBuffer;
      counterHandles[v]  = counters[v].GetInterfaceBinding(counters[v].GetPipeline()->FindInterface("Output", false))->m_hBuffer;
      struct Exports
      {
        xiiRenderGraphTextureHandle m_Depth, m_Color;
        xiiRenderGraphBufferHandle  m_Constants;
      };
      graph.AddPass<Exports>("Test-only image and constant readback exports", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
        data.m_Depth = builder.ReadTexture(depthHandles[v], xiiGALResourceStateFlags::ShaderResource);
        data.m_Color = builder.ReadTexture(colorHandles[v], xiiGALResourceStateFlags::ShaderResource);
        data.m_Constants = builder.ReadBuffer(constantHandles[v], xiiGALResourceStateFlags::ConstantBuffer);
        builder.ExportTexture(data.m_Depth, xiiGALResourceStateFlags::ShaderResource);
        builder.ExportTexture(data.m_Color, xiiGALResourceStateFlags::ShaderResource);
        builder.ExportBuffer(data.m_Constants, xiiGALResourceStateFlags::ConstantBuffer); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
      // Destroy every pass instance before command recording. Captured data must own
      // everything required by both preparation and managed raster execution.
      targets[v].Clear();
      outputs[v].Clear();
      counters[v].Clear();
      graph.EndSetup();
      xiiStringBuilder error;
      if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = false}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &contexts[v].GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
        return;
      XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
    }
    // Submit both views before any host readback wait.
    environment.GetDevice()->WaitIdle();
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      auto&      graph  = contexts[v].GetGraph();
      const auto target = graph.GetExportedTexture(outputHandles[v]);
      XII_TEST_BOOL(target == textures[v] && target->GetResourceState() == xiiGALResourceStateFlags::ShaderResource);
      xiiColorGammaUB       source, result;
      float                 depth = 1;
      xiiFinalBlitConstants constants;
      xiiUInt32             counter = 0;
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), graph.GetExportedTexture(colorHandles[v]).Borrow(), 1, 1, source) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), target.Borrow(), 1, 1, result) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), graph.GetExportedTexture(depthHandles[v]).Borrow(), 1, 1, depth) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(constantHandles[v]).Borrow(), 0, constants) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(counterHandles[v]).Borrow(), 0, counter))
        return;
      XII_TEST_INT(source.r, v ? 0 : 255);
      XII_TEST_INT(source.b, v ? 255 : 0);
      XII_TEST_INT(result.r, source.r);
      XII_TEST_INT(result.g, 0);
      XII_TEST_INT(result.b, source.b);
      XII_TEST_INT(result.a, 255);
      XII_TEST_FLOAT(depth, 0, 0.00001f);
      XII_TEST_FLOAT(constants.PaperWhiteNits, v ? 300.0f : 100.0f, 0.00001f);
      XII_TEST_INT(constants.ApplySRGBTransfer, 0);
      XII_TEST_INT(counter, 41U + v);
    }
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
  }
  // Failed setups must not expose an earlier output or silently choose a target.
  auto& graph = contexts[0].GetGraph();
  for (xiiUInt32 invalid = 0; invalid < 3; ++invalid)
  {
    graph.BeginSetup(100 + invalid);
    if (!XII_TEST_RESULT(targets[0].Synchronize(targetRoots[0])) || !XII_TEST_RESULT(outputs[0].Synchronize(outputRoots[0])) || !XII_TEST_RESULT(targets[0].Build(views[0], graph, contexts[0].GetBlackboard())))
      return;
    const auto source = targets[0].GetInterfaceBinding(targets[0].GetPipeline()->FindInterface(invalid == 1 ? "Depth" : "Color", false))->m_hTexture;
    if (!invalid)
      views[0]->SetRenderTargetView(nullptr);
    else
      views[0]->SetRenderTargetView(textures[0]->GetDefaultView(invalid == 2 ? xiiGALTextureViewType::ShaderResource : xiiGALTextureViewType::RenderTarget));
    const xiiRenderPipelineInputBinding input(graph, outputs[0].GetPipeline()->FindInterface("Color", true), source);
    XII_TEST_BOOL(outputs[0].Build(views[0], graph, contexts[0].GetBlackboard(), {}, xiiMakeArrayPtr(&input, 1)).Failed());
    XII_TEST_BOOL(!outputs[0].GetLastBuildError().IsEmpty());
    const auto* failedOutput = outputs[0].GetInterfaceBinding(outputs[0].GetPipeline()->FindInterface("ViewOutput", false));
    XII_TEST_BOOL(!failedOutput || !failedOutput->m_hTexture.IsValid());
    graph.EndSetup();
  }
}
