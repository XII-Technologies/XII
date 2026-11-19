/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Network/TcpListener.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCPPlugin/HttpServer.h>
#include <MCPPlugin/Json.h>

namespace
{
  constexpr xiiUInt32 MaximumHeaderBytes       = 8192;
  constexpr xiiUInt32 MaximumConnections       = 32;
  constexpr xiiUInt32 MaximumSessions          = 64;
  constexpr xiiUInt32 MaximumRequestsPerSecond = 64;

  struct xiiMCPHttpConnection
  {
    xiiTcpSocket              m_Socket;
    xiiDynamicArray<xiiUInt8> m_Input;
    xiiStringBuilder          m_sOutput;
    xiiTime                   m_Deadline;
    xiiUInt32                 m_uiSentBytes          = 0;
    xiiUInt32                 m_uiHeaderSearchOffset = 0;
  };

  struct xiiMCPHttpSession
  {
    xiiMCPSession m_Protocol;
    xiiTime       m_LastActivity;
  };

  struct xiiMCPHttpRequest
  {
    xiiStringView                m_Method;
    xiiStringView                m_Path;
    xiiMap<xiiString, xiiString> m_Headers;
    xiiUInt32                    m_uiBodyBytes = 0;
  };

  // Incoming bytes may contain malformed UTF-8 or NUL. HTTP framing uses byte offsets.
  const char* FindDelimiter(xiiStringView bytes, xiiStringView delimiter)
  {
    if (bytes.GetElementCount() < delimiter.GetElementCount())
    {
      return nullptr;
    }
    const char* pLast = bytes.GetEndPointer() - delimiter.GetElementCount();
    for (const char* pByte = bytes.GetStartPointer(); pByte <= pLast; ++pByte)
    {
      if (xiiMemoryUtils::IsEqual(pByte, delimiter.GetStartPointer(), delimiter.GetElementCount()))
      {
        return pByte;
      }
    }
    return nullptr;
  }

  bool IsTokenCharacter(char character)
  {
    return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
      (character >= '0' && character <= '9') || (character != '\0' && character > 0 && xiiStringView("!#$%&'*+-.^_`|~").FindSubString(xiiStringView(&character, 1)) != nullptr);
  }

  bool IsHeaderValue(xiiStringView sValue)
  {
    for (const char* pByte = sValue.GetStartPointer(); pByte != sValue.GetEndPointer(); ++pByte)
    {
      const char character = *pByte;
      if (character != '\t' && (character < ' ' || character > '~'))
      {
        return false;
      }
    }
    return true;
  }

  xiiStringView GetHeader(const xiiMCPHttpRequest& request, xiiStringView sHeader)
  {
    const auto it = request.m_Headers.Find(sHeader);
    return it.IsValid() ? it.Value().GetView() : xiiStringView();
  }

