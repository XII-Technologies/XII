/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

#if XII_ENABLED(XII_SUPPORTS_PROCESSES) && (XII_ENABLED(XII_PLATFORM_WINDOWS) || XII_ENABLED(XII_PLATFORM_LINUX))

namespace
{
  xiiString MakeAddress()
  {
    xiiStringBuilder address;
    xiiConversionUtils::ToString(xiiUuid::MakeUuid(), address);
    return address;
  }

  xiiProcessOptions MakeOptions(xiiStringView address, xiiOsProcessID host)
  {
    xiiStringBuilder executable(xiiOSFile::GetApplicationDirectory(), "/xiiEditorEngineProcess");
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
    executable.Append(".exe");
#  endif
    xiiProcessOptions options;
    options.m_sProcess = executable;
    options.AddArgument("-IPC");
    options.AddArgument(address);
    options.AddArgument("-PID");
    options.AddArgument("{}", host);
    options.AddArgument("-outputDir");
    options.AddArgument(xiiStringBuilder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/EngineShutdown"));
    return options;
  }

  bool WaitForConnection(const xiiIpcChannel& channel, const xiiProcess& process)
  {
    const auto start = xiiTime::Now();
    while (!channel.IsConnected() && process.GetState() == xiiProcessState::Running && xiiTime::Now() - start < xiiTime::MakeFromSeconds(10))
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    return XII_TEST_BOOL(channel.IsConnected());
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EngineProcessLifecycle);
XII_CREATE_SIMPLE_TEST(EngineProcessLifecycle, HostLostBeforeProjectSetup)
{
  const auto                  address = MakeAddress();
  xiiUniquePtr<xiiIpcChannel> server  = xiiIpcChannel::CreatePipeChannel(address, xiiIpcChannel::Mode::Server);
  if (!XII_TEST_BOOL(server != nullptr))
    return;
  server->Connect();

  // PID zero is explicitly treated as a missing host. Exercise normal application
  // teardown with its IPC member still alive after xiiRun_Shutdown returns.
  xiiProcess process;
  if (!XII_TEST_RESULT(process.Launch(MakeOptions(address, 0))))
    return;
  if (!XII_TEST_RESULT(process.WaitToFinish(xiiTime::MakeFromSeconds(30))))
    return;
  XII_TEST_INT(process.GetExitCode(), 0);
}

XII_CREATE_SIMPLE_TEST(EngineProcessLifecycle, ConnectedHostLoss)
{
  const auto                  hostAddress = MakeAddress(), address = MakeAddress();
  xiiUniquePtr<xiiIpcChannel> hostServer = xiiIpcChannel::CreatePipeChannel(hostAddress, xiiIpcChannel::Mode::Server);
  xiiUniquePtr<xiiIpcChannel> server     = xiiIpcChannel::CreatePipeChannel(address, xiiIpcChannel::Mode::Server);
  if (!XII_TEST_BOOL(hostServer != nullptr && server != nullptr))
    return;
  hostServer->Connect();
  server->Connect();

  // Use a second engine as the host identity so only test-owned processes are
  // terminated. Both channels must be established before simulating host loss.
  xiiProcess host, process;
  if (!XII_TEST_RESULT(host.Launch(MakeOptions(hostAddress, xiiProcess::GetCurrentProcessID()))) || !WaitForConnection(*hostServer, host))
    return;
  if (!XII_TEST_RESULT(process.Launch(MakeOptions(address, host.GetProcessID()))) || !WaitForConnection(*server, process))
    return;
  if (!XII_TEST_RESULT(host.Terminate()))
    return;
  server->Disconnect();
  if (!XII_TEST_RESULT(process.WaitToFinish(xiiTime::MakeFromSeconds(30))))
    return;
  XII_TEST_INT(process.GetExitCode(), 0);
}

#endif
