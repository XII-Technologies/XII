/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/ScopeExit.h>

template <typename EventData, typename MutexType, xiiEventType EventType>
xiiEventBase<EventData, MutexType, EventType>::xiiEventBase(xiiAllocator* pAllocator) :
  m_EventHandlers(pAllocator)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_pSelf = this;
#endif
}

template <typename EventData, typename MutexType, xiiEventType EventType>
xiiEventBase<EventData, MutexType, EventType>::~xiiEventBase()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_ALWAYS(m_pSelf == this, "The xiiEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif
}

/// A callback can be registered multiple times with different pass-through data (or even with the same,
/// though that is less useful).
template <typename EventData, typename MutexType, xiiEventType EventType>
xiiEventSubscriptionID xiiEventBase<EventData, MutexType, EventType>::AddEventHandler(Handler handler) const
{
  XII_LOCK(m_Mutex);

  if constexpr (std::is_same_v<MutexType, xiiNoMutex>)
  {
    if constexpr (EventType == xiiEventType::Default)
    {
      XII_ASSERT_DEV(m_uiRecursionDepth == 0, "Can't add or remove event handlers while broadcasting (without a mutex). Either enable the use of a mutex on this event, or switch to xiiCopyOnBroadcastEvent if this should be allowed. Since this event does not have a mutex, this error can also happen due to multi-threaded access.");
    }
  }

  XII_ASSERT_DEV(!handler.IsComparable() || !HasEventHandler(handler), "The same event handler cannot be added twice");

  auto& item            = m_EventHandlers.ExpandAndGetRef();
  item.m_Handler        = std::move(handler);
  item.m_SubscriptionID = ++m_NextSubscriptionID;

  return item.m_SubscriptionID;
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::AddEventHandler(Handler handler, Unsubscriber& ref_unsubscriber) const
{
  XII_LOCK(m_Mutex);

  if constexpr (std::is_same_v<MutexType, xiiNoMutex>)
  {
    if constexpr (EventType == xiiEventType::Default)
    {
      XII_ASSERT_DEV(m_uiRecursionDepth == 0, "Can't add or remove event handlers while broadcasting (without a mutex). Either enable the use of a mutex on this event, or switch to xiiCopyOnBroadcastEvent if this should be allowed. Since this event does not have a mutex, this error can also happen due to multi-threaded access.");
    }
  }

  ref_unsubscriber.Unsubscribe();
  ref_unsubscriber.m_pEvent         = this;
  ref_unsubscriber.m_SubscriptionID = AddEventHandler(std::move(handler));
}


/// Use exactly the same combination of callback/pass-through-data to unregister an event handlers.
/// Otherwise an error occurs.
template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::RemoveEventHandler(const Handler& handler) const
{
  XII_ASSERT_DEV(handler.IsComparable(), "Lambdas that capture data cannot be removed via function pointer. Use a xiiEventSubscriptionID instead.");

  XII_LOCK(m_Mutex);

  if constexpr (EventType == xiiEventType::Default)
  {
    if constexpr (std::is_same_v<MutexType, xiiNoMutex>)
    {
      XII_ASSERT_DEV(m_uiRecursionDepth == 0, "Can't add or remove event handlers while broadcasting (without a mutex). Either enable the use of a mutex on this event, or switch to xiiCopyOnBroadcastEvent if this should be allowed. Since this event does not have a mutex, this error can also happen due to multi-threaded access.");
    }
  }

  for (xiiUInt32 idx = 0; idx < m_EventHandlers.GetCount(); ++idx)
  {
    if (m_EventHandlers[idx].m_Handler.IsEqualIfComparable(handler))
    {
      if constexpr (EventType == xiiEventType::Default)
      {
        // if this event does not copy the handlers, and we are currently broadcasting
        // we can't shrink the size of the array, however, we can replace elements (the check above says that we have a mutex, so this is fine)

        if (m_uiRecursionDepth > 0)
        {
          // we just write an invalid handler here, and let the broadcast function clean it up for us
          m_EventHandlers[idx].m_Handler        = {};
          m_EventHandlers[idx].m_SubscriptionID = {};
          return;
        }
      }

      // if we are not broadcasting, or the broadcast uses a copy anyway, we can just modify the handler array directly
      m_EventHandlers.RemoveAtAndCopy(idx);
      return;
    }
  }

  XII_ASSERT_DEV(false, "xiiEvent::RemoveEventHandler: Handler has not been registered or already been unregistered.");
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::RemoveEventHandler(xiiEventSubscriptionID& ref_id) const
{
  if (ref_id == 0)
    return;

  const xiiEventSubscriptionID subId = ref_id;
  ref_id                             = 0;

  XII_LOCK(m_Mutex);

  if constexpr (EventType == xiiEventType::Default)
  {
    if constexpr (std::is_same_v<MutexType, xiiNoMutex>)
    {
      XII_ASSERT_DEV(m_uiRecursionDepth == 0, "Can't add or remove event handlers while broadcasting (without a mutex). Either enable the use of a mutex on this event, or switch to xiiCopyOnBroadcastEvent if this should be allowed. Since this event does not have a mutex, this error can also happen due to multi-threaded access.");
    }
  }

  for (xiiUInt32 idx = 0; idx < m_EventHandlers.GetCount(); ++idx)
  {
    if (m_EventHandlers[idx].m_SubscriptionID == subId)
    {
      if constexpr (EventType == xiiEventType::Default)
      {
        // if this event does not copy the handlers, and we are currently broadcasting
        // we can't shrink the size of the array, however, we can replace elements (the check above says that we have a mutex, so this is fine)

        if (m_uiRecursionDepth > 0)
        {
          // we just write an invalid handler here, and let the broadcast function clean it up for us
          m_EventHandlers[idx].m_Handler        = {};
          m_EventHandlers[idx].m_SubscriptionID = {};
          return;
        }
      }

      // if we are not broadcasting, or the broadcast uses a copy anyway, we can just modify the handler array directly
      m_EventHandlers.RemoveAtAndCopy(idx);
      return;
    }
  }

  XII_ASSERT_DEV(false, "xiiEvent::RemoveEventHandler: Invalid subscription ID '{0}'.", (xiiInt32)subId);
}

template <typename EventData, typename MutexType, xiiEventType EventType>
bool xiiEventBase<EventData, MutexType, EventType>::HasEventHandler(const Handler& handler) const
{
  XII_ASSERT_DEV(handler.IsComparable(), "Lambdas that capture data cannot be checked via function pointer. Use a xiiEventSubscriptionID instead.");

  XII_LOCK(m_Mutex);

  for (xiiUInt32 i = 0; i < m_EventHandlers.GetCount(); ++i)
  {
    if (m_EventHandlers[i].m_Handler.IsEqualIfComparable(handler))
      return true;
  }

  return false;
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::Clear()
{
  XII_LOCK(m_Mutex);

  m_EventHandlers.Clear();
}

template <typename EventData, typename MutexType, xiiEventType EventType>
bool xiiEventBase<EventData, MutexType, EventType>::IsEmpty() const
{
  XII_LOCK(m_Mutex);

  return m_EventHandlers.IsEmpty();
}

/// The notification is sent to all event handlers in the order that they were registered.
template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::Broadcast(EventData eventData, xiiUInt8 uiMaxRecursionDepth)
{
  if constexpr (EventType == xiiEventType::Default)
  {
    XII_LOCK(m_Mutex);

    XII_ASSERT_DEV(m_uiRecursionDepth <= uiMaxRecursionDepth, "The event has been triggered recursively or from several threads simultaneously.");

    if (m_uiRecursionDepth > uiMaxRecursionDepth)
      return;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    XII_ASSERT_ALWAYS(m_pSelf != nullptr, "This xiiEvent is broadcasted before it was initialized.");
    XII_ASSERT_ALWAYS(m_pSelf == this, "The xiiEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif

    ++m_uiRecursionDepth;

    // RAII to ensure correctness in case exceptions are used
    auto scopeExit = xiiMakeScopeExit([&]() {
      --m_uiRecursionDepth;
    });

    // don't execute handlers that are added while we are broadcasting
    xiiUInt32 uiMaxHandlers = m_EventHandlers.GetCount();

    for (xiiUInt32 ui = 0; ui < uiMaxHandlers;)
    {
      if (m_EventHandlers[ui].m_Handler.IsValid())
      {
        m_EventHandlers[ui].m_Handler(eventData);
        ++ui;
      }
      else
      {
        m_EventHandlers.RemoveAtAndCopy(ui);
        --uiMaxHandlers;
      }
    }
  }
  else
  {
    xiiHybridArray<HandlerData, 16> eventHandlers;
    {
      XII_LOCK(m_Mutex);

      if constexpr (RecursionDepthSupported)
      {
        XII_ASSERT_DEV(m_uiRecursionDepth <= uiMaxRecursionDepth, "The event has been triggered recursively or from several threads simultaneously.");

        if (m_uiRecursionDepth > uiMaxRecursionDepth)
          return;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
        XII_ASSERT_ALWAYS(m_pSelf == this, "The xiiEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif

        ++m_uiRecursionDepth;
      }
      else
      {
        XII_ASSERT_DEV(uiMaxRecursionDepth == 255, "uiMaxRecursionDepth is not supported if xiiEventType::CopyOnBroadcast is used and the event needs to be threadsafe.");
      }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      XII_ASSERT_ALWAYS(m_pSelf == this, "The xiiEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif

      eventHandlers = m_EventHandlers;
    }

    // RAII to ensure correctness in case exceptions are used
    auto scopeExit = xiiMakeScopeExit([&]() {
    // Bug in MSVC 2017. Can't use if constexpr.
#if XII_ENABLED(XII_COMPILER_MSVC) && _MSC_VER < 1920
      if (RecursionDepthSupported)
      {
        --m_uiRecursionDepth;
      }
#else
      if constexpr (RecursionDepthSupported)
      {
        --m_uiRecursionDepth;
      }
#endif
    });

    const xiiUInt32 uiHandlerCount = eventHandlers.GetCount();
    for (xiiUInt32 ui = 0; ui < uiHandlerCount; ++ui)
    {
      eventHandlers[ui].m_Handler(eventData);
    }
  }
}


template <typename EventData, typename MutexType, typename AllocatorWrapper, xiiEventType EventType>
xiiEvent<EventData, MutexType, AllocatorWrapper, EventType>::xiiEvent() :
  xiiEventBase<EventData, MutexType, EventType>(AllocatorWrapper::GetAllocator())
{
}

template <typename EventData, typename MutexType, typename AllocatorWrapper, xiiEventType EventType>
xiiEvent<EventData, MutexType, AllocatorWrapper, EventType>::xiiEvent(xiiAllocator* pAllocator) :
  xiiEventBase<EventData, MutexType, EventType>(pAllocator)
{
}
