/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/Security/NetworkTlsStream.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>
#include <cstring>

namespace
{
  bool TransferTestCiphertext(xiiNetworkTlsStream& source, xiiNetworkTlsStream& destination)
  {
    xiiUInt8 uiBytes[257] = {};
    for (xiiUInt32 uiIteration = 0; uiIteration < 1024; ++uiIteration)
    {
      const auto read = source.DrainCiphertext(uiBytes);
      if (read.m_Error.m_Code == xiiNetworkErrorCode::WouldBlock)
      {
        return true;
      }
      if (!XII_TEST_BOOL(read.Succeeded()))
      {
        return false;
      }
      const auto write = destination.FeedCiphertext({uiBytes, read.m_uiBytesTransferred});
      if (!XII_TEST_BOOL(write.Succeeded() && write.m_uiBytesTransferred == read.m_uiBytesTransferred))
      {
        return false;
      }
    }
    return false;
  }

  xiiNetworkError HandshakeTestTls(xiiNetworkTlsStream& client, xiiNetworkTlsStream& server)
  {
    for (xiiUInt32 uiIteration = 0; uiIteration < 1000; ++uiIteration)
    {
      const auto clientError = client.Handshake();
      if (clientError.Failed() && clientError.m_Code != xiiNetworkErrorCode::WouldBlock)
      {
        return clientError;
      }
      if (!TransferTestCiphertext(client, server))
      {
        return xiiNetworkError(xiiNetworkErrorCode::TlsError);
      }
      const auto serverError = server.Handshake();
      if (serverError.Failed() && serverError.m_Code != xiiNetworkErrorCode::WouldBlock)
      {
        return serverError;
      }
      if (!TransferTestCiphertext(server, client))
      {
        return xiiNetworkError(xiiNetworkErrorCode::TlsError);
      }
      if (client.IsEstablished() && server.IsEstablished())
      {
        // Process TLS 1.3 post-handshake tickets before the connection is retired.
        xiiUInt8   uiByte = 0;
        const auto read   = client.Read({&uiByte, 1});
        XII_TEST_BOOL(read.m_Error.m_Code == xiiNetworkErrorCode::WouldBlock);
        return {};
      }
    }
    return xiiNetworkError(xiiNetworkErrorCode::TimedOut);
  }

