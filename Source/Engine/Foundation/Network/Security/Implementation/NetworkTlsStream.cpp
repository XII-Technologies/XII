/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/NetworkAddress.h>
#include <Foundation/Network/Security/Implementation/NetworkSecurityInternal.h>
#include <Foundation/Network/Security/NetworkTlsStream.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Types/UniquePtr.h>
#include <cstring>
#include <ctime>
#include <openssl/err.h>
#include <openssl/x509v3.h>

class xiiNetworkTlsStreamState
{
public:
  ~xiiNetworkTlsStreamState()
  {
    SSL_free(m_pTls);
    BIO_free(m_pNetwork);
    sk_X509_pop_free(m_pResumeChain, X509_free);
  }
  xiiSharedPtr<xiiNetworkCredentials> m_pCredentials;
  SSL*                                m_pTls     = nullptr;
  BIO*                                m_pNetwork = nullptr;
  STACK_OF(X509) * m_pResumeChain                = nullptr;
  xiiString             m_sServerName;
  xiiString             m_sOrigin;
  xiiNetworkCertificate m_Peer;
  xiiNetworkError       m_Failure;
  bool                  m_bServer             = false;
  bool                  m_bEstablished        = false;
  xiiUInt32             m_uiPendingWriteBytes = 0;

  xiiNetworkError Translate(int iResult)
  {
    const int iError = SSL_get_error(m_pTls, iResult);
    if (iError == SSL_ERROR_WANT_READ || iError == SSL_ERROR_WANT_WRITE)
    {
      return xiiNetworkError(xiiNetworkErrorCode::WouldBlock);
    }
    if (iError == SSL_ERROR_ZERO_RETURN)
    {
      return xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed);
    }
    const long iVerification = SSL_get_verify_result(m_pTls);
    m_Failure                = iVerification != X509_V_OK ? xiiNetworkSecurityInternal::VerificationError(iVerification) : xiiNetworkSecurityInternal::LastError();
    return m_Failure;
  }

  void CacheSession()
  {
    auto* pCredentials = xiiNetworkSecurityAccess::Get(*m_pCredentials);
    if (m_bServer || !m_bEstablished || !pCredentials->m_Configuration.m_bEnableTransportResumption)
    {
      return;
    }
    SSL_SESSION* pSession = SSL_get1_session(m_pTls);
    if (pSession == nullptr || SSL_SESSION_is_resumable(pSession) != 1)
    {
      SSL_SESSION_free(pSession);
      return;
    }
    XII_LOCK(pCredentials->m_Mutex);
    for (auto& entry : pCredentials->m_Sessions)
    {
      if (entry.m_sOrigin == m_sOrigin && entry.m_pSession == pSession)
      {
        SSL_SESSION_free(pSession);
        return;
      }
    }
    STACK_OF(X509)* pChain = SSL_get0_verified_chain(m_pTls);
    if (pChain == nullptr)
    {
      pChain = m_pResumeChain;
    }
    STACK_OF(X509)* pCopy             = pChain != nullptr ? X509_chain_up_ref(pChain) : nullptr;
    xiiNetworkTlsSessionEntry* pEntry = nullptr;
    for (auto& entry : pCredentials->m_Sessions)
    {
      if (entry.m_sOrigin == m_sOrigin)
      {
        pEntry = &entry;
        break;
      }
      if (pEntry == nullptr || entry.m_uiAge < pEntry->m_uiAge)
      {
        pEntry = &entry;
      }
    }
    if (pCredentials->m_Sessions.GetCount() < pCredentials->m_Configuration.m_uiMaximumCachedSessions && (pEntry == nullptr || pEntry->m_sOrigin != m_sOrigin))
    {
      pEntry = &pCredentials->m_Sessions.ExpandAndGetRef();
    }
    SSL_SESSION_free(pEntry->m_pSession);
    sk_X509_pop_free(pEntry->m_pChain, X509_free);
    pEntry->m_pSession = pSession;
    pEntry->m_pChain   = pCopy;
    pEntry->m_sOrigin  = m_sOrigin;
    pEntry->m_uiAge    = ++pCredentials->m_uiSessionAge;
  }
};

