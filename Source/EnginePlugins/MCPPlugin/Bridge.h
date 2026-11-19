/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <Foundation/Diagnostics/TelemetryDiscovery.h>
#include <Foundation/Network/NetworkHost.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>
#include <MCPPlugin/Registry.h>

class xiiMCPBridgeState;

/// Same-user native MCP transport. All calls and callbacks belong to a single owning thread.
class XII_MCPPLUGIN_DLL xiiMCPBridge final
{
public:
  xiiMCPBridge();
  ~xiiMCPBridge();

  [[nodiscard]] xiiStatus StartClient();
  [[nodiscard]] xiiStatus StartServer(xiiMCPRegistry& registry, xiiStringView sApplication, xiiStringView sEndpoint, xiiUInt8 uiPermissions, bool bAdvertise);
  void Stop();
  void Update();

  [[nodiscard]] xiiStatus Connect(xiiStringView sEndpoint, xiiNetworkConnection& out_hConnection);
  void Disconnect(xiiNetworkConnection hConnection);
  [[nodiscard]] bool IsConnected(xiiNetworkConnection hConnection) const;
  [[nodiscard]] xiiStatus Call(xiiNetworkConnection hConnection, const xiiVariantDictionary& request, xiiTime deadline, xiiVariantDictionary& out_response);

  [[nodiscard]] static xiiResult Discover(xiiDynamicArray<xiiTelemetryInstance>& out_instances);

private:
  xiiUniquePtr<xiiMCPBridgeState> m_pState;
};