  bool CloseTestTls(xiiNetworkTlsStream& client, xiiNetworkTlsStream& server)
  {
    for (xiiUInt32 uiIteration = 0; uiIteration < 100; ++uiIteration)
    {
      const auto clientError = client.Shutdown();
      if (!TransferTestCiphertext(client, server))
      {
        return false;
      }
      const auto serverError = server.Shutdown();
      if (!TransferTestCiphertext(server, client))
      {
        return false;
      }
      if (clientError.Succeeded() && serverError.Succeeded())
      {
        return true;
      }
      XII_TEST_BOOL(clientError.Succeeded() || clientError.m_Code == xiiNetworkErrorCode::WouldBlock);
      XII_TEST_BOOL(serverError.Succeeded() || serverError.m_Code == xiiNetworkErrorCode::WouldBlock);
    }
    return false;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(NetworkTransport, TlsHandshakeTransferAndResumption)
{
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  xiiSharedPtr<xiiNetworkCredentials> pServer             = XII_DEFAULT_NEW(xiiNetworkCredentials);
  xiiSharedPtr<xiiNetworkCredentials> pClient             = XII_DEFAULT_NEW(xiiNetworkCredentials);
  auto                                serverConfiguration = files.Configuration();
  auto                                clientConfiguration = files.Configuration("client");
  serverConfiguration.m_bEnableTransportResumption        = true;
  clientConfiguration.m_bEnableTransportResumption        = true;
  if (!XII_TEST_BOOL(pServer->Load(serverConfiguration).Succeeded()) || !XII_TEST_BOOL(pClient->Load(clientConfiguration).Succeeded()))
  {
    return;
  }
  for (xiiUInt32 uiAttempt = 0; uiAttempt < 2; ++uiAttempt)
  {
    xiiNetworkTlsStream server;
    xiiNetworkTlsStream client;
    if (!XII_TEST_BOOL(server.Initialize(pServer, true).Succeeded()) || !XII_TEST_BOOL(client.Initialize(pClient, false, "localhost", 443).Succeeded()))
    {
      return;
    }
    const auto handshake = HandshakeTestTls(client, server);
    if (!XII_TEST_BOOL_MSG(handshake.Succeeded(), "%s (%d)", handshake.GetCodeName(), handshake.m_iNativeCode))
    {
      return;
    }
    XII_TEST_BOOL(client.WasSessionResumed() == (uiAttempt != 0));
    XII_TEST_BOOL(server.WasSessionResumed() == (uiAttempt != 0));
    xiiNetworkCertificate certificate;
    XII_TEST_BOOL(client.GetPeerCertificate(certificate).Succeeded());
    XII_TEST_BOOL(server.GetPeerCertificate(certificate).Succeeded());
    XII_TEST_BOOL(client.Write({}).Succeeded());
    XII_TEST_BOOL(server.Read({}).Succeeded());
    // Payload exceeds both BIO capacities and one TLS record; exact byte pattern catches ordering/copy bugs.
    xiiUInt8 uiPayload[16384]  = {};
    xiiUInt8 uiReceived[16384] = {};
    for (xiiUInt32 uiIndex = 0; uiIndex < XII_ARRAY_SIZE(uiPayload); ++uiIndex)
    {
      uiPayload[uiIndex] = static_cast<xiiUInt8>(uiIndex);
    }
    xiiUInt32           uiSent  = 0;
    xiiUInt32           uiRead  = 0;
    constexpr xiiUInt32 uiTotal = 1024 * 1024;
    for (xiiUInt32 uiIteration = 0; uiIteration < 10000 && uiRead < uiTotal; ++uiIteration)
    {
      if (uiSent < uiTotal)
      {
        const auto write = client.Write(uiPayload);
        XII_TEST_BOOL(write.Succeeded() || write.m_Error.m_Code == xiiNetworkErrorCode::WouldBlock);
        uiSent += write.m_uiBytesTransferred;
      }
      if (!TransferTestCiphertext(client, server))
      {
        return;
      }
      const auto read = server.Read(uiReceived);
      XII_TEST_BOOL(read.Succeeded() || read.m_Error.m_Code == xiiNetworkErrorCode::WouldBlock);
      for (xiiUInt32 uiIndex = 0; uiIndex < read.m_uiBytesTransferred; ++uiIndex)
      {
        if (uiReceived[uiIndex] != static_cast<xiiUInt8>(uiRead + uiIndex))
        {
          XII_TEST_BOOL(false);
          return;
        }
      }
      uiRead += read.m_uiBytesTransferred;
      if (!TransferTestCiphertext(server, client))
      {
        return;
      }
    }
    XII_TEST_INT(uiRead, uiTotal);
    XII_TEST_INT(uiSent, uiTotal);
    XII_TEST_BOOL(CloseTestTls(client, server));
    XII_TEST_BOOL(server.Read(uiReceived).m_Error.m_Code == xiiNetworkErrorCode::ConnectionClosed);
  }
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, TlsAuthenticationFailures)
{
  NetworkSecurityTestCertificates files;
  NetworkSecurityTestCertificates otherFiles;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()) || !XII_TEST_BOOL(otherFiles.Initialize().Succeeded()))
  {
    return;
  }
  for (xiiUInt32 uiCase = 0; uiCase < 5; ++uiCase)
  {
    xiiSharedPtr<xiiNetworkCredentials> pServer             = XII_DEFAULT_NEW(xiiNetworkCredentials);
    xiiSharedPtr<xiiNetworkCredentials> pClient             = XII_DEFAULT_NEW(xiiNetworkCredentials);
    auto                                clientConfiguration = files.Configuration(uiCase == 2 ? "" : "client");
    if (uiCase == 1)
    {
      clientConfiguration.m_sTrustAnchorsFile = otherFiles.Path("ca.pem");
    }
    if (uiCase == 4)
    {
      clientConfiguration.m_sApplicationProtocol = "other-protocol/1";
    }
    if (!XII_TEST_BOOL(pServer->Load(files.Configuration(uiCase == 3 ? "expired" : "server")).Succeeded()) || !XII_TEST_BOOL(pClient->Load(clientConfiguration).Succeeded()))
    {
      return;
    }
    xiiNetworkTlsStream server;
    xiiNetworkTlsStream client;
    XII_TEST_BOOL(server.Initialize(pServer, true).Succeeded());
    XII_TEST_BOOL(client.Initialize(pClient, false, uiCase == 0 ? "wrong.invalid" : "localhost").Succeeded());
    const auto error = HandshakeTestTls(client, server);
    XII_TEST_BOOL(error.Failed() && error.m_Code != xiiNetworkErrorCode::WouldBlock && error.m_Code != xiiNetworkErrorCode::TimedOut);
    if (uiCase == 0)
    {
      XII_TEST_BOOL(error.m_Code == xiiNetworkErrorCode::HostnameMismatch);
    }
    if (uiCase == 1)
    {
      XII_TEST_BOOL(error.m_Code == xiiNetworkErrorCode::CertificateUntrusted);
    }
    if (uiCase == 3)
    {
      XII_TEST_BOOL(error.m_Code == xiiNetworkErrorCode::CertificateExpired);
    }
  }
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, TlsUnexpectedEofAndBounds)
{
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  xiiSharedPtr<xiiNetworkCredentials> pServer = XII_DEFAULT_NEW(xiiNetworkCredentials);
  xiiSharedPtr<xiiNetworkCredentials> pClient = XII_DEFAULT_NEW(xiiNetworkCredentials);
  XII_TEST_BOOL(pServer->Load(files.Configuration()).Succeeded());
  XII_TEST_BOOL(pClient->Load(files.Configuration("client")).Succeeded());
  xiiNetworkTlsStream server;
  xiiNetworkTlsStream client;
  XII_TEST_BOOL(server.Initialize(pServer, true).Succeeded());
  XII_TEST_BOOL(client.Initialize(pClient, false, "127.0.0.1").Succeeded());
  if (!XII_TEST_BOOL(HandshakeTestTls(client, server).Succeeded()))
  {
    return;
  }
  server.NotifyEndOfInput();
  xiiUInt8 uiBytes[16384] = {};
  XII_TEST_BOOL(server.Read(uiBytes).m_Error.m_Code == xiiNetworkErrorCode::TlsError);
  XII_TEST_BOOL(server.Read(uiBytes).m_Error.m_Code == xiiNetworkErrorCode::TlsError);
  // A stalled receiver cannot make the outgoing BIO grow without a bound.
  xiiUInt32 uiAccepted = 0;
  for (xiiUInt32 uiIteration = 0; uiIteration < 16; ++uiIteration)
  {
    const auto write = client.Write(uiBytes);
    uiAccepted += write.m_uiBytesTransferred;
    if (write.m_Error.m_Code == xiiNetworkErrorCode::WouldBlock)
    {
      break;
    }
    XII_TEST_BOOL(write.Succeeded());
  }
  XII_TEST_BOOL(uiAccepted > 0 && uiAccepted <= 65536);
  XII_TEST_BOOL(client.GetPendingCiphertextBytes() <= 65536);
}
