/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Security/NetworkCredentials.h>

#include <Foundation/Network/NetworkAddress.h>
#include <Foundation/Network/Security/Implementation/NetworkSecurityInternal.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Types/UniquePtr.h>

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>

#include <climits>
#include <cstring>
#include <ctime>
#include <utility>

namespace
{
  struct CredentialFileBytes
  {
    CredentialFileBytes()                                      = default;
    CredentialFileBytes(const CredentialFileBytes&)            = delete;
    CredentialFileBytes& operator=(const CredentialFileBytes&) = delete;
    CredentialFileBytes(CredentialFileBytes&&)                 = delete;
    CredentialFileBytes&      operator=(CredentialFileBytes&&) = delete;
    xiiDynamicArray<xiiUInt8> m_Bytes;
    ~CredentialFileBytes() { xiiNetworkSecurityInternal::ClearSecret(m_Bytes); }
  };

  int SelectNetworkAlpn(SSL*, const unsigned char** ppOutput, unsigned char* pOutputLength, const unsigned char* pInput, unsigned int uiInputBytes, void* pData)
  {
    const auto&     configuration = static_cast<xiiNetworkCredentialsState*>(pData)->m_Configuration;
    const xiiUInt32 uiExpected    = configuration.m_sApplicationProtocol.GetElementCount();
    unsigned int    uiOffset      = 0;
    while (uiOffset < uiInputBytes)
    {
      const unsigned int uiLength = pInput[uiOffset++];
      if (uiLength == 0 || uiLength > uiInputBytes - uiOffset)
      {
        return SSL_TLSEXT_ERR_ALERT_FATAL;
      }
      if (uiLength == uiExpected && std::memcmp(pInput + uiOffset, configuration.m_sApplicationProtocol.GetData(), uiLength) == 0)
      {
        *ppOutput      = pInput + uiOffset;
        *pOutputLength = static_cast<unsigned char>(uiLength);
        return SSL_TLSEXT_ERR_OK;
      }
      uiOffset += uiLength;
    }
    return SSL_TLSEXT_ERR_ALERT_FATAL;
  }

  int PrivateKeyPassword(char* pBuffer, int iCapacity, int, void* pData)
  {
    if (pData == nullptr)
    {
      return 0;
    }
    const auto& bytes = *static_cast<const xiiArrayPtr<const xiiUInt8>*>(pData);
    if (iCapacity < 0 || bytes.GetCount() > static_cast<xiiUInt32>(iCapacity))
    {
      return 0;
    }
    if (!bytes.IsEmpty())
    {
      std::memcpy(pBuffer, bytes.GetPtr(), bytes.GetCount());
    }
    return static_cast<int>(bytes.GetCount());
  }

  xiiTimestamp CertificateTime(const ASN1_TIME* pTime)
  {
    std::tm time = {};
    if (ASN1_TIME_to_tm(pTime, &time) != 1 || time.tm_year < -1899 || time.tm_year > 8099 ||
        time.tm_mon < 0 || time.tm_mon > 11 || time.tm_mday < 1 || time.tm_mday > 31 ||
        time.tm_hour < 0 || time.tm_hour > 23 || time.tm_min < 0 || time.tm_min > 59 || time.tm_sec < 0 || time.tm_sec > 59)
    {
      return xiiTimestamp::MakeInvalid();
    }
    xiiDateTime date;
    date.SetYear(static_cast<xiiInt16>(time.tm_year + 1900));
    date.SetMonth(static_cast<xiiUInt8>(time.tm_mon + 1));
    date.SetDay(static_cast<xiiUInt8>(time.tm_mday));
    date.SetHour(static_cast<xiiUInt8>(time.tm_hour));
    date.SetMinute(static_cast<xiiUInt8>(time.tm_min));
    date.SetSecond(static_cast<xiiUInt8>(time.tm_sec));
    return date.GetTimestamp();
  }

