/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>
#include <FoundationTest/Network/Implementation/NetworkHostTestUtilities.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>

#include <Foundation/Utilities/CommandLineUtils.h>
#include <ctime>

using namespace xiiNetworkHostTestUtilities;

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostTransportBenchmark)
{
  if (!xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-networkTransportBenchmark"))
  {
    return;
  }
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  constexpr xiiUInt32 uiConnections             = 32;
  constexpr xiiUInt32 uiBatch                   = 32;
  constexpr xiiUInt32 uiBlocks                  = 8;
  constexpr xiiUInt32 uiPayloadBytes            = 16384;
  xiiUInt8            uiPayload[uiPayloadBytes] = {};
  for (xiiUInt32 uiIndex = 0; uiIndex < uiPayloadBytes; ++uiIndex)
  {
    uiPayload[uiIndex] = static_cast<xiiUInt8>(uiIndex);
  }

  for (const auto type : {xiiNetworkTransportType::Tcp, xiiNetworkTransportType::TlsTcp, xiiNetworkTransportType::SecureDatagram})
  {
    auto serverConfiguration = HostTestConfiguration(type);
    auto clientConfiguration = serverConfiguration;
    for (auto* pConfiguration : {&serverConfiguration, &clientConfiguration})
    {
      pConfiguration->m_uiMaximumConnections  = uiConnections;
      pConfiguration->m_uiMaximumQueuedEvents = 512;
      pConfiguration->m_IdleTimeout           = xiiTime::MakeFromSeconds(60);
      pConfiguration->m_HandshakeTimeout      = xiiTime::MakeFromSeconds(10);
      pConfiguration->m_CloseTimeout          = xiiTime::MakeFromSeconds(3);
    }
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
    if (!XII_TEST_BOOL(server.Start(serverConfiguration).Succeeded()) ||
        !XII_TEST_BOOL(client.Start(clientConfiguration).Succeeded()))
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

    const auto               idleStatistics = server.GetStatistics();
    const auto               start          = xiiTime::Now();
    const auto               cpuStart       = std::clock();
    xiiNetworkConnection     clients[uiConnections];
    xiiNetworkConnection     servers[uiConnections];
    xiiNetworkConnectOptions options;
    options.m_sServerName = "localhost";
    for (auto& connection : clients)
    {
      if (!XII_TEST_BOOL(client.Connect(endpoint, options, connection).Succeeded()))
      {
        return;
      }
    }
    xiiNetworkEvent event;
    for (auto& connection : servers)
    {
      if (!HostTestEvent(server, xiiNetworkEventType::Connected, event))
      {
        return;
      }
      connection = event.m_Connection;
    }
    for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections; ++uiIndex)
    {
      if (!HostTestEvent(client, xiiNetworkEventType::Connected, event))
      {
        return;
      }
    }
    const auto establishTime  = xiiTime::Now() - start;
    const auto busyStatistics = server.GetStatistics();
    XII_TEST_INT(busyStatistics.m_uiActiveSessions, uiConnections);
    xiiNetworkConnection excess;
    XII_TEST_BOOL(client.Connect(endpoint, options, excess).m_Code == xiiNetworkErrorCode::Backpressure);

    xiiNetworkChannel channels[4];
    if (!XII_TEST_BOOL(client.GetDefaultChannel(clients[0], channels[0]).Succeeded()))
    {
      return;
    }
    for (xiiUInt32 uiIndex = 1; uiIndex < 4; ++uiIndex)
    {
      if (!XII_TEST_BOOL(client.OpenChannel(clients[0], {}, channels[uiIndex]).Succeeded()) ||
          !HostTestEvent(client, xiiNetworkEventType::ChannelOpened, event) ||
          !HostTestEvent(server, xiiNetworkEventType::ChannelOpened, event))
      {
        return;
      }
    }
    double fThroughput[2] = {};
    for (xiiUInt32 uiMode = 0; uiMode < 2; ++uiMode)
    {
      const auto begin = xiiTime::Now();
      for (xiiUInt32 uiBlock = 0; uiBlock < uiBlocks; ++uiBlock)
      {
        for (xiiUInt32 uiIndex = 0; uiIndex < uiBatch; ++uiIndex)
        {
          xiiNetworkMessage message;
          if (!XII_TEST_BOOL(client.Send(channels[uiMode == 0 ? 0 : uiIndex % 4], uiPayload, message).Succeeded()))
          {
            return;
          }
        }
        for (xiiUInt32 uiIndex = 0; uiIndex < uiBatch; ++uiIndex)
        {
          if (!HostTestEvent(server, xiiNetworkEventType::Message, event) ||
              !XII_TEST_BOOL(event.m_Payload.GetBytes() == xiiArrayPtr<const xiiUInt8>(uiPayload)) ||
              !HostTestEvent(client, xiiNetworkEventType::MessageSent, event) ||
              !XII_TEST_BOOL(event.m_Error.Succeeded()))
          {
            return;
          }
        }
      }
      fThroughput[uiMode] = static_cast<double>(uiBlocks * uiBatch * uiPayloadBytes) / (1024.0 * 1024.0 * (xiiTime::Now() - begin).GetSeconds());
    }

    const auto          latencyStart     = xiiTime::Now();
    constexpr xiiUInt32 uiLatencySamples = 32;
    for (xiiUInt32 uiIndex = 0; uiIndex < uiLatencySamples; ++uiIndex)
    {
      xiiNetworkMessage message;
      if (!XII_TEST_BOOL(client.Send(channels[0], xiiArrayPtr<const xiiUInt8>(uiPayload, 64), message).Succeeded()) ||
          !HostTestEvent(server, xiiNetworkEventType::Message, event) ||
          !XII_TEST_BOOL(server.Send(event.m_Channel, event.m_Payload.GetBytes(), message).Succeeded()) ||
          !HostTestEvent(client, xiiNetworkEventType::Message, event) ||
          !XII_TEST_BOOL(event.m_Payload.GetCount() == 64))
      {
        return;
      }
    }
    const auto latency    = (xiiTime::Now() - latencyStart).GetMicroseconds() / uiLatencySamples;
    const auto churnStart = xiiTime::Now();
    for (auto connection : clients)
    {
      if (!XII_TEST_BOOL(client.Disconnect(connection).Succeeded()))
      {
        return;
      }
    }
    for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections; ++uiIndex)
    {
      if (!HostTestEvent(client, xiiNetworkEventType::Closed, event) || !HostTestEvent(server, xiiNetworkEventType::Closed, event))
      {
        return;
      }
    }
    const auto  churnTime   = xiiTime::Now() - churnStart;
    const auto  elapsed     = xiiTime::Now() - start;
    const auto  cpuElapsed  = std::clock() - cpuStart;
    const auto  uiPoolDelta = busyStatistics.m_IO.m_BufferPool.m_uiCommittedBytes - xiiMath::Min(busyStatistics.m_IO.m_BufferPool.m_uiCommittedBytes, idleStatistics.m_IO.m_BufferPool.m_uiCommittedBytes);
    const char* szTransport = type == xiiNetworkTransportType::Tcp ? "TCP" : type == xiiNetworkTransportType::TlsTcp ? "TLS" :
                                                                                                                       "QUIC";
    xiiTestFramework::Output(xiiTestOutput::Details,
                             "%s: %u concurrent, %.1f established/s, %.2f MiB/s one channel, %.2f MiB/s four channels, %.1f us 64-byte echo RTT, %.1f closed/s, %llu incremental server pool bytes/peer",
                             szTransport, uiConnections, uiConnections / establishTime.GetSeconds(), fThroughput[0], fThroughput[1], latency, uiConnections / churnTime.GetSeconds(),
                             static_cast<unsigned long long>(uiPoolDelta / uiConnections));
#if XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)
    xiiTestFramework::Output(xiiTestOutput::Details, "%s: CPU %.3f s / %.3f s elapsed", szTransport, static_cast<double>(cpuElapsed) / CLOCKS_PER_SEC, elapsed.GetSeconds());
#else
    XII_IGNORE_UNUSED(cpuElapsed);
    XII_IGNORE_UNUSED(elapsed);
#endif
  }
}
