/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Network/NetworkLocalEndpoint.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <algorithm>

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
namespace
{
  class Listener final : public xiiProcessCommunicationChannel
  {
  public:
    using xiiProcessCommunicationChannel::CloseTransport;
    using xiiProcessCommunicationChannel::ConnectLocal;
    using xiiProcessCommunicationChannel::ConnectRemote;
    using xiiProcessCommunicationChannel::SetExpectedProcessId;
    using xiiProcessCommunicationChannel::StartLocalListener;
  };

  xiiString MakeAddress()
  {
    xiiUInt64 low, high;
    xiiUuid::MakeUuid().GetValues(low, high);
    xiiStringBuilder address;
    address.SetFormat("xii-engine-test-{}", xiiArgU(low, 16, true, 16));
    return address;
  }

  xiiProcessOptions MakeOptions(xiiStringView address, xiiOsProcessID host)
  {
    xiiStringBuilder executable(xiiOSFile::GetApplicationDirectory(), "/xiiEditorEngineProcess");
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
    executable.Append(".exe");
#  endif
    xiiProcessOptions options;
    options.m_sProcess           = executable;
    options.m_bHideConsoleWindow = true;
    options.AddArgument("-IPC");
    options.AddArgument(address);
    options.AddArgument("-PID");
    options.AddArgument("{}", host);
    options.AddArgument("-outputDir");
    options.AddArgument(xiiStringBuilder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/EngineShutdown"));
    return options;
  }

  xiiResult Echo(Listener& listener, xiiUInt32 uiId)
  {
    xiiSyncWithProcessMsgToEngine request;
    request.m_uiRedrawCount = uiId;
    if (!listener.SendMessage(&request))
      return XII_FAILURE;
    xiiProcessCommunicationChannel::WaitForMessageCallback response = [uiId](xiiProcessMessage* pMessage) {
      return static_cast<const xiiSyncWithProcessMsgToEditor*>(pMessage)->m_uiRedrawCount == uiId;
    };
    return listener.WaitForMessage(xiiGetStaticRTTI<xiiSyncWithProcessMsgToEditor>(), xiiTime::MakeFromSeconds(5), &response);
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EngineProcessLifecycle);
XII_CREATE_SIMPLE_TEST(EngineProcessLifecycle, HostLostBeforeProjectSetup)
{
  xiiProcess process;
  if (!XII_TEST_RESULT(process.Launch(MakeOptions(MakeAddress(), 0))) ||
      !XII_TEST_RESULT(process.WaitToFinish(xiiTime::MakeFromSeconds(30))))
    return;
  XII_TEST_INT(process.GetExitCode(), 1);
}

XII_CREATE_SIMPLE_TEST(EngineProcessLifecycle, ConnectedHostLoss)
{
  const auto              address = MakeAddress();
  xiiNetworkLocalEndpoint endpoint;
  Listener                listener;
  if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(address, endpoint).Succeeded()) ||
      !XII_TEST_RESULT(listener.StartLocalListener(endpoint, xiiProcessMessageDomain::Engine)))
    return;
  xiiProcess process;
  if (!XII_TEST_RESULT(process.Launch(MakeOptions(address, xiiProcess::GetCurrentProcessID()))))
    return;
  listener.SetExpectedProcessId(static_cast<xiiUInt32>(process.GetProcessID()));
  const auto negotiationStart = xiiTime::Now();
  if (!XII_TEST_RESULT(listener.WaitForConnection(xiiTime::MakeFromSeconds(10))))
    return;

  const auto negotiation = xiiTime::Now() - negotiationStart;
  XII_TEST_RESULT(Echo(listener, 17));
  if (xiiCommandLineUtils::GetGlobalInstance()->HasOption("-editorIpcBenchmark"))
  {
    double latency[128];
    for (xiiUInt32 i = 0; i < 128; ++i)
    {
      const auto started = xiiTime::Now();
      if (!XII_TEST_RESULT(Echo(listener, i + 18)))
        return;
      latency[i] = (xiiTime::Now() - started).GetMicroseconds();
    }
    std::sort(std::begin(latency), std::end(latency));
    xiiTestFramework::Output(xiiTestOutput::Details, "Editor IPC: %.3f ms ready, reflected redraw echo p50/p99 %.1f/%.1f us",
                             negotiation.GetMilliseconds(), latency[63], latency[126]);
  }

