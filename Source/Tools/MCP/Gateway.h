/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/Bridge.h>
#include <MCPPlugin/HttpServer.h>

struct xiiMCPGatewayConfiguration
{
  xiiDynamicArray<xiiString> m_Endpoints;
  xiiMCPHttpConfiguration m_Http;
  xiiUInt8 m_uiPermissions = static_cast<xiiUInt8>(xiiMCPPermission::Read);
  bool m_bDiscovery = true;
};

class xiiMCPGateway final
{
public:
  xiiMCPGateway();
  [[nodiscard]] xiiStatus Start(const xiiMCPGatewayConfiguration& configuration);
  void Update();
  void Process(xiiStringView sRequest, xiiMCPSession& ref_session, xiiStringBuilder& out_sReply);
  [[nodiscard]] xiiNetworkEndpoint GetEndpoint() const { return m_Http.GetEndpoint(); }
  [[nodiscard]] xiiVariantDictionary GetSnapshot() const;

private:
  struct Capability
  {
    xiiMCPCapability m_Kind = xiiMCPCapability::Tool;
    xiiString m_sKey;
  };

  struct Target
  {
    xiiString m_sEndpoint;
    xiiString m_sInstance;
    xiiString m_sApplication;
    xiiString m_sExpectedInstance;
    xiiNetworkConnection m_hConnection;
    xiiTime m_NextAttempt;
    xiiUInt64 m_uiRevision = 0;
    xiiDynamicArray<Capability> m_Capabilities;
    bool m_bExplicit = false;
    bool m_bPublished = false;
  };

  void AddTarget(xiiStringView sEndpoint, xiiStringView sInstance, xiiStringView sApplication, bool bExplicit);
  void RefreshTargets();
  void UpdateTarget(Target& ref_target, xiiTime now);
  void RemoveCapabilities(Target& ref_target);
  bool RefreshCapabilities(Target& ref_target, xiiUInt64 uiRevision);
  xiiStatus ListTargets(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_reply);
  xiiStatus Forward(xiiStringView sEndpoint, xiiStringView sInstance, xiiMCPCapability kind, xiiStringView sRemoteKey, xiiStringView sPublicKey,
                    const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);

  xiiMCPGatewayConfiguration m_Configuration;
  xiiMCPRegistry m_Registry;
  xiiMCPProtocol m_Protocol;
  xiiMCPBridge m_Bridge;
  xiiMCPHttpServer m_Http;
  xiiMap<xiiString, Target> m_Targets;
  xiiVariantArray m_Activity;
  xiiTime m_NextDiscovery;
  xiiUInt32 m_uiNextTarget = 0;
};
