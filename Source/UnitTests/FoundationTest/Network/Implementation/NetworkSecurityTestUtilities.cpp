/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

namespace
{
  bool AddTestExtension(X509* pCertificate, X509* pIssuer, int iId, const char* szValue)
  {
    X509V3_CTX context = {};
    X509V3_set_ctx(&context, pIssuer, pCertificate, nullptr, nullptr, 0);
    X509_EXTENSION* pExtension = X509V3_EXT_conf_nid(nullptr, &context, iId, szValue);
    const bool      bAdded     = pExtension != nullptr && X509_add_ext(pCertificate, pExtension, -1) == 1;
    X509_EXTENSION_free(pExtension);
    return bAdded;
  }

  X509* MakeTestCertificate(EVP_PKEY* pKey, X509* pIssuer, EVP_PKEY* pIssuerKey, long iSerial, const char* szName, bool bExpired, bool bClient)
  {
    X509* pCertificate = X509_new();
    if (pCertificate == nullptr)
    {
      return nullptr;
    }
    X509_NAME* pName  = X509_get_subject_name(pCertificate);
    const bool bCa    = pIssuer == nullptr;
    bool       bValid = X509_set_version(pCertificate, 2) == 1 && ASN1_INTEGER_set(X509_get_serialNumber(pCertificate), iSerial) == 1 &&
      X509_gmtime_adj(X509_getm_notBefore(pCertificate), -86400) != nullptr &&
      X509_gmtime_adj(X509_getm_notAfter(pCertificate), bExpired ? -3600 : 86400) != nullptr && X509_set_pubkey(pCertificate, pKey) == 1 &&
      X509_NAME_add_entry_by_txt(pName, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>(szName), -1, -1, 0) == 1 &&
      X509_set_issuer_name(pCertificate, bCa ? pName : X509_get_subject_name(pIssuer)) == 1;
    bValid = bValid && AddTestExtension(pCertificate, bCa ? pCertificate : pIssuer, NID_basic_constraints, bCa ? "critical,CA:TRUE,pathlen:0" : "critical,CA:FALSE") &&
      AddTestExtension(pCertificate, bCa ? pCertificate : pIssuer, NID_key_usage, bCa ? "critical,keyCertSign,cRLSign" : "critical,digitalSignature") &&
      AddTestExtension(pCertificate, bCa ? pCertificate : pIssuer, NID_subject_key_identifier, "hash");
    if (!bCa)
    {
      bValid = bValid && AddTestExtension(pCertificate, pIssuer, NID_authority_key_identifier, "keyid:always") &&
        AddTestExtension(pCertificate, pIssuer, NID_ext_key_usage, bClient ? "clientAuth" : "serverAuth") &&
        AddTestExtension(pCertificate, pIssuer, NID_subject_alt_name, "DNS:localhost,IP:127.0.0.1,IP:::1");
    }
    if (!bValid || X509_sign(pCertificate, bCa ? pKey : pIssuerKey, EVP_sha256()) <= 0)
    {
      X509_free(pCertificate);
      return nullptr;
    }
    return pCertificate;
  }

  bool WriteTestPem(xiiStringView sPath, X509* pCertificate, EVP_PKEY* pKey)
  {
    BIO* pBio = BIO_new(BIO_s_mem());
    if (pBio == nullptr)
    {
      return false;
    }
    bool       bWritten = pCertificate != nullptr ? PEM_write_bio_X509(pBio, pCertificate) == 1 : PEM_write_bio_PrivateKey(pBio, pKey, nullptr, nullptr, 0, nullptr, nullptr) == 1;
    char*      pData    = nullptr;
    const long iBytes   = BIO_get_mem_data(pBio, &pData);
    xiiOSFile  file;
    bWritten = bWritten && iBytes > 0 && file.Open(sPath, xiiFileOpenMode::Write).Succeeded() && file.Write(pData, static_cast<xiiUInt64>(iBytes)).Succeeded();
    BIO_free(pBio);
    return bWritten;
  }
} // namespace

NetworkSecurityTestCertificates::~NetworkSecurityTestCertificates()
{
  if (!m_sDirectory.IsEmpty())
  {
    xiiOSFile::DeleteFolder(m_sDirectory).IgnoreResult();
  }
}

xiiString NetworkSecurityTestCertificates::Path(xiiStringView sName) const
{
  xiiStringBuilder sPath(m_sDirectory);
  sPath.AppendPath(sName);
  return sPath;
}

xiiNetworkSecurityConfig NetworkSecurityTestCertificates::Configuration(xiiStringView sIdentity) const
{
  xiiNetworkSecurityConfig configuration;
  configuration.m_bUseSystemTrust   = false;
  configuration.m_sTrustAnchorsFile = Path("ca.pem");
  if (!sIdentity.IsEmpty())
  {
    xiiStringBuilder sName(sIdentity);
    sName.Append(".pem");
    configuration.m_sCertificateChainFile = Path(sName);
    sName.Set(sIdentity, ".key");
    configuration.m_sPrivateKeyFile = Path(sName);
  }
  return configuration;
}

xiiResult NetworkSecurityTestCertificates::Initialize()
{
  if (OPENSSL_init_ssl(OPENSSL_INIT_NO_LOAD_CONFIG, nullptr) != 1)
  {
    return XII_FAILURE;
  }
  xiiStringBuilder sUuid;
  xiiConversionUtils::ToString(xiiUuid::MakeUuid(), sUuid);
  xiiStringBuilder sDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath());
  sDirectory.AppendPath("Certificates", sUuid);
  m_sDirectory = sDirectory;
  if (!XII_TEST_BOOL(xiiOSFile::CreateDirectoryStructure(m_sDirectory).Succeeded()))
  {
    xiiTestFramework::Output(xiiTestOutput::Error, "Fixture directory: %s", m_sDirectory.GetData());
    return XII_FAILURE;
  }
  EVP_PKEY* pCaKey  = EVP_EC_gen("prime256v1");
  X509*     pCa     = pCaKey != nullptr ? MakeTestCertificate(pCaKey, nullptr, nullptr, 1, "XII ephemeral test CA", false, false) : nullptr;
  bool      bValid  = pCa != nullptr && WriteTestPem(Path("ca.pem"), pCa, nullptr);
  long      iSerial = 2;
  for (const char* szIdentity : {"server", "client", "expired"})
  {
    EVP_PKEY*        pKey         = EVP_EC_gen("prime256v1");
    X509*            pCertificate = pKey != nullptr && pCa != nullptr ? MakeTestCertificate(pKey, pCa, pCaKey, iSerial++, szIdentity, xiiStringUtils::IsEqual(szIdentity, "expired"), xiiStringUtils::IsEqual(szIdentity, "client")) : nullptr;
    xiiStringBuilder sName(szIdentity, ".pem");
    bValid = bValid && pCertificate != nullptr && WriteTestPem(Path(sName), pCertificate, nullptr);
    sName.Set(szIdentity, ".key");
    bValid = bValid && pKey != nullptr && WriteTestPem(Path(sName), nullptr, pKey);
    X509_free(pCertificate);
    EVP_PKEY_free(pKey);
  }
  X509_free(pCa);
  EVP_PKEY_free(pCaKey);
  if (!bValid)
  {
    ERR_print_errors_fp(stderr);
  }
  return bValid ? XII_SUCCESS : XII_FAILURE;
}
