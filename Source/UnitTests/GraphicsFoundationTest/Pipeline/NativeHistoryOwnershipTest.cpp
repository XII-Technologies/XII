/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/BufferHistoryPass.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TextureHistoryPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<xiiBufferHistoryPass> && !std::is_copy_assignable_v<xiiBufferHistoryPass>);
static_assert(!std::is_copy_constructible_v<xiiTextureHistoryPass> && !std::is_copy_assignable_v<xiiTextureHistoryPass>);

XII_CREATE_SIMPLE_TEST_GROUP(NativeHistoryOwnership);
XII_CREATE_SIMPLE_TEST(NativeHistoryOwnership, RepeatedOccurrencesSurviveOwnerRetirement)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiRenderPipelineResourceDescriptor leaf;
  const auto                          depth = leaf.AddPass("Depth source", "xiiDepthTargetPass");
  leaf.m_Passes[depth].m_Properties.Insert("Width", 4U);
  leaf.m_Passes[depth].m_Properties.Insert("Height", 4U);
  const auto counter        = leaf.AddPass("Counter source", "xiiCounterResetPass");
  const auto textureHistory = leaf.AddPass("Private image history", "xiiTextureHistoryPass");
  const auto bufferHistory  = leaf.AddPass("Private counter history", "xiiBufferHistoryPass");
  XII_TEST_RESULT(leaf.Connect(depth, "DepthTarget", textureHistory, "Texture"));
  XII_TEST_RESULT(leaf.Connect(counter, "Counter", bufferHistory, "Buffer"));
  leaf.m_Interface.PushBack({"PreviousDepth", false, leaf.m_Passes[textureHistory].m_Id, "PreviousTexture"});
  leaf.m_Interface.PushBack({"PreviousValue", false, leaf.m_Passes[bufferHistory].m_Id, "PreviousBuffer"});
  xiiGraphDefinition definition;
  if (!XII_TEST_RESULT(leaf.MakeGraphDefinition(definition)))
    return;
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto node  = root.AddSubgraph(i ? "Second history" : "First history", leaf.m_Id);
    auto&      image = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    image.m_Path.PushBack(leaf.m_Passes[depth].m_Id);
    image.m_Properties.Insert("Depth", i ? 0.8f : 0.2f);
    auto& value = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    value.m_Path.PushBack(leaf.m_Passes[counter].m_Id);
    value.m_Properties.Insert("Value", i ? 91U : 17U);
    root.m_Interface.PushBack({i ? "DepthB" : "DepthA", false, root.m_Passes[node].m_Id, "PreviousDepth"});
    root.m_Interface.PushBack({i ? "ValueB" : "ValueA", false, root.m_Passes[node].m_Id, "PreviousValue"});
  }
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Retained native history occurrences", std::move(root));
  xiiRenderGraphContext     contexts[2];
  xiiRenderPipelineInstance instances[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
    if (!XII_TEST_RESULT(contexts[i].Initialize("Retained native histories")) || !XII_TEST_RESULT(instances[i].Synchronize(resource)))
      return;
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiSharedPtr<xiiGALTexture> images[2][2];
    xiiSharedPtr<xiiGALBuffer>  values[2][2];
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      auto& graph = contexts[view].GetGraph();
      auto& cache = contexts[view].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      auto&      instance = instances[view];
      const auto built    = instance.Build(nullptr, graph, contexts[view].GetBlackboard());
      if (!XII_TEST_RESULT_MSG(built, "%s", instance.GetLastBuildError().GetStartPointer()))
        return;
      xiiRenderGraphTextureHandle previousImages[2];
      xiiRenderGraphBufferHandle  previousValues[2];
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        previousImages[i] = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(i ? "DepthB" : "DepthA", false))->m_hTexture;
        previousValues[i] = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(i ? "ValueB" : "ValueA", false))->m_hBuffer;
        XII_TEST_BOOL(previousImages[i].IsValid() == (phase != 0));
        XII_TEST_BOOL(previousValues[i].IsValid() == (phase != 0));
      }
      if (phase)
      {
        graph.AddPass<xiiUInt32>("Test explicit previous-frame exports", xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) {
          for (xiiUInt32 i = 0; i < 2; ++i)
          {
            builder.ExportTexture(builder.ReadTexture(previousImages[i], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
            builder.ExportBuffer(builder.ReadBuffer(previousValues[i], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
          } }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
      }
      // Queued history copies must retain only their private state and owned resources.
      if (phase == 2)
        instance.Clear();
      graph.EndSetup();
      xiiStringBuilder error;
      const auto       compiled = graph.Compile({.m_bEnableAsyncQueues = false}, &error);
      if (!XII_TEST_RESULT_MSG(compiled, "%s", error.GetData()))
        return;
      const auto executed = graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache, nullptr, &error);
      if (!XII_TEST_RESULT_MSG(executed, "%s", error.GetData()))
        return;
      XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
      if (phase)
        for (xiiUInt32 i = 0; i < 2; ++i)
        {
          images[view][i] = graph.GetExportedTexture(previousImages[i]);
          values[view][i] = graph.GetExportedBuffer(previousValues[i]);
        }
    }
    frame.End();
    environment.GetDevice()->WaitIdle();
    if (phase)
      for (xiiUInt32 view = 0; view < 2; ++view)
        for (xiiUInt32 i = 0; i < 2; ++i)
        {
          XII_TEST_BOOL(images[view][i] != images[view][1 - i] && images[view][i] != images[1 - view][i]);
          XII_TEST_BOOL(values[view][i] != values[view][1 - i] && values[view][i] != values[1 - view][i]);
          float     depthValue   = 0;
          xiiUInt32 counterValue = 0;
          if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), images[view][i].Borrow(), 1, 1, depthValue) ||
              !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), values[view][i].Borrow(), 0, counterValue))
            return;
          XII_TEST_FLOAT(depthValue, i ? 0.8f : 0.2f, 0.00001f);
          XII_TEST_INT(counterValue, i ? 91 : 17);
        }
  }
}
