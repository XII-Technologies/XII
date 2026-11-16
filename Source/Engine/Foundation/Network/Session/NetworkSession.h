/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/NetworkChannel.h>
#include <Foundation/Network/Security/NetworkCredentials.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>

class xiiNetworkHostRuntime;

/// Immutable establishment metadata retained by connection events. Independent of native slot reuse;
/// may outlive the host. Contains peer certificates, never private keys or recovery capabilities.
/// State/statistics describe activation; use host queries for live connection statistics.
class XII_FOUNDATION_DLL xiiNetworkSessionInfo final : public xiiRefCountingImpl
{
public:
  xiiNetworkConnection     m_Connection;
  xiiNetworkChannel        m_DefaultChannel;
  xiiNetworkConnectionInfo m_ConnectionInfo;
  xiiNetworkCertificate    m_PeerCertificate;
};


/// Secret-bearing, immutable recovery capability. Explicit shared owners retain it until release.
/// There is no serialization or secret-byte accessor. Host-origin/certificate/protocol binding is
/// checked before transmitting it over an authenticated transport. Server restart invalidates it.
class XII_FOUNDATION_DLL xiiNetworkSessionTicket final : public xiiRefCountingImpl
{
public:
  xiiNetworkSessionTicket() = default;
  ~xiiNetworkSessionTicket();
  xiiNetworkSessionTicket(const xiiNetworkSessionTicket&)            = delete;
  xiiNetworkSessionTicket& operator=(const xiiNetworkSessionTicket&) = delete;
  [[nodiscard]] bool       IsValid() const;
  [[nodiscard]] xiiUuid    GetSession() const { return m_Session; }

private:
  friend class xiiNetworkHostRuntime;
  xiiUuid   m_Session;
  xiiUuid   m_Protocol;
  xiiString m_sOrigin;
  xiiUInt16 m_uiPort                  = 0;
  xiiUInt8  m_uiSecret[32]            = {};
  xiiUInt8  m_uiServerCertificate[32] = {};
  xiiTime   m_Expires;
};

enum class xiiNetworkReconnectMode : xiiUInt8
{
  Disabled,
  Fixed,
  Exponential
};

struct XII_FOUNDATION_DLL xiiNetworkReconnectPolicy
{
  xiiNetworkReconnectMode m_Mode = xiiNetworkReconnectMode::Disabled;
  /// Finite attempt count; zero is valid only with explicit unbounded retry opt-in.
  xiiUInt32 m_uiMaximumAttempts      = 5;
  bool      m_bAllowUnboundedRetries = false;
  xiiTime   m_InitialDelay           = xiiTime::MakeFromMilliseconds(250);
  xiiTime   m_MaximumDelay           = xiiTime::MakeFromSeconds(10);
  double    m_fJitterFraction        = 0.2;
};

struct XII_FOUNDATION_DLL xiiNetworkAuthorization
{
  xiiString m_sIdentity;
  bool      m_bMaySend          = true;
  bool      m_bMayOpenChannels  = true;
  bool      m_bMayResumeSession = true;
};

struct XII_FOUNDATION_DLL xiiNetworkConnectOptions
{
  /// Required for secure transports, even when connecting directly to an endpoint.
  xiiString                                  m_sServerName;
  xiiSharedPtr<xiiNetworkAuthenticationData> m_pAuthentication;
  xiiSharedPtr<xiiNetworkSessionTicket>      m_pSessionTicket;
  xiiNetworkReconnectPolicy                  m_Reconnect;
  /// Expired/rejected recovery defaults to failure; fresh-session fallback is explicit.
  bool m_bAllowFreshSession = false;
};
