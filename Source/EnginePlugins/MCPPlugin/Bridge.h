/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <Foundation/Diagnostics/TelemetryDiscovery.h>
#include <Foundation/Network/NetworkHost.h>
#include <Foundation/Network/Rpc/NetworkRpc.h>
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
  void                    Stop();
  void                    Update();
  void                    SetServerPermissions(xiiUInt8 uiPermissions);

  [[nodiscard]] xiiStatus Connect(xiiStringView sEndpoint, xiiNetworkConnection& out_hConnection);
  void                    Disconnect(xiiNetworkConnection hConnection);
  [[nodiscard]] bool      IsConnected(xiiNetworkConnection hConnection) const;
  [[nodiscard]] xiiStatus BeginCall(xiiNetworkConnection hConnection, const xiiVariantDictionary& request, xiiTime deadline, xiiNetworkRpcRequest& out_request);
  [[nodiscard]] bool      PollCall(xiiNetworkRpcRequest request, xiiStatus& out_status, xiiVariantDictionary& out_response);
  void                    CancelCall(xiiNetworkRpcRequest request);

  [[nodiscard]] static xiiResult Discover(xiiDynamicArray<xiiTelemetryInstance>& out_instances);

private:
  xiiUniquePtr<xiiMCPBridgeState> m_pState;
};
