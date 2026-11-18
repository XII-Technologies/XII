/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>
#include <FoundationTest/Network/Implementation/NetworkHostTestUtilities.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>

#include <Foundation/Network/UdpSocket.h>
#include <Foundation/Threading/Implementation/OSThread.h>
#include <atomic>
#include <cstring>
#include <utility>

using namespace xiiNetworkHostTestUtilities;

namespace
{
  class QuicTestPacketRelay
  {
  public:
    enum class Phase : xiiUInt8
    {
      Forwarding,
      InjectingFaults,
      Closing
    };

    QuicTestPacketRelay() : m_Thread(Entry, this, "QUIC test relay", 512 * 1024) {}
    ~QuicTestPacketRelay() { Stop(); }

    xiiNetworkError Start(const xiiNetworkEndpoint& server, xiiNetworkEndpoint& out_endpoint)
    {
      m_Server          = server;
      const auto family = server.GetAddress().GetFamily();
      auto       error  = m_Socket.Open(family);
      if (error.Succeeded())
      {
        error = m_Socket.Bind({xiiNetworkAddress::MakeLoopback(family), 0});
      }
      if (error.Succeeded())
      {
        error = m_Socket.GetLocalEndpoint(out_endpoint);
      }
      if (error.Succeeded())
      {
        m_bRunning.store(true);
        m_Thread.Start();
      }
      return error;
    }

    void Stop()
    {
      if (m_bRunning.exchange(false))
      {
        m_Thread.Join();
      }
    }

    std::atomic<Phase> m_Phase{Phase::Forwarding};
    xiiUInt32          m_uiDropped     = 0;
    xiiUInt32          m_uiReordered   = 0;
    xiiUInt32          m_uiDuplicated  = 0;
    xiiUInt32          m_uiRelayErrors = 0;
    xiiNetworkError    m_LastError;
    const char*        m_szLastOperation = "none";

  private:
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    static xiiMinWindows::DWORD __stdcall Entry(void* pData)
#else
    static void* Entry(void* pData)
#endif
    {
      static_cast<QuicTestPacketRelay*>(pData)->Run();
      return {};
    }

    bool IsClosingPeerReset(const xiiNetworkError& error) const
    {
      // Winsock can report a previous UDP send to a retired peer as ConnectionReset.
      return error.m_Code == xiiNetworkErrorCode::ConnectionReset && m_Phase.load() == Phase::Closing;
    }

    void Forward(const xiiNetworkEndpoint& endpoint, xiiArrayPtr<const xiiUInt8> packet)
    {
      const auto result = m_Socket.SendTo(endpoint, packet);
      if (IsClosingPeerReset(result.m_Error))
      {
        return;
      }
      if (result.Failed() || result.m_uiBytesTransferred != packet.GetCount())
      {
        ++m_uiRelayErrors;
        m_LastError       = result.m_Error;
        m_szLastOperation = "send";
      }
    }

    void Run()
    {
      xiiNetworkEndpoint client;
      xiiUInt8           uiBytes[65536] = {};
      xiiUInt8           uiHeld[65536]  = {};
      xiiUInt32          uiHeldBytes    = 0;
      xiiUInt32          uiPackets      = 0;
      xiiTime            holdDeadline;
      while (m_bRunning.load())
      {
        if (uiHeldBytes != 0 && xiiTime::Now() >= holdDeadline)
        {
          Forward(m_Server, {uiHeld, uiHeldBytes});
          uiHeldBytes = 0;
        }
        xiiNetworkEndpoint sender;
        const auto         result = m_Socket.ReceiveFrom(uiBytes, sender);
        if (result.m_Error.m_Code == xiiNetworkErrorCode::WouldBlock)
        {
          m_Socket.Wait(xiiNetworkSocketEvent::Readable, xiiTime::MakeFromMilliseconds(2)).IgnoreResult();
          continue;
        }
        if (IsClosingPeerReset(result.m_Error))
        {
          continue;
        }
        if (result.Failed())
        {
          ++m_uiRelayErrors;
          m_LastError       = result.m_Error;
          m_szLastOperation = "receive";
          break;
        }
        const xiiArrayPtr<const xiiUInt8> packet(uiBytes, result.m_uiBytesTransferred);
        if (sender == m_Server)
        {
          if (client.IsValid())
          {
            Forward(client, packet);
          }
          continue;
        }
        if (client.IsValid() && sender != client)
        {
          continue;
        }
        client = sender;
        if (m_Phase.load() != Phase::InjectingFaults)
        {
          Forward(m_Server, packet);
          continue;
        }
        ++uiPackets;
        if (uiPackets % 13 == 0)
        {
          ++m_uiDropped;
          continue;
        }
        if (uiPackets % 7 == 0 && uiHeldBytes == 0)
        {
          uiHeldBytes = packet.GetCount();
          std::memcpy(uiHeld, packet.GetPtr(), uiHeldBytes);
          holdDeadline = xiiTime::Now() + xiiTime::MakeFromMilliseconds(10);
          continue;
        }
        Forward(m_Server, packet);
        if (uiHeldBytes != 0)
        {
          Forward(m_Server, {uiHeld, uiHeldBytes});
          uiHeldBytes = 0;
          ++m_uiReordered;
        }
        if (uiPackets % 17 == 0)
        {
          Forward(m_Server, packet);
          ++m_uiDuplicated;
        }
      }
    }

