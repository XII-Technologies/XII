/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <MCPPlugin/Json.h>
#include <MCPPlugin/Protocol.h>

#include <Foundation/Math/Math.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace
{
  void Error(xiiVariantDictionary& ref_reply, xiiInt32 iCode, xiiStringView sMessage)
  {
    xiiVariantDictionary error;
    error["code"]      = iCode;
    error["message"]   = xiiString(sMessage);
    ref_reply["error"] = std::move(error);
  }

  xiiVariantArray MakeTextContent(xiiStringView sText)
  {
    xiiVariantDictionary text;
    text["type"] = xiiString("text");
    text["text"] = xiiString(sText);
    xiiVariantArray content;
    content.PushBack(std::move(text));
    return content;
  }

  bool IsId(const xiiVariant& value)
  {
    return value.IsA<xiiString>() || value.IsA<xiiInt64>() || value.IsA<xiiUInt64>();
  }

  xiiVariantArray Versions()
  {
    xiiVariantArray versions;
    versions.PushBack(xiiString(xiiMCPProtocol::Version));
    versions.PushBack(xiiString(xiiMCPProtocol::InitializeVersion));
    return versions;
  }

  void UnsupportedVersion(xiiVariantDictionary& ref_reply, xiiStringView sVersion)
  {
    xiiVariantDictionary data;
    data["requested"] = xiiString(sVersion);
    data["supported"] = Versions();
    xiiVariantDictionary error;
    error["code"]      = xiiInt32(-32022);
    error["message"]   = xiiString("Unsupported protocol version.");
    error["data"]      = std::move(data);
    ref_reply["error"] = std::move(error);
  }

  bool HeadersMatch(const xiiMCPHttpMetadata& headers, xiiStringView sVersion, xiiStringView sMethod, const xiiVariantDictionary* pParams)
  {
    if (sVersion.IsEmpty() || headers.m_sVersion != sVersion || headers.m_sMethod != sMethod)
    {
      return false;
    }
    if (pParams != nullptr && (sMethod == "tools/call" || sMethod == "resources/read" || sMethod == "prompts/get"))
    {
      const auto sName = xiiMCPJson::GetString(*pParams, sMethod == "resources/read" ? "uri" : "name");
      return !sName.IsEmpty() && headers.m_sName == sName;
    }
    return true;
  }

  bool ListCapabilities(const xiiMCPRegistry& registry, xiiMCPCapability kind, xiiStringView sMember, const xiiVariantDictionary& params,
                        const xiiMCPCallContext& context, xiiVariantDictionary& out_result)
  {
    // Each descriptor is at most 8 KiB. Even a page made entirely of maximum-sized
    // entries leaves room for the envelope within the protocol response budget.
    constexpr xiiUInt32 PageSize = 24;
    xiiUInt32           uiOffset = 0;
    if (const auto* pCursor = params.GetValue("cursor"))
    {
      if (!pCursor->IsA<xiiString>())
      {
        return false;
      }

      const auto  sCursor    = pCursor->Get<xiiString>().GetView();
      const char* pSeparator = sCursor.FindSubString(":");
      xiiInt64    iRevision  = 0;
      const char* pEnd       = nullptr;
      if (pSeparator == nullptr ||
          xiiConversionUtils::StringToInt64(xiiStringView(sCursor.GetStartPointer(), pSeparator), iRevision, &pEnd).Failed() ||
          pEnd != pSeparator || iRevision < 1 || static_cast<xiiUInt64>(iRevision) != registry.GetRevision() ||
          xiiConversionUtils::StringToUInt(xiiStringView(pSeparator + 1, sCursor.GetEndPointer()), uiOffset, &pEnd).Failed() ||
          pEnd != sCursor.GetEndPointer() || uiOffset == 0)
      {
        return false;
      }
    }

    xiiVariantArray entries;
    registry.List(kind, context, entries);
    if (uiOffset > entries.GetCount())
    {
      return false;
    }

    const xiiUInt32 uiEnd = xiiMath::Min(uiOffset + PageSize, entries.GetCount());
    xiiVariantArray page;
    for (xiiUInt32 uiIndex = uiOffset; uiIndex < uiEnd; ++uiIndex)
    {
      page.PushBack(entries[uiIndex]);
    }
    out_result[sMember] = std::move(page);
    if (uiEnd < entries.GetCount())
    {
      xiiStringBuilder sCursor;
      sCursor.SetFormat("{}:{}", registry.GetRevision(), uiEnd);
      out_result["nextCursor"] = xiiString(sCursor);
    }
    return true;
  }

  void CallTool(const xiiMCPRegistry& registry, const xiiVariantDictionary& params, const xiiMCPCallContext& context, xiiVariantDictionary& ref_reply)
  {
    const auto sName = xiiMCPJson::GetString(params, "name");
    if (!registry.Contains(xiiMCPCapability::Tool, sName, context))
    {
      Error(ref_reply, -32602, "Unknown or unauthorized tool.");
      return;
    }

    const xiiVariantDictionary empty;
    const auto*                pArguments = xiiMCPJson::GetObject(params, "arguments");
    if (pArguments == nullptr && params.Contains("arguments"))
    {
      Error(ref_reply, -32602, "Tool arguments must be a JSON object.");
      return;
    }

    xiiVariantDictionary structured;
    xiiVariantDictionary result;
    const auto           status = registry.Invoke(xiiMCPCapability::Tool, sName, context, pArguments != nullptr ? *pArguments : empty, structured);
    xiiStringBuilder     sText;
    if (status.Failed())
    {
      sText             = status.GetMessageString();
      result["isError"] = true;
    }
    else if (xiiMCPJson::Write(structured, sText, 96 * 1024).Failed())
    {
      sText             = "The tool result exceeded the JSON response budget or contained unsupported values.";
      result["isError"] = true;
    }
    else
    {
      result["structuredContent"] = std::move(structured);
    }

    result["content"]   = MakeTextContent(sText);
    ref_reply["result"] = std::move(result);
  }
} // namespace

