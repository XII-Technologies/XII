#include <Core/CorePCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiEventMessage);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEventMessage, 1, xiiRTTIDefaultAllocator<xiiEventMessage>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_CHECK_AT_COMPILETIME(sizeof(xiiEventMessageSender<xiiEventMessage>) == 16);

namespace xiiInternal
{
  void EventMessageSenderHelper::SendEventMessage(xiiComponent* pSenderComponent, xiiArrayPtr<xiiComponentHandle> receivers, xiiEventMessage& msg)
  {
    xiiWorld* pWorld = pSenderComponent->GetWorld();
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    for (auto hReceiver : receivers)
    {
      xiiComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        pReceiverComponent->SendMessage(msg);
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && msg.GetDebugMessageRouting())
    {
      xiiLog::Warning("xiiEventMessageSender::SendMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

  void EventMessageSenderHelper::SendEventMessage(const xiiComponent* pSenderComponent, xiiArrayPtr<xiiComponentHandle> receivers, xiiEventMessage& msg)
  {
    const xiiWorld* pWorld = pSenderComponent->GetWorld();
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    for (auto hReceiver : receivers)
    {
      const xiiComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        pReceiverComponent->SendMessage(msg);
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && msg.GetDebugMessageRouting())
    {
      xiiLog::Warning("xiiEventMessageSender::SendMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

  void EventMessageSenderHelper::PostEventMessage(const xiiComponent* pSenderComponent, xiiArrayPtr<xiiComponentHandle> receivers, const xiiEventMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType)
  {
    if (!receivers.IsEmpty())
    {
      const xiiWorld* pWorld = pSenderComponent->GetWorld();
      for (auto hReceiver : receivers)
      {
        pWorld->PostMessage(hReceiver, msg, delay, queueType);
      }
    }
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      xiiLog::Warning("xiiEventMessageSender::PostMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }
} // namespace xiiInternal



XII_STATICLINK_FILE(Core, Core_Messages_Implementation_EventMessage);