  listener.CloseTransport();
  if (!XII_TEST_RESULT(process.WaitToFinish(xiiTime::MakeFromSeconds(30))))
    return;
  XII_TEST_INT(process.GetExitCode(), 0);
}

XII_CREATE_SIMPLE_TEST(EngineProcessLifecycle, IndependentChildren)
{
  Listener   listeners[2];
  xiiProcess children[2];
  const auto parent = xiiProcess::GetCurrentProcessID();
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto              name = MakeAddress();
    xiiNetworkLocalEndpoint endpoint;
    if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(name, endpoint).Succeeded()) ||
        !XII_TEST_RESULT(listeners[i].StartLocalListener(endpoint, xiiProcessMessageDomain::Engine)) ||
        !XII_TEST_RESULT(children[i].Launch(MakeOptions(name, parent))))
      return;
    listeners[i].SetExpectedProcessId(static_cast<xiiUInt32>(children[i].GetProcessID()));
  }
  for (xiiUInt32 i = 0; i < 2; ++i)
    if (!XII_TEST_RESULT(listeners[i].WaitForConnection(xiiTime::MakeFromSeconds(10))) || !XII_TEST_RESULT(Echo(listeners[i], 42 + i)))
      return;

  if (!XII_TEST_RESULT(children[0].Terminate()))
    return;
  XII_TEST_BOOL(listeners[0].WaitForMessage(xiiGetStaticRTTI<xiiSyncWithProcessMsgToEditor>(), xiiTime::MakeFromSeconds(5)).Failed());
  XII_TEST_BOOL(!listeners[0].IsConnected());
  XII_TEST_RESULT(Echo(listeners[1], 43));
  listeners[1].CloseTransport();
  if (XII_TEST_RESULT(children[1].WaitToFinish(xiiTime::MakeFromSeconds(10))))
    XII_TEST_INT(children[1].GetExitCode(), 0);
}

XII_CREATE_SIMPLE_TEST(EngineProcessLifecycle, AuthenticatedRemoteEngineProcess)
{
  auto*           command      = xiiCommandLineUtils::GetGlobalInstance();
  const xiiString certificates = command->GetStringOption("-editor_test_certs");
  if (certificates.IsEmpty())
    return;
  const xiiString transport = command->GetStringOption("-editor_test_transport", 0, "tls");
  auto            original  = command->GetCommandLineArray();
  XII_SCOPE_EXIT(command->SetCommandLine(original.GetArrayPtr()));
  const xiiStringBuilder ca(certificates, "/ca.pem");
  command->InjectCustomArgument("-ipc_remote_transport");
  command->InjectCustomArgument(transport);
  command->InjectCustomArgument("-ipc_cert");
  command->InjectCustomArgument(xiiStringBuilder(certificates, "/host.pem"));
  command->InjectCustomArgument("-ipc_key");
  command->InjectCustomArgument(xiiStringBuilder(certificates, "/host.key"));
  command->InjectCustomArgument("-ipc_ca");
  command->InjectCustomArgument(ca);
  command->InjectCustomArgument("-ipc_server_name");
  command->InjectCustomArgument("localhost");

  xiiStringBuilder address;
  address.SetFormat("127.0.0.1:{}", 25000 + static_cast<xiiUInt32>(xiiProcess::GetCurrentProcessID()) % 20000);
  auto options = MakeOptions({}, xiiProcess::GetCurrentProcessID());
  options.AddArgument("-remote");
  options.AddArgument("-ipc_remote_transport");
  options.AddArgument(transport);
  options.AddArgument("-ipc_remote_listen");
  options.AddArgument(address);
  options.AddArgument("-ipc_cert");
  options.AddArgument(xiiStringBuilder(certificates, "/device.pem"));
  options.AddArgument("-ipc_key");
  options.AddArgument(xiiStringBuilder(certificates, "/device.key"));
  options.AddArgument("-ipc_ca");
  options.AddArgument(ca);

  xiiProcess process;
  if (!XII_TEST_RESULT(process.Launch(options)))
    return;
  Listener   peer;
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(10);
  while (process.GetState() == xiiProcessState::Running && xiiTime::Now() < deadline)
  {
    if (peer.ConnectRemote(address).Succeeded() && peer.WaitForConnection(xiiTime::MakeFromSeconds(1)).Succeeded())
      break;
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(20));
  }
  if (!XII_TEST_BOOL(peer.IsConnected()) || !XII_TEST_RESULT(Echo(peer, 29)))
    return;
  xiiShutdownProcessMsgToEngine shutdown;
  if (!XII_TEST_BOOL(peer.SendMessage(&shutdown)) || !XII_TEST_RESULT(process.WaitToFinish(xiiTime::MakeFromSeconds(10))))
    return;
  XII_TEST_INT(process.GetExitCode(), 0);
}

