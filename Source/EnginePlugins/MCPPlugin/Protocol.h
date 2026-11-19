/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <MCPPlugin/Registry.h>

struct XII_MCPPLUGIN_DLL xiiMCPSession
{
  enum class State : xiiUInt8
  {
    Created,
    Initializing,
    Ready
  };

  State     m_State = State::Created;
  xiiString m_sClient;
  bool      m_bListNotifications = false;
};

struct XII_MCPPLUGIN_DLL xiiMCPHttpMetadata
{
  xiiStringView m_sVersion;
  xiiStringView m_sMethod;
  xiiStringView m_sName;
};

enum class xiiMCPReplyStatus : xiiUInt8
{
  Reply,
  Notification,
  BadRequest,
  MethodNotFound
};

/// Transport-independent MCP request and reply handling. Transport-supplied context is authoritative.
class XII_MCPPLUGIN_DLL xiiMCPProtocol final
{
public:
  static constexpr const char* Version           = "2026-07-28";
  static constexpr const char* InitializeVersion = "2025-11-25";

  explicit xiiMCPProtocol(const xiiMCPRegistry& registry);

  /// A successful notification produces no output. Requests always produce one UTF-8 JSON reply.
  xiiMCPReplyStatus Process(xiiStringView sMessage, xiiMCPSession& ref_session, const xiiMCPCallContext& context, xiiStringBuilder& out_sReply,
                            const xiiMCPHttpMetadata* pHttpMetadata = nullptr) const;

private:
  void Dispatch(const xiiVariantDictionary& request, xiiMCPSession& ref_session, const xiiMCPCallContext& context, bool bModern, xiiVariantDictionary& ref_reply) const;

  const xiiMCPRegistry& m_Registry;
};
