/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Basics.h>
#include <TestFramework/Framework/TestFramework.h>

#ifdef XII_INSPECTOR_GPU_TESTS
#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <GraphicsCore/Pipeline/RenderGraph.h>
#  include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#  include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#  include <QJsonArray>
#  include <QJsonDocument>
#  include <QJsonObject>

XII_CREATE_SIMPLE_TEST(Inspector, GpuExecutionAndGraphIsolation)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  auto* device = environment.GetDevice();
  XII_SCOPE_EXIT(environment.Shutdown());
  xiiRenderGraphResourceCache     cache(environment.GetDeviceShared());
  xiiRenderGraphTimestampProfiler profiler(environment.GetDeviceShared());
  XII_SCOPE_EXIT(device->WaitIdle());
  xiiRenderGraph                                                          graphs[] = {xiiRenderGraph("Inspection"), xiiRenderGraph("Inspection")};
  xiiRenderGraphBlackboard                                                blackboard;
  xiiUInt32                                                               eventCount = 0;
  QJsonObject                                                             captures[2];
  xiiEvent<const xiiRenderGraphDebug::ExecutionEvent&, xiiMutex>::Handler handler = [&](const xiiRenderGraphDebug::ExecutionEvent& event) {
    XII_TEST_BOOL(event.m_bSucceeded);
    xiiContiguousMemoryStreamStorage storage;
    xiiMemoryStreamWriter            writer(&storage);
    XII_TEST_RESULT(xiiRenderGraphDebug::WriteSnapshot(event, writer));
    const int index = event.m_Graph.GetInstanceId() == graphs[0].GetInstanceId() ? 0 : 1;
    captures[index] = QJsonDocument::fromJson(QByteArray(reinterpret_cast<const char*>(storage.GetData()), storage.GetStorageSize32())).object();
    ++eventCount;
  };
  auto subscription = xiiRenderGraphDebug::s_ExecutionEvents.AddEventHandler(handler);
  XII_SCOPE_EXIT(xiiRenderGraphDebug::s_ExecutionEvents.RemoveEventHandler(subscription));
  for (xiiUInt64 frame = 1; frame <= 8; ++frame)
  {
    device->BeginFrame();
    cache.BeginFrame(frame, frame - 1);
    for (int i = 0; i < 2; ++i)
    {
      auto& graph = graphs[i];
      graph.BeginSetup(frame);
      struct Data
      {
        xiiRenderGraphBufferHandle m_Buffer;
        xiiUInt32                  m_uiValue;
      };
      graph.AddPass<Data>("Repeated pass", xiiGALCommandQueueFlags::Graphics, [i](Data& data, xiiRenderGraphBuilder& builder) {
        xiiGALBufferCreationDescription description;
        description.m_uiSize = 256;
        description.m_BindFlags = xiiGALBindFlags::ShaderResource;
        description.m_Mode = xiiGALBufferMode::Raw;
        data.m_Buffer = builder.WriteBuffer("Captured upload", description, xiiGALResourceStateFlags::CopyDestination);
        data.m_uiValue = 0xCAFE0000U + i; }, [](const Data& data, xiiRenderGraphPassContext& context) { context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_Buffer), 0, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&data.m_uiValue), sizeof(data.m_uiValue))); }, true);
      graph.EndSetup();
      xiiRenderGraphCompileSettings settings;
      settings.m_bEnableGPUProfiling = true;
      XII_TEST_RESULT(graph.Compile(settings));
      XII_TEST_RESULT(graph.Execute(device, nullptr, &blackboard, &cache, &profiler));
    }
    cache.EndFrame();
    device->EndFrame();
    device->WaitIdle(); // Test-only synchronization; production capture never waits.
  }
  XII_TEST_INT(eventCount, 16);
  XII_TEST_BOOL(captures[0]["instance"] != captures[1]["instance"]);
  for (int i = 0; i < 2; ++i)
  {
    const auto pass = captures[i]["passes"].toArray()[0].toObject();
    if (device->GetFeatures().m_DurationQueries == xiiGALDeviceFeatureState::Enabled)
    {
      XII_TEST_BOOL(pass["timingAvailable"].toBool());
      XII_TEST_BOOL(pass["gpuMs"].toDouble(-1) >= 0);
      XII_TEST_BOOL(pass["timingFrame"].toString().toULongLong() < 8);
    }
    float     ms          = 0;
    xiiUInt64 timingFrame = 0;
    XII_TEST_BOOL(!profiler.GetPassTiming(graphs[i].GetInstanceId(), 0, "Different topology", ms, timingFrame));
  }
}
#endif
