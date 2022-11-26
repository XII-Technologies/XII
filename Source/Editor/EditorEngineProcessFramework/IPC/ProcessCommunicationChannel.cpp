#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>

xiiProcessCommunicationChannel::xiiProcessCommunicationChannel() {}

xiiProcessCommunicationChannel::~xiiProcessCommunicationChannel()
{
  if (m_pChannel)
  {
    XII_DEFAULT_DELETE(m_pChannel);
  }
}

void xiiProcessCommunicationChannel::SendMessage(xiiProcessMessage* pMessage)
{
  if (m_pFirstAllowedMessageType != nullptr)
  {
    // ignore all messages that are not the first allowed message
    // this is necessary to make sure that during an engine restart we don't accidentally send stray messages while
    // the engine is not yet correctly set up
    if (!pMessage->GetDynamicRTTI()->IsDerivedFrom(m_pFirstAllowedMessageType))
      return;

    m_pFirstAllowedMessageType = nullptr;
  }

  {
    if (m_pChannel == nullptr)
      return;

    m_pChannel->Send(pMessage);
  }
}

bool xiiProcessCommunicationChannel::ProcessMessages()
{
  if (!m_pChannel)
    return false;

  return m_pChannel->ProcessMessages();
}


void xiiProcessCommunicationChannel::WaitForMessages()
{
  if (!m_pChannel)
    return;

  m_pChannel->WaitForMessages();
}

void xiiProcessCommunicationChannel::MessageFunc(const xiiProcessMessage* msg)
{
  const xiiRTTI* pRtti = msg->GetDynamicRTTI();

  if (m_pWaitForMessageType != nullptr && msg->GetDynamicRTTI()->IsDerivedFrom(m_pWaitForMessageType))
  {
    if (m_WaitForMessageCallback.IsValid())
    {
      if (m_WaitForMessageCallback(const_cast<xiiProcessMessage*>(msg)))
      {
        m_WaitForMessageCallback = WaitForMessageCallback();
        m_pWaitForMessageType    = nullptr;
      }
    }
    else
    {
      m_pWaitForMessageType = nullptr;
    }
  }

  XII_ASSERT_DEV(pRtti != nullptr, "Message Type unknown");
  XII_ASSERT_DEV(msg != nullptr, "Object could not be allocated");
  XII_ASSERT_DEV(pRtti->IsDerivedFrom<xiiProcessMessage>(), "Msg base type is invalid");

  Event e;
  e.m_pMessage = msg;
  m_Events.Broadcast(e);
}

xiiResult xiiProcessCommunicationChannel::WaitForMessage(const xiiRTTI* pMessageType, xiiTime tTimeout, WaitForMessageCallback* pMessageCallack)
{
  XII_ASSERT_DEV(m_pChannel != nullptr, "Need to connect first before waiting for a message.");
  // XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "This function is not thread safe");
  XII_ASSERT_DEV(m_pWaitForMessageType == nullptr, "Already waiting for another message!");

  m_pWaitForMessageType = pMessageType;
  if (pMessageCallack)
  {
    m_WaitForMessageCallback = *pMessageCallack;
  }
  else
  {
    m_WaitForMessageCallback = WaitForMessageCallback();
  }

  XII_SCOPE_EXIT(m_WaitForMessageCallback = WaitForMessageCallback(););

  const xiiTime tStart = xiiTime::Now();

  while (m_pWaitForMessageType != nullptr)
  {
    if (tTimeout == xiiTime())
    {
      m_pChannel->WaitForMessages();
    }
    else
    {
      xiiTime tTimeLeft = tTimeout - (xiiTime::Now() - tStart);

      if (tTimeLeft < xiiTime::Zero())
      {
        m_pWaitForMessageType = nullptr;
        xiiLog::Dev("Reached time-out of {0} seconds while waiting for {1}", xiiArgF(tTimeout.GetSeconds(), 1), pMessageType->GetTypeName());
        return XII_FAILURE;
      }

      m_pChannel->WaitForMessages(tTimeLeft).IgnoreResult();
    }

    if (!m_pChannel->IsConnected())
    {
      m_pWaitForMessageType = nullptr;
      xiiLog::Dev("Lost connection while waiting for {}", pMessageType->GetTypeName());
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiProcessCommunicationChannel::WaitForConnection(xiiTime tTimeout)
{
  if (m_pChannel->IsConnected())
  {
    return XII_SUCCESS;
  }

  xiiThreadSignal waitForConnectionSignal;

  xiiEventSubscriptionID eventSubscriptionId = m_pChannel->m_Events.AddEventHandler([&](const xiiIpcChannelEvent& event) {
    switch (event.m_Type)
    {
      case xiiIpcChannelEvent::ConnectedToClient:
      case xiiIpcChannelEvent::ConnectedToServer:
      case xiiIpcChannelEvent::DisconnectedFromClient:
      case xiiIpcChannelEvent::DisconnectedFromServer:
        waitForConnectionSignal.RaiseSignal();
        break;
      default:
        break;
    }
  });

  XII_SCOPE_EXIT(m_pChannel->m_Events.RemoveEventHandler(eventSubscriptionId));

  if (m_pChannel->IsConnected())
  {
    return XII_SUCCESS;
  }

  if (tTimeout == xiiTime())
  {
    waitForConnectionSignal.WaitForSignal();
  }
  else
  {
    if (waitForConnectionSignal.WaitForSignal(tTimeout) == xiiThreadSignal::WaitResult::Timeout)
    {
      return XII_FAILURE;
    }
  }

  return m_pChannel->IsConnected() ? XII_SUCCESS : XII_FAILURE;
}