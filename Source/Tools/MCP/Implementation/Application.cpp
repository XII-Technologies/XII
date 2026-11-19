/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCP/MCPPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <MCP/Application.h>
#include <MCPPlugin/Json.h>

#include <cstdio>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <fcntl.h>
#  include <io.h>
#else
#  include <cerrno>
#  include <csignal>
#  include <poll.h>
#  include <unistd.h>
#endif

namespace
{
  enum class xiiMCPInputState
  {
    Idle,
    Available,
    Closed
  };

  std::FILE* OpenProtocolOutput()
  {
    std::fflush(stdout);
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    const int iProtocol = _dup(_fileno(stdout));
    if (iProtocol < 0)
    {
      return nullptr;
    }
    if (_dup2(_fileno(stderr), _fileno(stdout)) < 0)
    {
      _close(iProtocol);
      return nullptr;
    }
    auto* pOutput = _fdopen(iProtocol, "wb");
    if (pOutput == nullptr)
    {
      _close(iProtocol);
    }
#else
    const int iProtocol = dup(STDOUT_FILENO);
    if (iProtocol < 0)
    {
      return nullptr;
    }
    if (dup2(STDERR_FILENO, STDOUT_FILENO) < 0)
    {
      close(iProtocol);
      return nullptr;
    }
    std::signal(SIGPIPE, SIG_IGN);
    auto* pOutput = fdopen(iProtocol, "wb");
    if (pOutput == nullptr)
    {
      close(iProtocol);
    }
#endif
    return pOutput;
  }

  xiiMCPInputState ReadInput(xiiArrayPtr<char> out_input, xiiUInt32& out_uiCount)
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    const HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (GetFileType(hInput) == FILE_TYPE_PIPE)
    {
      DWORD uiAvailable = 0;
      if (!PeekNamedPipe(hInput, nullptr, 0, nullptr, &uiAvailable, nullptr))
      {
        return xiiMCPInputState::Closed;
      }
      if (uiAvailable == 0)
      {
        return xiiMCPInputState::Idle;
      }
      DWORD uiRead = 0;
      if (!ReadFile(hInput, out_input.GetPtr(), xiiMath::Min(static_cast<DWORD>(out_input.GetCount()), uiAvailable), &uiRead, nullptr))
      {
        return xiiMCPInputState::Closed;
      }
      out_uiCount = uiRead;
      return xiiMCPInputState::Available;
    }
    if (GetFileType(hInput) != FILE_TYPE_DISK)
    {
      return xiiMCPInputState::Closed;
    }
    const int iRead = _read(_fileno(stdin), out_input.GetPtr(), out_input.GetCount());
#else
    pollfd    input{STDIN_FILENO, POLLIN, 0};
    const int iReady = poll(&input, 1, 0);
    if (iReady == 0 || (iReady < 0 && errno == EINTR))
    {
      return xiiMCPInputState::Idle;
    }
    if (iReady < 0)
    {
      return xiiMCPInputState::Closed;
    }
    const auto iRead = read(STDIN_FILENO, out_input.GetPtr(), out_input.GetCount());
    if (iRead < 0 && (errno == EINTR || errno == EAGAIN))
    {
      return xiiMCPInputState::Idle;
    }
#endif
    if (iRead <= 0)
    {
      return xiiMCPInputState::Closed;
    }
    out_uiCount = static_cast<xiiUInt32>(iRead);
    return xiiMCPInputState::Available;
  }

  bool WriteFrame(std::FILE* pOutput, xiiStringView sMessage)
  {
    return std::fwrite(sMessage.GetStartPointer(), 1, sMessage.GetElementCount(), pOutput) == sMessage.GetElementCount() &&
      std::fputc('\n', pOutput) != EOF && std::fflush(pOutput) == 0;
  }
} // namespace

xiiMCPApplication::xiiMCPApplication() :
  xiiApplication("xiiMCP")
{
}

