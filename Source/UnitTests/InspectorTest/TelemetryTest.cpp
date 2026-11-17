/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Diagnostics/TelemetryDiscovery.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Semaphore.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <Inspector/ProfilingWidget.h>
#include <Inspector/RenderGraphWidget.h>
#include <QApplication>
#include <QTreeWidget>
#include <TestFramework/Framework/TestFramework.h>
#include <ads/DockManager.h>

XII_CREATE_SIMPLE_TEST_GROUP(InspectorDiagnostics);
XII_CREATE_SIMPLE_TEST_GROUP(InspectorDiagnosticsBenchmark);

// Separate process exercises the real telemetry protocol and dynamically loaded producer plugin.
// GPU command execution and timestamp readback are covered by GpuExecutionAndGraphIsolation.
int RunInspectorCaptureServer()
{
  if (xiiPlugin::LoadPlugin("xiiInspectorPlugin").Failed())
  {
    return 1;
  }
  auto& service = xiiApplication::GetApplicationInstance()->CreateTelemetryService();
  if (service.StartLocal("InspectorTest", xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-capture-endpoint")).Failed())
    return 3;
  xiiSemaphore ready, unload;
  if (ready.Open(xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-capture-ready")).Failed() ||
      unload.Open(xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-capture-unload")).Failed())
    return 4;
  ready.ReturnToken();
  xiiRenderGraph graph("Telemetry fixture");
  bool           bPluginLoaded = true;
  for (xiiUInt64 frame = 1; frame <= static_cast<xiiUInt64>(xiiCommandLineUtils::GetGlobalInstance()->GetIntOption("-capture-frames", 1500)); ++frame)
  {
    if (bPluginLoaded && unload.TryAcquireToken().Succeeded())
    {
      xiiPlugin::UnloadAllPlugins();
      bPluginLoaded = false;
    }
    graph.BeginSetup(frame);
    struct Data
    {
      XII_DECLARE_POD_TYPE();
    };
    graph.AddPass<Data>("Telemetry rendering pass", xiiGALCommandQueueFlags::Graphics, [](Data&, xiiRenderGraphBuilder&) {}, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    graph.EndSetup();
    if (graph.Compile().Failed())
    {
      return 2;
    }
    xiiClock::GetGlobalClock()->Update();
    if (frame % 16 == 0)
    {
      xiiLog::Info("Diagnostics fixture frame {}", frame);
      xiiStringBuilder path;
      path.SetFormat("{}/xii-diagnostics-fixture-{}.tmp", xiiOSFile::GetTempDataFolder(), xiiProcess::GetCurrentProcessID());
      xiiOSFile file;
      if (file.Open(path, xiiFileOpenMode::Write).Succeeded())
      {
        file.Write(&frame, sizeof(frame)).IgnoreResult();
        file.Close();
        xiiOSFile::DeleteFile(path).IgnoreResult();
      }
    }
    xiiProfilingSystem::StartNewFrame();
    const auto now = xiiTime::Now();
    xiiProfilingSystem::AddCPUScope("Rendering telemetry fixture", "RunInspectorCaptureServer", now - xiiTime::MakeFromMilliseconds(1), now, {});
    service.Update();
    xiiRenderGraphDebug::s_ExecutionEvents.Broadcast({graph, nullptr, frame, true, {}});
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }
  service.Shutdown();
  xiiPlugin::UnloadAllPlugins();
  return 0;
}

XII_CREATE_SIMPLE_TEST(InspectorDiagnostics, PluginTelemetryRoundTrip)
{
  xiiStringBuilder address;
  address.SetFormat("inspector-{}-{}", xiiProcess::GetCurrentProcessID(), xiiArgU(xiiTime::Now().GetMicroseconds(), 1, false, 16));
  // Keep named semaphore identities below Darwin's length limit.
  xiiUInt64 nonce, unused;
  xiiUuid::MakeUuid().GetValues(nonce, unused);
  xiiStringBuilder readyName, unloadName;
  readyName.SetFormat("xii-diag-{}-r", xiiArgU(nonce, 16, true, 16));
  unloadName.SetFormat("xii-diag-{}-u", xiiArgU(nonce, 16, true, 16));
  xiiSemaphore ready, unload;
  if (!XII_TEST_RESULT(ready.Create(0, readyName)) || !XII_TEST_RESULT(unload.Create(0, unloadName)))
    return;
  xiiProcessOptions options;
  options.m_sProcess           = xiiOSFile::GetApplicationPath();
  options.m_bHideConsoleWindow = true;
  options.AddArgument("-capture-server");
  options.AddArgument("-capture-endpoint");
  options.AddArgument("{}", address);
  options.AddArgument("-capture-ready");
  options.AddArgument("{}", readyName);
  options.AddArgument("-capture-unload");
  options.AddArgument("{}", unloadName);
  xiiProcess server;
  if (!XII_TEST_RESULT(server.Launch(options)))
  {
    return;
  }
  if (!XII_TEST_RESULT(ready.TryAcquireToken(xiiTime::MakeFromSeconds(5))))
    return;
  xiiTelemetryClient           client, observerClient;
  xiiInspectorProfilingSession observer;
  observer.BeginStream(4242);
  observerClient.SetSampleHandler([&](const xiiTelemetrySample& sample) {
    if (sample.m_uiSource == xiiTelemetrySources::Profiling)
    {
      xiiRawMemoryStreamReader reader(sample.m_Data.GetData(), sample.m_Data.GetCount());
      XII_TEST_BOOL(observer.Receive(reader));
    }
  });
  xiiSet<xiiUInt64>      sourcesReceived;
  ads::CDockManager      manager;
  xiiQtRenderGraphWidget graph(&manager);
  xiiQtProfilingWidget   profile(&manager);
  graph.SetClient(&client);
  profile.GetSession()->m_pClient = &client;
  client.SetSampleHandler([&](const xiiTelemetrySample& sample) {
    sourcesReceived.Insert(sample.m_uiSource);
    if (sample.m_uiSource == xiiTelemetrySources::RenderGraphs)
      graph.ReceiveDiagnostics(sample);
    else
      profile.ReceiveDiagnostics(sample);
  });
  xiiTelemetryConfiguration config;
  config.m_Network.m_Transport = xiiNetworkTransportType::Local;
  xiiNetworkLocalEndpoint endpoint;
  if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(address, endpoint).Succeeded()) || !XII_TEST_BOOL(client.Connect(config, endpoint).Succeeded()) || !XII_TEST_BOOL(observerClient.Connect(config, endpoint).Succeeded()))
    return;
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
  while ((!client.IsReady() || !observerClient.IsReady()) && xiiTime::Now() < deadline)
  {
    client.Update();
    observerClient.Update();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }
  if (!XII_TEST_BOOL(client.IsReady() && observerClient.IsReady()))
  {
    return;
  }
  xiiTelemetrySubscription subscription;
  subscription.m_uiSource     = xiiTelemetrySources::Profiling;
  subscription.m_uiRequestId  = 1;
  subscription.m_uiGeneration = 4242;
  XII_TEST_BOOL(observerClient.Subscribe(subscription).Succeeded());
  xiiUInt64 request = 0;
  for (auto source : {xiiTelemetrySources::Counters, xiiTelemetrySources::Memory, xiiTelemetrySources::Plugins, xiiTelemetrySources::Subsystems,
                      xiiTelemetrySources::GlobalEvents, xiiTelemetrySources::Input, xiiTelemetrySources::Reflection, xiiTelemetrySources::Resources, xiiTelemetrySources::CVars})
  {
    xiiTelemetrySubscription snapshot;
    snapshot.m_uiSource     = source;
    snapshot.m_uiRequestId  = ++request;
    snapshot.m_uiGeneration = request;
    snapshot.m_bSnapshot    = true;
    XII_TEST_BOOL(client.Subscribe(snapshot).Succeeded());
  }
  for (auto source : {xiiTelemetrySources::Logs, xiiTelemetrySources::Time, xiiTelemetrySources::Files})
  {
    xiiTelemetrySubscription stream;
    stream.m_uiSource     = source;
    stream.m_uiRequestId  = ++request;
    stream.m_uiGeneration = request;
    XII_TEST_BOOL(client.Subscribe(stream).Succeeded());
  }
  auto* passes   = graph.findChild<QTreeWidget*>("Graph passes");
  auto* hotspots = profile.findChild<QTreeWidget*>("Profiling hotspots");
  XII_TEST_INT(passes->topLevelItemCount(), 0); // Producer sends no graph data before a request.
  graph.RequestCapture();
  profile.show();
  manager.show();
  profile.RequestCapture();
  while ((passes->topLevelItemCount() == 0 || hotspots->topLevelItemCount() == 0) && xiiTime::Now() < deadline)
  {
    client.Update();
    observerClient.Update();
    QApplication::processEvents();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }
  profile.Refresh();
  XII_TEST_INT(passes->topLevelItemCount(), 1);
  XII_TEST_BOOL(hotspots->topLevelItemCount() > 0);
  XII_TEST_BOOL(passes->topLevelItem(0) && passes->topLevelItem(0)->text(1) == "Telemetry rendering pass");
  auto session = profile.GetSession();
  session->StopRecording(false);
  session->SetPaused(false);
  const auto liveUntil = xiiTime::Now() + xiiTime::MakeFromSeconds(1);
  while (xiiTime::Now() < liveUntil)
  {
    client.Update();
    observerClient.Update();
    QApplication::processEvents();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(5));
  }
  XII_TEST_BOOL(session->m_uiSequence >= 10); // Several incremental updates within a second.
  XII_TEST_BOOL(session->m_Live.m_Frames.size() >= 20);
  if (session->m_Live.m_Frames.isEmpty())
  {
    return;
  }
  XII_TEST_BOOL(session->m_Live.m_Frames.back().m_uiIndex > session->m_Live.m_Frames.front().m_uiIndex);
  XII_TEST_BOOL(observer.m_uiSequence >= 10 && !observer.m_Live.m_Scopes.isEmpty());
  for (auto source : {xiiTelemetrySources::Counters, xiiTelemetrySources::Memory, xiiTelemetrySources::Plugins, xiiTelemetrySources::Subsystems,
                      xiiTelemetrySources::GlobalEvents, xiiTelemetrySources::Input, xiiTelemetrySources::Reflection, xiiTelemetrySources::Resources, xiiTelemetrySources::CVars})
    XII_TEST_BOOL(sourcesReceived.Contains(source));
  for (auto source : {xiiTelemetrySources::Logs, xiiTelemetrySources::Time, xiiTelemetrySources::Files})
    XII_TEST_BOOL(sourcesReceived.Contains(source));
  const auto previousObserver = observer.m_uiSequence;
  const auto previousSession  = session->m_uiSession;
  client.Shutdown();
  QApplication::processEvents();
  session->ResetConnection();
  XII_TEST_BOOL(!session->m_bSubscribed);
  XII_TEST_BOOL(client.Connect(config, endpoint).Succeeded());
  const auto reconnectUntil = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  while ((session->m_uiSession == previousSession || session->m_uiSequence < 2) && xiiTime::Now() < reconnectUntil)
  {
    client.Update();
    observerClient.Update();
    QApplication::processEvents();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(5));
  }
  XII_TEST_BOOL(session->m_uiSession != previousSession && session->m_uiSequence >= 2);
  XII_TEST_BOOL(observer.m_uiSequence > previousObserver && observer.m_uiSession == 4242);
  profile.RequestHistorySnapshot();
  const auto snapshotUntil = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  while (session->m_bSnapshotPending && xiiTime::Now() < snapshotUntil)
  {
    client.Update();
    observerClient.Update();
    QApplication::processEvents();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(5));
  }
  XII_TEST_BOOL(!session->m_bSnapshotPending && session->m_bPaused);
  XII_TEST_BOOL(!session->Display().m_Scopes.isEmpty());
  XII_TEST_BOOL(session->Display().m_Application == "InspectorTest");
  XII_TEST_BOOL(session->Display().m_uiProcessId > 0);
  unload.ReturnToken();
  const auto unloadDeadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  while ((client.IsReady() || observerClient.IsReady()) && xiiTime::Now() < unloadDeadline)
  {
    client.Update();
    observerClient.Update();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }
  XII_TEST_BOOL(!client.IsReady() && !observerClient.IsReady());
  XII_TEST_BOOL(server.GetState() == xiiProcessState::Running);
  const auto instance = client.GetServerInfo().m_Instance;
  XII_TEST_RESULT(server.Terminate());
  xiiDynamicArray<xiiTelemetryInstance> instances;
  XII_TEST_RESULT(xiiTelemetryDiscovery::Enumerate(instances));
  for (const auto& entry : instances)
    XII_TEST_BOOL(entry.m_Instance != instance);
}

XII_CREATE_SIMPLE_TEST(InspectorDiagnosticsBenchmark, ProfilePublication)
{
  if (!xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-inspectorDiagnosticsBenchmark"))
    return;

  auto* application = xiiApplication::GetApplicationInstance();
  if (!XII_TEST_RESULT(xiiPlugin::LoadPlugin("xiiInspectorPlugin")))
    return;
  auto& service = application->CreateTelemetryService();
  XII_SCOPE_EXIT(service.Shutdown(); xiiPlugin::UnloadAllPlugins(); application->DestroyTelemetryService());
  if (!XII_TEST_BOOL(service.StartLocal("Inspector profiling benchmark").Succeeded()))
    return;
  xiiNetworkLocalEndpoint endpoint;
  if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(service.GetLocalEndpointName(), endpoint).Succeeded()))
    return;

  constexpr xiiUInt32       Frames = 30, ScopesPerFrame = 512, GpuSamplesPerFrame = 256;
  xiiTelemetryConfiguration config;
  config.m_Network.m_Transport = xiiNetworkTransportType::Local;
  xiiTelemetryClient   peers[4];
  ads::CDockManager    manager;
  xiiQtProfilingWidget profiler(&manager);
  profiler.show();
  manager.show();
  xiiUInt32 deliveries = 0;
  xiiUInt32 connected  = 0;
  auto      pump       = [&] {
    service.Update();
    for (xiiUInt32 i = 0; i < connected; ++i)
      peers[i].Update();
  };
  auto wait = [&](auto&& done) {
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    while (xiiTime::Now() < deadline)
    {
      pump();
      if (done())
        return true;
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
    }
    return false;
  };
  xiiRenderGraphTimestampProfiler::TimingSample gpu;
  gpu.m_uiGraphId   = 1;
  gpu.m_bAvailable  = true;
  gpu.m_fDurationMs = 0.2;
  gpu.m_sName.Assign("Published timing fixture");
  for (const xiiUInt32 target : {0U, 1U, 4U})
  {
    while (connected < target)
    {
      const auto peer = connected++;
      peers[peer].SetSampleHandler([&, peer](const xiiTelemetrySample& sample) {
        if (sample.m_uiSource != xiiTelemetrySources::Profiling)
          return;
        ++deliveries;
        if (peer == 0)
          profiler.ReceiveDiagnostics(sample);
      });
      if (!XII_TEST_BOOL(peers[peer].Connect(config, endpoint).Succeeded()) || !XII_TEST_BOOL(wait([&] { return peers[peer].IsReady(); })))
        return;
      xiiTelemetrySubscription request;
      request.m_uiSource     = xiiTelemetrySources::Profiling;
      request.m_uiGeneration = request.m_uiRequestId = 1;
      request.m_uiIntervalMilliseconds               = 33;
      if (!XII_TEST_BOOL(peers[peer].Subscribe(request).Succeeded()))
        return;
    }
    if (!XII_TEST_BOOL(wait([&] { return service.GetStatistics().m_uiSubscriptions == target; })))
      return;
    if (target == 1)
    {
      profiler.GetSession()->m_pClient = &peers[0];
      profiler.GetSession()->BeginStream(1);
    }

    const auto before            = service.GetStatistics();
    const auto allocationsBefore = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount;
    const auto deliveriesBefore  = deliveries;
    xiiTime    cpuProducer, gpuProducer, guiTotal, guiMax;
    for (xiiUInt32 frame = 0; frame < Frames; ++frame)
    {
      xiiProfilingSystem::StartNewFrame();
      const auto started = xiiTime::Now();
      for (xiiUInt32 i = 0; i < ScopesPerFrame; ++i)
        xiiProfilingSystem::AddCPUScope("CPU publication fixture", "ProfilePublication", started - xiiTime::MakeFromMilliseconds(1), started, {});
      const auto gpuStarted = xiiTime::Now();
      cpuProducer += gpuStarted - started;
      gpu.m_uiFrameIndex = target * Frames + frame;
      gpu.m_RecordedAt   = started;
      for (xiiUInt32 i = 0; i < GpuSamplesPerFrame; ++i)
      {
        gpu.m_uiPassIndex = i;
        xiiRenderGraphTimestampProfiler::s_TimingEvents.Broadcast(gpu);
      }
      gpuProducer += xiiTime::Now() - gpuStarted;
      do
      {
        pump();
        const auto guiStarted = xiiTime::Now();
        profiler.Refresh();
        QApplication::processEvents();
        const auto guiElapsed = xiiTime::Now() - guiStarted;
        guiTotal += guiElapsed;
        guiMax = xiiMath::Max(guiMax, guiElapsed);
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
      } while (xiiTime::Now() - started < xiiTime::MakeFromMilliseconds(40));
    }
    if (target != 0)
      XII_TEST_BOOL(deliveries > deliveriesBefore && !profiler.GetSession()->m_Live.m_Gpu.isEmpty());
    const auto after       = service.GetStatistics();
    const auto allocations = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount - allocationsBefore;
    xiiTestFramework::Output(xiiTestOutput::Details,
                             "Profiling %u clients: %.1f ns/CPU scope (%.2f M scopes/s), %.1f ns/GPU retirement event, %u deliveries, %llu encoded bytes, %llu allocations, %llu drops, collection/serialization %.2f/%.2f ms, GUI refresh & events total/max %.2f/%.2f ms",
                             target, cpuProducer.GetNanoseconds() / (Frames * ScopesPerFrame), (Frames * ScopesPerFrame) / cpuProducer.GetMicroseconds(),
                             gpuProducer.GetNanoseconds() / (Frames * GpuSamplesPerFrame), deliveries - deliveriesBefore,
                             static_cast<unsigned long long>(after.m_uiPublishedBytes - before.m_uiPublishedBytes), static_cast<unsigned long long>(allocations),
                             static_cast<unsigned long long>(after.m_uiSamplesDropped - before.m_uiSamplesDropped),
                             (after.m_CollectionTime - before.m_CollectionTime).GetMilliseconds(), (after.m_SerializationTime - before.m_SerializationTime).GetMilliseconds(),
                             guiTotal.GetMilliseconds(), guiMax.GetMilliseconds());
  }
}
