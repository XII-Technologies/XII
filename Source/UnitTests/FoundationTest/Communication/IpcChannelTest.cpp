/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Time/Stopwatch.h>
#include <optional>

#if XII_ENABLED(XII_PLATFORM_WINDOWS) || XII_ENABLED(XII_PLATFORM_LINUX)

class ChannelTester
{
public:
  ChannelTester(xiiIpcChannel* pChannel, bool bPing)
  {
    m_bPing    = bPing;
    m_pChannel = pChannel;
    m_pChannel->SetReceiveCallback(xiiMakeDelegate(&ChannelTester::ReceiveMessageData, this));
    m_pChannel->m_Events.AddEventHandler(xiiMakeDelegate(&ChannelTester::OnIpcEventReceived, this));
  }

  ~ChannelTester()
  {
    m_pChannel->m_Events.RemoveEventHandler(xiiMakeDelegate(&ChannelTester::OnIpcEventReceived, this));
    m_pChannel->SetReceiveCallback({});
  }

  void OnIpcEventReceived(const xiiIpcChannelEvent& e)
  {
    XII_LOCK(m_Mutex);
    m_ReceivedEvents.ExpandAndGetRef() = e;
  }

  std::optional<xiiIpcChannelEvent> WaitForEvents(xiiTime timeout)
  {
    xiiStopwatch sw;

    while (sw.GetRunningTotal() < timeout)
    {
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(m_Mutex);

      if (!m_ReceivedEvents.IsEmpty())
      {
        xiiIpcChannelEvent e = m_ReceivedEvents.PeekFront();
        m_ReceivedEvents.PopFront();
        return e;
      }
    }

    return {};
  }

  void ReceiveMessageData(xiiArrayPtr<const xiiUInt8> data)
  {
    XII_LOCK(m_Mutex);

    if (m_bPing)
    {
      m_pChannel->Send(data);
    }
    else
    {
      m_ReceivedMessages.ExpandAndGetRef() = data;
    }
  }

  std::optional<xiiDynamicArray<xiiUInt8>> WaitForMessage(xiiTime timeout)
  {
    xiiResult res = m_pChannel->WaitForMessages(timeout);
    if (res.Succeeded())
    {
      XII_LOCK(m_Mutex);

      if (m_ReceivedMessages.GetCount() > 0)
      {
        auto res2 = m_ReceivedMessages.PeekFront();
        m_ReceivedMessages.PopFront();
        return res2;
      }
    }
    return {};
  }

private:
  bool                                m_bPing = false;
  xiiMutex                            m_Mutex;
  xiiIpcChannel*                      m_pChannel = nullptr;
  xiiDeque<xiiDynamicArray<xiiUInt8>> m_ReceivedMessages;
  xiiDeque<xiiIpcChannelEvent>        m_ReceivedEvents;
};

