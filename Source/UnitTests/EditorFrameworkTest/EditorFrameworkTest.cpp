/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Network/NetworkLocalEndpoint.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/GameApplication/PlayerSessionProtocol.h>
#include <TestFramework/Utilities/TestSetup.h>

// Production asset plugins resolve application data during subsystem startup.
// Keep a real application alive for the entire test and shutdown lifecycle.
class EditorFrameworkTestApplication : public xiiApplication
{
public:
  EditorFrameworkTestApplication() : xiiApplication("EditorFrameworkTest"), m_pEditorApp(new xiiQtEditorApp) {}

  xiiResult BeforeCoreSystemsStartup() override
  {
    m_pEditorApp->InitQt(GetArgumentCount(), const_cast<char**>(GetArgumentsArray()));
    return xiiApplication::BeforeCoreSystemsStartup();
  }

  void AfterCoreSystemsShutdown() override
  {
    m_pEditorApp->DeInitQt();
    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  Execution Run() override
  {
    // The test executable can act as a child process without creating a GPU device.
    auto* pCommand = xiiCommandLineUtils::GetGlobalInstance();
    if (pCommand->HasOption("-playerSessionIgnoreStopPeer"))
    {
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
      return Execution::Continue;
    }
    if (pCommand->HasOption("-playerSessionCleanExitPeer"))
    {
      SetReturnCode(RunPlayerSessionPeer(*pCommand).Succeeded() ? 0 : 1);
      return Execution::Quit;
    }
    if (pCommand->HasOption("-playerSessionExitPeer"))
    {
      SetReturnCode(23);
      return Execution::Quit;
    }
    if (pCommand->HasOption("-splashIpcTestPeer"))
    {
      class Peer final : public xiiProcessCommunicationChannel
      {
      public:
        using xiiProcessCommunicationChannel::ConnectLocal;
      };
      Peer                       peer;
      xiiNetworkLocalEndpoint    endpoint;
      xiiProjectReadyMsgToEditor response;
      const bool                 bSent = xiiNetworkLocalEndpoint::Make(pCommand->GetStringOption("-IPC"), endpoint).Succeeded() &&
        peer.ConnectLocal(endpoint, xiiProcessMessageDomain::Engine, pCommand->GetUIntOption("-PID")).Succeeded() &&
        peer.WaitForConnection(xiiTime::MakeFromSeconds(5)).Succeeded() && peer.SendMessage(&response);
      const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
      while (bSent && peer.IsConnected() && xiiTime::Now() < deadline)
        peer.WaitForMessages();
      SetReturnCode(bSent ? 0 : 1);
      return Execution::Quit;
    }
    xiiTestSetup::InitTestFramework("EditorFrameworkTest", "Editor Framework Tests", GetArgumentCount(), GetArgumentsArray());
    while (xiiTestSetup::RunTests() == xiiTestAppRun::Continue)
    {
    }
    SetReturnCode(xiiTestSetup::GetFailedTestCount());
    xiiTestSetup::DeInitTestFramework();
    return Execution::Quit;
  }

private:
  static xiiResult RunPlayerSessionPeer(const xiiCommandLineUtils& command)
  {
    xiiPlayerSessionProtocol protocol;
    XII_SUCCEED_OR_RETURN(protocol.StartPlayer([](xiiNetworkChannel, const xiiPlayerSceneMessage&) {}, [](xiiNetworkChannel, const xiiPlayerStopMessage&) {}));
    xiiNetworkLocalEndpoint endpoint;
    xiiNetworkConnection    connection;
    XII_SUCCEED_OR_RETURN(xiiNetworkLocalEndpoint::Make(command.GetStringOption("-player-ipc"), endpoint).GetResult());
    XII_SUCCEED_OR_RETURN(protocol.GetHost().Connect(endpoint, {}, connection).GetResult());

    xiiNetworkChannel channel;
    xiiNetworkEvent   event;
    const auto        deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    while (!protocol.IsReady(channel))
    {
      const auto remaining = deadline - xiiTime::Now();
      if (!remaining.IsPositive() || protocol.GetHost().WaitForEvent(event, remaining).Failed() || event.m_Type == xiiNetworkEventType::Closed)
        return XII_FAILURE;
      if (event.m_Type == xiiNetworkEventType::Connected)
      {
        if (event.m_Connection != connection || event.m_pSessionInfo == nullptr ||
            event.m_pSessionInfo->m_ConnectionInfo.m_uiLocalPeerProcessId != command.GetUIntOption("-player-editor-pid"))
          return XII_FAILURE;
        channel = event.m_pSessionInfo->m_DefaultChannel;
      }
      XII_SUCCEED_OR_RETURN(protocol.ProcessEvent(std::move(event)));
    }

    xiiPlayerStatusMessage message;
    message.m_sInstance = command.GetStringOption("-player-instance");
    message.m_sStage    = "running";
    XII_SUCCEED_OR_RETURN(protocol.Send(channel, message).GetResult());
    xiiNetworkMessage submission;
    message.m_sStage = "stopping";
    XII_SUCCEED_OR_RETURN(protocol.Send(channel, message, &submission).GetResult());
    return protocol.WaitForFinalStatus(submission, xiiTime::MakeFromSeconds(2));
  }

  xiiQtEditorApp* m_pEditorApp;
};

XII_CONSOLEAPP_ENTRY_POINT(EditorFrameworkTestApplication);
