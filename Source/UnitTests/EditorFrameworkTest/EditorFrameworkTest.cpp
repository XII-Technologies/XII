/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Utilities/CommandLineUtils.h>
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
    // A real pipe/protocol peer exercises startup response delivery without creating a GPU
    // device or adding a simulated operation to the Editor. Only the test executable uses it.
    auto* pCommand = xiiCommandLineUtils::GetGlobalInstance();
    if (pCommand->HasOption("-splashIpcTestPeer"))
    {
      xiiUniquePtr<xiiIpcChannel>                                 peer = xiiIpcChannel::CreatePipeChannel(pCommand->GetStringOption("-IPC"), xiiIpcChannel::Mode::Client);
      xiiThreadSignal                                             connected;
      xiiThreadSignal                                             disconnected;
      xiiEvent<const xiiIpcChannelEvent&, xiiMutex>::Unsubscriber subscription;
      peer->m_Events.AddEventHandler([&](const xiiIpcChannelEvent& e) {
        if (e.m_Type == xiiIpcChannelEvent::Connected)
          connected.RaiseSignal();
        else if (e.m_Type == xiiIpcChannelEvent::Disconnected)
          disconnected.RaiseSignal();
      },
                                     subscription);
      xiiIpcProcessMessageProtocol protocol(peer.Borrow());
      peer->Connect();
      if (!peer->IsConnected())
        connected.WaitForSignal(xiiTime::MakeFromSeconds(5));
      xiiProjectReadyMsgToEditor response;
      const bool                 bSent = peer->IsConnected() && protocol.Send(&response);
      if (bSent)
        disconnected.WaitForSignal(xiiTime::MakeFromSeconds(5));
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
  xiiQtEditorApp* m_pEditorApp;
};

XII_CONSOLEAPP_ENTRY_POINT(EditorFrameworkTestApplication);
