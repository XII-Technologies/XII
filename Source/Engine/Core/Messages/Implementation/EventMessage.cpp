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
  template <typename World, typename GameObject>
  static void UpdateCachedReceivers(const xiiMessage& msg, World& ref_world, GameObject pSearchObject, xiiSmallArray<xiiComponentHandle, 1>& ref_cachedReceivers)
  {
    if (ref_cachedReceivers.GetUserData<xiiUInt32>() == 0)
    {
      using ComponentType = typename std::conditional<std::is_const<World>::value, const xiiComponent*, xiiComponent*>::type;

      xiiHybridArray<ComponentType, 4> eventMsgHandlers;
      ref_world.FindEventMsgHandlers(msg, pSearchObject, eventMsgHandlers);

      for (auto pEventMsgHandler : eventMsgHandlers)
      {
        ref_cachedReceivers.PushBack(pEventMsgHandler->GetHandle());
      }

      ref_cachedReceivers.GetUserData<xiiUInt32>() = 1;
    }
  }

  bool EventMessageSenderHelper::SendEventMessage(xiiMessage& ref_msg, xiiComponent* pSenderComponent, xiiGameObject* pSearchObject, xiiSmallArray<xiiComponentHandle, 1>& ref_cachedReceivers)
  {
    xiiWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(ref_msg, *pWorld, pSearchObject, ref_cachedReceivers);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    bool bResult = false;
    for (auto hReceiver : ref_cachedReceivers)
    {
      xiiComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        bResult |= pReceiverComponent->SendMessage(ref_msg);
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && ref_msg.GetDebugMessageRouting())
    {
      xiiLog::Warning("xiiEventMessageSender::SendMessage: No event message handler found for message of type {0}.", ref_msg.GetId());
    }
#endif

    return bResult;
  }

  bool EventMessageSenderHelper::SendEventMessage(xiiMessage& ref_msg, const xiiComponent* pSenderComponent, const xiiGameObject* pSearchObject, xiiSmallArray<xiiComponentHandle, 1>& ref_cachedReceivers)
  {
    const xiiWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(ref_msg, *pWorld, pSearchObject, ref_cachedReceivers);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    bool bResult = false;
    for (auto hReceiver : ref_cachedReceivers)
    {
      const xiiComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        bResult |= pReceiverComponent->SendMessage(ref_msg);
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && ref_msg.GetDebugMessageRouting())
    {
      xiiLog::Warning("xiiEventMessageSender::SendMessage: No event message handler found for message of type {0}.", ref_msg.GetId());
    }
#endif

    return bResult;
  }

  void EventMessageSenderHelper::PostEventMessage(const xiiMessage& msg, const xiiComponent* pSenderComponent, const xiiGameObject* pSearchObject, xiiSmallArray<xiiComponentHandle, 1>& ref_cachedReceivers, xiiTime delay, xiiObjectMsgQueueType::Enum queueType)
  {
    const xiiWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(msg, *pWorld, pSearchObject, ref_cachedReceivers);

    if (!ref_cachedReceivers.IsEmpty())
    {
      for (auto hReceiver : ref_cachedReceivers)
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