xiiMCPApplication::~xiiMCPApplication()
{
  if (m_pProtocolOutput != nullptr)
  {
    std::fclose(m_pProtocolOutput);
  }
}

xiiResult xiiMCPApplication::BeforeCoreSystemsStartup()
{
  const auto* pOptions = xiiCommandLineUtils::GetGlobalInstance();
  if (pOptions->GetBoolOption("-help"))
  {
    std::fputs("xiiMCP launches its graphical workspace in Qt builds when started without arguments.\n"
               "xiiMCP [-MCPTransport http|stdio] [-MCPListen 127.0.0.1:8372] [-MCPTarget NAME ...]\n"
               "       [-MCPNoDiscovery] [-MCPAllowCapture] [-MCPAllowEdit] [-MCPAllowExecute] [-MCPTimeoutMs 5000] [-gui]\n"
               "       Set XII_MCP_TOKEN to require Bearer authentication for HTTP.\n",
               stderr);
    RequestQuit();
    return XII_SUCCESS;
  }

  const auto sTransport = pOptions->GetStringOption("-MCPTransport", 0, "http");
  m_bStdio              = sTransport == "stdio";
  m_bGui                = pOptions->GetBoolOption("-gui");
#ifdef XII_USE_QT
  m_Configuration.m_bRestorePreferences = pOptions->GetParameterCount() <= 1;
  m_bGui                                = m_bGui || m_Configuration.m_bRestorePreferences;
#endif
  if ((m_bStdio && m_bGui) || (!m_bStdio && sTransport != "http"))
  {
    std::fputs("Select HTTP or stdio. The GUI uses HTTP.\n", stderr);
    SetReturnCode(1);
    return XII_FAILURE;
  }
#ifndef XII_USE_QT
  if (m_bGui)
  {
    std::fputs("This build has no Qt support.\n", stderr);
    SetReturnCode(1);
    return XII_FAILURE;
  }
#endif

  if (m_bStdio)
  {
    m_StdioSession.m_bListNotifications = true;
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    if (_setmode(_fileno(stdin), _O_BINARY) == -1)
    {
      std::fputs("Could not configure binary MCP stdio streams.\n", stderr);
      SetReturnCode(1);
      return XII_FAILURE;
    }
#endif
    m_pProtocolOutput = OpenProtocolOutput();
    if (m_pProtocolOutput == nullptr)
    {
      std::fputs("Could not open the MCP protocol output stream.\n", stderr);
      SetReturnCode(1);
      return XII_FAILURE;
    }
  }
  else
  {
    const auto sEndpoint = pOptions->GetStringOption("-MCPListen", 0, "127.0.0.1:8372");
    if (xiiNetworkEndpoint::Parse(sEndpoint, m_Configuration.m_Http.m_Endpoint).Failed())
    {
      std::fputs("Invalid MCP HTTP endpoint.\n", stderr);
      SetReturnCode(1);
      return XII_FAILURE;
    }
  }

  const auto iTimeoutMs = pOptions->GetIntOption("-MCPTimeoutMs", 5000);
  if (iTimeoutMs < 100 || iTimeoutMs > 10000)
  {
    std::fputs("MCP request timeout must be between 100 and 10000 ms.\n", stderr);
    SetReturnCode(1);
    return XII_FAILURE;
  }

  m_Configuration.m_Http.m_RequestTimeout = xiiTime::MakeFromMilliseconds(iTimeoutMs);
  m_Configuration.m_bDiscovery            = !pOptions->GetBoolOption("-MCPNoDiscovery");
  for (xiiUInt32 uiIndex = 0; uiIndex < pOptions->GetStringOptionArguments("-MCPTarget"); ++uiIndex)
  {
    m_Configuration.m_Endpoints.PushBack(xiiString(pOptions->GetStringOption("-MCPTarget", uiIndex)));
  }
  if (pOptions->GetBoolOption("-MCPAllowEdit"))
  {
    m_Configuration.m_uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Edit);
  }
  if (pOptions->GetBoolOption("-MCPAllowCapture"))
  {
    m_Configuration.m_uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Capture);
  }
  if (pOptions->GetBoolOption("-MCPAllowExecute"))
  {
    m_Configuration.m_uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Execute);
  }
  m_Configuration.m_Http.m_uiPermissions = m_Configuration.m_uiPermissions;
  m_Configuration.m_Http.m_sBearerToken  = xiiEnvironmentVariableUtils::GetValueString("XII_MCP_TOKEN");
  return XII_SUCCESS;
}

