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

  State m_State = State::Created;
  xiiString m_sClient;
};

/// Transport-independent MCP request and reply handling. Transport-supplied context is authoritative.
class XII_MCPPLUGIN_DLL xiiMCPProtocol final
{
public:
  static constexpr const char* Version = "2025-11-25";

  explicit xiiMCPProtocol(const xiiMCPRegistry& registry);

  /// A successful notification produces no output. Requests always produce one UTF-8 JSON reply.
  void Process(xiiStringView sMessage, xiiMCPSession& ref_session, const xiiMCPCallContext& context, xiiStringBuilder& out_sReply) const;

private:
  void Dispatch(const xiiVariantDictionary& request, xiiMCPSession& ref_session, const xiiMCPCallContext& context, xiiVariantDictionary& ref_reply) const;

  const xiiMCPRegistry& m_Registry;
};
