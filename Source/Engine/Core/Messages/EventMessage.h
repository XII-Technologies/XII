#pragma once

#include <Core/World/World.h>
#include <Foundation/Communication/Message.h>

/// \brief Base class for all messages that are sent as 'events'
struct XII_CORE_DLL xiiEventMessage : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiEventMessage, xiiMessage);

  xiiGameObjectHandle m_hSenderObject;
  xiiComponentHandle  m_hSenderComponent;
};

namespace xiiInternal
{
  struct XII_CORE_DLL EventMessageSenderHelper
  {
    static void SendEventMessage(xiiComponent* pSenderComponent, xiiArrayPtr<xiiComponentHandle> receivers, xiiEventMessage& msg);
    static void SendEventMessage(const xiiComponent* pSenderComponent, xiiArrayPtr<xiiComponentHandle> receivers, xiiEventMessage& msg);
    static void PostEventMessage(const xiiComponent* pSenderComponent, xiiArrayPtr<xiiComponentHandle> receivers, const xiiEventMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame);
  };
} // namespace xiiInternal

/// \brief A message sender that sends all messages to the next component derived from xiiEventMessageHandlerComponent
///   up in the hierarchy starting with the given search object. If none is found the message is sent to
///   all components registered as global event message handler. The receiver is cached after the first send/post call.
template <typename EventMessageType>
class xiiEventMessageSender : public xiiMessageSenderBase<EventMessageType>
{
public:
  XII_ALWAYS_INLINE void SendEventMessage(EventMessageType& msg, xiiComponent* pSenderComponent, xiiGameObject* pSearchObject)
  {
    UpdateMessageAndCachedReceivers(msg, pSenderComponent, pSearchObject);

    xiiInternal::EventMessageSenderHelper::SendEventMessage(pSenderComponent, m_CachedReceivers, msg);
  }

  XII_ALWAYS_INLINE void SendEventMessage(EventMessageType& msg, const xiiComponent* pSenderComponent, const xiiGameObject* pSearchObject) const
  {
    UpdateMessageAndCachedReceivers(msg, pSenderComponent, pSearchObject);

    xiiInternal::EventMessageSenderHelper::SendEventMessage(pSenderComponent, m_CachedReceivers, msg);
  }

  XII_ALWAYS_INLINE void PostEventMessage(EventMessageType& msg, xiiComponent* pSenderComponent, xiiGameObject* pSearchObject, xiiTime delay, xiiObjectMsgQueueType::Enum queueType)
  {
    UpdateMessageAndCachedReceivers(msg, pSenderComponent, pSearchObject);

    xiiInternal::EventMessageSenderHelper::PostEventMessage(pSenderComponent, m_CachedReceivers, msg, delay, queueType);
  }

  XII_ALWAYS_INLINE void PostEventMessage(EventMessageType& msg, const xiiComponent* pSenderComponent, const xiiGameObject* pSearchObject, xiiTime delay, xiiObjectMsgQueueType::Enum queueType) const
  {
    UpdateMessageAndCachedReceivers(msg, pSenderComponent, pSearchObject);

    xiiInternal::EventMessageSenderHelper::PostEventMessage(pSenderComponent, m_CachedReceivers, msg, delay, queueType);
  }

  XII_ALWAYS_INLINE void Invalidate()
  {
    m_CachedReceivers.Clear();
    m_CachedReceivers.GetUserData<xiiUInt32>() = 0;
  }

private:
  void UpdateMessageAndCachedReceivers(xiiEventMessage& msg, xiiComponent* pSenderComponent, xiiGameObject* pSearchObject)
  {
    msg.m_hSenderObject    = pSenderComponent->GetOwner() != nullptr ? pSenderComponent->GetOwner()->GetHandle() : xiiGameObjectHandle();
    msg.m_hSenderComponent = pSenderComponent->GetHandle();

    if (m_CachedReceivers.GetUserData<xiiUInt32>() == 0)
    {
      xiiHybridArray<xiiComponent*, 4> eventMsgHandlers;
      pSenderComponent->GetWorld()->FindEventMsgHandlers(msg, pSearchObject, eventMsgHandlers);

      for (auto pEventMsgHandler : eventMsgHandlers)
      {
        m_CachedReceivers.PushBack(pEventMsgHandler->GetHandle());
      }

      m_CachedReceivers.GetUserData<xiiUInt32>() = 1;
    }
  }

  void UpdateMessageAndCachedReceivers(xiiEventMessage& msg, const xiiComponent* pSenderComponent, const xiiGameObject* pSearchObject) const
  {
    msg.m_hSenderObject    = pSenderComponent->GetOwner() != nullptr ? pSenderComponent->GetOwner()->GetHandle() : xiiGameObjectHandle();
    msg.m_hSenderComponent = pSenderComponent->GetHandle();

    if (m_CachedReceivers.GetUserData<xiiUInt32>() == 0)
    {
      xiiHybridArray<const xiiComponent*, 4> eventMsgHandlers;
      pSenderComponent->GetWorld()->FindEventMsgHandlers(msg, pSearchObject, eventMsgHandlers);

      for (auto pEventMsgHandler : eventMsgHandlers)
      {
        m_CachedReceivers.PushBack(pEventMsgHandler->GetHandle());
      }

      m_CachedReceivers.GetUserData<xiiUInt32>() = 1;
    }
  }

  mutable xiiSmallArray<xiiComponentHandle, 1> m_CachedReceivers;
};