xiiMCPProtocol::xiiMCPProtocol(const xiiMCPRegistry& registry) :
  m_Registry(registry)
{
}

xiiMCPReplyStatus xiiMCPProtocol::Process(xiiStringView sMessage, xiiMCPSession& ref_session, const xiiMCPCallContext& context, xiiStringBuilder& out_sReply,
                                          const xiiMCPHttpMetadata* pHttpMetadata) const
{
  out_sReply.Clear();
  xiiVariantDictionary reply;
  reply["jsonrpc"]  = xiiString("2.0");
  reply["id"]       = xiiVariant();
  auto       status = xiiMCPReplyStatus::Reply;
  xiiVariant input;
  if (xiiMCPJson::Parse(sMessage, input).Failed())
  {
    Error(reply, -32700, "Invalid JSON message.");
  }
  else if (!input.IsA<xiiVariantDictionary>())
  {
    Error(reply, -32600, "Expected one JSON-RPC request object.");
  }
  else
  {
    const auto& request   = input.Get<xiiVariantDictionary>();
    const auto* pId       = request.GetValue("id");
    const auto  sMethod   = xiiMCPJson::GetString(request, "method");
    const auto* pParams   = xiiMCPJson::GetObject(request, "params");
    const auto* pMetadata = pParams != nullptr ? xiiMCPJson::GetObject(*pParams, "_meta") : nullptr;
    const auto  sVersion  = pMetadata != nullptr ? xiiMCPJson::GetString(*pMetadata, "io.modelcontextprotocol/protocolVersion") : xiiStringView();
    const bool  bModern   = !sVersion.IsEmpty() || sMethod == "server/discover" ||
      (pHttpMetadata != nullptr && (!pHttpMetadata->m_sMethod.IsEmpty() || pHttpMetadata->m_sVersion == Version));
    if (xiiMCPJson::GetString(request, "jsonrpc") != "2.0" || sMethod.IsEmpty() || (pId != nullptr && !IsId(*pId)))
    {
      Error(reply, -32600, "Invalid JSON-RPC request.");
    }
    else if (pId == nullptr)
    {
      if (sMethod == "notifications/initialized" && ref_session.m_State == xiiMCPSession::State::Initializing)
      {
        ref_session.m_State = xiiMCPSession::State::Ready;
      }
      return xiiMCPReplyStatus::Notification;
    }
    else
    {
      reply["id"] = *pId;
      if (bModern && pHttpMetadata != nullptr && !HeadersMatch(*pHttpMetadata, sVersion, sMethod, pParams))
      {
        Error(reply, -32020, "Required MCP HTTP headers are missing or do not match the request body.");
        status = xiiMCPReplyStatus::BadRequest;
      }
      else if (bModern && sVersion != Version)
      {
        UnsupportedVersion(reply, sVersion);
        status = xiiMCPReplyStatus::BadRequest;
      }
      else if (!bModern && pHttpMetadata != nullptr && !pHttpMetadata->m_sVersion.IsEmpty() && pHttpMetadata->m_sVersion != InitializeVersion)
      {
        UnsupportedVersion(reply, pHttpMetadata->m_sVersion);
        status = xiiMCPReplyStatus::BadRequest;
      }
      else
      {
        Dispatch(request, ref_session, context, bModern, reply);
        if (bModern)
        {
          const auto* pResult = xiiMCPJson::GetObject(reply, "result");
          if (pResult != nullptr)
          {
            auto result          = *pResult;
            result["resultType"] = xiiString("complete");
            reply["result"]      = std::move(result);
          }
          else if (const auto* pError = xiiMCPJson::GetObject(reply, "error"))
          {
            if (const auto* pCode = pError->GetValue("code"); pCode != nullptr && *pCode == xiiVariant(xiiInt32(-32601)))
            {
              status = xiiMCPReplyStatus::MethodNotFound;
            }
          }
        }
      }
    }
  }

  if (xiiMCPJson::Write(reply, out_sReply).Failed())
  {
    reply.Remove("result");
    Error(reply, -32603, "The response could not be encoded within the server limit.");
    if (xiiMCPJson::Write(reply, out_sReply).Failed())
    {
      out_sReply = R"({"jsonrpc":"2.0","id":null,"error":{"code":-32603,"message":"Response exceeds server limits."}})";
    }
  }
  return status;
}

