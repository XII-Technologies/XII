/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <signal.h>
#endif

bool xiiEngineProcessCommunicationChannel::IsHostAlive() const
{
  if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return true;

  if (m_iHostPID == 0)
    return false;

  bool bValid = true;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  DWORD  uiPID    = static_cast<DWORD>(m_iHostPID);
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, uiPID);
  bValid          = (hProcess != INVALID_HANDLE_VALUE) && (hProcess != nullptr);

  DWORD uiExitCode = 0;
  if (GetExitCodeProcess(hProcess, &uiExitCode) && uiExitCode != STILL_ACTIVE)
  {
    bValid = false;
  }

  CloseHandle(hProcess);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  // We send the signal 0 to the given PID (signal 0 is a no-op)
  // If this succeeds, the process with the given PID exists
  // if it fails, the process does not / no longer exist.
  if (kill(m_iHostPID, 0) < 0)
  {
    bValid = false;
  }
#else
#  error Not implemented
#endif

  return bValid;
}

xiiResult xiiEngineProcessCommunicationChannel::ConnectToHostProcess()
{
  XII_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");

  if (!xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
  {
    if (xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC").IsEmpty())
    {
      XII_REPORT_FAILURE("Command Line does not contain -IPC parameter");
      return XII_FAILURE;
    }

    if (xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID").IsEmpty())
    {
      XII_REPORT_FAILURE("Command Line does not contain -PID parameter");
      return XII_FAILURE;
    }

    m_iHostPID = 0;
    XII_SUCCEED_OR_RETURN(xiiConversionUtils::StringToInt64(xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID"), m_iHostPID));

    xiiLog::Debug("Host Process ID: {0}", m_iHostPID);

    m_pChannel = xiiIpcChannel::CreatePipeChannel(xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC"), xiiIpcChannel::Mode::Client);
  }
  else
  {
    m_pChannel = xiiIpcChannel::CreateNetworkChannel("localhost:1050", xiiIpcChannel::Mode::Server);
  }

  m_pProtocol = XII_DEFAULT_NEW(xiiIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(xiiMakeDelegate(&xiiProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

  return XII_SUCCESS;
}
