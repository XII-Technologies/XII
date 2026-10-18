/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>

XII_CREATE_SIMPLE_TEST_GROUP(RenderGraphAllocations);
XII_CREATE_SIMPLE_TEST(RenderGraphAllocations, ExportedBufferOutlivesGraphUses)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderGraphContext context;
  if (!XII_TEST_RESULT(context.Initialize("Export lifetime")))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = context.GetGraph();
  auto& cache = context.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame());
  graph.BeginSetup(frame.GetFrameIndex());
  struct Data
  {
    xiiRenderGraphBufferHandle m_Buffer;
    xiiUInt32                  m_uiValue;
  };
  xiiRenderGraphBufferHandle exported;
  for (xiiUInt32 value = 1; value <= 3; ++value)
  {
    xiiStringBuilder name;
    name.SetFormat("Allocation {}", value);
    const auto pass = graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Transfer, [value, &name](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription description;
      description.m_uiSize = description.m_uiElementByteStride = sizeof(xiiUInt32);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data.m_Buffer = builder.WriteBuffer(name, description, xiiGALResourceStateFlags::CopyDestination);
      data.m_uiValue = value;
      if (value == 1) builder.ExportBuffer(data.m_Buffer, xiiGALResourceStateFlags::CopyDestination);
      builder.SetPassAllowMerge(false); }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Buffer), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_uiValue), sizeof(data.m_uiValue))); }, true);
    if (value == 1)
      exported = pass.first->m_Buffer;
  }
  graph.EndSetup();
  xiiStringBuilder error;
  if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = false}, &error), "%s", error.GetData()))
    return;
  const auto resources = graph.GetResourceDescriptions();
  XII_TEST_INT(resources.GetCount(), 3);
  XII_TEST_BOOL(resources[0].m_uiLastUsePassIndex < resources[1].m_uiFirstUsePassIndex);
  XII_TEST_BOOL(resources[1].m_uiLastUsePassIndex < resources[2].m_uiFirstUsePassIndex);
  XII_TEST_BOOL(resources[0].m_uiAliasGroup != resources[1].m_uiAliasGroup);
  XII_TEST_INT(resources[1].m_uiAliasGroup, resources[2].m_uiAliasGroup);
  if (!XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
    return;
  xiiUInt32 value = 0;
  if (xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(exported).Borrow(), 0, value))
    XII_TEST_INT(value, 1);
}