XII_CREATE_SIMPLE_TEST(EngineProcessLifecycle, ReflectedContractsSurviveRegistryRefresh)
{
  xiiNetworkLocalEndpoint endpoint;
  Listener                server, client;
  const auto              uiProcessId = static_cast<xiiUInt32>(xiiProcess::GetCurrentProcessID());
  if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(MakeAddress(), endpoint).Succeeded()) ||
      !XII_TEST_RESULT(server.StartLocalListener(endpoint, xiiProcessMessageDomain::Engine)) ||
      !XII_TEST_RESULT(client.ConnectLocal(endpoint, xiiProcessMessageDomain::Engine, uiProcessId)))
    return;
  server.SetExpectedProcessId(uiProcessId);
  auto pumpUntil = [&](auto&& done) {
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    while (xiiTime::Now() < deadline)
    {
      server.ProcessMessages();
      client.ProcessMessages();
      if (done())
        return true;
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
    }
    return false;
  };
  auto connected = [&] {
    return server.IsConnected() && client.IsConnected();
  };
  if (!XII_TEST_BOOL(pumpUntil(connected)))
    return;

  xiiPlugin::BeginPluginChanges();
  xiiSyncWithProcessMsgToEngine pausedMessage;
  const bool                    bAcceptedDuringPluginChange = client.SendMessage(&pausedMessage);
  xiiPlugin::EndPluginChanges();
  XII_TEST_BOOL(!bAcceptedDuringPluginChange);
  if (!XII_TEST_BOOL(pumpUntil(connected)))
    return;

  bool                                                                 bReceived = false;
  xiiEvent<const xiiProcessCommunicationChannel::Event&>::Unsubscriber subscription;
  server.m_Events.AddEventHandler([&](const xiiProcessCommunicationChannel::Event& event) {
    if (const auto* message = xiiDynamicCast<const xiiUpdateReflectionTypeMsgToEditor*>(event.m_pMessage))
    {
      bReceived = true;
      XII_TEST_STRING(message->m_desc.m_sTypeName, "Phase6ReflectionFixture");
      if (XII_TEST_INT(message->m_desc.m_Properties.GetCount(), 1))
      {
        const auto& property = message->m_desc.m_Properties[0];
        XII_TEST_STRING(property.m_sName, "Exposure");
        if (XII_TEST_INT(property.GetCount(), 1))
          XII_TEST_BOOL(property.GetValue(0)->GetDynamicRTTI() == xiiGetStaticRTTI<xiiReadOnlyAttribute>());
      }
    }
  },
                                  subscription);
  xiiUpdateReflectionTypeMsgToEditor descriptor;
  descriptor.m_desc.m_sTypeName   = "Phase6ReflectionFixture";
  auto& property                  = descriptor.m_desc.m_Properties.ExpandAndGetRef();
  property.m_sName                = "Exposure";
  property.m_sType                = "float";
  property.m_Category             = xiiPropertyCategory::Member;
  property.m_Flags                = xiiPropertyFlags::StandardType;
  xiiReadOnlyAttribute* attribute = xiiGetStaticRTTI<xiiReadOnlyAttribute>()->GetAllocator()->Allocate<xiiReadOnlyAttribute>();
  property.Insert(0, attribute);
  if (XII_TEST_BOOL(client.SendMessage(&descriptor)))
    XII_TEST_BOOL(pumpUntil([&] { return bReceived; }));
}
#endif