xiiNetworkTlsStream::xiiNetworkTlsStream() = default;
xiiNetworkTlsStream::~xiiNetworkTlsStream() { XII_DEFAULT_DELETE(m_pState); }

xiiNetworkError xiiNetworkTlsStream::Initialize(xiiSharedPtr<xiiNetworkCredentials> pCredentials, bool bServer, xiiStringView sServerName, xiiUInt16 uiOriginPort)
{
  using namespace xiiNetworkSecurityInternal;
  if (m_pState != nullptr)
  {
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyOpen);
  }
  if (pCredentials == nullptr || !pCredentials->IsLoaded() || (bServer && !pCredentials->HasIdentity()) ||
      (!bServer && sServerName.IsEmpty()) || sServerName.GetElementCount() > 253 ||
      (!sServerName.IsEmpty() && std::memchr(sServerName.GetStartPointer(), 0, sServerName.GetElementCount()) != nullptr))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  ERR_clear_error();
  xiiUniquePtr<xiiNetworkTlsStreamState> pState = XII_DEFAULT_NEW(xiiNetworkTlsStreamState);
  pState->m_pCredentials                        = pCredentials;
  pState->m_sServerName                         = sServerName;
  pState->m_bServer                             = bServer;
  auto* pCredentialState                        = xiiNetworkSecurityAccess::Get(*pCredentials);
  pState->m_pTls                                = SSL_new(pCredentialState->m_pContext);
  BIO* pInternal                                = nullptr;
  if (pState->m_pTls == nullptr || BIO_new_bio_pair(&pInternal, 65536, &pState->m_pNetwork, 65536) != 1)
  {
    BIO_free(pInternal);
    return LastError();
  }
  SSL_set_bio(pState->m_pTls, pInternal, pInternal);
  if (bServer)
  {
    SSL_set_accept_state(pState->m_pTls);
    SSL_set_verify(pState->m_pTls, SSL_VERIFY_PEER | (pCredentialState->m_Configuration.m_bRequireClientCertificate ? SSL_VERIFY_FAIL_IF_NO_PEER_CERT : 0), nullptr);
  }
  else
  {
    SSL_set_connect_state(pState->m_pTls);
    const auto&   sProtocol   = pCredentialState->m_Configuration.m_sApplicationProtocol;
    unsigned char uiAlpn[256] = {};
    uiAlpn[0]                 = static_cast<unsigned char>(sProtocol.GetElementCount());
    std::memcpy(uiAlpn + 1, sProtocol.GetData(), sProtocol.GetElementCount());
    if (SSL_set_alpn_protos(pState->m_pTls, uiAlpn, sProtocol.GetElementCount() + 1) != 0)
    {
      return LastError();
    }
    xiiNetworkAddress address;
    auto*             pParameters = SSL_get0_param(pState->m_pTls);
    X509_VERIFY_PARAM_set_hostflags(pParameters, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS | X509_CHECK_FLAG_NEVER_CHECK_SUBJECT);
    if (xiiNetworkAddress::Parse(sServerName, address).Succeeded())
    {
      if (X509_VERIFY_PARAM_set1_ip(pParameters, address.GetBytes().GetPtr(), address.GetBytes().GetCount()) != 1)
      {
        return LastError();
      }
    }
    else if (SSL_set_tlsext_host_name(pState->m_pTls, pState->m_sServerName.GetData()) != 1 || SSL_set1_host(pState->m_pTls, pState->m_sServerName.GetData()) != 1)
    {
      return LastError();
    }
    xiiStringBuilder sOrigin;
    sOrigin.SetFormat("[{}]:{}/{}", sServerName, uiOriginPort, sProtocol);
    pState->m_sOrigin = sOrigin;
    if (pCredentialState->m_Configuration.m_bEnableTransportResumption)
    {
      XII_LOCK(pCredentialState->m_Mutex);
      for (auto& entry : pCredentialState->m_Sessions)
      {
        if (entry.m_sOrigin == sOrigin && SSL_SESSION_is_resumable(entry.m_pSession) == 1)
        {
          if (SSL_set_session(pState->m_pTls, entry.m_pSession) != 1)
          {
            return LastError();
          }
          pState->m_pResumeChain = entry.m_pChain != nullptr ? X509_chain_up_ref(entry.m_pChain) : nullptr;
          break;
        }
      }
    }
  }
  m_pState = pState.Release();
  return {};
}

