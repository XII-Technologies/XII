/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Network/TcpListener.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCPPlugin/HttpServer.h>
#include <MCPPlugin/Json.h>

namespace
{
  constexpr xiiUInt32 MaximumHeaderBytes       = 8192;
  constexpr xiiUInt32 MaximumConnections       = 32;
  constexpr xiiUInt32 MaximumPendingRequests   = 24;
  constexpr xiiUInt32 MaximumSessions          = 64;
  constexpr xiiUInt32 MaximumStatelessClients  = 64;
  constexpr xiiUInt32 MaximumRequestsPerSecond = 64;

  struct xiiMCPHttpConnection
  {
    xiiTcpSocket              m_Socket;
    xiiDynamicArray<xiiUInt8> m_Input;
    xiiStringBuilder          m_sOutput;
    xiiMCPPendingReply        m_Pending;
    xiiString                 m_sSessionId;
    xiiString                 m_sStatelessClientKey;
    xiiTime                   m_Deadline;
    xiiUInt32                 m_uiSentBytes          = 0;
    xiiUInt32                 m_uiHeaderSearchOffset = 0;
    bool                      m_bStreaming           = false;
  };

  struct xiiMCPHttpSession
  {
    xiiMCPSession m_Protocol;
    xiiTime       m_LastActivity;
  };

  struct xiiMCPHttpClient
  {
    xiiString m_sName;
    xiiString m_sVersion;
    xiiTime   m_LastActivity;
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

  double ReadMediaQuality(xiiArrayPtr<const xiiStringView> parameters)
  {
    for (auto parameter : parameters)
    {
      const char* pEquals = parameter.FindSubString("=");
      if (pEquals == nullptr)
      {
        continue;
      }
      xiiStringView sName(parameter.GetStartPointer(), pEquals);
      sName.Trim(" \t");
      if (!sName.IsEqual_NoCase("q"))
      {
        continue;
      }
      xiiStringView sQuality(pEquals + 1, parameter.GetEndPointer());
      sQuality.Trim(" \t");
      double      fQuality = 0.0;
      const char* pEnd     = nullptr;
      if (xiiConversionUtils::StringToFloat(sQuality, fQuality, &pEnd).Succeeded() && pEnd == sQuality.GetEndPointer() &&
          fQuality >= 0.0 && fQuality <= 1.0)
      {
        return fQuality;
      }
      return 0.0;
    }
    return 1.0;
  }

  bool PrefersEventStream(xiiStringView sAccept)
  {
    xiiHybridArray<xiiStringView, 8> mediaRanges;
    sAccept.Split(false, mediaRanges, ",");
    double fEventQuality = 0.0;
    double fJsonQuality  = 0.0;
    for (auto range : mediaRanges)
    {
      xiiHybridArray<xiiStringView, 4> parts;
      range.Split(false, parts, ";");
      if (parts.IsEmpty())
      {
        continue;
      }
      auto sMediaType = parts[0];
      sMediaType.Trim(" \t");
      if (sMediaType.IsEqual_NoCase("text/event-stream"))
      {
        fEventQuality = xiiMath::Max(fEventQuality, ReadMediaQuality(parts.GetArrayPtr().GetSubArray(1)));
      }
      else if (sMediaType.IsEqual_NoCase("application/json"))
      {
        fJsonQuality = xiiMath::Max(fJsonQuality, ReadMediaQuality(parts.GetArrayPtr().GetSubArray(1)));
      }
    }
    return fEventQuality > 0.0 && fEventQuality >= fJsonQuality;
  }

  void AppendEventChunk(xiiMCPHttpConnection& ref_connection, xiiStringView sEvent)
  {
    ref_connection.m_sOutput.AppendFormat("{}\r\n", xiiArgU(sEvent.GetElementCount(), 1, false, 16));
    ref_connection.m_sOutput.Append(sEvent, "\r\n");
  }

  void BeginEventStream(xiiMCPHttpConnection& ref_connection)
  {
    ref_connection.m_bStreaming = true;
    ref_connection.m_sOutput    = "HTTP/1.1 200 OK\r\nConnection: close\r\nCache-Control: no-store\r\n"
                                  "Content-Type: text/event-stream\r\nTransfer-Encoding: chunked\r\nX-Accel-Buffering: no\r\n\r\n";
    AppendEventChunk(ref_connection, ": accepted\n\n");
  }

