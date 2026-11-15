/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/NetworkHost.h>
#include <Foundation/Network/NetworkResolver.h>
#include <Foundation/Network/UdpSocket.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Time/Time.h>
#include <FoundationTest/Network/Implementation/NetworkHostTestUtilities.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>
#include <cstring>
#include <utility>

using namespace xiiNetworkHostTestUtilities;

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostReliableMessaging)
{
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  for (auto type : {xiiNetworkTransportType::Tcp, xiiNetworkTransportType::TlsTcp, xiiNetworkTransportType::SecureDatagram})
  {
    auto serverConfiguration = HostTestConfiguration(type);
    auto clientConfiguration = HostTestConfiguration(type);
    if (type != xiiNetworkTransportType::Tcp)
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

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostBroadcastAdmission)
{
  const auto     configuration = HostTestConfiguration(xiiNetworkTransportType::Tcp);
  xiiNetworkHost server;
  xiiNetworkHost client;
  if (!XII_TEST_BOOL(server.Start(configuration).Succeeded()) || !XII_TEST_BOOL(client.Start(configuration).Succeeded()))
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
  xiiNetworkConnection connections[2];
  xiiNetworkChannel    channels[3];
  xiiNetworkEvent      event;
  for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
  {
    if (!XII_TEST_BOOL(client.Connect(endpoint, {}, connections[uiIndex]).Succeeded()) ||
        !HostTestEvent(server, xiiNetworkEventType::Connected, event) ||
        !XII_TEST_BOOL(server.GetDefaultChannel(event.m_Connection, channels[uiIndex * 2]).Succeeded()) ||
        !HostTestEvent(client, xiiNetworkEventType::Connected, event))
    {
      return;
    }
  }
  // Leave the middle recipient invalid: admission must continue to the second peer.
  xiiNetworkBroadcastResult results[3];
  const xiiUInt8            uiRejected[] = {0xEE};
  XII_TEST_BOOL(server.Broadcast(xiiMakeArrayPtr(channels), uiRejected, xiiMakeArrayPtr(results).GetSubArray(0, 2)).m_Code == xiiNetworkErrorCode::InvalidArgument);
  const xiiUInt8 uiPayload[] = {1, 2, 3, 4};
  if (!XII_TEST_BOOL(server.Broadcast(xiiMakeArrayPtr(channels), uiPayload, xiiMakeArrayPtr(results), 73).Succeeded()))
  {
    return;
  }
  XII_TEST_BOOL(results[0].m_Error.Succeeded() && results[0].m_Message.IsValid());
  XII_TEST_BOOL(results[1].m_Error.m_Code == xiiNetworkErrorCode::InvalidHandle && !results[1].m_Message.IsValid());
  XII_TEST_BOOL(results[2].m_Error.Succeeded() && results[2].m_Message.IsValid());
  XII_TEST_BOOL(results[0].m_Message != results[2].m_Message);
  bool bReceived[2] = {};
  for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
  {
    if (!HostTestEvent(client, xiiNetworkEventType::Message, event))
    {
      return;
    }
    XII_TEST_INT(event.m_Payload.GetCount(), 4);
    XII_TEST_BOOL(event.m_Payload.GetCount() == 4 && std::memcmp(event.m_Payload.GetBytes().GetPtr(), uiPayload, 4) == 0);
    const xiiUInt32 uiPeer = event.m_Connection == connections[0] ? 0 : 1;
    XII_TEST_BOOL(event.m_Connection == connections[uiPeer] && !bReceived[uiPeer]);
    bReceived[uiPeer] = true;
  }
  bool bCompleted[2] = {};
  for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
  {
    if (!HostTestEvent(server, xiiNetworkEventType::MessageSent, event))
    {
      return;
    }
    const xiiUInt32 uiPeer = event.m_Message == results[0].m_Message ? 0 : 1;
    XII_TEST_BOOL(event.m_Message == results[uiPeer * 2].m_Message && !bCompleted[uiPeer]);
    XII_TEST_BOOL(event.m_Error.Succeeded());
    XII_TEST_INT(event.m_uiUserData, 73);
    bCompleted[uiPeer] = true;
  }
  XII_TEST_INT(server.GetStatistics().m_uiMessagesSent, 2);
  XII_TEST_BOOL(server.Broadcast({}, {}, {}).Succeeded());
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
    if (!HostTestEvent(sender, xiiNetworkEventType::MessageSent, event) ||
        !XII_TEST_BOOL_MSG(event.m_Error.Succeeded(), "UDP send of %u bytes: %s (%d)", uiSize, event.m_Error.GetCodeName(), event.m_Error.m_iNativeCode))
    {
      return;
    }
    XII_TEST_BOOL(event.m_Message == message);
    if (!HostTestEvent(receiver, xiiNetworkEventType::Datagram, event))
    {
      return;
    }
    XII_TEST_BOOL(event.m_Error.Succeeded());
    XII_TEST_INT(event.m_Payload.GetCount(), uiSize);
    XII_TEST_BOOL(event.m_Endpoint == sourceEndpoint);
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
  for (const auto type : {xiiNetworkTransportType::TlsTcp, xiiNetworkTransportType::SecureDatagram})
  {
    auto serverConfiguration = HostTestConfiguration(type);
    auto clientConfiguration = HostTestConfiguration(type);
    if (type == xiiNetworkTransportType::SecureDatagram)
    {
      serverConfiguration.m_IdleTimeout = clientConfiguration.m_IdleTimeout = xiiTime::MakeFromMilliseconds(750);
    }
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
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostChannelAuthorizationTimeout)
{
  auto configuration                           = HostTestConfiguration(xiiNetworkTransportType::Tcp);
  configuration.m_bRequireChannelAuthorization = true;
  configuration.m_AuthenticationTimeout        = xiiTime::MakeFromMilliseconds(100);
  xiiNetworkHost server;
  xiiNetworkHost client;
  if (!XII_TEST_BOOL(server.Start(configuration).Succeeded()) || !XII_TEST_BOOL(client.Start(configuration).Succeeded()))
  {
    return;
  }
  xiiNetworkListener listener;
  xiiNetworkEndpoint endpoint;
  XII_TEST_BOOL(server.Listen({xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 0}, listener).Succeeded());
  XII_TEST_BOOL(server.GetLocalEndpoint(listener, endpoint).Succeeded());
  xiiNetworkConnection connection;
  XII_TEST_BOOL(client.Connect(endpoint, {}, connection).Succeeded());
  xiiNetworkEvent event;
  if (!HostTestEvent(server, xiiNetworkEventType::Connected, event) || !HostTestEvent(client, xiiNetworkEventType::Connected, event))
  {
    return;
  }
  xiiNetworkChannel channel;
  XII_TEST_BOOL(client.OpenChannel(connection, {}, channel).Succeeded());
  if (!HostTestEvent(server, xiiNetworkEventType::ChannelRequested, event))
  {
    return;
  }
  XII_TEST_BOOL(server.AuthorizeChannel(event.m_Channel, false).Succeeded());
  XII_TEST_BOOL(server.AuthorizeChannel(event.m_Channel, true).m_Code == xiiNetworkErrorCode::InvalidArgument);
  if (!HostTestEvent(client, xiiNetworkEventType::ChannelClosed, event))
  {
    return;
  }
  XII_TEST_BOOL(event.m_Error.m_Code == xiiNetworkErrorCode::PermissionDenied);
  const auto old = channel;
  XII_TEST_BOOL(client.OpenChannel(connection, {}, channel).Succeeded());
  XII_TEST_BOOL(channel.m_uiId != old.m_uiId);
  if (!HostTestEvent(server, xiiNetworkEventType::ChannelRequested, event))
  {
    return;
  }
  if (!HostTestEvent(server, xiiNetworkEventType::Closed, event))
  {
    return;
  }
  XII_TEST_BOOL(event.m_Error.m_Code == xiiNetworkErrorCode::TimedOut);
  XII_TEST_BOOL(HostTestEvent(client, xiiNetworkEventType::Closed, event));
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostQuicDeliveryModes)
{
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  auto serverConfiguration           = HostTestConfiguration(xiiNetworkTransportType::SecureDatagram);
  auto clientConfiguration           = serverConfiguration;
  serverConfiguration.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
  clientConfiguration.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
  if (!XII_TEST_BOOL(serverConfiguration.m_pCredentials->Load(files.Configuration()).Succeeded()) ||
      !XII_TEST_BOOL(clientConfiguration.m_pCredentials->Load(files.Configuration("client")).Succeeded()))
  {
    return;
  }
  xiiNetworkHost server;
  xiiNetworkHost client;
  const auto     serverStart = server.Start(serverConfiguration);
  const auto     clientStart = client.Start(clientConfiguration);
  if (!XII_TEST_BOOL_MSG(serverStart.Succeeded(), "%s (%d)", serverStart.GetCodeName(), serverStart.m_iNativeCode) ||
      !XII_TEST_BOOL_MSG(clientStart.Succeeded(), "%s (%d)", clientStart.GetCodeName(), clientStart.m_iNativeCode))
  {
    return;
  }

  xiiNetworkListener listener;
  xiiNetworkEndpoint endpoint;
  const auto         listen = server.Listen({xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 0}, listener);
  if (!XII_TEST_BOOL_MSG(listen.Succeeded(), "%s (%d)", listen.GetCodeName(), listen.m_iNativeCode) ||
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
  if (!HostTestEvent(server, xiiNetworkEventType::Connected, event) ||
      !HostTestEvent(client, xiiNetworkEventType::Connected, event))
  {
    return;
  }

  xiiNetworkConnectionInfo info;
  if (!XII_TEST_BOOL(client.GetConnectionInfo(connection, info).Succeeded()))
  {
    return;
  }
  XII_TEST_BOOL(info.m_Capabilities.m_bIndependentChannelProgress);
  XII_TEST_BOOL(info.m_Capabilities.m_bEncrypted);
  XII_TEST_BOOL(info.m_Capabilities.m_uiMaximumUnreliablePayload >= 256);

  for (auto mode : {xiiNetworkDeliveryMode::ReliableOrdered, xiiNetworkDeliveryMode::ReliableUnordered, xiiNetworkDeliveryMode::UnreliableSequenced, xiiNetworkDeliveryMode::UnreliableUnsequenced})
  {
    xiiNetworkChannelConfiguration configuration;
    configuration.m_Delivery = mode;
    xiiNetworkChannel channel;
    if (!XII_TEST_BOOL(info.m_Capabilities.Supports(mode)) ||
        !XII_TEST_BOOL(client.OpenChannel(connection, configuration, channel).Succeeded()) ||
        !HostTestEvent(client, xiiNetworkEventType::ChannelOpened, event) ||
        !HostTestEvent(server, xiiNetworkEventType::ChannelOpened, event))
    {
      return;
    }
    const auto serverChannel = event.m_Channel;

    const bool bUnreliable = mode == xiiNetworkDeliveryMode::UnreliableSequenced || mode == xiiNetworkDeliveryMode::UnreliableUnsequenced;
    if (bUnreliable)
    {
      xiiNetworkIOBuffer large;
      if (!XII_TEST_BOOL(client.AcquireBuffer(65536, large).Succeeded()))
      {
        return;
      }
      xiiNetworkMessage rejected;
      XII_TEST_BOOL(client.Send(channel, std::move(large), rejected).m_Code == xiiNetworkErrorCode::MessageTooLarge);
      XII_TEST_BOOL(large.IsValid());
    }
    // Concurrent reliable messages exercise independent stream lifetimes. Unreliable messages are
    // sent one at a time so the sequenced policy does not depend on loopback packet arrival order.
    const xiiUInt32   uiBatch       = bUnreliable ? 1 : 24;
    bool              bReceived[24] = {};
    xiiNetworkMessage ids[24];
    for (xiiUInt32 uiIndex = 0; uiIndex < uiBatch; ++uiIndex)
    {
      const xiiUInt8 uiPayload[] = {static_cast<xiiUInt8>(uiIndex), 17, 255};
      if (!XII_TEST_BOOL(client.Send(channel, uiPayload, ids[uiIndex], uiIndex).Succeeded()))
      {
        return;
      }
    }
    for (xiiUInt32 uiIndex = 0; uiIndex < uiBatch; ++uiIndex)
    {
      if (!HostTestEvent(server, xiiNetworkEventType::Message, event) || !XII_TEST_INT(event.m_Payload.GetCount(), 3))
      {
        return;
      }
      XII_TEST_BOOL(event.m_Channel == serverChannel);
      const auto bytes = event.m_Payload.GetBytes();
      if (!XII_TEST_BOOL(bytes[0] < uiBatch))
      {
        return;
      }
      XII_TEST_BOOL(!bReceived[bytes[0]]);
      bReceived[bytes[0]] = true;
      if (mode == xiiNetworkDeliveryMode::ReliableOrdered)
      {
        XII_TEST_INT(bytes[0], uiIndex);
      }
      XII_TEST_INT(bytes[1], 17);
      XII_TEST_INT(bytes[2], 255);
    }
    for (xiiUInt32 uiIndex = 0; uiIndex < uiBatch; ++uiIndex)
    {
      if (!HostTestEvent(client, xiiNetworkEventType::MessageSent, event) ||
          !XII_TEST_BOOL_MSG(event.m_Error.Succeeded(), "%s (%d)", event.m_Error.GetCodeName(), event.m_Error.m_iNativeCode) ||
          !XII_TEST_BOOL(event.m_uiUserData < uiBatch))
      {
        return;
      }
      XII_TEST_BOOL(event.m_Message == ids[event.m_uiUserData]);
    }
    if (!XII_TEST_BOOL(client.CloseChannel(channel).Succeeded()) ||
        !HostTestEvent(client, xiiNetworkEventType::ChannelClosed, event) ||
        !HostTestEvent(server, xiiNetworkEventType::ChannelClosed, event))
    {
      return;
    }
  }
  XII_TEST_BOOL(client.Disconnect(connection).Succeeded());
  if (!HostTestEvent(client, xiiNetworkEventType::Closed, event) ||
      !HostTestEvent(server, xiiNetworkEventType::Closed, event))
  {
    return;
  }
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostQuicRejectsUnverifiedPeers)
{
  NetworkSecurityTestCertificates files;
  NetworkSecurityTestCertificates untrusted;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()) || !XII_TEST_BOOL(untrusted.Initialize().Succeeded()))
  {
    return;
  }
  auto serverConfiguration           = HostTestConfiguration(xiiNetworkTransportType::SecureDatagram);
  serverConfiguration.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
  if (!XII_TEST_BOOL(serverConfiguration.m_pCredentials->Load(files.Configuration()).Succeeded()))
  {
    return;
  }
  xiiNetworkHost server;
  if (!XII_TEST_BOOL(server.Start(serverConfiguration).Succeeded()))
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

  for (xiiUInt32 uiCase = 0; uiCase < 3; ++uiCase)
  {
    auto clientConfiguration = HostTestConfiguration(xiiNetworkTransportType::SecureDatagram);
    auto security            = files.Configuration(uiCase == 2 ? "" : "client");
    if (uiCase == 1)
    {
      security.m_sTrustAnchorsFile = untrusted.Configuration().m_sTrustAnchorsFile;
    }
    clientConfiguration.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
    xiiNetworkHost client;
    if (!XII_TEST_BOOL(clientConfiguration.m_pCredentials->Load(security).Succeeded()) ||
        !XII_TEST_BOOL(client.Start(clientConfiguration).Succeeded()))
    {
      return;
    }
    xiiNetworkConnectOptions options;
    options.m_sServerName = uiCase == 0 ? "unmatched.invalid" : "localhost";
    xiiNetworkConnection connection;
    if (!XII_TEST_BOOL(client.Connect(endpoint, options, connection).Succeeded()))
    {
      return;
    }
    xiiNetworkEvent event;
    if (!HostTestEvent(client, xiiNetworkEventType::Closed, event))
    {
      return;
    }
    if (uiCase == 0)
    {
      XII_TEST_BOOL(event.m_Error.m_Code == xiiNetworkErrorCode::HostnameMismatch);
    }
    if (uiCase == 1)
    {
      XII_TEST_BOOL(event.m_Error.m_Code == xiiNetworkErrorCode::CertificateUntrusted);
    }
    if (!HostTestEvent(server, xiiNetworkEventType::Closed, event))
    {
      return;
    }
    if (uiCase == 2)
    {
      XII_TEST_BOOL(event.m_Error.m_Code == xiiNetworkErrorCode::CertificateUntrusted);
    }
    XII_TEST_INT(client.GetStatistics().m_uiConnectionsEstablished, 0);
  }
  XII_TEST_INT(server.GetStatistics().m_uiConnectionsEstablished, 0);
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostReconnectionAndBackpressure)
{
  auto configuration                      = HostTestConfiguration(xiiNetworkTransportType::Tcp);
  configuration.m_uiMaximumQueuedMessages = 1;
  xiiNetworkHost client;
  xiiNetworkHost server;
  if (!XII_TEST_BOOL(client.Start(configuration).Succeeded()) || !XII_TEST_BOOL(server.Start(configuration).Succeeded()))
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
  options.m_Reconnect.m_Mode            = xiiNetworkReconnectMode::Fixed;
  options.m_Reconnect.m_fJitterFraction = 0;
  options.m_Reconnect.m_InitialDelay    = xiiTime::MakeFromMilliseconds(20);
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
  xiiNetworkChannel oldChannel;
  if (!XII_TEST_BOOL(client.GetDefaultChannel(connection, oldChannel).Succeeded()))
  {
    return;
  }
  const xiiUInt8    uiPayload[] = {255, 0, 17};
  xiiNetworkMessage first;
  if (!XII_TEST_BOOL(client.Send(oldChannel, uiPayload, first).Succeeded()))
  {
    return;
  }
  xiiNetworkIOBuffer leased;
  if (!XII_TEST_BOOL(client.AcquireBuffer(3, leased).Succeeded()))
  {
    return;
  }
  xiiNetworkMessage rejected;
  XII_TEST_BOOL(client.Send(oldChannel, std::move(leased), rejected).m_Code == xiiNetworkErrorCode::Backpressure);
  XII_TEST_BOOL(leased.IsValid());
  if (!HostTestEvent(client, xiiNetworkEventType::MessageSent, event) ||
      !HostTestEvent(server, xiiNetworkEventType::Message, event))
  {
    return;
  }
  XII_TEST_INT(client.GetStatistics().m_uiMessagesSent, 1);

  if (!XII_TEST_BOOL(server.Disconnect(serverConnection, false).Succeeded()) ||
      !HostTestEvent(server, xiiNetworkEventType::Closed, event) ||
      !HostTestEvent(client, xiiNetworkEventType::ReconnectScheduled, event) ||
      !HostTestEvent(server, xiiNetworkEventType::Connected, event) ||
      !HostTestEvent(client, xiiNetworkEventType::Connected, event))
  {
    return;
  }
  XII_TEST_BOOL(event.m_Connection == connection);
  XII_TEST_BOOL(client.Send(oldChannel, uiPayload, rejected).m_Code == xiiNetworkErrorCode::InvalidHandle);
  xiiNetworkChannel newChannel;
  XII_TEST_BOOL(client.GetDefaultChannel(connection, newChannel).Succeeded());
  XII_TEST_BOOL(newChannel != oldChannel);
  XII_TEST_INT(client.GetStatistics().m_uiConnectionsEstablished, 2);
  XII_TEST_BOOL(server.WaitForEvent(event, xiiTime::MakeFromMilliseconds(30)).m_Code == xiiNetworkErrorCode::TimedOut);
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostQuicIpv6AndProtocolMismatch)
{
  const xiiNetworkEndpoint loopback(xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv6), 0);
  xiiUdpSocket             probe;
  auto                     probeError = probe.Open(xiiNetworkAddressFamily::Ipv6);
  if (probeError.Succeeded())
  {
    probeError = probe.Bind(loopback);
  }
  if (probeError.m_Code == xiiNetworkErrorCode::UnsupportedAddressFamily || probeError.m_Code == xiiNetworkErrorCode::AddressNotAvailable)
  {
    xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped QUIC IPv6 loopback: %s (%d).", probeError.GetCodeName(), probeError.m_iNativeCode);
    return;
  }
  if (!XII_TEST_BOOL(probeError.Succeeded()))
  {
    return;
  }
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  auto serverConfiguration           = HostTestConfiguration(xiiNetworkTransportType::SecureDatagram);
  auto clientConfiguration           = serverConfiguration;
  serverConfiguration.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
  clientConfiguration.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
  if (!XII_TEST_BOOL(serverConfiguration.m_pCredentials->Load(files.Configuration()).Succeeded()) ||
      !XII_TEST_BOOL(clientConfiguration.m_pCredentials->Load(files.Configuration("client")).Succeeded()))
  {
    return;
  }
  serverConfiguration.m_uiSupportedFeatures = 1;
  serverConfiguration.m_uiRequiredFeatures  = 1;
  xiiNetworkHost server;
  if (!XII_TEST_BOOL(server.Start(serverConfiguration).Succeeded()))
  {
    return;
  }
  xiiNetworkListener listener;
  xiiNetworkEndpoint endpoint;
  if (!XII_TEST_BOOL(server.Listen(loopback, listener).Succeeded()) ||
      !XII_TEST_BOOL(server.GetLocalEndpoint(listener, endpoint).Succeeded()))
  {
    return;
  }
  xiiNetworkConnectOptions options;
  options.m_sServerName = "::1";
  for (const bool bCompatible : {false, true})
  {
    clientConfiguration.m_uiSupportedFeatures = bCompatible ? 1 : 0;
    xiiNetworkHost       client;
    xiiNetworkConnection connection;
    if (!XII_TEST_BOOL(client.Start(clientConfiguration).Succeeded()) ||
        !XII_TEST_BOOL(client.Connect(endpoint, options, connection).Succeeded()))
    {
      return;
    }
    xiiNetworkEvent event;
    if (bCompatible)
    {
      if (!HostTestEvent(client, xiiNetworkEventType::Connected, event) ||
          !HostTestEvent(server, xiiNetworkEventType::Connected, event))
      {
        return;
      }
      xiiNetworkConnectionInfo info;
      XII_TEST_BOOL(client.GetConnectionInfo(connection, info).Succeeded());
      XII_TEST_BOOL(info.m_RemoteEndpoint == endpoint);
      XII_TEST_INT(info.m_uiNegotiatedFeatures, 1);
      xiiNetworkCertificate certificate;
      XII_TEST_BOOL(client.GetPeerCertificate(connection, certificate).Succeeded());
    }
    else
    {
      if (!HostTestEvent(client, xiiNetworkEventType::Closed, event))
      {
        return;
      }
      XII_TEST_BOOL(event.m_Error.m_Code == xiiNetworkErrorCode::ProtocolMismatch);
      if (!HostTestEvent(server, xiiNetworkEventType::Closed, event))
      {
        return;
      }
      XII_TEST_BOOL(event.m_Error.m_Code == xiiNetworkErrorCode::ProtocolMismatch);
    }
  }
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostQuicTriesResolvedEndpointCandidates)
{
  xiiDynamicArray<xiiNetworkEndpoint> candidates;
  if (!XII_TEST_BOOL(xiiNetworkResolver::Resolve("localhost", "0", candidates, xiiNetworkAddressFamily::Unspecified, xiiNetworkTransport::Udp).Succeeded()))
  {
    return;
  }
  if (candidates.GetCount() < 2)
  {
    xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped QUIC endpoint fallback because localhost resolves to only one address.");
    return;
  }
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  auto serverConfiguration           = HostTestConfiguration(xiiNetworkTransportType::SecureDatagram);
  auto clientConfiguration           = serverConfiguration;
  serverConfiguration.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
  clientConfiguration.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
  if (!XII_TEST_BOOL(serverConfiguration.m_pCredentials->Load(files.Configuration()).Succeeded()) ||
      !XII_TEST_BOOL(clientConfiguration.m_pCredentials->Load(files.Configuration("client")).Succeeded()))
  {
    return;
  }
  xiiNetworkHost server;
  xiiNetworkHost client;
  if (!XII_TEST_BOOL(server.Start(serverConfiguration).Succeeded()) ||
      !XII_TEST_BOOL(client.Start(clientConfiguration).Succeeded()))
  {
    return;
  }
  // Only the last DNS candidate is listening. Prior UDP candidates fail by refusal or a bounded wait.
  xiiNetworkListener listener;
  xiiNetworkEndpoint endpoint;
  const auto         bindError = server.Listen(candidates.PeekBack(), listener);
  if (bindError.m_Code == xiiNetworkErrorCode::UnsupportedAddressFamily || bindError.m_Code == xiiNetworkErrorCode::AddressNotAvailable)
  {
    xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped QUIC endpoint fallback because the last localhost address cannot listen.");
    return;
  }
  if (!XII_TEST_BOOL(bindError.Succeeded()) ||
      !XII_TEST_BOOL(server.GetLocalEndpoint(listener, endpoint).Succeeded()))
  {
    return;
  }
  xiiStringBuilder sPort;
  sPort.SetFormat("{}", endpoint.GetPort());
  xiiNetworkConnectOptions options;
  options.m_sServerName = "localhost";
  xiiNetworkConnection connection;
  if (!XII_TEST_BOOL(client.Connect("localhost", sPort, options, connection).Succeeded()))
  {
    return;
  }
  xiiNetworkEvent event;
  if (!HostTestEvent(client, xiiNetworkEventType::Connected, event) ||
      !HostTestEvent(server, xiiNetworkEventType::Connected, event))
  {
    return;
  }
  xiiNetworkConnectionInfo info;
  XII_TEST_BOOL(client.GetConnectionInfo(connection, info).Succeeded());
  XII_TEST_BOOL(info.m_RemoteEndpoint == endpoint);
}
