/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Foundation/Network/TcpListener.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCPPlugin/HttpServer.h>
#include <MCPPlugin/Json.h>

#include <charconv>
#include <string_view>

namespace
{
  constexpr xiiUInt32 MaximumHeaderBytes = 8192;
  constexpr xiiUInt32 MaximumConnections = 32;
  constexpr xiiUInt32 MaximumSessions = 64;
  constexpr xiiUInt32 MaximumRequestsPerSecond = 64;

  struct xiiMCPHttpConnection
  {
    xiiTcpSocket m_Socket;
    xiiDynamicArray<xiiUInt8> m_Input;
    xiiStringBuilder m_sOutput;
    xiiTime m_Deadline;
    xiiUInt32 m_uiSentBytes = 0;
    xiiUInt32 m_uiHeaderSearchOffset = 0;
  };

  struct xiiMCPHttpSession
  {
    xiiMCPSession m_Protocol;
    xiiTime m_LastActivity;
  };

  struct xiiMCPHttpRequest
  {
    std::string_view m_Method;
    std::string_view m_Path;
    xiiMap<xiiString, xiiString> m_Headers;
    xiiUInt32 m_uiBodyBytes = 0;
  };

  bool IsTokenCharacter(char character)
  {
    return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9') || std::string_view("!#$%&'*+-.^_`|~").find(character) != std::string_view::npos;
  }

  bool IsHeaderValue(std::string_view sValue)
  {
    for (const auto character : sValue)
    {
      if (character != '\t' && (character < ' ' || character > '~'))
      {
        return false;
      }
    }
    return true;
  }

  std::string_view TrimWhitespace(std::string_view sValue)
  {
    const auto uiStart = sValue.find_first_not_of(" \t");
    if (uiStart == std::string_view::npos)
    {
      return {};
    }

    return sValue.substr(uiStart, sValue.find_last_not_of(" \t") - uiStart + 1);
  }

  xiiStringView GetHeader(const xiiMCPHttpRequest& request, xiiStringView sHeader)
  {
    const auto it = request.m_Headers.Find(sHeader);
    return it.IsValid() ? it.Value().GetView() : xiiStringView();
  }

  bool ParseHeaders(std::string_view sHeaders, xiiMCPHttpRequest& out_request)
  {
    const auto uiLineEnd = sHeaders.find("\r\n");
    if (uiLineEnd == std::string_view::npos)
    {
      return false;
    }

    const auto requestLine = sHeaders.substr(0, uiLineEnd);
    const auto uiMethodEnd = requestLine.find(' ');
    const auto uiPathEnd = uiMethodEnd == std::string_view::npos ? uiMethodEnd : requestLine.find(' ', uiMethodEnd + 1);
    if (uiPathEnd == std::string_view::npos || requestLine.substr(uiPathEnd + 1) != "HTTP/1.1")
    {
      return false;
    }

    out_request.m_Method = requestLine.substr(0, uiMethodEnd);
    out_request.m_Path = requestLine.substr(uiMethodEnd + 1, uiPathEnd - uiMethodEnd - 1);
    sHeaders.remove_prefix(uiLineEnd + 2);
    while (sHeaders != "\r\n")
    {
      const auto uiEnd = sHeaders.find("\r\n");
      if (uiEnd == std::string_view::npos || uiEnd == 0 || out_request.m_Headers.GetCount() >= 64)
      {
        return false;
      }
      const auto line = sHeaders.substr(0, uiEnd);
      const auto uiSeparator = line.find(':');
      if (uiSeparator == std::string_view::npos || uiSeparator == 0)
      {
        return false;
      }

      const auto sHeader = line.substr(0, uiSeparator);
      for (const char character : sHeader)
      {
        if (!IsTokenCharacter(character))
        {
          return false;
        }
      }

      const auto sValue = TrimWhitespace(line.substr(uiSeparator + 1));
      if (!IsHeaderValue(sValue))
      {
        return false;
      }

      xiiStringBuilder sKey(xiiStringView(sHeader.data(), static_cast<xiiUInt32>(sHeader.size())));
      sKey.ToLower();
      if (out_request.m_Headers.Contains(sKey))
      {
        return false;
      }
      out_request.m_Headers.Insert(xiiString(sKey), xiiString(xiiStringView(sValue.data(), static_cast<xiiUInt32>(sValue.size()))));
      sHeaders.remove_prefix(uiEnd + 2);
    }

    const auto sLength = GetHeader(out_request, "content-length");
    if (out_request.m_Headers.Contains("transfer-encoding") || (sLength.IsEmpty() && out_request.m_Method == "POST"))
    {
      return false;
    }
    if (!sLength.IsEmpty())
    {
      const auto result = std::from_chars(sLength.GetStartPointer(), sLength.GetEndPointer(), out_request.m_uiBodyBytes);
      if (result.ec != std::errc() || result.ptr != sLength.GetEndPointer())
      {
        return false;
      }
    }
    return true;
  }

  bool IsTransient(const xiiNetworkError& error)
  {
    return error.m_Code == xiiNetworkErrorCode::WouldBlock || error.m_Code == xiiNetworkErrorCode::Interrupted;
  }

  bool EqualSecret(xiiStringView sLeft, xiiStringView sRight)
  {
    if (sLeft.GetElementCount() != sRight.GetElementCount())
    {
      return false;
    }

    xiiUInt8 uiDifference = 0;
    for (xiiUInt32 uiIndex = 0; uiIndex < sRight.GetElementCount(); ++uiIndex)
    {
      uiDifference |= static_cast<xiiUInt8>(sLeft.GetStartPointer()[uiIndex] ^ sRight.GetStartPointer()[uiIndex]);
    }
    return uiDifference == 0;
  }

  void Respond(xiiMCPHttpConnection& ref_connection, xiiStringView sStatus, xiiStringView sBody = {}, xiiStringView sSessionId = {}, xiiStringView sHeaders = {})
  {
    ref_connection.m_sOutput.SetFormat("HTTP/1.1 {}\r\nConnection: close\r\nCache-Control: no-store\r\nContent-Length: {}\r\n", sStatus, sBody.GetElementCount());
    if (!sBody.IsEmpty())
    {
      ref_connection.m_sOutput.Append("Content-Type: application/json\r\n");
    }
    if (!sSessionId.IsEmpty())
    {
      ref_connection.m_sOutput.Append("Mcp-Session-Id: ", sSessionId, "\r\n");
    }
    ref_connection.m_sOutput.Append(sHeaders, "\r\n", sBody);
    ref_connection.m_Deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  }
} // namespace

class xiiMCPHttpState
{
public:
  explicit xiiMCPHttpState(const xiiMCPProtocol& protocol) :
    m_Protocol(protocol)
  {
  }

  bool Receive(xiiMCPHttpConnection& ref_connection);
  void Dispatch(xiiMCPHttpConnection& ref_connection, const xiiMCPHttpRequest& request, xiiStringView sBody);
  void Update();

  const xiiMCPProtocol& m_Protocol;
  xiiMCPHttpConfiguration m_Configuration;
  xiiTcpListener m_Listener;
  xiiNetworkEndpoint m_Endpoint;
  xiiString m_sHost;
  xiiString m_sLocalhost;
  xiiString m_sExpectedAuthorization;
  xiiDynamicArray<xiiMCPHttpConnection> m_Connections;
  xiiMap<xiiString, xiiMCPHttpSession> m_Sessions;
  xiiTime m_RateWindow;
  xiiUInt32 m_uiRequests = 0;
};

void xiiMCPHttpState::Dispatch(xiiMCPHttpConnection& ref_connection, const xiiMCPHttpRequest& request, xiiStringView sBody)
{
  const auto sHost = GetHeader(request, "host");
  const auto sOrigin = GetHeader(request, "origin");
  xiiStringBuilder sSameOrigin("http://", sHost);
  if ((sHost != m_sHost && sHost != m_sLocalhost) ||
      (request.m_Headers.Contains("origin") && sOrigin != sSameOrigin && (sOrigin.IsEmpty() || sOrigin != m_Configuration.m_sAllowedOrigin)))
  {
    Respond(ref_connection, "403 Forbidden");
    return;
  }
  if (!m_sExpectedAuthorization.IsEmpty() && !EqualSecret(GetHeader(request, "authorization"), m_sExpectedAuthorization))
  {
    Respond(ref_connection, "401 Unauthorized", {}, {}, "WWW-Authenticate: Bearer\r\n");
    return;
  }
  if (request.m_Path != "/mcp")
  {
    Respond(ref_connection, "404 Not Found");
    return;
  }

  const auto now = xiiTime::Now();
  if (now >= m_RateWindow)
  {
    m_uiRequests = 0;
    m_RateWindow = now + xiiTime::MakeFromSeconds(1);
  }
  if (++m_uiRequests > MaximumRequestsPerSecond)
  {
    Respond(ref_connection, "429 Too Many Requests", {}, {}, "Retry-After: 1\r\n");
    return;
  }

  const auto sSessionId = GetHeader(request, "mcp-session-id");
  auto it = m_Sessions.Find(sSessionId);
  if (!sSessionId.IsEmpty() && !it.IsValid())
  {
    Respond(ref_connection, "404 Not Found");
    return;
  }
  const auto sVersion = GetHeader(request, "mcp-protocol-version");
  if (!sVersion.IsEmpty() && sVersion != xiiMCPProtocol::Version)
  {
    Respond(ref_connection, "400 Bad Request");
    return;
  }

  if (request.m_Method == "DELETE")
  {
    if (!it.IsValid())
    {
      Respond(ref_connection, "400 Bad Request");
      return;
    }
    m_Sessions.Remove(it);
    Respond(ref_connection, "204 No Content");
    return;
  }
  if (request.m_Method != "POST")
  {
    Respond(ref_connection, "405 Method Not Allowed", {}, {}, "Allow: POST, DELETE\r\n");
    return;
  }
  const auto sContentType = GetHeader(request, "content-type");
  if (sContentType != "application/json" && !sContentType.StartsWith("application/json;"))
  {
    Respond(ref_connection, "415 Unsupported Media Type");
    return;
  }

  xiiMCPSession fresh;
  if (!it.IsValid() && m_Sessions.GetCount() >= MaximumSessions)
  {
    Respond(ref_connection, "503 Service Unavailable");
    return;
  }

  xiiMCPCallContext context;
  context.m_Deadline = now + m_Configuration.m_RequestTimeout;
  context.m_uiPermissions = m_Configuration.m_uiPermissions;
  context.m_sIdentity = "streamable-http";
  auto& session = it.IsValid() ? it.Value().m_Protocol : fresh;
  xiiStringBuilder sReply;
  m_Protocol.Process(sBody, session, context, sReply);

  if (it.IsValid())
  {
    it.Value().m_LastActivity = now;
  }
  xiiStringBuilder sNewSession;
  if (fresh.m_State == xiiMCPSession::State::Initializing)
  {
    xiiConversionUtils::ToString(xiiUuid::MakeUuid(), sNewSession);
    sNewSession.ReplaceAll(" ", "");
    m_Sessions.Insert(xiiString(sNewSession), xiiMCPHttpSession{std::move(fresh), now});
  }

  Respond(ref_connection, sReply.IsEmpty() ? "202 Accepted" : "200 OK", sReply, sNewSession);
}

bool xiiMCPHttpState::Receive(xiiMCPHttpConnection& ref_connection)
{
  xiiUInt8 uiBuffer[8192];
  const auto transfer = ref_connection.m_Socket.Receive(uiBuffer);
  if (transfer.Failed())
  {
    return IsTransient(transfer.m_Error);
  }
  ref_connection.m_Input.PushBackRange(xiiArrayPtr<const xiiUInt8>(uiBuffer, transfer.m_uiBytesTransferred));
  const std::string_view input(reinterpret_cast<const char*>(ref_connection.m_Input.GetData()), ref_connection.m_Input.GetCount());
  const auto uiHeaderEnd = input.find("\r\n\r\n", ref_connection.m_uiHeaderSearchOffset);
  if (uiHeaderEnd == std::string_view::npos)
  {
    if (input.size() >= MaximumHeaderBytes)
    {
      Respond(ref_connection, "431 Request Header Fields Too Large");
    }
    ref_connection.m_uiHeaderSearchOffset = input.size() > 3 ? static_cast<xiiUInt32>(input.size() - 3) : 0;
    return true;
  }

  const auto uiBodyStart = uiHeaderEnd + 4;
  xiiMCPHttpRequest request;
  if (uiBodyStart > MaximumHeaderBytes || !ParseHeaders(input.substr(0, uiBodyStart), request))
  {
    Respond(ref_connection, "400 Bad Request");
  }
  else if (request.m_uiBodyBytes > xiiMCPJson::DefaultMaximumBytes)
  {
    Respond(ref_connection, "413 Content Too Large");
  }
  else if (input.size() >= uiBodyStart + request.m_uiBodyBytes)
  {
    if (input.size() != uiBodyStart + request.m_uiBodyBytes)
    {
      Respond(ref_connection, "400 Bad Request");
    }
    else
    {
      Dispatch(ref_connection, request, xiiStringView(input.data() + uiBodyStart, request.m_uiBodyBytes));
    }
  }
  return true;
}

void xiiMCPHttpState::Update()
{
  const auto now = xiiTime::Now();
  for (auto it = m_Sessions.GetIterator(); it.IsValid();)
  {
    if (now - it.Value().m_LastActivity > xiiTime::MakeFromMinutes(15))
    {
      it = m_Sessions.Remove(it);
    }
    else
    {
      ++it;
    }
  }

  for (xiiUInt32 uiIndex = 0; uiIndex < m_Connections.GetCount();)
  {
    auto& connection = m_Connections[uiIndex];
    bool bKeep = now < connection.m_Deadline;
    if (bKeep && connection.m_sOutput.IsEmpty())
    {
      bKeep = Receive(connection);
    }

    if (bKeep && !connection.m_sOutput.IsEmpty())
    {
      const auto* pBytes = reinterpret_cast<const xiiUInt8*>(connection.m_sOutput.GetData());
      const auto uiLength = connection.m_sOutput.GetElementCount();
      const auto transfer = connection.m_Socket.Send(xiiArrayPtr<const xiiUInt8>(pBytes + connection.m_uiSentBytes, uiLength - connection.m_uiSentBytes));
      connection.m_uiSentBytes += transfer.m_uiBytesTransferred;
      bKeep = connection.m_uiSentBytes < uiLength && (transfer.Succeeded() || IsTransient(transfer.m_Error));
    }

    if (bKeep)
    {
      ++uiIndex;
    }
    else
    {
      m_Connections.RemoveAtAndSwap(uiIndex);
    }
  }

  for (xiiUInt32 uiAttempt = 0; uiAttempt < 4 && m_Connections.GetCount() < MaximumConnections; ++uiAttempt)
  {
    xiiMCPHttpConnection connection;
    if (m_Listener.Accept(connection.m_Socket).Failed())
    {
      break;
    }
    connection.m_Deadline = now + xiiTime::MakeFromSeconds(5);
    m_Connections.PushBack(std::move(connection));
  }
}

xiiMCPHttpServer::xiiMCPHttpServer(const xiiMCPProtocol& protocol) :
  m_Protocol(protocol)
{
}

xiiMCPHttpServer::~xiiMCPHttpServer() = default;

xiiStatus xiiMCPHttpServer::Start(const xiiMCPHttpConfiguration& configuration)
{
  if (m_pState || !configuration.m_Endpoint.GetAddress().IsLoopback() || !configuration.m_RequestTimeout.IsPositive())
  {
    return xiiStatus("Provide a loopback endpoint and a positive deadline for a new MCP HTTP listener.");
  }
  if (configuration.m_sBearerToken.IsEmpty() && configuration.m_uiPermissions != static_cast<xiiUInt8>(xiiMCPPermission::Read))
  {
    return xiiStatus("MCP HTTP capture, editing, and execution require a bearer token.");
  }

  xiiUniquePtr<xiiMCPHttpState> pState = XII_DEFAULT_NEW(xiiMCPHttpState, m_Protocol);
  pState->m_Configuration = configuration;
  auto& listener = pState->m_Listener;
  if (listener.Open(configuration.m_Endpoint.GetAddress().GetFamily()).Failed() || listener.Bind(configuration.m_Endpoint).Failed() ||
      listener.Listen(MaximumConnections).Failed() || listener.GetLocalEndpoint(pState->m_Endpoint).Failed())
  {
    return xiiStatus("Could not open the loopback MCP HTTP listener.");
  }

  xiiStringBuilder sHost;
  pState->m_Endpoint.ToString(sHost);
  pState->m_sHost = sHost;
  sHost.SetFormat("localhost:{}", pState->m_Endpoint.GetPort());
  pState->m_sLocalhost = sHost;
  if (!configuration.m_sBearerToken.IsEmpty())
  {
    pState->m_sExpectedAuthorization = xiiStringBuilder("Bearer ", configuration.m_sBearerToken);
  }

  m_pState = std::move(pState);
  return XII_SUCCESS;
}

void xiiMCPHttpServer::Stop()
{
  m_pState.Clear();
}

void xiiMCPHttpServer::Update()
{
  if (m_pState)
  {
    m_pState->Update();
  }
}

xiiNetworkEndpoint xiiMCPHttpServer::GetEndpoint() const
{
  return m_pState ? m_pState->m_Endpoint : xiiNetworkEndpoint();
}

xiiUInt32 xiiMCPHttpServer::GetSessionCount() const
{
  return m_pState ? m_pState->m_Sessions.GetCount() : 0;
}