  void FinishEventStream(xiiMCPHttpConnection& ref_connection, xiiStringView sReply)
  {
    xiiStringBuilder event("data: ", sReply, "\n\n");
    AppendEventChunk(ref_connection, event);
    ref_connection.m_sOutput.Append("0\r\n\r\n");
    ref_connection.m_Deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
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
  void CancelSessionRequests(xiiStringView sSessionId, const xiiVariant* pId = nullptr);
  void TrackStatelessClient(xiiMCPHttpConnection& ref_connection, const xiiMCPSession& requestInfo, xiiStringView sUserAgent, xiiTime now);

  const xiiMCPProtocol&                 m_Protocol;
  xiiMCPHttpConfiguration               m_Configuration;
  xiiTcpListener                        m_Listener;
  xiiNetworkEndpoint                    m_Endpoint;
  xiiString                             m_sHost;
  xiiString                             m_sLocalhost;
  xiiString                             m_sExpectedAuthorization;
  xiiDynamicArray<xiiMCPHttpConnection> m_Connections;
  xiiMap<xiiString, xiiMCPHttpSession>  m_Sessions;
  xiiMap<xiiString, xiiMCPHttpClient>   m_StatelessClients;
  xiiTime                               m_RateWindow;
  xiiUInt32                             m_uiRequests = 0;
};

void xiiMCPHttpState::TrackStatelessClient(xiiMCPHttpConnection& ref_connection, const xiiMCPSession& requestInfo, xiiStringView sUserAgent, xiiTime now)
{
  const bool bDeclared = !requestInfo.m_sClient.IsEmpty();
  auto       sName     = bDeclared ? requestInfo.m_sClient.GetView() : sUserAgent.GetSubString(0, 128);
  if (sName.IsEmpty())
  {
    sName = "Unknown client";
  }
  const auto       sVersion = bDeclared ? requestInfo.m_sClientVersion.GetView() : xiiStringView();
  xiiStringBuilder sKey;
  sKey.SetFormat("{}:{}:{}{}", bDeclared, sName.GetElementCount(), sName, sVersion);
  ref_connection.m_sStatelessClientKey = sKey;

  if (auto it = m_StatelessClients.Find(sKey); it.IsValid())
  {
    it.Value().m_LastActivity = now;
    return;
  }

  if (m_StatelessClients.GetCount() >= MaximumStatelessClients)
  {
    auto oldest = m_StatelessClients.GetIterator();
    for (auto it = oldest; it.IsValid(); ++it)
    {
      if (it.Value().m_LastActivity < oldest.Value().m_LastActivity)
      {
        oldest = it;
      }
    }
    m_StatelessClients.Remove(oldest);
  }
  m_StatelessClients.Insert(xiiString(sKey), xiiMCPHttpClient{xiiString(sName), xiiString(sVersion), now});
}

void xiiMCPHttpState::CancelSessionRequests(xiiStringView sSessionId, const xiiVariant* pId)
{
  for (auto& connection : m_Connections)
  {
    if (connection.m_sSessionId == sSessionId && connection.m_Pending.IsPending() && (pId == nullptr || connection.m_Pending.MatchesId(*pId)))
    {
      connection.m_Pending  = xiiMCPPendingReply();
      connection.m_Deadline = xiiTime::MakeZero();
    }
  }
}

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
    CancelSessionRequests(sSessionId);
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
  xiiVariant       cancellationId;
  // Keep listener slots available for cancellation notifications and immediate requests.
  xiiUInt32 uiPendingCount = 0;
  for (const auto& connection : m_Connections)
  {
    if (connection.m_Pending.IsPending())
    {
      ++uiPendingCount;
    }
  }
  auto*      pPending          = uiPendingCount < MaximumPendingRequests ? &ref_connection.m_Pending : nullptr;
  const bool bSessionCanCancel = it.IsValid() && session.m_State == xiiMCPSession::State::Ready &&
    (metadata.m_sVersion.IsEmpty() || metadata.m_sVersion == xiiMCPProtocol::InitializeVersion);
  const auto replyStatus = m_Protocol.Process(sBody, session, context, sReply, &metadata, pPending, bSessionCanCancel ? &cancellationId : nullptr);
  if (fresh.m_bModernRequest)
  {
    TrackStatelessClient(ref_connection, fresh, GetHeader(request, "user-agent"), now);
  }
  if (cancellationId.IsValid())
  {
    CancelSessionRequests(sSessionId, &cancellationId);
  }

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
    case xiiMCPReplyStatus::Deferred:
      ref_connection.m_sSessionId = sSessionId;
      ref_connection.m_Deadline   = context.m_Deadline + xiiTime::MakeFromSeconds(1);
      if (bModern && PrefersEventStream(GetHeader(request, "accept")))
      {
        BeginEventStream(ref_connection);
      }
      return;
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
      CancelSessionRequests(it.Key());
      it = m_Sessions.Remove(it);
    }
    else
    {
      ++it;
    }
  }
  for (auto it = m_StatelessClients.GetIterator(); it.IsValid();)
  {
    if (now - it.Value().m_LastActivity > xiiTime::MakeFromMinutes(15))
    {
      it = m_StatelessClients.Remove(it);
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
    if (bKeep && connection.m_bStreaming && connection.m_Pending.IsPending())
    {
      // A request-scoped stream disconnect cancels exactly the invocation owned by this socket.
      xiiUInt8   uiUnexpectedByte = 0;
      const auto transfer         = connection.m_Socket.Receive(xiiArrayPtr(&uiUnexpectedByte, 1));
      bKeep                       = transfer.Failed() && IsTransient(transfer.m_Error);
    }
    if (bKeep && connection.m_Pending.IsPending())
    {
      xiiStringBuilder sReply;
      if (connection.m_Pending.Poll(sReply))
      {
        if (connection.m_bStreaming)
        {
          FinishEventStream(connection, sReply);
        }
        else
        {
          Respond(connection, "200 OK", sReply);
        }
      }
    }
    else if (bKeep && connection.m_sOutput.IsEmpty())
    {
      bKeep = Receive(connection);
    }

    if (bKeep && !connection.m_sOutput.IsEmpty())
    {
      const auto* pBytes   = reinterpret_cast<const xiiUInt8*>(connection.m_sOutput.GetData());
      const auto  uiLength = connection.m_sOutput.GetElementCount();
      const auto  transfer = connection.m_Socket.Send(xiiArrayPtr<const xiiUInt8>(pBytes + connection.m_uiSentBytes, uiLength - connection.m_uiSentBytes));
      connection.m_uiSentBytes += transfer.m_uiBytesTransferred;
      bKeep = transfer.Succeeded() || IsTransient(transfer.m_Error);
      if (bKeep && connection.m_uiSentBytes == uiLength)
      {
        bKeep = connection.m_Pending.IsPending();
        connection.m_sOutput.Clear();
        connection.m_uiSentBytes = 0;
      }
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

void xiiMCPHttpServer::ListClients(xiiVariantArray& out_clients) const
{
  out_clients.Clear();
  if (!m_pState)
  {
    return;
  }
  const auto now    = xiiTime::Now();
  const auto append = [&](xiiStringView sKey, bool bStateless, xiiStringView sName, xiiStringView sVersion, xiiStringView sState, xiiTime lastActivity) {
    xiiUInt32 uiPending = 0;
    for (const auto& connection : m_pState->m_Connections)
    {
      const auto& sOwner = bStateless ? connection.m_sStatelessClientKey : connection.m_sSessionId;
      if (connection.m_Pending.IsPending() && sOwner == sKey)
      {
        ++uiPending;
      }
    }
    xiiVariantDictionary entry;
    entry["name"]            = xiiString(sName);
    entry["version"]         = xiiString(sVersion);
    entry["state"]           = xiiString(sState);
    entry["pendingRequests"] = uiPending;
    entry["idleSeconds"]     = static_cast<xiiUInt32>((now - lastActivity).GetSeconds());
    out_clients.PushBack(std::move(entry));
  };
  for (auto it = m_pState->m_Sessions.GetIterator(); it.IsValid(); ++it)
  {
    const auto& session = it.Value();
    append(it.Key(), false, session.m_Protocol.m_sClient, session.m_Protocol.m_sClientVersion,
           session.m_Protocol.m_State == xiiMCPSession::State::Ready ? "Ready" : "Initializing", session.m_LastActivity);
  }
  for (auto it = m_pState->m_StatelessClients.GetIterator(); it.IsValid(); ++it)
  {
    const auto& client = it.Value();
    append(it.Key(), true, client.m_sName, client.m_sVersion, "Stateless", client.m_LastActivity);
  }
}