xiiNetworkError xiiNetworkTlsStream::Handshake()
{
  using namespace xiiNetworkSecurityInternal;
  if (m_pState == nullptr)
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  auto& state = *m_pState;
  if (state.m_Failure.Failed() || state.m_bEstablished)
  {
    return state.m_Failure;
  }
  ERR_clear_error();
  const int iResult = SSL_do_handshake(state.m_pTls);
  if (iResult != 1)
  {
    return state.Translate(iResult);
  }
  const unsigned char* pAlpn       = nullptr;
  unsigned int         uiAlpnBytes = 0;
  SSL_get0_alpn_selected(state.m_pTls, &pAlpn, &uiAlpnBytes);
  const auto& configuration = state.m_pCredentials->GetConfiguration();
  if (uiAlpnBytes != configuration.m_sApplicationProtocol.GetElementCount() ||
      std::memcmp(pAlpn, configuration.m_sApplicationProtocol.GetData(), uiAlpnBytes) != 0)
  {
    state.m_Failure = xiiNetworkError(xiiNetworkErrorCode::ProtocolMismatch);
    return state.m_Failure;
  }
  X509* pPeer            = SSL_get0_peer_certificate(state.m_pTls);
  STACK_OF(X509)* pChain = SSL_get0_verified_chain(state.m_pTls);
  if (pChain == nullptr)
  {
    pChain = state.m_pResumeChain;
  }
  if (pPeer != nullptr)
  {
    state.m_Failure = VerifyPeer(*state.m_pCredentials, pPeer, pChain, state.m_sServerName, !state.m_bServer, state.m_Peer);
  }
  else if (!state.m_bServer || configuration.m_bRequireClientCertificate)
  {
    state.m_Failure = xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
  }
  state.m_bEstablished = state.m_Failure.Succeeded();
  state.CacheSession();
  return state.m_Failure;
}

xiiNetworkOperationResult xiiNetworkTlsStream::FeedCiphertext(xiiArrayPtr<const xiiUInt8> bytes)
{
  if (m_pState == nullptr)
  {
    return {xiiNetworkError(xiiNetworkErrorCode::NotOpen)};
  }
  if (bytes.IsEmpty())
  {
    return {};
  }
  if (bytes.GetPtr() == nullptr)
  {
    return {xiiNetworkError(xiiNetworkErrorCode::InvalidArgument)};
  }
  const int iBytes = BIO_write(m_pState->m_pNetwork, bytes.GetPtr(), static_cast<int>(xiiMath::Min(bytes.GetCount(), 16384U)));
  return iBytes > 0 ? xiiNetworkOperationResult{{}, static_cast<xiiUInt32>(iBytes)} : xiiNetworkOperationResult{xiiNetworkError(BIO_should_retry(m_pState->m_pNetwork) ? xiiNetworkErrorCode::WouldBlock : xiiNetworkErrorCode::TlsError)};
}

xiiNetworkOperationResult xiiNetworkTlsStream::DrainCiphertext(xiiArrayPtr<xiiUInt8> bytes)
{
  if (m_pState == nullptr)
  {
    return {xiiNetworkError(xiiNetworkErrorCode::NotOpen)};
  }
  if (bytes.IsEmpty())
  {
    return {};
  }
  if (bytes.GetPtr() == nullptr)
  {
    return {xiiNetworkError(xiiNetworkErrorCode::InvalidArgument)};
  }
  const int iBytes = BIO_read(m_pState->m_pNetwork, bytes.GetPtr(), static_cast<int>(xiiMath::Min(bytes.GetCount(), 16384U)));
  return iBytes > 0 ? xiiNetworkOperationResult{{}, static_cast<xiiUInt32>(iBytes)} : xiiNetworkOperationResult{xiiNetworkError(xiiNetworkErrorCode::WouldBlock)};
}

