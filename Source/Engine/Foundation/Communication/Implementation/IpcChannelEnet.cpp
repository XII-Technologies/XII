/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Communication/RemoteInterfaceEnet.h>
#  include <Foundation/Communication/RemoteMessage.h>
#  include <Foundation/Logging/Log.h>

xiiIpcChannelEnet::xiiIpcChannelEnet(xiiStringView sAddress, Mode::Enum mode) :
  xiiIpcChannel(sAddress, mode), m_sAddress(sAddress)
{
  m_pNetwork = xiiRemoteInterfaceEnet::Make();
  m_pNetwork->SetMessageHandler(0, xiiMakeDelegate(&xiiIpcChannelEnet::NetworkMessageHandler, this));
  m_pNetwork->m_RemoteEvents.AddEventHandler(xiiMakeDelegate(&xiiIpcChannelEnet::EnetEventHandler, this));

  m_pOwner->AddChannel(this);
}

xiiIpcChannelEnet::~xiiIpcChannelEnet()
{
  m_pNetwork->ShutdownConnection();

  m_pOwner->RemoveChannel(this);
}

void xiiIpcChannelEnet::InternalConnect()
{
  if (m_Mode == Mode::Server)
  {
    m_pNetwork->StartServer('RMOT', m_sAddress, false).IgnoreResult();

    SetConnectionState(ConnectionState::Connecting);
  }
  else
  {
    SetConnectionState(ConnectionState::Connecting);

    if ((m_sLastAddress != m_sAddress) || (xiiTime::Now() - m_LastConnectAttempt > xiiTime::MakeFromSeconds(10)))
    {
      m_sLastAddress       = m_sAddress;
      m_LastConnectAttempt = xiiTime::Now();
      m_pNetwork->ConnectToServer('RMOT', m_sAddress, false).IgnoreResult();
    }

    m_pNetwork->WaitForConnectionToServer(xiiTime::MakeFromMilliseconds(10.0)).IgnoreResult();
  }

  SetConnectionState(m_pNetwork->IsConnectedToOther() ? ConnectionState::Connected : ConnectionState::Disconnected);
}

void xiiIpcChannelEnet::InternalDisconnect()
{
  m_pNetwork->ShutdownConnection();
  m_pNetwork->m_RemoteEvents.RemoveEventHandler(xiiMakeDelegate(&xiiIpcChannelEnet::EnetEventHandler, this));

  SetConnectionState(ConnectionState::Disconnected);
}

void xiiIpcChannelEnet::InternalSend()
{
  {
    XII_LOCK(m_OutputQueueMutex);

    while (!m_OutputQueue.IsEmpty())
    {
      xiiContiguousMemoryStreamStorage& storage = m_OutputQueue.PeekFront();

      m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, 0, 0, storage);

      m_OutputQueue.PopFront();
    }
  }

  m_pNetwork->UpdateRemoteInterface();
}

bool xiiIpcChannelEnet::NeedWakeup() const
{
  return true;
}

void xiiIpcChannelEnet::Tick()
{
  m_pNetwork->UpdateRemoteInterface();

  SetConnectionState(m_pNetwork->IsConnectedToOther() ? ConnectionState::Connected : ConnectionState::Disconnected);

  m_pNetwork->ExecuteAllMessageHandlers();
}

void xiiIpcChannelEnet::NetworkMessageHandler(xiiRemoteMessage& msg)
{
  ReceiveData(msg.GetMessageData());
}

void xiiIpcChannelEnet::EnetEventHandler(const xiiRemoteEvent& e)
{
  if (e.m_Type == xiiRemoteEvent::DisconnectedFromServer)
  {
    xiiLog::Info("Disconnected from remote engine process.");
    Disconnect();
  }

  if (e.m_Type == xiiRemoteEvent::ConnectedToServer)
  {
    xiiLog::Info("Connected to remote engine process.");
  }
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcChannelEnet);