void xiiMCPProtocol::Dispatch(const xiiVariantDictionary& request, xiiMCPSession& ref_session, const xiiMCPCallContext& context, bool bModern, xiiVariantDictionary& ref_reply) const
{
  const auto  sMethod = xiiMCPJson::GetString(request, "method");
  const auto* pParams = xiiMCPJson::GetObject(request, "params");
  if (pParams == nullptr && request.Contains("params"))
  {
    Error(ref_reply, -32602, "MCP parameters must be a JSON object.");
    return;
  }

  const xiiVariantDictionary empty;
  const auto&                params = pParams != nullptr ? *pParams : empty;
  xiiVariantDictionary       result;

  if ((sMethod == "initialize" && !bModern) || (sMethod == "server/discover" && bModern))
  {
    const auto* pClientInfo = xiiMCPJson::GetObject(params, "clientInfo");
    if (!bModern && (ref_session.m_State != xiiMCPSession::State::Created || pClientInfo == nullptr || xiiMCPJson::GetString(*pClientInfo, "name").IsEmpty() || xiiMCPJson::GetString(*pClientInfo, "version").IsEmpty() || xiiMCPJson::GetString(params, "protocolVersion").IsEmpty() || xiiMCPJson::GetObject(params, "capabilities") == nullptr))
    {
      Error(ref_reply, -32602, "Expected a new session and complete MCP initialize parameters.");
      return;
    }

    xiiVariantDictionary capabilities;
    for (const char* szKind : {"tools", "resources", "prompts"})
    {
      xiiVariantDictionary capability;
      if (!bModern && ref_session.m_bListNotifications)
      {
        capability["listChanged"] = true;
      }
      capabilities[szKind] = std::move(capability);
    }
    xiiVariantDictionary server;
    server["name"]         = xiiString("xiiMCP");
    server["version"]      = xiiString("1.0.0");
    result["capabilities"] = std::move(capabilities);
    if (bModern)
    {
      xiiVariantDictionary metadata;
      metadata["io.modelcontextprotocol/serverInfo"] = std::move(server);
      result["_meta"]                                = std::move(metadata);
      result["supportedVersions"]                    = Versions();
    }
    else
    {
      result["serverInfo"]      = std::move(server);
      result["protocolVersion"] = xiiString(InitializeVersion);
      ref_session.m_sClient     = xiiMCPJson::GetString(*pClientInfo, "name");
      ref_session.m_State       = xiiMCPSession::State::Initializing;
    }
  }
  else if (sMethod == "ping")
  {
    // Ping may be used during initialization.
  }
  else if (!bModern && ref_session.m_State != xiiMCPSession::State::Ready)
  {
    Error(ref_reply, -32002, "Initialize the MCP session before sending this request.");
    return;
  }
  else if (sMethod == "tools/call")
  {
    CallTool(m_Registry, params, context, ref_reply);
    return;
  }
  else if (sMethod == "resources/read" || sMethod == "prompts/get")
  {
    const bool  bResource  = sMethod == "resources/read";
    const auto  kind       = bResource ? xiiMCPCapability::Resource : xiiMCPCapability::Prompt;
    const auto  sKey       = xiiMCPJson::GetString(params, bResource ? "uri" : "name");
    const auto* pArguments = bResource ? &params : xiiMCPJson::GetObject(params, "arguments");
    if (pArguments == nullptr && params.Contains("arguments"))
    {
      Error(ref_reply, -32602, "Prompt arguments must be a JSON object.");
      return;
    }

    const auto status = bResource ? m_Registry.ReadResource(sKey, context, result) :
                                    m_Registry.Invoke(kind, sKey, context, pArguments != nullptr ? *pArguments : empty, result);
    if (status.Failed())
    {
      Error(ref_reply, bResource && !bModern ? -32002 : -32602, status.GetMessageString());
      return;
    }
  }
  else
  {
    struct ListMethod
    {
      const char*      m_szMethod;
      const char*      m_szResultKey;
      xiiMCPCapability m_Kind;
    };
    constexpr ListMethod Methods[] = {
      {"tools/list", "tools", xiiMCPCapability::Tool},
      {"resources/list", "resources", xiiMCPCapability::Resource},
      {"resources/templates/list", "resourceTemplates", xiiMCPCapability::ResourceTemplate},
      {"prompts/list", "prompts", xiiMCPCapability::Prompt},
    };

    bool bMatched = false;
    for (const auto& method : Methods)
    {
      if (sMethod == method.m_szMethod)
      {
        bMatched = true;
        if (!ListCapabilities(m_Registry, method.m_Kind, method.m_szResultKey, params, context, result))
        {
          Error(ref_reply, -32602, "The pagination cursor is invalid or the capability list changed. Restart discovery without a cursor.");
          return;
        }
        break;
      }
    }

    if (!bMatched)
    {
      Error(ref_reply, -32601, "MCP method not found.");
      return;
    }
  }

  ref_reply["result"] = std::move(result);
}