xiiNetworkOperationResult xiiNetworkTlsStream::Write(xiiArrayPtr<const xiiUInt8> bytes)
{
  if (!IsEstablished())
  {
    return {m_pState != nullptr && m_pState->m_Failure.Failed() ? m_pState->m_Failure : xiiNetworkError(xiiNetworkErrorCode::NotConnected)};
  }
  if (bytes.IsEmpty())
  {
    return {};
  }
  const xiiUInt32 uiCount = xiiMath::Min(bytes.GetCount(), 16384U);
  if (bytes.GetPtr() == nullptr || (m_pState->m_uiPendingWriteBytes != 0 && m_pState->m_uiPendingWriteBytes != uiCount))
  {
    return {xiiNetworkError(xiiNetworkErrorCode::InvalidArgument)};
  }
  ERR_clear_error();
  size_t uiWritten = 0;
  if (SSL_write_ex(m_pState->m_pTls, bytes.GetPtr(), uiCount, &uiWritten) == 1)
  {
    m_pState->m_uiPendingWriteBytes = 0;
    return {{}, static_cast<xiiUInt32>(uiWritten)};
  }
  const auto error                = m_pState->Translate(0);
  m_pState->m_uiPendingWriteBytes = error.m_Code == xiiNetworkErrorCode::WouldBlock ? uiCount : 0;
  return {error};
}

xiiNetworkOperationResult xiiNetworkTlsStream::Read(xiiArrayPtr<xiiUInt8> bytes)
{
  if (!IsEstablished())
  {
    return {m_pState != nullptr && m_pState->m_Failure.Failed() ? m_pState->m_Failure : xiiNetworkError(xiiNetworkErrorCode::NotConnected)};
  }
  if (bytes.IsEmpty())
  {
    return {};
  }
  if (bytes.GetPtr() == nullptr)
  {
    return {xiiNetworkError(xiiNetworkErrorCode::InvalidArgument)};
  }
  ERR_clear_error();
  size_t     uiRead  = 0;
  const int  iResult = SSL_read_ex(m_pState->m_pTls, bytes.GetPtr(), xiiMath::Min(bytes.GetCount(), 16384U), &uiRead);
  const auto error   = iResult == 1 ? xiiNetworkError() : m_pState->Translate(iResult);
  m_pState->CacheSession();
  return {error, static_cast<xiiUInt32>(uiRead)};
}

void xiiNetworkTlsStream::NotifyEndOfInput()
{
  if (m_pState != nullptr)
  {
    BIO_shutdown_wr(m_pState->m_pNetwork);
  }
}

xiiNetworkError xiiNetworkTlsStream::Shutdown()
{
  if (!IsEstablished())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotConnected);
  }
  ERR_clear_error();
  const int iResult = SSL_shutdown(m_pState->m_pTls);
  return iResult == 1 ? xiiNetworkError() : iResult == 0 ? xiiNetworkError(xiiNetworkErrorCode::WouldBlock) :
                                                           m_pState->Translate(iResult);
}

bool            xiiNetworkTlsStream::IsEstablished() const { return m_pState != nullptr && m_pState->m_bEstablished && m_pState->m_Failure.Succeeded(); }
bool            xiiNetworkTlsStream::WasSessionResumed() const { return IsEstablished() && SSL_session_reused(m_pState->m_pTls) == 1; }
xiiUInt32       xiiNetworkTlsStream::GetPendingCiphertextBytes() const { return m_pState != nullptr ? static_cast<xiiUInt32>(BIO_ctrl_pending(m_pState->m_pNetwork)) : 0; }
xiiNetworkError xiiNetworkTlsStream::GetPeerCertificate(xiiNetworkCertificate& out_certificate) const
{
  if (!IsEstablished() || m_pState->m_Peer.m_Der.IsEmpty())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotConnected);
  }
  out_certificate = m_pState->m_Peer;
  return {};
}
