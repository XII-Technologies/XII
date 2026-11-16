/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/Security/Implementation/NetworkCertificatePlatform.h>
#include <Foundation/Network/Security/NetworkCredentials.h>
#include <Foundation/Threading/Mutex.h>

#include <openssl/ssl.h>

XII_FOUNDATION_INTERNAL_HEADER

struct xiiNetworkTlsSessionEntry
{
  xiiString    m_sOrigin;
  SSL_SESSION* m_pSession   = nullptr;
  xiiUInt64    m_uiAge      = 0;
  STACK_OF(X509) * m_pChain = nullptr;
};

class xiiNetworkCredentialsState
{
public:
  xiiNetworkCredentialsState() = default;
  ~xiiNetworkCredentialsState();
  xiiNetworkCredentialsState(const xiiNetworkCredentialsState&)                      = delete;
  xiiNetworkCredentialsState& operator=(const xiiNetworkCredentialsState&)           = delete;
  xiiNetworkCredentialsState(xiiNetworkCredentialsState&&)                           = delete;
  xiiNetworkCredentialsState&                operator=(xiiNetworkCredentialsState&&) = delete;
  xiiNetworkSecurityConfig                   m_Configuration;
  SSL_CTX*                                   m_pContext = nullptr;
  xiiDynamicArray<xiiUInt8>                  m_Pkcs12;
  mutable xiiMutex                           m_Mutex;
  xiiDynamicArray<xiiNetworkTlsSessionEntry> m_Sessions;
  xiiUInt64                                  m_uiSessionAge = 0;
};

class xiiNetworkSecurityAccess
{
public:
  static xiiNetworkCredentialsState* Get(xiiNetworkCredentials& credentials) { return credentials.m_pState; }
};

namespace xiiNetworkSecurityInternal
{
  xiiNetworkError LastError(xiiNetworkErrorCode fallback = xiiNetworkErrorCode::TlsError);
  xiiNetworkError VerificationError(long iError);
  xiiNetworkError InspectCertificate(X509* pCertificate, xiiNetworkCertificate& out_certificate);
  xiiNetworkError VerifyPeer(xiiNetworkCredentials& credentials, X509* pCertificate, STACK_OF(X509) * pChain,
                             xiiStringView sServerName, bool bServerCertificate, xiiNetworkCertificate& out_certificate);
  xiiNetworkError RandomBytes(xiiArrayPtr<xiiUInt8> bytes);
  void            ClearSecret(xiiArrayPtr<xiiUInt8> bytes);
} // namespace xiiNetworkSecurityInternal
