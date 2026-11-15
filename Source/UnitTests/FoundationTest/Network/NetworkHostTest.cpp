/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/NetworkHost.h>
#include <Foundation/Time/Time.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>
#include <cstring>
#include <utility>

namespace
{
  bool HostTestEvent(xiiNetworkHost& host, xiiNetworkEventType type, xiiNetworkEvent& out_event)
  {
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    for (xiiUInt32 uiCount = 0; uiCount < 1000; ++uiCount)
    {
      const auto remaining = deadline - xiiTime::Now();
      if (remaining.IsNegative())
      {
        break;
      }
      const auto error = host.WaitForEvent(out_event, remaining);
      if (!XII_TEST_BOOL_MSG(error.Succeeded(), "Wait %u: %s", static_cast<unsigned>(type), error.GetCodeName()))
      {
        return false;
      }
      if (out_event.m_Type == type)
      {
        return true;
      }
      if (out_event.m_Type == xiiNetworkEventType::Closed)
      {
        XII_TEST_BOOL_MSG(false, "Unexpected close: %s (%d)", out_event.m_Error.GetCodeName(), out_event.m_Error.m_iNativeCode);
        return false;
      }
    }
    return XII_TEST_BOOL(false);
  }

  xiiNetworkHostConfiguration HostTestConfiguration(xiiNetworkTransportType type)
  {
    xiiNetworkHostConfiguration configuration;
    configuration.m_Transport                      = type;
    configuration.m_uiMaximumConnections           = 8;
    configuration.m_uiMaximumChannelsPerConnection = 8;
    configuration.m_uiMaximumQueuedMessages        = 64;
    configuration.m_uiMaximumQueuedEvents          = 128;
    configuration.m_uiMaximumSessions              = 16;
    configuration.m_IO.m_uiWorkerThreadCount       = 1;
    configuration.m_IO.m_uiResolverThreadCount     = 1;
    configuration.m_HandshakeTimeout               = xiiTime::MakeFromSeconds(3);
    configuration.m_CloseTimeout                   = xiiTime::MakeFromMilliseconds(500);
    if (type == xiiNetworkTransportType::Tcp || type == xiiNetworkTransportType::NativeUdp)
    {
      configuration.m_Authentication            = xiiNetworkAuthenticationPolicy::AnonymousDevelopment;
      configuration.m_bAllowInsecureDevelopment = true;
    }
    return configuration;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostTcpAndTlsMessaging)
{
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  for (auto type : {xiiNetworkTransportType::Tcp, xiiNetworkTransportType::TlsTcp})
  {
    auto serverConfiguration = HostTestConfiguration(type);
    auto clientConfiguration = HostTestConfiguration(type);
    if (type == xiiNetworkTransportType::TlsTcp)
    {
      serverConfiguration.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
      clientConfiguration.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
      if (!XII_TEST_BOOL(serverConfiguration.m_pCredentials->Load(files.Configuration()).Succeeded()) ||
          !XII_TEST_BOOL(clientConfiguration.m_pCredentials->Load(files.Configuration("client")).Succeeded()))
      {
        return;
      }
    }
    xiiNetworkHost server;
    xiiNetworkHost client;
    if (!XII_TEST_BOOL(server.Start(serverConfiguration).Succeeded()) || !XII_TEST_BOOL(client.Start(clientConfiguration).Succeeded()))
    {
      return;
    }
    xiiNetworkListener listener;
    xiiNetworkEndpoint endpoint;
    if (!XII_TEST_BOOL(server.Listen({xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 0}, listener).Succeeded()) ||
        !XII_TEST_BOOL(server.GetLocalEndpoint(listener, endpoint).Succeeded()))
    {
      return;
    }
    xiiNetworkConnectOptions options;
    options.m_sServerName = "localhost";
    xiiNetworkConnection connection;
    if (!XII_TEST_BOOL(client.Connect(endpoint, options, connection).Succeeded()))
    {
      return;
    }
    xiiNetworkEvent event;
    if (!HostTestEvent(server, xiiNetworkEventType::Connected, event))
    {
      return;
    }
    const auto serverConnection = event.m_Connection;
    if (!HostTestEvent(client, xiiNetworkEventType::Connected, event))
    {
      return;
    }
    XII_TEST_BOOL(event.m_Connection == connection);
    xiiNetworkChannel channel;
    if (!XII_TEST_BOOL(client.GetDefaultChannel(connection, channel).Succeeded()))
    {
      return;
    }
    xiiNetworkIOBuffer payload;
    if (!XII_TEST_BOOL(client.AcquireBuffer(1024 * 1024, payload).Succeeded()))
    {
      return;
    }
    for (xiiUInt32 uiIndex = 0; uiIndex < payload.GetCount(); ++uiIndex)
    {
      payload.GetBytes()[uiIndex] = static_cast<xiiUInt8>(uiIndex * 17);
    }
    xiiNetworkMessage message;
    if (!XII_TEST_BOOL(client.Send(channel, std::move(payload), message, 42).Succeeded()))
    {
      return;
    }
    if (!HostTestEvent(server, xiiNetworkEventType::Message, event))
    {
      return;
    }
    XII_TEST_INT(event.m_Payload.GetCount(), 1024 * 1024);
    for (xiiUInt32 uiIndex = 0; uiIndex < event.m_Payload.GetCount(); ++uiIndex)
    {
      if (event.m_Payload.GetBytes()[uiIndex] != static_cast<xiiUInt8>(uiIndex * 17))
      {
        XII_TEST_BOOL(false);
        return;
      }
    }
    if (!HostTestEvent(client, xiiNetworkEventType::MessageSent, event))
    {
      return;
    }
    XII_TEST_BOOL(event.m_Message == message && event.m_Error.Succeeded());
    XII_TEST_INT(event.m_uiUserData, 42);
    xiiNetworkChannelConfiguration channelConfiguration;
    channelConfiguration.m_uiWeight  = 3;
    channelConfiguration.m_uiService = 123;
    if (!XII_TEST_BOOL(client.OpenChannel(connection, channelConfiguration, channel).Succeeded()))
    {
      return;
    }
    if (!HostTestEvent(client, xiiNetworkEventType::ChannelOpened, event) || !HostTestEvent(server, xiiNetworkEventType::ChannelOpened, event))
    {
      return;
    }
    XII_TEST_INT(event.m_ChannelConfiguration.m_uiService, 123);
    xiiNetworkChannel remoteChannel = event.m_Channel;
    if (!XII_TEST_BOOL(server.Send(remoteChannel, xiiArrayPtr<const xiiUInt8>(), message).Succeeded()))
    {
      return;
    }
    if (!HostTestEvent(client, xiiNetworkEventType::Message, event))
    {
      return;
    }
    XII_TEST_INT(event.m_Payload.GetCount(), 0);
    if (!XII_TEST_BOOL(client.CloseChannel(channel).Succeeded()))
    {
      return;
    }
    if (!HostTestEvent(client, xiiNetworkEventType::ChannelClosed, event) || !HostTestEvent(server, xiiNetworkEventType::ChannelClosed, event))
    {
      return;
    }
    XII_TEST_BOOL(client.Send(channel, xiiArrayPtr<const xiiUInt8>(), message).m_Code == xiiNetworkErrorCode::ChannelClosed);
    xiiNetworkConnectionInfo info;
    XII_TEST_BOOL(server.GetConnectionInfo(serverConnection, info).Succeeded());
    XII_TEST_INT(info.m_Statistics.m_uiMessagesReceived, 1);
    XII_TEST_INT(info.m_Statistics.m_uiPayloadBytesReceived, 1024 * 1024);
    XII_TEST_BOOL(client.Disconnect(connection).Succeeded());
    if (!HostTestEvent(client, xiiNetworkEventType::Closed, event) || !HostTestEvent(server, xiiNetworkEventType::Closed, event))
    {
      return;
    }
    server.Shutdown();
    client.Shutdown();
    XII_TEST_BOOL(!client.IsRunning());
    XII_TEST_BOOL(client.Start(clientConfiguration).m_Code == xiiNetworkErrorCode::AlreadyOpen);
  }
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostNativeUdpDatagrams)
{
  auto           configuration = HostTestConfiguration(xiiNetworkTransportType::NativeUdp);
  xiiNetworkHost sender;
  xiiNetworkHost receiver;
  if (!XII_TEST_BOOL(sender.Start(configuration).Succeeded()) || !XII_TEST_BOOL(receiver.Start(configuration).Succeeded()))
  {
    return;
  }
  xiiNetworkListener       source;
  xiiNetworkListener       destination;
  const xiiNetworkEndpoint loopback{xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 0};
  if (!XII_TEST_BOOL(sender.Listen(loopback, source).Succeeded()) || !XII_TEST_BOOL(receiver.Listen(loopback, destination).Succeeded()))
  {
    return;
  }
  xiiNetworkEndpoint endpoint;
  xiiNetworkEndpoint sourceEndpoint;
  XII_TEST_BOOL(receiver.GetLocalEndpoint(destination, endpoint).Succeeded());
  XII_TEST_BOOL(sender.GetLocalEndpoint(source, sourceEndpoint).Succeeded());
  for (xiiUInt32 uiSize : {0U, 1U, 256U, 16000U})
  {
    xiiNetworkIOBuffer buffer;
    if (!XII_TEST_BOOL(sender.AcquireBuffer(uiSize, buffer).Succeeded()))
    {
      return;
    }
    if (uiSize != 0)
    {
      std::memset(buffer.GetBytes().GetPtr(), 0x5A, uiSize);
    }
    xiiNetworkMessage message;
    if (!XII_TEST_BOOL(sender.SendDatagram(source, endpoint, std::move(buffer), message).Succeeded()))
    {
      return;
    }
    xiiNetworkEvent event;
    if (!HostTestEvent(receiver, xiiNetworkEventType::Datagram, event))
    {
      return;
    }
    XII_TEST_BOOL(event.m_Error.Succeeded());
    XII_TEST_INT(event.m_Payload.GetCount(), uiSize);
    XII_TEST_BOOL(event.m_Endpoint == sourceEndpoint);
    if (!HostTestEvent(sender, xiiNetworkEventType::MessageSent, event))
    {
      return;
    }
    XII_TEST_BOOL(event.m_Error.Succeeded() && event.m_Message == message);
  }
  xiiNetworkConnection connection;
  XII_TEST_BOOL(sender.Connect(endpoint, {}, connection).m_Code == xiiNetworkErrorCode::UnsupportedOperation);
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostAuthenticationAndSessionRecovery)
{
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  auto serverConfiguration                     = HostTestConfiguration(xiiNetworkTransportType::TlsTcp);
  auto clientConfiguration                     = HostTestConfiguration(xiiNetworkTransportType::TlsTcp);
  serverConfiguration.m_bEnableSessionRecovery = clientConfiguration.m_bEnableSessionRecovery = true;
  serverConfiguration.m_Authentication = clientConfiguration.m_Authentication = xiiNetworkAuthenticationPolicy::Application;
  serverConfiguration.m_pCredentials                                          = XII_DEFAULT_NEW(xiiNetworkCredentials);
  clientConfiguration.m_pCredentials                                          = XII_DEFAULT_NEW(xiiNetworkCredentials);
  if (!XII_TEST_BOOL(serverConfiguration.m_pCredentials->Load(files.Configuration()).Succeeded()) ||
      !XII_TEST_BOOL(clientConfiguration.m_pCredentials->Load(files.Configuration("client")).Succeeded()))
  {
    return;
  }
  xiiNetworkHost server;
  xiiNetworkHost client;
  if (!XII_TEST_BOOL(server.Start(serverConfiguration).Succeeded()) || !XII_TEST_BOOL(client.Start(clientConfiguration).Succeeded()))
  {
    return;
  }
  xiiNetworkListener listener;
  xiiNetworkEndpoint endpoint;
  XII_TEST_BOOL(server.Listen({xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 0}, listener).Succeeded());
  XII_TEST_BOOL(server.GetLocalEndpoint(listener, endpoint).Succeeded());
  xiiNetworkConnectOptions options;
  options.m_sServerName          = "localhost";
  options.m_pAuthentication      = XII_DEFAULT_NEW(xiiNetworkAuthenticationData);
  const xiiUInt8 uiCredentials[] = {1, 2, 3, 4};
  XII_TEST_BOOL(options.m_pAuthentication->Initialize(uiCredentials).Succeeded());
  xiiNetworkConnection                  connection;
  xiiSharedPtr<xiiNetworkSessionTicket> pTicket;
  xiiUuid                               firstSession;
  xiiNetworkChannel                     oldChannel;
  for (xiiUInt32 uiAttempt = 0; uiAttempt < 2; ++uiAttempt)
  {
    if (!XII_TEST_BOOL(client.Connect(endpoint, options, connection).Succeeded()))
    {
      return;
    }
    xiiNetworkEvent event;
    if (!HostTestEvent(server, xiiNetworkEventType::AuthenticationRequired, event))
    {
      return;
    }
    XII_TEST_BOOL(event.m_pAuthentication != nullptr);
    XII_TEST_BOOL(event.m_pAuthentication->GetBytes() == xiiArrayPtr<const xiiUInt8>(uiCredentials));
    const auto               serverConnection = event.m_Connection;
    xiiNetworkConnectionInfo info;
    XII_TEST_BOOL(server.GetConnectionInfo(serverConnection, info).Succeeded());
    XII_TEST_BOOL(info.m_State == xiiNetworkConnectionState::Authenticating);
    xiiNetworkAuthorization authorization;
    authorization.m_sIdentity        = "test-account";
    authorization.m_bMayOpenChannels = false;
    if (!XII_TEST_BOOL(server.Authorize(serverConnection, authorization, true).Succeeded()))
    {
      return;
    }
    if (!HostTestEvent(server, xiiNetworkEventType::Connected, event) || !HostTestEvent(client, xiiNetworkEventType::Connected, event))
    {
      return;
    }
    if (!XII_TEST_BOOL(event.m_pSessionTicket != nullptr && event.m_pSessionTicket->IsValid()))
    {
      return;
    }
    pTicket = event.m_pSessionTicket;
    XII_TEST_BOOL(client.GetConnectionInfo(connection, info).Succeeded());
    XII_TEST_BOOL(info.m_bSessionResumed == (uiAttempt != 0));
    if (uiAttempt == 0)
    {
      firstSession = info.m_Session;
    }
    else
    {
      XII_TEST_BOOL(info.m_Session == firstSession);
    }
    xiiNetworkChannel channel;
    XII_TEST_BOOL(client.GetDefaultChannel(connection, channel).Succeeded());
    if (uiAttempt == 0)
    {
      oldChannel = channel;
    }
    else
    {
      xiiNetworkMessage message;
      XII_TEST_BOOL(client.Send(oldChannel, xiiArrayPtr<const xiiUInt8>(), message).m_Code == xiiNetworkErrorCode::InvalidHandle);
    }
    xiiNetworkChannelConfiguration channelConfiguration;
    XII_TEST_BOOL(client.OpenChannel(connection, channelConfiguration, channel).m_Code == xiiNetworkErrorCode::PermissionDenied);
    options.m_pSessionTicket = pTicket;
    // Abort the native stream to exercise recovery rather than authenticated logout.
    XII_TEST_BOOL(client.Disconnect(connection, false).Succeeded());
    if (!HostTestEvent(client, xiiNetworkEventType::Closed, event) || !HostTestEvent(server, xiiNetworkEventType::Closed, event))
    {
      return;
    }
  }
  XII_TEST_BOOL(server.RevokeSession(firstSession).Succeeded());
  XII_TEST_BOOL(server.RevokeSession(firstSession).m_Code == xiiNetworkErrorCode::SessionExpired);
  if (!XII_TEST_BOOL(client.Connect(endpoint, options, connection).Succeeded()))
  {
    return;
  }
  xiiNetworkEvent event;
  if (!HostTestEvent(server, xiiNetworkEventType::AuthenticationRequired, event))
  {
    return;
  }
  xiiNetworkAuthorization authorization;
  authorization.m_sIdentity = "test-account";
  XII_TEST_BOOL(server.Authorize(event.m_Connection, authorization, true).Succeeded());
  if (!HostTestEvent(server, xiiNetworkEventType::Closed, event))
  {
    return;
  }
  XII_TEST_BOOL(event.m_Error.m_Code == xiiNetworkErrorCode::SessionExpired);
  XII_TEST_BOOL(HostTestEvent(client, xiiNetworkEventType::Closed, event));
}