  xiiString CertificateName(X509_NAME* pName)
  {
    xiiString result;
    BIO*      pBio = BIO_new(BIO_s_mem());
    if (pBio != nullptr)
    {
      if (X509_NAME_print_ex(pBio, pName, 0, XN_FLAG_RFC2253) >= 0)
      {
        char*      pText  = nullptr;
        const long iCount = BIO_get_mem_data(pBio, &pText);
        if (iCount > 0)
        {
          result = xiiStringView(pText, static_cast<xiiUInt32>(xiiMath::Min(iCount, 1024L)));
        }
      }
      BIO_free(pBio);
    }
    return result;
  }

  xiiNetworkError LoadIdentity(xiiNetworkCredentialsState& state, xiiArrayPtr<const xiiUInt8> password)
  {
    using namespace xiiNetworkSecurityInternal;
    CredentialFileBytes certificate;
    CredentialFileBytes key;
    auto                error = LoadCredentialFile(state.m_Configuration.m_sCertificateChainFile, 1024 * 1024, certificate.m_Bytes);
    if (error.Failed())
    {
      return error;
    }
    error = LoadCredentialFile(state.m_Configuration.m_sPrivateKeyFile, 1024 * 1024, key.m_Bytes);
    if (error.Failed())
    {
      return error;
    }
    BIO* pCertificateBio = BIO_new_mem_buf(certificate.m_Bytes.GetData(), static_cast<int>(certificate.m_Bytes.GetCount()));
    if (pCertificateBio == nullptr)
    {
      return LastError();
    }
    X509* pCertificate = PEM_read_bio_X509_AUX(pCertificateBio, nullptr, nullptr, nullptr);
    if (pCertificate == nullptr || SSL_CTX_use_certificate(state.m_pContext, pCertificate) != 1)
    {
      X509_free(pCertificate);
      BIO_free(pCertificateBio);
      return LastError();
    }
    X509_free(pCertificate);
    xiiUInt32 uiDepth = 0;
    while (true)
    {
      X509* pChain = PEM_read_bio_X509(pCertificateBio, nullptr, nullptr, nullptr);
      if (pChain == nullptr)
      {
        const unsigned long uiError = ERR_peek_last_error();
        if (ERR_GET_REASON(uiError) != PEM_R_NO_START_LINE && uiError != 0)
        {
          BIO_free(pCertificateBio);
          return LastError();
        }
        ERR_clear_error();
        break;
      }
      if (++uiDepth > 16 || SSL_CTX_add_extra_chain_cert(state.m_pContext, pChain) != 1)
      {
        X509_free(pChain);
        BIO_free(pCertificateBio);
        return LastError(xiiNetworkErrorCode::CertificateUntrusted);
      }
    }
    BIO_free(pCertificateBio);
    BIO* pKeyBio = BIO_new_mem_buf(key.m_Bytes.GetData(), static_cast<int>(key.m_Bytes.GetCount()));
    if (pKeyBio == nullptr)
    {
      return LastError();
    }
    EVP_PKEY* pKey = PEM_read_bio_PrivateKey(pKeyBio, nullptr, PrivateKeyPassword, &password);
    BIO_free(pKeyBio);
    if (pKey == nullptr)
    {
      return LastError();
    }
    const bool bValid = SSL_CTX_use_PrivateKey(state.m_pContext, pKey) == 1 && SSL_CTX_check_private_key(state.m_pContext) == 1;
    EVP_PKEY_free(pKey);
    if (!bValid)
    {
      return LastError();
    }
    STACK_OF(X509)* pChain = nullptr;
    SSL_CTX_get_extra_chain_certs(state.m_pContext, &pChain);
    PKCS12* pPkcs12 = PKCS12_create("", "XII Network", SSL_CTX_get0_privatekey(state.m_pContext), SSL_CTX_get0_certificate(state.m_pContext), pChain, 0, 0, 0, 0, 0);
    if (pPkcs12 == nullptr)
    {
      return LastError();
    }
    const int iLength = i2d_PKCS12(pPkcs12, nullptr);
    if (iLength <= 0 || iLength > 1024 * 1024)
    {
      PKCS12_free(pPkcs12);
      return LastError();
    }
    state.m_Pkcs12.SetCountUninitialized(static_cast<xiiUInt32>(iLength));
    unsigned char* pOutput  = state.m_Pkcs12.GetData();
    const int      iWritten = i2d_PKCS12(pPkcs12, &pOutput);
    PKCS12_free(pPkcs12);
    return iWritten == iLength ? xiiNetworkError() : LastError();
  }

