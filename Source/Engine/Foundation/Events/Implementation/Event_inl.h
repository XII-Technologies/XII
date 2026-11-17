/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Types/ScopeExit.h>

#include <utility>

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
  XII_ASSERT_ALWAYS(m_pSelf == this, "An event cannot be relocated.");
#endif
  XII_ASSERT_DEV(m_uiRecursionDepth == 0, "An event cannot be destroyed during broadcast.");
  Clear();
}

template <typename EventData, typename MutexType, xiiEventType EventType>
xiiEventBase<EventData, MutexType, EventType>::Unsubscriber::Unsubscriber(Unsubscriber&& other)
{
  if (const auto* pEvent = other.m_pEvent.load(std::memory_order_acquire))
  {
    pEvent->TransferSubscription(other, *this);
  }
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::Unsubscriber::operator=(Unsubscriber&& other)
{
  if (this == &other)
  {
    return;
  }

  Unsubscribe();
  if (const auto* pEvent = other.m_pEvent.load(std::memory_order_acquire))
  {
    pEvent->TransferSubscription(other, *this);
  }
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::Unsubscriber::Unsubscribe()
{
  if (const auto* pEvent = m_pEvent.load(std::memory_order_acquire))
  {
    pEvent->RemoveSubscription(*this);
  }
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::TransferSubscription(Unsubscriber& ref_source, Unsubscriber& ref_destination) const
{
  XII_LOCK(m_Mutex);

  for (auto& entry : m_EventHandlers)
  {
    if (entry.m_pUnsubscriber == &ref_source)
    {
      entry.m_pUnsubscriber = &ref_destination;
      ref_destination.m_pEvent.store(this, std::memory_order_release);
      ref_source.m_pEvent.store(nullptr, std::memory_order_release);
      return;
    }
  }
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::RemoveSubscription(Unsubscriber& ref_unsubscriber) const
{
  XII_LOCK(m_Mutex);

  for (xiiUInt32 uiIndex = 0; uiIndex < m_EventHandlers.GetCount(); ++uiIndex)
  {
    if (m_EventHandlers[uiIndex].m_pUnsubscriber == &ref_unsubscriber)
    {
      RemoveHandlerAt(uiIndex);
      return;
    }
  }
}

template <typename EventData, typename MutexType, xiiEventType EventType>
xiiEventSubscriptionID xiiEventBase<EventData, MutexType, EventType>::AddEventHandler(Handler handler) const
{
  return AddEventHandler(std::move(handler), nullptr);
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::AddEventHandler(Handler handler, Unsubscriber& ref_unsubscriber) const
{
  // The previous subscription may use a different mutex.
  ref_unsubscriber.Unsubscribe();
  AddEventHandler(std::move(handler), &ref_unsubscriber);
}

template <typename EventData, typename MutexType, xiiEventType EventType>
xiiEventSubscriptionID xiiEventBase<EventData, MutexType, EventType>::AddEventHandler(Handler&& handler, Unsubscriber* pUnsubscriber) const
{
  XII_LOCK(m_Mutex);

  if constexpr (EventType == xiiEventType::Default && std::is_same_v<MutexType, xiiNoMutex>)
  {
    if (m_uiRecursionDepth != 0)
    {
      XII_ASSERT_DEV(false, "Register handlers outside a default event's broadcast or use xiiCopyOnBroadcastEvent.");
      return 0;
    }
  }

  if (!handler.IsValid())
  {
    XII_ASSERT_DEV(false, "An event handler must be bound before registration.");
    return 0;
  }

  XII_ASSERT_DEV(!handler.IsComparable() || !HasEventHandler(handler), "The same event handler cannot be added twice.");

  if (++m_uiNextSubscriptionID == 0)
  {
    ++m_uiNextSubscriptionID;
  }

  auto& entry              = m_EventHandlers.ExpandAndGetRef();
  entry.m_Handler          = std::move(handler);
  entry.m_uiSubscriptionID = m_uiNextSubscriptionID;
  entry.m_pUnsubscriber    = pUnsubscriber;

  if (pUnsubscriber != nullptr)
  {
    pUnsubscriber->m_pEvent.store(this, std::memory_order_release);
  }

  return entry.m_uiSubscriptionID;
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::RemoveHandlerAt(xiiUInt32 uiIndex) const
{
  auto& entry = m_EventHandlers[uiIndex];
  if (entry.m_pUnsubscriber != nullptr)
  {
    entry.m_pUnsubscriber->m_pEvent.store(nullptr, std::memory_order_release);
    entry.m_pUnsubscriber = nullptr;
  }

  if constexpr (EventType == xiiEventType::Default)
  {
    if (m_uiRecursionDepth != 0)
    {
      // Keep the callable alive while it may still be on the stack.
      entry.m_uiSubscriptionID = 0;
      m_bHasInactiveHandlers   = true;
      return;
    }
  }

  for (xiiUInt32 uiNext = uiIndex + 1; uiNext < m_EventHandlers.GetCount(); ++uiNext)
  {
    m_EventHandlers[uiNext - 1] = std::move(m_EventHandlers[uiNext]);
  }

  m_EventHandlers.PopBack();
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::RemoveInactiveHandlers()
{
  xiiUInt32 uiActiveCount = 0;
  for (xiiUInt32 uiIndex = 0; uiIndex < m_EventHandlers.GetCount(); ++uiIndex)
  {
    if (m_EventHandlers[uiIndex].m_uiSubscriptionID != 0)
    {
      if (uiActiveCount != uiIndex)
      {
        m_EventHandlers[uiActiveCount] = std::move(m_EventHandlers[uiIndex]);
      }

      ++uiActiveCount;
    }
  }

  m_EventHandlers.SetCount(uiActiveCount);
  m_bHasInactiveHandlers = false;
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::RemoveEventHandler(const Handler& handler) const
{
  XII_ASSERT_DEV(handler.IsComparable(), "Lambdas that capture data cannot be removed via function pointer. Use a xiiEventSubscriptionID instead.");
  XII_LOCK(m_Mutex);

  for (xiiUInt32 uiIndex = 0; uiIndex < m_EventHandlers.GetCount(); ++uiIndex)
  {
    const auto& entry = m_EventHandlers[uiIndex];
    if (entry.m_uiSubscriptionID != 0 && entry.m_Handler.IsEqualIfComparable(handler))
    {
      RemoveHandlerAt(uiIndex);
      return;
    }
  }

  XII_ASSERT_DEV(false, "xiiEvent::RemoveEventHandler: Handler has not been registered or already been unregistered.");
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::RemoveEventHandler(xiiEventSubscriptionID& ref_id) const
{
  if (ref_id == 0)
  {
    return;
  }

  const xiiEventSubscriptionID uiSubscriptionID = ref_id;
  ref_id                                        = 0;
  XII_LOCK(m_Mutex);

  for (xiiUInt32 uiIndex = 0; uiIndex < m_EventHandlers.GetCount(); ++uiIndex)
  {
    if (m_EventHandlers[uiIndex].m_uiSubscriptionID == uiSubscriptionID)
    {
      RemoveHandlerAt(uiIndex);
      return;
    }
  }

  XII_ASSERT_DEV(false, "xiiEvent::RemoveEventHandler: Invalid subscription ID '{0}'.", uiSubscriptionID);
}

template <typename EventData, typename MutexType, xiiEventType EventType>
bool xiiEventBase<EventData, MutexType, EventType>::HasEventHandler(const Handler& handler) const
{
  XII_ASSERT_DEV(handler.IsComparable(), "Lambdas that capture data cannot be checked via function pointer.");
  XII_LOCK(m_Mutex);

  for (const auto& entry : m_EventHandlers)
  {
    if (entry.m_uiSubscriptionID != 0 && entry.m_Handler.IsEqualIfComparable(handler))
    {
      return true;
    }
  }

  return false;
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::Clear()
{
  XII_LOCK(m_Mutex);

  if constexpr (EventType == xiiEventType::Default)
  {
    if (m_uiRecursionDepth != 0)
    {
      for (xiiUInt32 uiIndex = 0; uiIndex < m_EventHandlers.GetCount(); ++uiIndex)
      {
        RemoveHandlerAt(uiIndex);
      }

      return;
    }
  }

  for (auto& entry : m_EventHandlers)
  {
    if (entry.m_pUnsubscriber != nullptr)
    {
      entry.m_pUnsubscriber->m_pEvent.store(nullptr, std::memory_order_release);
    }
  }

  m_EventHandlers.Clear();
}

template <typename EventData, typename MutexType, xiiEventType EventType>
bool xiiEventBase<EventData, MutexType, EventType>::IsEmpty() const
{
  XII_LOCK(m_Mutex);

  if constexpr (EventType == xiiEventType::Default)
  {
    if (m_bHasInactiveHandlers)
    {
      for (const auto& entry : m_EventHandlers)
      {
        if (entry.m_uiSubscriptionID != 0)
        {
          return false;
        }
      }

      return true;
    }
  }

  return m_EventHandlers.IsEmpty();
}

template <typename EventData, typename MutexType, xiiEventType EventType>
void xiiEventBase<EventData, MutexType, EventType>::Broadcast(EventData eventData, xiiUInt8 uiMaxRecursionDepth)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_ALWAYS(m_pSelf == this, "The event must be initialized and remain at its original address.");
#endif

  if constexpr (EventType == xiiEventType::Default)
  {
    XII_LOCK(m_Mutex);

    XII_ASSERT_DEV(m_uiRecursionDepth <= uiMaxRecursionDepth, "The event has been triggered recursively or from several threads simultaneously.");
    if (m_uiRecursionDepth > uiMaxRecursionDepth)
    {
      return;
    }

    ++m_uiRecursionDepth;
    auto scopeExit = xiiMakeScopeExit([this]() {
      if (--m_uiRecursionDepth == 0 && m_bHasInactiveHandlers)
      {
        RemoveInactiveHandlers();
      }
    });

    const xiiUInt32 uiHandlerCount = m_EventHandlers.GetCount();
    for (xiiUInt32 uiIndex = 0; uiIndex < uiHandlerCount; ++uiIndex)
    {
      const auto& entry = m_EventHandlers[uiIndex];
      if (entry.m_uiSubscriptionID != 0)
      {
        entry.m_Handler(eventData);
      }
    }
  }
  else
  {
    xiiHybridArray<Handler, 16> handlers;
    {
      XII_LOCK(m_Mutex);

      if constexpr (RecursionDepthSupported)
      {
        XII_ASSERT_DEV(m_uiRecursionDepth <= uiMaxRecursionDepth, "The event has been triggered recursively or from several threads simultaneously.");
        if (m_uiRecursionDepth > uiMaxRecursionDepth)
        {
          return;
        }

        ++m_uiRecursionDepth;
      }
      else
      {
        XII_ASSERT_DEV(uiMaxRecursionDepth == MaxRecursionDepthDefault, "Recursion limits are unavailable for thread-safe snapshot events.");
      }

      handlers.Reserve(m_EventHandlers.GetCount());
      for (const auto& entry : m_EventHandlers)
      {
        handlers.PushBack(entry.m_Handler);
      }
    }

    auto scopeExit = xiiMakeScopeExit([this]() {
      if constexpr (RecursionDepthSupported)
      {
        --m_uiRecursionDepth;
      }
    });

    for (const Handler& handler : handlers)
    {
      handler(eventData);
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
