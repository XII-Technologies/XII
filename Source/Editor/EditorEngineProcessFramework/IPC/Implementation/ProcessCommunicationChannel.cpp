#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>

xiiProcessCommunicationChannel::xiiProcessCommunicationChannel() = default;

xiiProcessCommunicationChannel::~xiiProcessCommunicationChannel()
{
  m_pProtocol.Clear();
  m_pChannel.Clear();
}

bool xiiProcessCommunicationChannel::SendMessage(xiiProcessMessage* pMessage)
{
  if (m_pFirstAllowedMessageType != nullptr)
  {
    // ignore all messages that are not the first allowed message
    // this is necessary to make sure that during an engine restart we don't accidentally send stray messages while
    // the engine is not yet correctly set up
    if (!pMessage->GetDynamicRTTI()->IsDerivedFrom(m_pFirstAllowedMessageType))
    {
      xiiLog::Warning("[IPC]Ignored send message of type {} because it is not a {}", pMessage->GetDynamicRTTI()->GetTypeName(), m_pFirstAllowedMessageType->GetTypeName());
      return false;
    }

    m_pFirstAllowedMessageType = nullptr;
  }

  {
    if (m_pProtocol == nullptr)
      return false;

    return m_pProtocol->Send(pMessage);
  }
}

bool xiiProcessCommunicationChannel::ProcessMessages()
{
  if (!m_pProtocol)
    return false;

  return m_pProtocol->ProcessMessages();
}

void xiiProcessCommunicationChannel::WaitForMessages()
{
  if (!m_pProtocol)
    return;

  m_pProtocol->WaitForMessages().IgnoreResult();
}

void xiiProcessCommunicationChannel::MessageFunc(const xiiProcessMessage* pMsg)
{
  const xiiRTTI* pRtti = pMsg->GetDynamicRTTI();

  if (m_pWaitForMessageType != nullptr && pMsg->GetDynamicRTTI()->IsDerivedFrom(m_pWaitForMessageType))
  {
    if (m_WaitForMessageCallback.IsValid())
    {
      if (m_WaitForMessageCallback(const_cast<xiiProcessMessage*>(pMsg)))
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
  XII_ASSERT_DEV(pMsg != nullptr, "Object could not be allocated");
  XII_ASSERT_DEV(pRtti->IsDerivedFrom<xiiProcessMessage>(), "Msg base type is invalid");

  Event e;
  e.m_pMessage = pMsg;
  m_Events.Broadcast(e);
}

xiiResult xiiProcessCommunicationChannel::WaitForMessage(const xiiRTTI* pMessageType, xiiTime timeout, WaitForMessageCallback* pMessageCallack)
{
  XII_ASSERT_DEV(m_pProtocol != nullptr && m_pChannel != nullptr, "Need to connect first before waiting for a message.");
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
    if (timeout == xiiTime())
    {
      m_pProtocol->WaitForMessages().IgnoreResult();
    }
    else
    {
      xiiTime tTimeLeft = timeout - (xiiTime::Now() - tStart);

      if (tTimeLeft < xiiTime::MakeZero())
      {
        // Don't time out if a debugger is attached to make stepping easier.
        if (xiiSystemInformation::IsDebuggerAttached())
        {
          tTimeLeft = xiiTime::MakeFromSeconds(1);
        }
        else
        {
          m_pWaitForMessageType = nullptr;
          xiiLog::Dev("Reached time-out of {0} seconds while waiting for {1}", xiiArgF(timeout.GetSeconds(), 1), pMessageType->GetTypeName());
          return XII_FAILURE;
        }
      }

      m_pProtocol->WaitForMessages(tTimeLeft).IgnoreResult();
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

xiiResult xiiProcessCommunicationChannel::WaitForConnection(xiiTime timeout)
{
  if (m_pChannel->IsConnected())
  {
    return XII_SUCCESS;
  }

  xiiThreadSignal waitForConnectionSignal;

  xiiEventSubscriptionID eventSubscriptionId = m_pChannel->m_Events.AddEventHandler([&](const xiiIpcChannelEvent& event) {
    switch (event.m_Type)
    {
      case xiiIpcChannelEvent::Connected:
      case xiiIpcChannelEvent::Disconnected:
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

  if (timeout == xiiTime())
  {
    waitForConnectionSignal.WaitForSignal();
  }
  else
  {
    if (waitForConnectionSignal.WaitForSignal(timeout) == xiiThreadSignal::WaitResult::Timeout)
    {
      return XII_FAILURE;
    }
  }

  return m_pChannel->IsConnected() ? XII_SUCCESS : XII_FAILURE;
}

bool xiiProcessCommunicationChannel::IsConnected() const
{
  return m_pChannel->IsConnected();
}
