/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/World.h>
#include <Foundation/Communication/Message.h>

/// Base class for all messages that are sent as 'events'
struct XII_CORE_DLL xiiEventMessage : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiEventMessage, xiiMessage);

  xiiGameObjectHandle m_hSenderObject;
  xiiComponentHandle  m_hSenderComponent;

  XII_ALWAYS_INLINE void FillFromSenderComponent(const xiiComponent* pSenderComponent)
  {
    if (pSenderComponent != nullptr)
    {
      m_hSenderComponent = pSenderComponent->GetHandle();
      m_hSenderObject    = pSenderComponent->GetOwner()->GetHandle();
    }
  }
};

namespace xiiInternal
{
  struct XII_CORE_DLL EventMessageSenderHelper
  {
    static bool SendEventMessage(xiiMessage& ref_msg, xiiComponent* pSenderComponent, xiiGameObject* pSearchObject, xiiSmallArray<xiiComponentHandle, 1>& inout_cachedReceivers);
    static bool SendEventMessage(xiiMessage& ref_msg, const xiiComponent* pSenderComponent, const xiiGameObject* pSearchObject, xiiSmallArray<xiiComponentHandle, 1>& inout_cachedReceivers);
    static void PostEventMessage(const xiiMessage& msg, const xiiComponent* pSenderComponent, const xiiGameObject* pSearchObject, xiiSmallArray<xiiComponentHandle, 1>& inout_cachedReceivers, xiiTime delay, xiiObjectMsgQueueType::Enum queueType);
  };
} // namespace xiiInternal

/// A message sender that sends all messages to the next component derived from xiiEventMessageHandlerComponent
///   up in the hierarchy starting with the given search object. If none is found the message is sent to
///   all components registered as global event message handler. The receiver is cached after the first send/post call.
template <typename EventMessageType>
class xiiEventMessageSender : public xiiMessageSenderBase<EventMessageType>
{
public:
  XII_ALWAYS_INLINE bool SendEventMessage(EventMessageType& inout_msg, xiiComponent* pSenderComponent, xiiGameObject* pSearchObject)
  {
    if constexpr (XII_IS_DERIVED_FROM_STATIC(xiiEventMessage, EventMessageType))
    {
      inout_msg.FillFromSenderComponent(pSenderComponent);
    }
    return xiiInternal::EventMessageSenderHelper::SendEventMessage(inout_msg, pSenderComponent, pSearchObject, m_CachedReceivers);
  }

  XII_ALWAYS_INLINE bool SendEventMessage(EventMessageType& inout_msg, const xiiComponent* pSenderComponent, const xiiGameObject* pSearchObject) const
  {
    if constexpr (XII_IS_DERIVED_FROM_STATIC(xiiEventMessage, EventMessageType))
    {
      inout_msg.FillFromSenderComponent(pSenderComponent);
    }
    return xiiInternal::EventMessageSenderHelper::SendEventMessage(inout_msg, pSenderComponent, pSearchObject, m_CachedReceivers);
  }

  XII_ALWAYS_INLINE void PostEventMessage(EventMessageType& ref_msg, xiiComponent* pSenderComponent, xiiGameObject* pSearchObject, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame)
  {
    if constexpr (XII_IS_DERIVED_FROM_STATIC(xiiEventMessage, EventMessageType))
    {
      ref_msg.FillFromSenderComponent(pSenderComponent);
    }
    xiiInternal::EventMessageSenderHelper::PostEventMessage(ref_msg, pSenderComponent, pSearchObject, m_CachedReceivers, delay, queueType);
  }

  XII_ALWAYS_INLINE void PostEventMessage(EventMessageType& ref_msg, const xiiComponent* pSenderComponent, const xiiGameObject* pSearchObject, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const
  {
    if constexpr (XII_IS_DERIVED_FROM_STATIC(xiiEventMessage, EventMessageType))
    {
      ref_msg.FillFromSenderComponent(pSenderComponent);
    }
    xiiInternal::EventMessageSenderHelper::PostEventMessage(ref_msg, pSenderComponent, pSearchObject, m_CachedReceivers, delay, queueType);
  }

  XII_ALWAYS_INLINE void Invalidate()
  {
    m_CachedReceivers.Clear();
    m_CachedReceivers.GetUserData<xiiUInt32>() = 0;
  }

private:
  mutable xiiSmallArray<xiiComponentHandle, 1> m_CachedReceivers;
};
