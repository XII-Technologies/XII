/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <Foundation/Network/NetworkEndpoint.h>
#include <Foundation/Types/UniquePtr.h>
#include <MCPPlugin/Protocol.h>

struct XII_MCPPLUGIN_DLL xiiMCPHttpConfiguration
{
  xiiNetworkEndpoint m_Endpoint;
  xiiString          m_sBearerToken;
  xiiString          m_sAllowedOrigin;
  xiiTime            m_RequestTimeout = xiiTime::MakeFromSeconds(5);
  xiiUInt8           m_uiPermissions  = static_cast<xiiUInt8>(xiiMCPPermission::Read);
};

class xiiMCPHttpState;

/// Bounded nonblocking Streamable HTTP binding. Pump on the protocol's owning thread.
class XII_MCPPLUGIN_DLL xiiMCPHttpServer final
{
public:
  explicit xiiMCPHttpServer(const xiiMCPProtocol& protocol);
  ~xiiMCPHttpServer();

  [[nodiscard]] xiiStatus          Start(const xiiMCPHttpConfiguration& configuration);
  void                             Stop();
  void                             Update();
  [[nodiscard]] xiiNetworkEndpoint GetEndpoint() const;
  [[nodiscard]] xiiUInt32          GetSessionCount() const;
  void                             ListSessions(xiiVariantArray& out_sessions) const;

private:
  const xiiMCPProtocol&         m_Protocol;
  xiiUniquePtr<xiiMCPHttpState> m_pState;
};