void TestIPCChannel(xiiIpcChannel* pServer, ChannelTester* pServerTester, xiiIpcChannel* pClient, ChannelTester* pClientTester)
{
  auto MessageMatches = [](const xiiStringView& sReference, const xiiDataBuffer& msg) -> bool {
    xiiStringView sTemp(reinterpret_cast<const char*>(msg.GetData()), msg.GetCount());
    return sTemp == sReference;
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Connect")
  {
    XII_TEST_BOOL(pServer->GetConnectionState() == xiiIpcChannel::ConnectionState::Disconnected);
    XII_TEST_BOOL(pClient->GetConnectionState() == xiiIpcChannel::ConnectionState::Disconnected);
    {
      auto res = pServerTester->WaitForEvents(xiiTime::MakeFromMilliseconds(100));
      XII_TEST_BOOL(!res.has_value());
      auto res2 = pClientTester->WaitForEvents(xiiTime::MakeFromMilliseconds(100));
      XII_TEST_BOOL(!res2.has_value());
    }
    {
      pServer->Connect();
      auto res = pServerTester->WaitForEvents(xiiTime::MakeFromMilliseconds(100));
      XII_TEST_BOOL(res.has_value() && res->m_Type == xiiIpcChannelEvent::Connecting);
      XII_TEST_BOOL(pServer->GetConnectionState() == xiiIpcChannel::ConnectionState::Connecting);
    }
    {
      pClient->Connect();
      auto res = pClientTester->WaitForEvents(xiiTime::MakeFromMilliseconds(100));
      XII_TEST_BOOL(res.has_value() && res->m_Type == xiiIpcChannelEvent::Connecting);
    }
    auto res = pServerTester->WaitForEvents(xiiTime::MakeFromSeconds(100));
    XII_TEST_BOOL(res.has_value() && res->m_Type == xiiIpcChannelEvent::Connected);
    auto res2 = pClientTester->WaitForEvents(xiiTime::MakeFromSeconds(100));
    XII_TEST_BOOL(res2.has_value() && res2->m_Type == xiiIpcChannelEvent::Connected);

    XII_TEST_BOOL(pServer->GetConnectionState() == xiiIpcChannel::ConnectionState::Connected);
    XII_TEST_BOOL(pClient->GetConnectionState() == xiiIpcChannel::ConnectionState::Connected);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Connect When Already Connected")
  {
    pServer->Connect();
    pClient->Connect();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ClientSend")
  {
    xiiStringView sMsg = "TestMessage"_xiisv;

    XII_TEST_BOOL(pClient->Send(xiiConstByteArrayPtr(reinterpret_cast<const xiiUInt8*>(sMsg.GetStartPointer()), sMsg.GetElementCount())));

    auto res = pServerTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res.has_value() && res->m_Type == xiiIpcChannelEvent::NewMessages);
    auto res2 = pClientTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res2.has_value() && res2->m_Type == xiiIpcChannelEvent::NewMessages);

    auto res3 = pClientTester->WaitForMessage(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res3.has_value() && MessageMatches(sMsg, res3.value()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ServerSend")
  {
    xiiStringView sMsg = "TestMessage2"_xiisv;

    XII_TEST_BOOL(pServer->Send(xiiConstByteArrayPtr(reinterpret_cast<const xiiUInt8*>(sMsg.GetStartPointer()), sMsg.GetElementCount())));

    auto res2 = pClientTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res2.has_value() && res2->m_Type == xiiIpcChannelEvent::NewMessages);

    auto res3 = pClientTester->WaitForMessage(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res3.has_value() && MessageMatches(sMsg, res3.value()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ClientDisconnect")
  {
    pClient->Disconnect();
    pClient->Disconnect();

    auto res = pServerTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res.has_value() && res->m_Type == xiiIpcChannelEvent::Disconnected);
    auto res2 = pClientTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res2.has_value() && res2->m_Type == xiiIpcChannelEvent::Disconnected);

    XII_TEST_BOOL(pServer->GetConnectionState() == xiiIpcChannel::ConnectionState::Disconnected);
    XII_TEST_BOOL(pClient->GetConnectionState() == xiiIpcChannel::ConnectionState::Disconnected);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reconnect")
  {
    {
      pServer->Connect();
      auto res = pServerTester->WaitForEvents(xiiTime::MakeFromMilliseconds(100));
      XII_TEST_BOOL(res.has_value() && res->m_Type == xiiIpcChannelEvent::Connecting);
      XII_TEST_BOOL(pServer->GetConnectionState() == xiiIpcChannel::ConnectionState::Connecting);
    }
    {
      pClient->Connect();
      auto res = pClientTester->WaitForEvents(xiiTime::MakeFromMilliseconds(100));
      XII_TEST_BOOL(res.has_value() && res->m_Type == xiiIpcChannelEvent::Connecting);
    }

    auto res = pServerTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res.has_value() && res->m_Type == xiiIpcChannelEvent::Connected);
    auto res2 = pClientTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res2.has_value() && res2->m_Type == xiiIpcChannelEvent::Connected);

    XII_TEST_BOOL(pServer->GetConnectionState() == xiiIpcChannel::ConnectionState::Connected);
    XII_TEST_BOOL(pClient->GetConnectionState() == xiiIpcChannel::ConnectionState::Connected);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ClientSend after reconnect")
  {
    xiiStringView sMsg = "TestMessage"_xiisv;

    XII_TEST_BOOL(pClient->Send(xiiConstByteArrayPtr(reinterpret_cast<const xiiUInt8*>(sMsg.GetStartPointer()), sMsg.GetElementCount())));

    auto res = pServerTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res.has_value() && res->m_Type == xiiIpcChannelEvent::NewMessages);
    auto res2 = pClientTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res2.has_value() && res2->m_Type == xiiIpcChannelEvent::NewMessages);

    auto res3 = pClientTester->WaitForMessage(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res3.has_value() && MessageMatches(sMsg, res3.value()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ServerDisconnect")
  {
    pServer->Disconnect();
    pServer->Disconnect();

    auto res = pServerTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res.has_value() && res->m_Type == xiiIpcChannelEvent::Disconnected);
    auto res2 = pClientTester->WaitForEvents(xiiTime::MakeFromSeconds(1));
    XII_TEST_BOOL(res2.has_value() && res2->m_Type == xiiIpcChannelEvent::Disconnected);

    XII_TEST_BOOL(pServer->GetConnectionState() == xiiIpcChannel::ConnectionState::Disconnected);
    XII_TEST_BOOL(pClient->GetConnectionState() == xiiIpcChannel::ConnectionState::Disconnected);
  }
}

/* TODO: Enet does not connect in process.
XII_CREATE_SIMPLE_TEST(Communication, IpcChannel_Network)
{
  xiiUniquePtr<xiiIpcChannel> pServer = xiiIpcChannel::CreateNetworkChannel("127.0.0.1:1050"_xiisv, xiiIpcChannel::Mode::Server);
  xiiUniquePtr<ChannelTester> pServerTester = XII_DEFAULT_NEW(ChannelTester, pServer.Borrow(), true);

  xiiUniquePtr<xiiIpcChannel> pClient = xiiIpcChannel::CreateNetworkChannel("127.0.0.1:1050"_xiisv, xiiIpcChannel::Mode::Client);
  xiiUniquePtr<ChannelTester> pClientTester = XII_DEFAULT_NEW(ChannelTester, pClient.Borrow(), false);

  TestIPCChannel(pServer.Borrow(), pServerTester.Borrow(), pClient.Borrow(), pClientTester.Borrow());

  pClientTester.Clear();
  pClient.Clear();

  pServerTester.Clear();
  pServer.Clear();
}
*/

XII_CREATE_SIMPLE_TEST(Communication, IpcChannel_Pipe)
{
  xiiUniquePtr<xiiIpcChannel> pServer       = xiiIpcChannel::CreatePipeChannel("XII_unit_test_channel", xiiIpcChannel::Mode::Server);
  xiiUniquePtr<ChannelTester> pServerTester = XII_DEFAULT_NEW(ChannelTester, pServer.Borrow(), true);

  xiiUniquePtr<xiiIpcChannel> pClient       = xiiIpcChannel::CreatePipeChannel("XII_unit_test_channel", xiiIpcChannel::Mode::Client);
  xiiUniquePtr<ChannelTester> pClientTester = XII_DEFAULT_NEW(ChannelTester, pClient.Borrow(), false);

  TestIPCChannel(pServer.Borrow(), pServerTester.Borrow(), pClient.Borrow(), pClientTester.Borrow());

  pClientTester.Clear();
  pClient.Clear();

  pServerTester.Clear();
  pServer.Clear();
}

#endif
