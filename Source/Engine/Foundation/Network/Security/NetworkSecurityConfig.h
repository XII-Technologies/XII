/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Strings/String.h>

/// Configuration is consumed into an immutable credential snapshot; no key is read per connection.
/// TLS 1.3 and peer certificate validation are mandatory. Development certificates must be supplied
/// as explicit trust anchors; there is no option to silently bypass trust or hostname checks.
struct XII_FOUNDATION_DLL xiiNetworkSecurityConfig
{
  xiiString m_sCertificateChainFile;
  xiiString m_sPrivateKeyFile;
  xiiString m_sTrustAnchorsFile;
  xiiString m_sCertificateRevocationFile;
  xiiString m_sApplicationProtocol       = "xii-network/1";
  bool      m_bUseSystemTrust            = true;
  bool      m_bRequireClientCertificate  = true;
  bool      m_bEnableTransportResumption = false;
  xiiUInt32 m_uiMaximumCachedSessions    = 128;
  xiiUInt32 m_uiMaximumCertificateBytes  = 65536;
};

/// Application authorization remains separate from TLS/QUIC peer authentication.
enum class xiiNetworkAuthenticationPolicy : xiiUInt8
{
  Certificate,
  Application,
  AnonymousDevelopment
};