void xiiMCPApplication::AfterCoreSystemsStartup()
{
  if (m_bGui || WasQuitRequested())
  {
    return;
  }

  m_pGateway        = XII_DEFAULT_NEW(xiiMCPGateway);
  const auto status = m_pGateway->Start(m_Configuration);
  if (status.Failed())
  {
    std::fprintf(stderr, "xiiMCP: %s\n", status.GetMessageString().GetData());
    SetReturnCode(1);
    RequestQuit();
    m_pGateway.Clear();
    return;
  }

  if (!m_bStdio)
  {
    xiiStringBuilder sEndpoint;
    m_pGateway->GetEndpoint().ToString(sEndpoint);
    std::fprintf(stderr, "xiiMCP listening at http://%s/mcp\n", sEndpoint.GetData());
  }
}

void xiiMCPApplication::BeforeCoreSystemsShutdown()
{
  m_pGateway.Clear();
}

xiiApplication::Execution xiiMCPApplication::Run()
{
#ifdef XII_USE_QT
  if (m_bGui)
  {
    SetReturnCode(RunMCPWindow(m_Configuration));
    return Execution::Quit;
  }
#endif
  if (!m_pGateway)
  {
    return Execution::Quit;
  }

  m_pGateway->Update();
  if (m_bStdio)
  {
    const auto uiRevision = m_pGateway->GetCapabilityRevision();
    if (m_StdioSession.m_State == xiiMCPSession::State::Ready && m_uiLastCapabilityRevision != 0 && uiRevision != m_uiLastCapabilityRevision)
    {
      if (!WriteFrame(m_pProtocolOutput, R"({"jsonrpc":"2.0","method":"notifications/tools/list_changed"})") ||
          !WriteFrame(m_pProtocolOutput, R"({"jsonrpc":"2.0","method":"notifications/resources/list_changed"})") ||
          !WriteFrame(m_pProtocolOutput, R"({"jsonrpc":"2.0","method":"notifications/prompts/list_changed"})"))
      {
        return Execution::Quit;
      }
    }
    m_uiLastCapabilityRevision = uiRevision;

    char       input[4096];
    xiiUInt32  uiCount = 0;
    const auto state   = ReadInput(input, uiCount);
    if (state == xiiMCPInputState::Closed)
    {
      return Execution::Quit;
    }

    for (xiiUInt32 uiIndex = 0; uiIndex < uiCount; ++uiIndex)
    {
      const char character = input[uiIndex];
      if (character != '\n')
      {
        if (m_Input.GetCount() == xiiMCPJson::DefaultMaximumBytes)
        {
          m_bDiscardLine = true;
        }
        if (!m_bDiscardLine)
        {
          m_Input.PushBack(character);
        }
        continue;
      }
      xiiStringBuilder sReply;
      if (m_bDiscardLine)
      {
        sReply = R"({"jsonrpc":"2.0","id":null,"error":{"code":-32700,"message":"MCP frame exceeded the request limit."}})";
      }
      else
      {
        m_pGateway->Process(xiiStringView(m_Input.GetData(), m_Input.GetCount()), m_StdioSession, sReply);
      }
      m_Input.Clear();
      m_bDiscardLine = false;
      if (!sReply.IsEmpty() && !WriteFrame(m_pProtocolOutput, sReply))
      {
        return Execution::Quit;
      }
    }
  }

  xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(5));
  return Execution::Continue;
}

XII_CONSOLEAPP_ENTRY_POINT(xiiMCPApplication);
