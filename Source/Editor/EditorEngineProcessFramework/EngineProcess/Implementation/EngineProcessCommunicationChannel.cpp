/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <Foundation/Network/NetworkLocalEndpoint.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#else
#  include <signal.h>
#endif

void xiiEngineProcessCommunicationChannel::CloseConnection()
{
  CloseTransport();
  m_iHostPID = 0;
}

bool xiiEngineProcessCommunicationChannel::IsHostAlive() const
{
  if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return HasTransport();

  if (m_iHostPID == 0 || !HasPendingConnection())
    return false;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(m_iHostPID));
  if (process == nullptr)
    return false;
  DWORD      uiExitCode = 0;
  const bool bAlive     = GetExitCodeProcess(process, &uiExitCode) && uiExitCode == STILL_ACTIVE;
  CloseHandle(process);
  return bAlive;
#else
  return kill(static_cast<pid_t>(m_iHostPID), 0) == 0;
#endif
}

xiiResult xiiEngineProcessCommunicationChannel::ConnectToHostProcess(xiiProcessMessageDomain domain)
{
  const auto& command = *xiiCommandLineUtils::GetGlobalInstance();
  if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return StartRemoteListener(command.GetStringOption("-ipc_remote_listen", 0, "127.0.0.1:1050"));

  xiiUInt32   uiProcessId = 0;
  const auto  sProcessId  = command.GetStringOption("-PID");
  const char* pEnd        = nullptr;
  if (sProcessId.IsEmpty() || xiiConversionUtils::StringToUInt(sProcessId, uiProcessId, &pEnd).Failed() ||
      pEnd != sProcessId.GetEndPointer() || uiProcessId == 0 || uiProcessId > static_cast<xiiUInt32>(xiiMath::MaxValue<xiiInt32>()))
  {
    xiiLog::Error("EngineProcess requires a valid parent process ID.");
    return XII_FAILURE;
  }

  xiiNetworkLocalEndpoint endpoint;
  XII_SUCCEED_OR_RETURN(xiiNetworkLocalEndpoint::Make(command.GetStringOption("-IPC"), endpoint).GetResult());
  XII_SUCCEED_OR_RETURN(ConnectLocal(endpoint, domain, uiProcessId));
  m_iHostPID = uiProcessId;
  return XII_SUCCESS;
}