  bool ParseHeaders(xiiStringView sHeaders, xiiMCPHttpRequest& out_request)
  {
    const char* pLineEnd = FindDelimiter(sHeaders, "\r\n");
    if (pLineEnd == nullptr)
    {
      return false;
    }

    const xiiStringView requestLine(sHeaders.GetStartPointer(), pLineEnd);
    if (!IsHeaderValue(requestLine))
    {
      return false;
    }
    const char* pMethodEnd = requestLine.FindSubString(" ");
    if (pMethodEnd == nullptr || pMethodEnd == requestLine.GetStartPointer())
    {
      return false;
    }
    const xiiStringView pathAndVersion(pMethodEnd + 1, pLineEnd);
    const char*         pPathEnd = pathAndVersion.FindSubString(" ");
    if (pPathEnd == nullptr || pPathEnd == pathAndVersion.GetStartPointer() ||
        xiiStringView(pPathEnd + 1, pLineEnd) != "HTTP/1.1")
    {
      return false;
    }

    out_request.m_Method = xiiStringView(requestLine.GetStartPointer(), pMethodEnd);
    out_request.m_Path   = xiiStringView(pathAndVersion.GetStartPointer(), pPathEnd);
    sHeaders.SetStartPosition(pLineEnd + 2);
    while (sHeaders.GetElementCount() != 2)
    {
      const char* pEnd = FindDelimiter(sHeaders, "\r\n");
      if (pEnd == nullptr || pEnd == sHeaders.GetStartPointer() || out_request.m_Headers.GetCount() >= 64)
      {
        return false;
      }
      const xiiStringView line(sHeaders.GetStartPointer(), pEnd);
      const char*         pSeparator = FindDelimiter(line, ":");
      if (pSeparator == nullptr || pSeparator == line.GetStartPointer())
      {
        return false;
      }

      const xiiStringView sHeader(line.GetStartPointer(), pSeparator);
      for (const char* pByte = sHeader.GetStartPointer(); pByte != sHeader.GetEndPointer(); ++pByte)
      {
        if (!IsTokenCharacter(*pByte))
        {
          return false;
        }
      }

      xiiStringView sValue(pSeparator + 1, pEnd);
      if (!IsHeaderValue(sValue))
      {
        return false;
      }
      sValue.Trim(" \t");

      xiiStringBuilder sKey(sHeader);
      sKey.ToLower();
      if (out_request.m_Headers.Contains(sKey))
      {
        return false;
      }
      out_request.m_Headers.Insert(xiiString(sKey), xiiString(sValue));
      sHeaders.SetStartPosition(pEnd + 2);
    }

    const auto sLength = GetHeader(out_request, "content-length");
    if (out_request.m_Headers.Contains("transfer-encoding") || (sLength.IsEmpty() && out_request.m_Method == "POST"))
    {
      return false;
    }
    if (!sLength.IsEmpty())
    {
      xiiUInt64 uiBodyBytes = 0;
      if (xiiConversionUtils::StringToUInt64Strict(sLength, uiBodyBytes).Failed() || uiBodyBytes > xiiMath::MaxValue<xiiUInt32>())
      {
        return false;
      }
      out_request.m_uiBodyBytes = static_cast<xiiUInt32>(uiBodyBytes);
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

  const xiiMCPProtocol&                 m_Protocol;
  xiiMCPHttpConfiguration               m_Configuration;
  xiiTcpListener                        m_Listener;
  xiiNetworkEndpoint                    m_Endpoint;
  xiiString                             m_sHost;
  xiiString                             m_sLocalhost;
  xiiString                             m_sExpectedAuthorization;
  xiiDynamicArray<xiiMCPHttpConnection> m_Connections;
  xiiMap<xiiString, xiiMCPHttpSession>  m_Sessions;
  xiiTime                               m_RateWindow;
  xiiUInt32                             m_uiRequests = 0;
};

void xiiMCPHttpState::Dispatch(xiiMCPHttpConnection& ref_connection, const xiiMCPHttpRequest& request, xiiStringView sBody)
{
  const auto       sHost   = GetHeader(request, "host");
  const auto       sOrigin = GetHeader(request, "origin");
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

  const xiiMCPHttpMetadata metadata{GetHeader(request, "mcp-protocol-version"), GetHeader(request, "mcp-method"), GetHeader(request, "mcp-name")};
  const bool               bModern    = metadata.m_sVersion == xiiMCPProtocol::Version || !metadata.m_sMethod.IsEmpty();
  const auto               sSessionId = bModern ? xiiStringView() : GetHeader(request, "mcp-session-id");
  auto                     it         = m_Sessions.Find(sSessionId);
  if (!sSessionId.IsEmpty() && !it.IsValid())
  {
    Respond(ref_connection, "404 Not Found");
    return;
  }
  if (request.m_Method == "DELETE" && !bModern)
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
  if (!bModern && !it.IsValid() && m_Sessions.GetCount() >= MaximumSessions)
  {
    Respond(ref_connection, "503 Service Unavailable");
    return;
  }

  xiiMCPCallContext context;
  context.m_Deadline       = now + m_Configuration.m_RequestTimeout;
  context.m_uiPermissions  = m_Configuration.m_uiPermissions;
  context.m_sIdentity      = "streamable-http";
  auto&            session = it.IsValid() ? it.Value().m_Protocol : fresh;
  xiiStringBuilder sReply;
  const auto       replyStatus = m_Protocol.Process(sBody, session, context, sReply, &metadata);

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

  const char* szStatus = "200 OK";
  switch (replyStatus)
  {
    case xiiMCPReplyStatus::Notification:
      szStatus = "202 Accepted";
      break;
    case xiiMCPReplyStatus::BadRequest:
      szStatus = "400 Bad Request";
      break;
    case xiiMCPReplyStatus::MethodNotFound:
      szStatus = "404 Not Found";
      break;
    case xiiMCPReplyStatus::Reply:
      break;
  }
  Respond(ref_connection, szStatus, sReply, sNewSession);
}

bool xiiMCPHttpState::Receive(xiiMCPHttpConnection& ref_connection)
{
  xiiUInt8   uiBuffer[8192];
  const auto transfer = ref_connection.m_Socket.Receive(uiBuffer);
  if (transfer.Failed())
  {
    return IsTransient(transfer.m_Error);
  }
  ref_connection.m_Input.PushBackRange(xiiArrayPtr<const xiiUInt8>(uiBuffer, transfer.m_uiBytesTransferred));
  if (ref_connection.m_Input.IsEmpty())
  {
    return false;
  }
  const auto*         pInput  = reinterpret_cast<const char*>(ref_connection.m_Input.GetData());
  const auto          uiBytes = ref_connection.m_Input.GetCount();
  const xiiStringView headerSearch(pInput + ref_connection.m_uiHeaderSearchOffset, pInput + uiBytes);
  const char*         pHeaderEnd = FindDelimiter(headerSearch, "\r\n\r\n");
  if (pHeaderEnd == nullptr)
  {
    if (uiBytes >= MaximumHeaderBytes)
    {
      Respond(ref_connection, "431 Request Header Fields Too Large");
    }
    ref_connection.m_uiHeaderSearchOffset = uiBytes > 3 ? uiBytes - 3 : 0;
    return true;
  }

  const auto        uiBodyStart = static_cast<xiiUInt32>(pHeaderEnd - pInput) + 4;
  xiiMCPHttpRequest request;
  if (uiBodyStart > MaximumHeaderBytes || !ParseHeaders(xiiStringView(pInput, uiBodyStart), request))
  {
    Respond(ref_connection, "400 Bad Request");
  }
  else if (request.m_uiBodyBytes > xiiMCPJson::DefaultMaximumBytes)
  {
    Respond(ref_connection, "413 Content Too Large");
  }
  else if (uiBytes >= uiBodyStart + request.m_uiBodyBytes)
  {
    if (uiBytes != uiBodyStart + request.m_uiBodyBytes)
    {
      Respond(ref_connection, "400 Bad Request");
    }
    else
    {
      Dispatch(ref_connection, request, xiiStringView(pInput + uiBodyStart, request.m_uiBodyBytes));
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
    bool  bKeep      = now < connection.m_Deadline;
    if (bKeep && connection.m_sOutput.IsEmpty())
    {
      bKeep = Receive(connection);
    }

    if (bKeep && !connection.m_sOutput.IsEmpty())
    {
      const auto* pBytes   = reinterpret_cast<const xiiUInt8*>(connection.m_sOutput.GetData());
      const auto  uiLength = connection.m_sOutput.GetElementCount();
      const auto  transfer = connection.m_Socket.Send(xiiArrayPtr<const xiiUInt8>(pBytes + connection.m_uiSentBytes, uiLength - connection.m_uiSentBytes));
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
  pState->m_Configuration              = configuration;
  auto& listener                       = pState->m_Listener;
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

void xiiMCPHttpServer::ListSessions(xiiVariantArray& out_sessions) const
{
  out_sessions.Clear();
  if (!m_pState)
  {
    return;
  }
  const auto now = xiiTime::Now();
  for (auto it = m_pState->m_Sessions.GetIterator(); it.IsValid(); ++it)
  {
    const auto&          session = it.Value();
    xiiVariantDictionary entry;
    entry["name"]        = session.m_Protocol.m_sClient;
    entry["state"]       = xiiString(session.m_Protocol.m_State == xiiMCPSession::State::Ready ? "Ready" : "Initializing");
    entry["idleSeconds"] = static_cast<xiiUInt32>((now - session.m_LastActivity).GetSeconds());
    out_sessions.PushBack(entry);
  }
}