  xiiNetworkError LoadTrustFile(SSL_CTX* pContext, xiiStringView sPath, bool bRevocation)
  {
    using namespace xiiNetworkSecurityInternal;
    CredentialFileBytes file;
    auto                error = LoadCredentialFile(sPath, 16 * 1024 * 1024, file.m_Bytes);
    if (error.Failed())
    {
      return error;
    }
    BIO* pBio = BIO_new_mem_buf(file.m_Bytes.GetData(), static_cast<int>(file.m_Bytes.GetCount()));
    if (pBio == nullptr)
    {
      return LastError();
    }
    STACK_OF(X509_INFO)* pEntries = PEM_X509_INFO_read_bio(pBio, nullptr, PrivateKeyPassword, nullptr);
    BIO_free(pBio);
    if (pEntries == nullptr)
    {
      return LastError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    bool        bAdded  = false;
    bool        bFailed = false;
    X509_STORE* pStore  = SSL_CTX_get_cert_store(pContext);
    for (int iIndex = 0; iIndex < sk_X509_INFO_num(pEntries); ++iIndex)
    {
      const auto* pEntry  = sk_X509_INFO_value(pEntries, iIndex);
      int         iResult = 1;
      if (bRevocation && pEntry->crl != nullptr)
      {
        iResult = X509_STORE_add_crl(pStore, pEntry->crl);
        bAdded  = true;
      }
      else if (!bRevocation && pEntry->x509 != nullptr)
      {
        iResult = X509_STORE_add_cert(pStore, pEntry->x509);
        bAdded  = true;
      }
      if (iResult != 1)
      {
        if (ERR_GET_REASON(ERR_peek_last_error()) == X509_R_CERT_ALREADY_IN_HASH_TABLE)
        {
          ERR_clear_error();
        }
        else
        {
          bFailed = true;
        }
      }
    }
    sk_X509_INFO_pop_free(pEntries, X509_INFO_free);
    if (bFailed || !bAdded)
    {
      return LastError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    if (bRevocation && X509_STORE_set_flags(pStore, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL) != 1)
    {
      return LastError();
    }
    return {};
  }
} // namespace

namespace xiiNetworkSecurityInternal
{
  xiiNetworkError LastError(xiiNetworkErrorCode fallback)
  {
    const auto uiError = ERR_get_error();
    ERR_clear_error();
    return xiiNetworkError(fallback, static_cast<xiiInt32>(static_cast<xiiUInt32>(uiError)), xiiNetworkErrorDomain::Tls);
  }

  xiiNetworkError VerificationError(long iError)
  {
    auto code = xiiNetworkErrorCode::CertificateUntrusted;
    if (iError == X509_V_OK)
    {
      return {};
    }
    if (iError == X509_V_ERR_CERT_HAS_EXPIRED || iError == X509_V_ERR_CERT_NOT_YET_VALID)
    {
      code = xiiNetworkErrorCode::CertificateExpired;
    }
    else if (iError == X509_V_ERR_HOSTNAME_MISMATCH || iError == X509_V_ERR_IP_ADDRESS_MISMATCH)
    {
      code = xiiNetworkErrorCode::HostnameMismatch;
    }
    return xiiNetworkError(code, static_cast<xiiInt32>(iError), xiiNetworkErrorDomain::Tls);
  }

  xiiNetworkError RandomBytes(xiiArrayPtr<xiiUInt8> bytes)
  {
    if (bytes.GetCount() > INT_MAX || (!bytes.IsEmpty() && bytes.GetPtr() == nullptr))
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    return bytes.IsEmpty() || RAND_bytes(bytes.GetPtr(), static_cast<int>(bytes.GetCount())) == 1 ? xiiNetworkError() : LastError();
  }

  void ClearSecret(xiiArrayPtr<xiiUInt8> bytes)
  {
    if (!bytes.IsEmpty())
    {
      OPENSSL_cleanse(bytes.GetPtr(), bytes.GetCount());
    }
  }

  xiiNetworkError InspectCertificate(X509* pCertificate, xiiNetworkCertificate& out_certificate)
  {
    if (pCertificate == nullptr)
    {
      return xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    xiiNetworkCertificate certificate;
    unsigned int          uiSize  = 0;
    const int             iLength = i2d_X509(pCertificate, nullptr);
    if (iLength <= 0 || iLength > 65536 || X509_digest(pCertificate, EVP_sha256(), certificate.m_uiSha256, &uiSize) != 1 || uiSize != 32)
    {
      return LastError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    certificate.m_Der.SetCountUninitialized(static_cast<xiiUInt32>(iLength));
    unsigned char* pOutput = certificate.m_Der.GetData();
    if (i2d_X509(pCertificate, &pOutput) != iLength)
    {
      return LastError();
    }
    certificate.m_sSubject   = CertificateName(X509_get_subject_name(pCertificate));
    certificate.m_sIssuer    = CertificateName(X509_get_issuer_name(pCertificate));
    certificate.m_ValidFrom  = CertificateTime(X509_get0_notBefore(pCertificate));
    certificate.m_ValidUntil = CertificateTime(X509_get0_notAfter(pCertificate));
    if (!certificate.m_ValidFrom.IsValid() || !certificate.m_ValidUntil.IsValid())
    {
      return xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    out_certificate = std::move(certificate);
    return {};
  }

  xiiNetworkError LoadDefaultTrust(void* pContext)
  {
    return SSL_CTX_set_default_verify_paths(static_cast<SSL_CTX*>(pContext)) == 1 ? xiiNetworkError() : LastError(xiiNetworkErrorCode::InitializationFailed);
  }

  xiiNetworkError AddTrustCertificate(void* pContext, xiiArrayPtr<const xiiUInt8> bytes)
  {
    if (bytes.IsEmpty() || bytes.GetCount() > INT_MAX)
    {
      return xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    const unsigned char* pInput       = bytes.GetPtr();
    X509*                pCertificate = d2i_X509(nullptr, &pInput, static_cast<long>(bytes.GetCount()));
    if (pCertificate == nullptr || pInput != bytes.GetEndPtr())
    {
      X509_free(pCertificate);
      return LastError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    const int iResult = X509_STORE_add_cert(SSL_CTX_get_cert_store(static_cast<SSL_CTX*>(pContext)), pCertificate);
    X509_free(pCertificate);
    if (iResult == 1)
    {
      return {};
    }
    if (ERR_GET_REASON(ERR_peek_last_error()) == X509_R_CERT_ALREADY_IN_HASH_TABLE)
    {
      ERR_clear_error();
      return {};
    }
    return LastError();
  }

  xiiNetworkError VerifyPeer(xiiNetworkCredentials& credentials, X509* pCertificate, STACK_OF(X509) * pChain,
                             xiiStringView sServerName, bool bServerCertificate, xiiNetworkCertificate& out_certificate)
  {
    auto* pState = xiiNetworkSecurityAccess::Get(credentials);
    if (pState == nullptr || pCertificate == nullptr || sServerName.GetElementCount() > 253 ||
        (!sServerName.IsEmpty() && std::memchr(sServerName.GetStartPointer(), 0, sServerName.GetElementCount()) != nullptr))
    {
      return xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    const int iLeafBytes         = i2d_X509(pCertificate, nullptr);
    xiiUInt32 uiCertificateBytes = 0;
    const int iChainCount        = sk_X509_num(pChain);
    if (iLeafBytes <= 0 || static_cast<xiiUInt32>(iLeafBytes) > pState->m_Configuration.m_uiMaximumCertificateBytes || iChainCount > 17)
    {
      return xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    for (int iIndex = 0; iIndex < iChainCount; ++iIndex)
    {
      const int iBytes = i2d_X509(sk_X509_value(pChain, iIndex), nullptr);
      if (iBytes <= 0 || static_cast<xiiUInt32>(iBytes) > pState->m_Configuration.m_uiMaximumCertificateBytes - uiCertificateBytes)
      {
        return xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
      }
      uiCertificateBytes += static_cast<xiiUInt32>(iBytes);
    }
    X509_STORE_CTX* pVerification = X509_STORE_CTX_new();
    if (pVerification == nullptr)
    {
      return LastError();
    }
    if (X509_STORE_CTX_init(pVerification, SSL_CTX_get_cert_store(pState->m_pContext), pCertificate, pChain) != 1)
    {
      X509_STORE_CTX_free(pVerification);
      return LastError();
    }
    X509_VERIFY_PARAM* pParameters = X509_STORE_CTX_get0_param(pVerification);
    X509_VERIFY_PARAM_set_depth(pParameters, 16);
    X509_VERIFY_PARAM_set_flags(pParameters, X509_V_FLAG_X509_STRICT | X509_V_FLAG_TRUSTED_FIRST);
    X509_VERIFY_PARAM_set_hostflags(pParameters, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS | X509_CHECK_FLAG_NEVER_CHECK_SUBJECT);
    int iConfigured = X509_VERIFY_PARAM_set_purpose(pParameters, bServerCertificate ? X509_PURPOSE_SSL_SERVER : X509_PURPOSE_SSL_CLIENT);
    if (bServerCertificate)
    {
      if (sServerName.IsEmpty())
      {
        X509_STORE_CTX_free(pVerification);
        return xiiNetworkError(xiiNetworkErrorCode::HostnameMismatch);
      }
      xiiNetworkAddress address;
      if (xiiNetworkAddress::Parse(sServerName, address).Succeeded())
      {
        iConfigured = iConfigured == 1 ? X509_VERIFY_PARAM_set1_ip(pParameters, address.GetBytes().GetPtr(), address.GetBytes().GetCount()) : 0;
      }
      else
      {
        iConfigured = iConfigured == 1 ? X509_VERIFY_PARAM_set1_host(pParameters, sServerName.GetStartPointer(), sServerName.GetElementCount()) : 0;
      }
    }
    if (iConfigured != 1)
    {
      X509_STORE_CTX_free(pVerification);
      return LastError();
    }
    const int  iVerified = X509_verify_cert(pVerification);
    const long iError    = X509_STORE_CTX_get_error(pVerification);
    X509_STORE_CTX_free(pVerification);
    return iVerified == 1 ? InspectCertificate(pCertificate, out_certificate) : VerificationError(iError);
  }
} // namespace xiiNetworkSecurityInternal

xiiNetworkCredentialsState::~xiiNetworkCredentialsState()
{
  for (auto& session : m_Sessions)
  {
    SSL_SESSION_free(session.m_pSession);
    sk_X509_pop_free(session.m_pChain, X509_free);
  }
  xiiNetworkSecurityInternal::ClearSecret(m_Pkcs12);
  SSL_CTX_free(m_pContext);
}

xiiNetworkCredentials::xiiNetworkCredentials() = default;
xiiNetworkCredentials::~xiiNetworkCredentials() { XII_DEFAULT_DELETE(m_pState); }
bool                            xiiNetworkCredentials::IsLoaded() const { return m_pState != nullptr; }
bool                            xiiNetworkCredentials::HasIdentity() const { return m_pState != nullptr && SSL_CTX_get0_certificate(m_pState->m_pContext) != nullptr; }
const char*                     xiiNetworkCredentials::GetProviderVersion() { return OpenSSL_version(OPENSSL_VERSION); }
const xiiNetworkSecurityConfig& xiiNetworkCredentials::GetConfiguration() const
{
  XII_ASSERT_DEV(m_pState != nullptr, "Load credentials before querying their immutable configuration.");
  return m_pState->m_Configuration;
}
xiiNetworkError xiiNetworkCredentials::GetCertificate(xiiNetworkCertificate& out_certificate) const
{
  return m_pState != nullptr ? xiiNetworkSecurityInternal::InspectCertificate(SSL_CTX_get0_certificate(m_pState->m_pContext), out_certificate) : xiiNetworkError(xiiNetworkErrorCode::NotRunning);
}

xiiNetworkError xiiNetworkCredentials::VerifyCertificate(xiiArrayPtr<const xiiUInt8> certificateDer, xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>> chainDer,
                                                         xiiStringView sServerName, xiiNetworkCertificateUsage usage, xiiNetworkCertificate& out_certificate)
{
  if (m_pState == nullptr || certificateDer.IsEmpty() || certificateDer.GetPtr() == nullptr || certificateDer.GetCount() > m_pState->m_Configuration.m_uiMaximumCertificateBytes ||
      chainDer.GetCount() > 16 || (usage != xiiNetworkCertificateUsage::Server && usage != xiiNetworkCertificateUsage::Client))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  ERR_clear_error();
  const unsigned char* pInput       = certificateDer.GetPtr();
  X509*                pCertificate = d2i_X509(nullptr, &pInput, static_cast<long>(certificateDer.GetCount()));
  if (pCertificate == nullptr || pInput != certificateDer.GetEndPtr())
  {
    X509_free(pCertificate);
    return xiiNetworkSecurityInternal::LastError(xiiNetworkErrorCode::CertificateUntrusted);
  }
  STACK_OF(X509)* pChain = sk_X509_new_null();
  if (pChain == nullptr)
  {
    X509_free(pCertificate);
    return xiiNetworkSecurityInternal::LastError();
  }
  xiiNetworkError error;
  xiiUInt32       uiTotalBytes = 0;
  for (const auto& der : chainDer)
  {
    if (der.IsEmpty() || der.GetPtr() == nullptr || der.GetCount() > m_pState->m_Configuration.m_uiMaximumCertificateBytes - uiTotalBytes)
    {
      error = xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
      break;
    }
    uiTotalBytes += der.GetCount();
    pInput       = der.GetPtr();
    X509* pEntry = d2i_X509(nullptr, &pInput, static_cast<long>(der.GetCount()));
    if (pEntry == nullptr || pInput != der.GetEndPtr() || sk_X509_push(pChain, pEntry) == 0)
    {
      X509_free(pEntry);
      error = xiiNetworkSecurityInternal::LastError(xiiNetworkErrorCode::CertificateUntrusted);
      break;
    }
  }
  if (error.Succeeded())
  {
    error = xiiNetworkSecurityInternal::VerifyPeer(*this, pCertificate, pChain, sServerName, usage == xiiNetworkCertificateUsage::Server, out_certificate);
  }
  sk_X509_pop_free(pChain, X509_free);
  X509_free(pCertificate);
  return error;
}

xiiNetworkError xiiNetworkCredentials::Load(const xiiNetworkSecurityConfig& configuration, xiiArrayPtr<const xiiUInt8> privateKeyPassword)
{
  using namespace xiiNetworkSecurityInternal;
  if (m_pState != nullptr)
  {
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyOpen);
  }
  if (configuration.m_sApplicationProtocol.IsEmpty() || configuration.m_sApplicationProtocol.GetElementCount() > 255 ||
      configuration.m_sCertificateChainFile.IsEmpty() != configuration.m_sPrivateKeyFile.IsEmpty() || privateKeyPassword.GetCount() > 1024 ||
      configuration.m_uiMaximumCachedSessions == 0 || configuration.m_uiMaximumCertificateBytes < 1024 || configuration.m_uiMaximumCertificateBytes > 65536 ||
      (!configuration.m_bUseSystemTrust && configuration.m_sTrustAnchorsFile.IsEmpty()))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  if (OpenSSL_version_num() != OPENSSL_VERSION_NUMBER)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InitializationFailed);
  }
  if (OPENSSL_init_ssl(OPENSSL_INIT_NO_LOAD_CONFIG, nullptr) != 1)
  {
    return LastError(xiiNetworkErrorCode::InitializationFailed);
  }
  ERR_clear_error();
  xiiUniquePtr<xiiNetworkCredentialsState> pState = XII_DEFAULT_NEW(xiiNetworkCredentialsState);
  pState->m_Configuration                         = configuration;
  pState->m_pContext                              = SSL_CTX_new(TLS_method());
  if (pState->m_pContext == nullptr || SSL_CTX_set_min_proto_version(pState->m_pContext, TLS1_3_VERSION) != 1 || SSL_CTX_set_max_proto_version(pState->m_pContext, TLS1_3_VERSION) != 1)
  {
    return LastError();
  }
  SSL_CTX_set_options(pState->m_pContext, SSL_OP_NO_COMPRESSION | SSL_OP_NO_RENEGOTIATION);
  SSL_CTX_set_mode(pState->m_pContext, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
  SSL_CTX_set_alpn_select_cb(pState->m_pContext, SelectNetworkAlpn, pState.Borrow());
  SSL_CTX_set_verify(pState->m_pContext, SSL_VERIFY_PEER, nullptr);
  SSL_CTX_set_verify_depth(pState->m_pContext, 16);
  X509_VERIFY_PARAM_set_flags(SSL_CTX_get0_param(pState->m_pContext), X509_V_FLAG_X509_STRICT | X509_V_FLAG_TRUSTED_FIRST);
  SSL_CTX_set_max_cert_list(pState->m_pContext, configuration.m_uiMaximumCertificateBytes);
  SSL_CTX_set_max_early_data(pState->m_pContext, 0);
  SSL_CTX_set_session_cache_mode(pState->m_pContext, configuration.m_bEnableTransportResumption ? SSL_SESS_CACHE_BOTH : SSL_SESS_CACHE_OFF);
  SSL_CTX_sess_set_cache_size(pState->m_pContext, configuration.m_uiMaximumCachedSessions);
  if (!configuration.m_bEnableTransportResumption)
  {
    SSL_CTX_set_num_tickets(pState->m_pContext, 0);
  }
  xiiUInt8 uiContext[32] = {};
  auto     error         = RandomBytes(uiContext);
  if (error.Failed() || SSL_CTX_set_session_id_context(pState->m_pContext, uiContext, sizeof(uiContext)) != 1)
  {
    return error.Failed() ? error : LastError();
  }
  ClearSecret(uiContext);
  if (configuration.m_bUseSystemTrust)
  {
    error = LoadSystemTrust(pState->m_pContext);
    if (error.Failed())
    {
      return error;
    }
  }
  if (!configuration.m_sTrustAnchorsFile.IsEmpty())
  {
    error = LoadTrustFile(pState->m_pContext, configuration.m_sTrustAnchorsFile, false);
    if (error.Failed())
    {
      return error;
    }
  }
  if (!configuration.m_sCertificateRevocationFile.IsEmpty())
  {
    error = LoadTrustFile(pState->m_pContext, configuration.m_sCertificateRevocationFile, true);
    if (error.Failed())
    {
      return error;
    }
  }
  if (!configuration.m_sCertificateChainFile.IsEmpty())
  {
    error = LoadIdentity(*pState, privateKeyPassword);
    if (error.Failed())
    {
      return error;
    }
  }
  m_pState = pState.Release();
  return {};
}

xiiNetworkAuthenticationData::~xiiNetworkAuthenticationData() { xiiNetworkSecurityInternal::ClearSecret(m_Bytes); }
xiiNetworkError xiiNetworkAuthenticationData::Initialize(xiiArrayPtr<const xiiUInt8> bytes)
{
  if (m_bInitialized)
  {
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyOpen);
  }
  if (bytes.GetCount() > 4096 || (!bytes.IsEmpty() && bytes.GetPtr() == nullptr))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  m_Bytes        = bytes;
  m_bInitialized = true;
  return {};
}
