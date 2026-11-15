/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/Security/NetworkCredentials.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>

XII_CREATE_SIMPLE_TEST(NetworkTransport, CertificateValidation)
{
  NetworkSecurityTestCertificates files;
  NetworkSecurityTestCertificates otherFiles;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()) || !XII_TEST_BOOL(otherFiles.Initialize().Succeeded()))
  {
    return;
  }
  xiiNetworkCredentials server;
  xiiNetworkCredentials client;
  xiiNetworkCredentials expired;
  xiiNetworkCredentials otherTrust;
  if (!XII_TEST_BOOL(server.Load(files.Configuration()).Succeeded()) ||
      !XII_TEST_BOOL(client.Load(files.Configuration("client")).Succeeded()) ||
      !XII_TEST_BOOL(expired.Load(files.Configuration("expired")).Succeeded()) ||
      !XII_TEST_BOOL(otherTrust.Load(otherFiles.Configuration("")).Succeeded()))
  {
    return;
  }
  XII_TEST_BOOL(server.HasIdentity());
  XII_TEST_BOOL(!otherTrust.HasIdentity());
  xiiNetworkCertificate certificate;
  xiiNetworkCertificate verified;
  if (!XII_TEST_BOOL(server.GetCertificate(certificate).Succeeded()))
  {
    return;
  }
  XII_TEST_BOOL(certificate.m_ValidFrom.IsValid());
  XII_TEST_BOOL(certificate.m_ValidUntil.IsValid());
  XII_TEST_BOOL(!certificate.m_sSubject.IsEmpty());
  for (const char* szName : {"localhost", "127.0.0.1", "::1"})
  {
    XII_TEST_BOOL(client.VerifyCertificate(certificate.m_Der, {}, szName, xiiNetworkCertificateUsage::Server, verified).Succeeded());
    XII_TEST_BOOL(verified.m_Der == certificate.m_Der);
  }
  auto error = client.VerifyCertificate(certificate.m_Der, {}, "wrong.invalid", xiiNetworkCertificateUsage::Server, verified);
  XII_TEST_BOOL(error.m_Code == xiiNetworkErrorCode::HostnameMismatch);
  XII_TEST_BOOL(error.m_Domain == xiiNetworkErrorDomain::Tls && error.m_iNativeCode != 0);
  XII_TEST_BOOL(client.VerifyCertificate(certificate.m_Der, {}, "", xiiNetworkCertificateUsage::Server, verified).Failed());
  XII_TEST_BOOL(otherTrust.VerifyCertificate(certificate.m_Der, {}, "localhost", xiiNetworkCertificateUsage::Server, verified).m_Code == xiiNetworkErrorCode::CertificateUntrusted);
  XII_TEST_BOOL(server.VerifyCertificate(certificate.m_Der, {}, "", xiiNetworkCertificateUsage::Client, verified).Failed());
  XII_TEST_BOOL(client.GetCertificate(certificate).Succeeded());
  XII_TEST_BOOL(server.VerifyCertificate(certificate.m_Der, {}, "", xiiNetworkCertificateUsage::Client, verified).Succeeded());
  XII_TEST_BOOL(server.VerifyCertificate(certificate.m_Der, {}, "localhost", xiiNetworkCertificateUsage::Server, verified).Failed());
  XII_TEST_BOOL(expired.GetCertificate(certificate).Succeeded());
  XII_TEST_BOOL(client.VerifyCertificate(certificate.m_Der, {}, "localhost", xiiNetworkCertificateUsage::Server, verified).m_Code == xiiNetworkErrorCode::CertificateExpired);
  certificate.m_Der.PushBack(0);
  XII_TEST_BOOL(client.VerifyCertificate(certificate.m_Der, {}, "localhost", xiiNetworkCertificateUsage::Server, verified).Failed());
  XII_TEST_BOOL(server.Load(files.Configuration()).m_Code == xiiNetworkErrorCode::AlreadyOpen);
  // Loaded snapshots retain identity/trust after files disappear; replacements are independent.
  XII_TEST_BOOL(files.DeleteFiles().Succeeded());
  XII_TEST_BOOL(server.GetCertificate(certificate).Succeeded());
  XII_TEST_BOOL(client.VerifyCertificate(certificate.m_Der, {}, "localhost", xiiNetworkCertificateUsage::Server, verified).Succeeded());
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, CredentialFailuresAndBounds)
{
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  xiiNetworkCredentials credentials;
  auto                  configuration = files.Configuration();
  configuration.m_sPrivateKeyFile     = files.Path("client.key");
  XII_TEST_BOOL(credentials.Load(configuration).Failed());
  XII_TEST_BOOL(!credentials.IsLoaded());
  configuration                         = files.Configuration();
  configuration.m_sCertificateChainFile = files.Path("absent.pem");
  const auto error                      = credentials.Load(configuration);
  XII_TEST_BOOL(error.Failed() && error.m_iNativeCode != 0 && error.m_Domain == xiiNetworkErrorDomain::CredentialFile);
  configuration = files.Configuration();
  configuration.m_sApplicationProtocol.Clear();
  XII_TEST_BOOL(credentials.Load(configuration).m_Code == xiiNetworkErrorCode::InvalidArgument);
  configuration                             = files.Configuration();
  configuration.m_uiMaximumCertificateBytes = 0;
  XII_TEST_BOOL(credentials.Load(configuration).m_Code == xiiNetworkErrorCode::InvalidArgument);
  XII_TEST_BOOL(credentials.Load(files.Configuration()).Succeeded());
  xiiNetworkCertificate certificate;
  XII_TEST_BOOL(credentials.VerifyCertificate({}, {}, "localhost", xiiNetworkCertificateUsage::Server, certificate).Failed());
  xiiNetworkAuthenticationData authentication;
  xiiUInt8                     uiSecret[4097] = {};
  XII_TEST_BOOL(authentication.Initialize(uiSecret).m_Code == xiiNetworkErrorCode::InvalidArgument);
  XII_TEST_BOOL(authentication.Initialize({uiSecret, 4096}).Succeeded());
  XII_TEST_INT(authentication.GetBytes().GetCount(), 4096);
  XII_TEST_BOOL(authentication.Initialize({}).m_Code == xiiNetworkErrorCode::AlreadyOpen);
}
