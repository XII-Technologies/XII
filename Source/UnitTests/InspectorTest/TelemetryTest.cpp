/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#include <Inspector/ProfilingWidget.h>
#include <Inspector/RenderGraphWidget.h>
#include <QApplication>
#include <QTreeWidget>
#include <TestFramework/Framework/TestFramework.h>
#include <ads/DockManager.h>

// Separate process exercises the real telemetry protocol and dynamically loaded producer plugin.
// GPU command execution and timestamp readback are covered by GpuExecutionAndGraphIsolation.
int RunInspectorCaptureServer()
{
  if (xiiPlugin::LoadPlugin("xiiInspectorPlugin").Failed())
    return 1;
  xiiTelemetry::s_uiPort = static_cast<xiiUInt16>(xiiCommandLineUtils::GetGlobalInstance()->GetIntOption("-capture-port", 24731));
  xiiTelemetry::CreateServer();
  xiiRenderGraph graph("Telemetry fixture");
  for (xiiUInt64 frame = 1; frame <= 500; ++frame)
  {
    graph.BeginSetup(frame);
    struct Data
    {
      XII_DECLARE_POD_TYPE();
    };
    graph.AddPass<Data>("Telemetry rendering pass", xiiGALCommandQueueFlags::Graphics, [](Data&, xiiRenderGraphBuilder&) {}, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    graph.EndSetup();
    if (graph.Compile().Failed())
      return 2;
    xiiProfilingSystem::StartNewFrame();
    const auto now = xiiTime::Now();
    xiiProfilingSystem::AddCPUScope("Rendering telemetry fixture", "RunInspectorCaptureServer", now - xiiTime::MakeFromMilliseconds(1), now, {});
    xiiTelemetry::PerFrameUpdate();
    xiiRenderGraphDebug::s_ExecutionEvents.Broadcast({graph, nullptr, frame, true, {}});
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }
  xiiTelemetry::CloseConnection();
  xiiPlugin::UnloadAllPlugins();
  return 0;
}

XII_CREATE_SIMPLE_TEST(Inspector, PluginTelemetryRoundTrip)
{
  const xiiUInt32   port = 24000 + xiiProcess::GetCurrentProcessID() % 10000;
  xiiProcessOptions options;
  options.m_sProcess           = xiiOSFile::GetApplicationPath();
  options.m_bHideConsoleWindow = true;
  options.AddArgument("-capture-server");
  options.AddArgument("-capture-port");
  options.AddArgument("{}", port);
  xiiProcess server;
  if (!XII_TEST_RESULT(server.Launch(options)))
    return;
  ads::CDockManager      manager;
  xiiQtRenderGraphWidget graph(&manager);
  xiiQtProfilingWidget   profile(&manager);
  xiiTelemetry::AcceptMessagesForSystem('RGRF', true, xiiQtRenderGraphWidget::ProcessTelemetry);
  xiiTelemetry::AcceptMessagesForSystem('PROF', true, xiiQtProfilingWidget::ProcessTelemetry);
  XII_SCOPE_EXIT(xiiTelemetry::CloseConnection(); xiiTelemetry::AcceptMessagesForSystem('RGRF', false); xiiTelemetry::AcceptMessagesForSystem('PROF', false));
  xiiStringBuilder address;
  address.SetFormat("localhost:{}", port);
  XII_TEST_RESULT(xiiTelemetry::ConnectToServer(address));
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
  while (!xiiTelemetry::IsConnectedToServer() && xiiTime::Now() < deadline)
  {
    xiiTelemetry::PerFrameUpdate();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }
  if (!XII_TEST_BOOL(xiiTelemetry::IsConnectedToServer()))
    return;
  auto* passes   = graph.findChild<QTreeWidget*>("Graph passes");
  auto* hotspots = profile.findChild<QTreeWidget*>("Profiling hotspots");
  XII_TEST_INT(passes->topLevelItemCount(), 0); // Producer sends no graph data before a request.
  graph.RequestCapture();
  profile.RequestCapture();
  while ((passes->topLevelItemCount() == 0 || hotspots->topLevelItemCount() == 0) && xiiTime::Now() < deadline)
  {
    xiiTelemetry::PerFrameUpdate();
    QApplication::processEvents();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }
  XII_TEST_INT(passes->topLevelItemCount(), 1);
  XII_TEST_BOOL(hotspots->topLevelItemCount() > 0);
  XII_TEST_BOOL(passes->topLevelItem(0) && passes->topLevelItem(0)->text(1) == "Telemetry rendering pass");
}