    xiiUdpSocket       m_Socket;
    xiiNetworkEndpoint m_Server;
    std::atomic<bool>  m_bRunning{false};
    xiiOSThread        m_Thread;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(NetworkTransport, HostQuicSurvivesPacketFaults)
{
  NetworkSecurityTestCertificates files;
  if (!XII_TEST_BOOL(files.Initialize(true).Succeeded()))
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
  xiiNetworkListener listener;
  xiiNetworkEndpoint serverEndpoint;
  if (!XII_TEST_BOOL(server.Listen({xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 0}, listener).Succeeded()) ||
      !XII_TEST_BOOL(server.GetLocalEndpoint(listener, serverEndpoint).Succeeded()))
  {
    return;
  }
  QuicTestPacketRelay relay;
  xiiNetworkEndpoint  relayEndpoint;
  if (!XII_TEST_BOOL(relay.Start(serverEndpoint, relayEndpoint).Succeeded()))
  {
    return;
  }
  xiiNetworkConnectOptions options;
  options.m_sServerName = "127.0.0.1";
  xiiNetworkConnection connection;
  if (!XII_TEST_BOOL(client.Connect(relayEndpoint, options, connection).Succeeded()))
  {
    return;
  }
  xiiNetworkEvent event;
  if (!HostTestEvent(server, xiiNetworkEventType::Connected, event) ||
      !HostTestEvent(client, xiiNetworkEventType::Connected, event))
  {
    return;
  }

  xiiNetworkChannel channel;
  if (!XII_TEST_BOOL(client.GetDefaultChannel(connection, channel).Succeeded()))
  {
    return;
  }
  xiiNetworkIOBuffer  payload;
  constexpr xiiUInt32 uiBytes = 512 * 1024;
  if (!XII_TEST_BOOL(client.AcquireBuffer(uiBytes, payload).Succeeded()))
  {
    return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < uiBytes; ++uiIndex)
  {
    payload.GetBytes()[uiIndex] = static_cast<xiiUInt8>(uiIndex * 31);
  }
  relay.m_Phase.store(QuicTestPacketRelay::Phase::InjectingFaults);
  xiiNetworkMessage message;
  if (!XII_TEST_BOOL(client.Send(channel, std::move(payload), message).Succeeded()) ||
      !HostTestEvent(server, xiiNetworkEventType::Message, event) ||
      !XII_TEST_INT(event.m_Payload.GetCount(), uiBytes))
  {
    return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < uiBytes; ++uiIndex)
  {
    if (event.m_Payload.GetBytes()[uiIndex] != static_cast<xiiUInt8>(uiIndex * 31))
    {
      XII_TEST_BOOL(false);
      return;
    }
  }
  if (!HostTestEvent(client, xiiNetworkEventType::MessageSent, event) || !XII_TEST_BOOL(event.m_Error.Succeeded()))
  {
    return;
  }
  XII_TEST_BOOL(event.m_Message == message);
  relay.m_Phase.store(QuicTestPacketRelay::Phase::Closing);
  if (!XII_TEST_BOOL(client.Disconnect(connection).Succeeded()) ||
      !HostTestEvent(client, xiiNetworkEventType::Closed, event) ||
      !HostTestEvent(server, xiiNetworkEventType::Closed, event))
  {
    return;
  }
  relay.Stop();
  XII_TEST_BOOL(relay.m_uiDropped > 0);
  XII_TEST_BOOL(relay.m_uiReordered > 0);
  XII_TEST_BOOL(relay.m_uiDuplicated > 0);
  XII_TEST_BOOL_MSG(relay.m_uiRelayErrors == 0, "Relay %s failed with %s (%d).", relay.m_szLastOperation,
                    relay.m_LastError.GetCodeName(), relay.m_LastError.m_iNativeCode);
  xiiNetworkConnectionInfo info;
  if (!XII_TEST_BOOL(client.GetConnectionInfo(connection, info).Succeeded()))
  {
    return;
  }
  XII_TEST_BOOL(info.m_Statistics.m_bHasNativeCongestionStatistics);
  XII_TEST_BOOL(info.m_Statistics.m_uiNativeCongestionWindowBytes > 0);
  XII_TEST_BOOL(info.m_Statistics.m_uiNativeLostPackets > 0);
}
