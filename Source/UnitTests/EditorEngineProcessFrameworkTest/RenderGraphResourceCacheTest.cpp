/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

XII_CREATE_SIMPLE_TEST_GROUP(RenderGraphResources);

XII_CREATE_SIMPLE_TEST(RenderGraphResources, ChangingResolutionRetention)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiRenderGraphTestContext contexts[2];
  for (auto& context : contexts)
    if (!XII_TEST_RESULT(context.Initialize("Changing viewport allocation")))
      return;

  xiiUInt32 maximumPooled = 0;
  for (xiiUInt32 iteration = 0; iteration < 64; ++iteration)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin(2)))
      return;
    auto& context = contexts[iteration % 2];
    auto& cache   = context.GetResourceCache();
    auto& graph   = context.GetGraph();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    struct Data
    {
      xiiRenderGraphTextureHandle m_Texture;
      xiiRenderGraphBufferHandle  m_Buffer;
      xiiUInt32                   m_uiValue;
    };
    graph.AddPass<Data>("Changing viewport", xiiGALCommandQueueFlags::Graphics, [iteration](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription texture;
      texture.m_Type = xiiGALResourceDimension::Texture2D;
      texture.m_Size = {512 + iteration * 8, 512};
      texture.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
      texture.m_BindFlags = xiiGALBindFlags::RenderTarget;
      data.m_Texture = builder.WriteTexture("Changing target", texture, xiiGALResourceStateFlags::CopyDestination);
      xiiGALBufferCreationDescription buffer;
      buffer.m_uiSize = 256 + iteration * 16;
      buffer.m_BindFlags = xiiGALBindFlags::ShaderResource;
      buffer.m_Mode = xiiGALBufferMode::Raw;
      data.m_Buffer = builder.WriteBuffer("Changing upload", buffer, xiiGALResourceStateFlags::CopyDestination);
      data.m_uiValue = iteration; }, [](const Data& data, xiiRenderGraphPassContext& context) {
      context.GetCommandList().ClearRenderTargetView(context.GetTexture(data.m_Texture)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black);
      context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_Buffer), 0, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&data.m_uiValue), sizeof(data.m_uiValue))); }, true);
    graph.EndSetup();
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
      return;
    maximumPooled = xiiMath::Max(maximumPooled, cache.GetPooledResourceCount());
    XII_TEST_INT(cache.GetActiveResourceCount(), 2);
  }
  // Both per-view caches must remain bounded as every frame introduces a new description.
  // Without automatic eviction, each retains all 32 historical resolutions and sizes.
  XII_TEST_BOOL_MSG(maximumPooled <= 8, "Changing resolutions retained %u idle GPU resources per view.", maximumPooled);
  xiiTestFramework::Output(xiiTestOutput::Message, "Maximum idle resources per changing viewport: %u", maximumPooled);
}

XII_CREATE_SIMPLE_TEST(RenderGraphResources, CompletionBeforeReuseAndEviction)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  XII_SCOPE_EXIT(environment.Shutdown());
  xiiRenderGraphResourceCache      cache(environment.GetDeviceShared());
  xiiGALTextureCreationDescription description;
  description.m_Type      = xiiGALResourceDimension::Texture2D;
  description.m_Size      = {32, 32};
  description.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  description.m_BindFlags = xiiGALBindFlags::RenderTarget;
  cache.BeginFrame(1, 0);
  auto first  = cache.AcquireTexture(description);
  auto second = cache.AcquireTexture(description);
  if (!XII_TEST_BOOL(first != nullptr && second != nullptr))
    return;
  XII_TEST_BOOL(first != second);
  cache.EndFrame();
  cache.BeginFrame(2, 0);
  cache.ReleaseStaleResources(0);
  XII_TEST_INT(cache.GetPooledResourceCount(), 0);
  auto third = cache.AcquireTexture(description);
  XII_TEST_BOOL(third != first && third != second);
  cache.EndFrame();
  cache.BeginFrame(3, 1);
  XII_TEST_INT(cache.GetPooledResourceCount(), 2);
  auto reused = cache.AcquireTexture(description);
  XII_TEST_BOOL(reused == first || reused == second);
  cache.EndFrame();
  const auto references = third->GetRefCount();
  // Age alone cannot release frame 2's allocation while completion remains at frame 1.
  cache.BeginFrame(100, 1);
  cache.ReleaseStaleResources(0);
  XII_TEST_INT(third->GetRefCount(), references);
  XII_TEST_INT(cache.GetPooledResourceCount(), 0);
  cache.BeginFrame(101, 100);
  cache.BeginFrame(102, 100);
  XII_TEST_INT(third->GetRefCount(), references - 1);
  XII_TEST_INT(cache.GetPooledResourceCount(), 0);
  cache.BeginFrame(200, 100);
  auto delayed = cache.AcquireTexture(description);
  cache.EndFrame();
  cache.BeginFrame(208, 200);
  // A long completion delay must not force hot resources to be reallocated every frame.
  auto delayedReuse = cache.AcquireTexture(description);
  XII_TEST_BOOL(delayedReuse == delayed);
  cache.EndFrame();
}

