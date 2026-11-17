/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>

xiiProcessCommunicationChannel::xiiProcessCommunicationChannel() = default;

xiiProcessCommunicationChannel::~xiiProcessCommunicationChannel()
{
  m_pProtocol.Clear();
  m_pChannel.Clear();
}

bool xiiProcessCommunicationChannel::SendMessage(xiiProcessMessage* pMessage)
{
  if (m_pProtocol == nullptr)
    return false;

  if (m_pFirstAllowedMessageType == nullptr)
    return m_pProtocol->Send(pMessage);

  // Ignore stray messages while the engine is being restarted and set up.
  if (!pMessage->GetDynamicRTTI()->IsDerivedFrom(m_pFirstAllowedMessageType))
  {
    xiiLog::Warning("[IPC] Ignored message of type {} while waiting for {}", pMessage->GetDynamicRTTI()->GetTypeName(), m_pFirstAllowedMessageType->GetTypeName());
    return false;
  }
  if (!m_pProtocol->Send(pMessage))
    return false;
  m_pFirstAllowedMessageType = nullptr;
  return true;
}

bool xiiProcessCommunicationChannel::ProcessMessages()
{
  if (!m_pProtocol || m_bDispatchingMessage)
    return false;

  return m_pProtocol->ProcessMessages();
}

void xiiProcessCommunicationChannel::WaitForMessages()
{
  if (!m_pProtocol)
    return;

  m_pProtocol->WaitForMessages().IgnoreResult();
}

void xiiProcessCommunicationChannel::MessageFunc(const xiiIpcProcessMessageProtocol::Event& msg)
{
  m_bDispatchingMessage = true;
  XII_SCOPE_EXIT(m_bDispatchingMessage = false);
  const xiiProcessMessage* pMsg  = msg.m_pMessage;
  const xiiRTTI*           pRtti = pMsg->GetDynamicRTTI();

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
  e.m_pMessage                    = pMsg;
  e.m_bInterruptMessageProcessing = msg.m_bInterruptMessageProcessing;
  m_Events.Broadcast(e);
  msg.m_bInterruptMessageProcessing = e.m_bInterruptMessageProcessing;
}

xiiResult xiiProcessCommunicationChannel::WaitForMessage(const xiiRTTI* pMessageType, xiiTime timeout, WaitForMessageCallback* pMessageCallback)
{
  if (!m_pProtocol || !IsConnected() || pMessageType == nullptr || !timeout.IsPositive() ||
      m_bDispatchingMessage || m_pWaitForMessageType != nullptr)
    return XII_FAILURE;

  m_pWaitForMessageType    = pMessageType;
  m_WaitForMessageCallback = pMessageCallback ? *pMessageCallback : WaitForMessageCallback();
  XII_SCOPE_EXIT(m_pWaitForMessageType = nullptr; m_WaitForMessageCallback = {});

  const auto deadline = xiiTime::Now() + timeout;
  while (m_pWaitForMessageType != nullptr)
  {
    const auto remaining = deadline - xiiTime::Now();
    if (!remaining.IsPositive() || !IsConnected())
      return XII_FAILURE;
    m_pProtocol->WaitForMessages(remaining).IgnoreResult();
  }

  return XII_SUCCESS;
}

xiiResult xiiProcessCommunicationChannel::WaitForConnection(xiiTime timeout)
{
  if (m_pChannel == nullptr || !timeout.IsPositive())
    return XII_FAILURE;
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

  if (waitForConnectionSignal.WaitForSignal(timeout) == xiiThreadSignal::WaitResult::Timeout)
    return XII_FAILURE;

  return m_pChannel->IsConnected() ? XII_SUCCESS : XII_FAILURE;
}

bool xiiProcessCommunicationChannel::IsConnected() const
{
  return m_pChannel != nullptr && m_pChannel->IsConnected();
}
