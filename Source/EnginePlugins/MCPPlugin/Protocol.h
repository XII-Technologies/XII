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
  MethodNotFound,
  Deferred
};

/// Owns a deferred response independently of its request bytes and session.
class XII_MCPPLUGIN_DLL xiiMCPPendingReply
{
public:
  [[nodiscard]] bool              IsPending() const { return m_pInvocation != nullptr; }
  [[nodiscard]] const xiiVariant& GetId() const { return m_Id; }

  /// Checks the provider and writes a JSON-RPC response if it has finished.
  [[nodiscard]] bool Poll(xiiStringBuilder& out_sReply);

private:
  friend class xiiMCPProtocol;

  xiiUniquePtr<xiiMCPInvocation> m_pInvocation;
  xiiVariant                     m_Id;
  xiiMCPCapability               m_Kind    = xiiMCPCapability::Tool;
  bool                           m_bModern = false;
};

/// Transport-independent MCP request and reply handling. Transport-supplied context is authoritative.
class XII_MCPPLUGIN_DLL xiiMCPProtocol final
{
public:
  static constexpr const char* Version           = "2026-07-28";
  static constexpr const char* InitializeVersion = "2025-11-25";

  explicit xiiMCPProtocol(const xiiMCPRegistry& registry);

  /// A successful notification produces no output. Requests always produce one UTF-8 JSON reply.
  /// Poll an admitted deferred request through its pending reply, or destroy the reply to cancel it.
  xiiMCPReplyStatus Process(xiiStringView sMessage, xiiMCPSession& ref_session, const xiiMCPCallContext& context, xiiStringBuilder& out_sReply,
                            const xiiMCPHttpMetadata* pHttpMetadata = nullptr, xiiMCPPendingReply* pPending = nullptr) const;

private:
  void Dispatch(const xiiVariantDictionary& request, xiiMCPSession& ref_session, const xiiMCPCallContext& context, bool bModern,
                xiiVariantDictionary& ref_reply, xiiMCPPendingReply* pPending) const;

  const xiiMCPRegistry& m_Registry;
};
