/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <MCPPlugin/Bridge.h>
#include <MCPPlugin/HttpServer.h>

struct XII_MCPPLUGIN_DLL xiiMCPRuntimeConfiguration
{
  xiiString m_sApplication;
  xiiString m_sLocalEndpoint;
  xiiUInt8 m_uiPermissions = static_cast<xiiUInt8>(xiiMCPPermission::Read);
  bool m_bNative = false;
  bool m_bAdvertise = true;
  xiiMCPHttpConfiguration m_Http;
};

/// Applications with a custom main loop call Update at an idle world-update boundary.
class XII_MCPPLUGIN_DLL xiiMCPRuntime final
{
public:
  xiiMCPRuntime();
  ~xiiMCPRuntime();

  [[nodiscard]] xiiStatus Start(const xiiMCPRuntimeConfiguration& configuration);
  void Update();
  void Stop();
  [[nodiscard]] xiiMCPRegistry& GetRegistry() { return m_Registry; }

private:
  xiiMCPRegistry m_Registry;
  xiiMCPProtocol m_Protocol;
  xiiMCPBridge m_Bridge;
  xiiMCPHttpServer m_Http;
};
