/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>
#include <FoundationTest/Network/Implementation/NetworkHostTestUtilities.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>

#include <Foundation/Utilities/CommandLineUtils.h>
#include <algorithm>

using namespace xiiNetworkHostTestUtilities;

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostTransportBenchmark)
{
  const auto* pCommandLine = xiiCommandLineUtils::GetGlobalInstance();
  const bool  bLocalOnly   = pCommandLine->GetBoolOption("-networkLocalBenchmark");
  if (!bLocalOnly && !pCommandLine->GetBoolOption("-networkTransportBenchmark"))
  {
    return;
  }
  NetworkSecurityTestCertificates files;
  if (!bLocalOnly && !XII_TEST_BOOL(files.Initialize().Succeeded()))
  {
    return;
  }
  constexpr xiiUInt32 MaxConnections            = 32;
  const auto          uiConnections             = static_cast<xiiUInt32>(xiiMath::Clamp(pCommandLine->GetIntOption("-networkBenchmarkConnections", MaxConnections), 1, static_cast<xiiInt32>(MaxConnections)));
  constexpr xiiUInt32 uiBatch                   = 32;
  constexpr xiiUInt32 uiBlocks                  = 128;
  constexpr xiiUInt32 uiPayloadBytes            = 16384;
  xiiUInt8            uiPayload[uiPayloadBytes] = {};
  for (xiiUInt32 uiIndex = 0; uiIndex < uiPayloadBytes; ++uiIndex)
  {
    uiPayload[uiIndex] = static_cast<xiiUInt8>(uiIndex);
  }

  for (const auto type : {xiiNetworkTransportType::Tcp, xiiNetworkTransportType::Local, xiiNetworkTransportType::TlsTcp, xiiNetworkTransportType::SecureDatagram})
  {
    const bool bSecure = type == xiiNetworkTransportType::TlsTcp || type == xiiNetworkTransportType::SecureDatagram;
    const bool bLocal  = type == xiiNetworkTransportType::Local;
    if (bLocalOnly && bSecure)
    {
      continue;
    }
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
    if (bSecure)
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
    xiiNetworkListener      listener;
    xiiNetworkEndpoint      endpoint;
    xiiNetworkLocalEndpoint local;
    if (bLocal)
    {
      local = MakeLocalTestEndpoint("local-bench");
      if (!XII_TEST_BOOL(server.Listen(local, listener).Succeeded()))
      {
        return;
      }
    }
    else if (!XII_TEST_BOOL(server.Listen({xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 0}, listener).Succeeded()) ||
             !XII_TEST_BOOL(server.GetLocalEndpoint(listener, endpoint).Succeeded()))
    {
      return;
    }

    const auto               idleStatistics = server.GetStatistics();
    const auto               start          = xiiTime::Now();
    xiiNetworkConnection     clients[MaxConnections];
    xiiNetworkConnectOptions options;
    if (bSecure)
    {
      options.m_sServerName = "localhost";
    }
    const auto Connect = [&](xiiNetworkConnection& ref_connection) {
      return bLocal ? client.Connect(local, options, ref_connection) : client.Connect(endpoint, options, ref_connection);
    };
    xiiNetworkEvent event;
    for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections; ++uiIndex)
    {
      if (!XII_TEST_BOOL(Connect(clients[uiIndex]).Succeeded()) ||
          !HostTestEvent(server, xiiNetworkEventType::Connected, event) ||
          !HostTestEvent(client, xiiNetworkEventType::Connected, event))
      {
        return;
      }
    }
    const auto establishTime  = xiiTime::Now() - start;
    const auto busyStatistics = server.GetStatistics();
    XII_TEST_INT(busyStatistics.m_uiActiveSessions, uiConnections);
    xiiNetworkConnection excess;
    XII_TEST_BOOL(Connect(excess).m_Code == xiiNetworkErrorCode::Backpressure);

    xiiNetworkChannel channels[4];
    xiiNetworkChannel peerChannels[MaxConnections];
    for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections; ++uiIndex)
    {
      if (!XII_TEST_BOOL(client.GetDefaultChannel(clients[uiIndex], peerChannels[uiIndex]).Succeeded()))
      {
        return;
      }
    }
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
          const auto        channel = uiMode == 0 ? channels[0] :
            uiConnections > 1                     ? peerChannels[uiIndex % uiConnections] :
                                                    channels[uiIndex % 4];
          if (!XII_TEST_BOOL(client.Send(channel, uiPayload, message).Succeeded()))
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

    constexpr xiiUInt32 uiLatencySamples             = 128;
    double              fRoundTrip[uiLatencySamples] = {};
    for (xiiUInt32 uiIndex = 0; uiIndex < uiLatencySamples; ++uiIndex)
    {
      const auto        latencyStart = xiiTime::Now();
      xiiNetworkMessage message;
      if (!XII_TEST_BOOL(client.Send(channels[0], xiiArrayPtr<const xiiUInt8>(uiPayload, 64), message).Succeeded()) ||
          !HostTestEvent(server, xiiNetworkEventType::Message, event) ||
          !XII_TEST_BOOL(server.Send(event.m_Channel, event.m_Payload.GetBytes(), message).Succeeded()) ||
          !HostTestEvent(client, xiiNetworkEventType::Message, event) ||
          !XII_TEST_BOOL(event.m_Payload.GetCount() == 64))
      {
        return;
      }
      fRoundTrip[uiIndex] = (xiiTime::Now() - latencyStart).GetMicroseconds();
    }
    std::sort(std::begin(fRoundTrip), std::end(fRoundTrip));
    const auto churnStart = xiiTime::Now();
    for (xiiUInt32 uiIndex = 0; uiIndex < uiConnections; ++uiIndex)
    {
      if (!XII_TEST_BOOL(client.Disconnect(clients[uiIndex]).Succeeded()))
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
    const auto  uiPoolDelta = busyStatistics.m_IO.m_BufferPool.m_uiCommittedBytes - xiiMath::Min(busyStatistics.m_IO.m_BufferPool.m_uiCommittedBytes, idleStatistics.m_IO.m_BufferPool.m_uiCommittedBytes);
    const auto  pool        = server.GetStatistics().m_IO.m_BufferPool;
    const char* szTransport = bLocal ? "Local" : type == xiiNetworkTransportType::Tcp ? "TCP" :
      type == xiiNetworkTransportType::TlsTcp                                         ? "TLS" :
                                                                                        "QUIC";
    xiiTestFramework::Output(xiiTestOutput::Details,
                             "%s: %u concurrent, %.1f us establish mean, %.2f MiB/s one channel, %.2f MiB/s spread, %.1f/%.1f us 64-byte echo p50/p99, %.1f closed/s, %llu incremental server pool bytes/peer",
                             szTransport, uiConnections, establishTime.GetMicroseconds() / uiConnections, fThroughput[0], fThroughput[1],
                             fRoundTrip[63], fRoundTrip[126], uiConnections / churnTime.GetSeconds(),
                             static_cast<unsigned long long>(uiPoolDelta / uiConnections));
    client.Shutdown();
    server.Shutdown();
    const auto clientIO = client.GetStatistics().m_IO;
    const auto serverIO = server.GetStatistics().m_IO;
    xiiTestFramework::Output(xiiTestOutput::Details, "%s: worker CPU %.3f s / %.3f s elapsed, %llu server pool allocations, %llu reuses, %llu backpressure events",
                             szTransport, (clientIO.m_WorkerCpuTime + serverIO.m_WorkerCpuTime).GetSeconds(), elapsed.GetSeconds(),
                             static_cast<unsigned long long>(pool.m_uiAllocations), static_cast<unsigned long long>(pool.m_uiReuses),
                             static_cast<unsigned long long>(pool.m_uiBackpressureEvents));
  }
}