XII_CREATE_SIMPLE_TEST(RenderGraphResources, BuildFailureStopsExecution)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  bool       failBuild = true;
  bool       executed  = false;
  const auto handle    = xiiRenderGraphManager::CreateGraph("Build failure", [&](xiiRenderGraph& graph, xiiRenderGraphBlackboard&, const xiiView*) -> xiiResult {
    struct Data
    {
      XII_DECLARE_POD_TYPE();
    };
    graph.AddPass<Data>("Must not execute after failed build", xiiGALCommandQueueFlags::Graphics, [](Data&, xiiRenderGraphBuilder&) {}, [&](const Data&, xiiRenderGraphPassContext&) { executed = true; }, true);
    return failBuild ? XII_FAILURE : XII_SUCCESS;
  });
  if (!XII_TEST_BOOL(!handle.IsInvalidated()))
    return;
  XII_SCOPE_EXIT(xiiRenderGraphManager::DestroyGraph(handle).AssertSuccess());

  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  XII_TEST_BOOL(xiiRenderGraphManager::ExecuteGraph(frame, handle, nullptr).Failed());
  XII_TEST_BOOL(!executed);
  XII_TEST_BOOL(!xiiRenderGraphManager::GetGraph(handle)->IsCompiled());
  XII_TEST_BOOL(xiiRenderGraphManager::GetGraph(handle)->GetCompiledPasses().IsEmpty());
  failBuild = false;
  XII_TEST_RESULT(xiiRenderGraphManager::ExecuteGraph(frame, handle, nullptr));
  XII_TEST_BOOL(executed);
}

XII_CREATE_SIMPLE_TEST(RenderGraphResources, HandleRetirementDuringFrame)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  bool                                       destroyed = false;
  const xiiRenderGraphManager::BuildDelegate build     = [&](xiiRenderGraph& graph, xiiRenderGraphBlackboard&, const xiiView*) -> xiiResult {
    struct Data
    {
      bool* m_pDestroyed = nullptr;
      ~Data() { *m_pDestroyed = true; }
    };
    graph.AddPass<Data>("Retained pass data", xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder&) { data.m_pDestroyed = &destroyed; }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  };
  const auto original = xiiRenderGraphManager::CreateGraph("Retired graph", build);
  if (!XII_TEST_BOOL(!original.IsInvalidated()))
    return;
  XII_SCOPE_EXIT(if (xiiRenderGraphManager::GetGraph(original) != nullptr) xiiRenderGraphManager::DestroyGraph(original).AssertSuccess());

  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  XII_TEST_RESULT(xiiRenderGraphManager::ExecuteGraph(frame, original, nullptr));
  XII_TEST_RESULT(xiiRenderGraphManager::DestroyGraph(original));
  XII_TEST_BOOL(xiiRenderGraphManager::GetGraph(original) == nullptr);
  XII_TEST_BOOL(!destroyed);
  XII_TEST_BOOL(xiiRenderGraphManager::ExecuteGraph(frame, original, nullptr).Failed());

  const auto replacement = xiiRenderGraphManager::CreateGraph("Retired graph", build);
  if (!XII_TEST_BOOL(!replacement.IsInvalidated()))
    return;
  XII_SCOPE_EXIT(xiiRenderGraphManager::DestroyGraph(replacement).AssertSuccess());
  XII_TEST_BOOL(original != replacement);
  XII_TEST_BOOL(xiiRenderGraphManager::DestroyGraph(original).Failed());
  XII_TEST_RESULT(xiiRenderGraphManager::ExecuteGraph(frame, replacement, nullptr));
  frame.End();
  XII_TEST_BOOL(destroyed);
  XII_TEST_BOOL(xiiRenderGraphManager::GetGraph(replacement) != nullptr);
}
