/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/Delegate.h>

#include <atomic>
#include <type_traits>

/// Identifies an event subscription. Zero is always an invalid subscription ID.
using xiiEventSubscriptionID = xiiUInt32;

/// Specifies the type of xiiEvent implementation to use
enum class xiiEventType
{
  Default,        ///< Invokes registered handlers directly.
  CopyOnBroadcast ///< Invokes a snapshot of the registered handlers.
};

/// This class propagates event information to registered event handlers.
///
/// An event can be anything that "happens" that might be of interest to other code, such
/// that it can react on it in some way.
/// Just create an instance of xiiEvent and call Broadcast() on it. Other interested code needs const access to
/// the event variable to be able to call AddEventHandler() and RemoveEventHandler().
/// To pass information to the handlers, create a custom struct with event information
/// and then pass a (const) reference to that data through Broadcast().
///
/// Broadcast is synchronous and invokes handlers in registration order. The default event skips handlers
/// removed before their turn. New handlers are visible to subsequent broadcasts, including allowed recursive
/// broadcasts. Register handlers on a default event with xiiNoMutex outside its callbacks.
///
/// A default event with a mutex serializes dispatch and registration. Its callbacks run under the event mutex
/// and must avoid lock-order inversions. Use xiiCopyOnBroadcastEvent when callbacks need to run outside that lock.
/// A snapshot owns copies of its delegates, so it requires copyable handlers. Removing a handler from the event
/// leaves any already-captured snapshots intact. Drain those broadcasts before destroying a receiver or unloading code.
///
/// Destruction of the event must be synchronized with dispatch, registration and access through Unsubscriber.
/// An event using xiiNoMutex also requires external synchronization for concurrent access.
///
/// \note A class holding a xiiEvent member needs to provide public access to the member for external code to
/// be able to register as an event handler. To make it possible to prevent external code from also raising events,
/// all functions that are needed for listening are const, and all others are non-const.
/// Therefore, simply make event members private and provide const reference access through a public getter.
template <typename EventData, typename MutexType, xiiEventType EventType>
class xiiEventBase
{
protected:
  /// Constructor.
  xiiEventBase(xiiAllocator* pAllocator);
  ~xiiEventBase();

public:
  /// Notification callback type for events.
  using Handler = xiiDelegate<void(EventData)>;

  /// An object that can be passed to xiiEvent::AddEventHandler to store the subscription information
  /// and automatically remove the event handler upon destruction.
  class Unsubscriber
  {
    XII_DISALLOW_COPY_AND_ASSIGN(Unsubscriber);

  public:
    Unsubscriber() = default;
    Unsubscriber(Unsubscriber&& other);
    ~Unsubscriber() { Unsubscribe(); }

    void operator=(Unsubscriber&& other);

    /// Removes the subscription, if any. Serialize operations on the same token.
    void Unsubscribe();

    /// Returns whether the registration is still active. Removal, clearing and event destruction invalidate it.
    bool IsSubscribed() const { return m_pEvent.load(std::memory_order_acquire) != nullptr; }

  private:
    friend class xiiEventBase<EventData, MutexType, EventType>;

    std::atomic<const xiiEventBase*> m_pEvent{nullptr};
  };

  /// Implementation specific constants.
  enum
  {
    /// Whether the uiMaxRecursionDepth parameter to Broadcast() is supported in this implementation or not.
    RecursionDepthSupported = EventType == xiiEventType::Default || std::is_same_v < MutexType,
    xiiNoMutex >,

    /// Default value for the maximum recursion depth of Broadcast.
    /// As limiting the recursion depth is not supported when EventType == xiiEventType::CopyOnBroadcast and MutexType != xiiNoMutex
    /// the default value for that case is the maximum.
    MaxRecursionDepthDefault = RecursionDepthSupported ? 0 : 255
  };

  /// This function will broadcast to all registered users, that this event has just happened.
  ///  Setting uiMaxRecursionDepth will allow you to permit recursions. When broadcasting consider up to what depth
  ///  you want recursions to be permitted. By default no recursion is allowed.
  void Broadcast(EventData pEventData, xiiUInt8 uiMaxRecursionDepth = MaxRecursionDepthDefault); // [tested]

  /// Adds a function as an event handler. All handlers will be notified in the order that they were registered.
  ///
  /// The return value can be stored and used to remove the event handler later again.
  xiiEventSubscriptionID AddEventHandler(Handler handler) const; // [tested]

  /// An overload that adds an event handler and initializes the given \a Unsubscriber object.
  ///
  /// When the Unsubscriber is destroyed, it will automatically remove the event handler.
  void AddEventHandler(Handler handler, Unsubscriber& ref_unsubscriber) const; // [tested]

  /// Removes a previously registered handler. It is an error to remove a handler that was not registered.
  void RemoveEventHandler(const Handler& handler) const; // [tested]

  /// Removes a previously registered handler via the returned subscription ID.
  ///
  /// The ID will be reset to zero.
  /// If this is called with a zero ID, nothing happens.
  void RemoveEventHandler(xiiEventSubscriptionID& ref_id) const;

  /// Checks whether an event handler has already been registered.
  bool HasEventHandler(const Handler& handler) const;

  /// Removes all registered event handlers.
  void Clear();

  /// Returns true, if no event handlers are registered.
  bool IsEmpty() const;

  XII_DISALLOW_COPY_AND_ASSIGN(xiiEventBase);

private:
  xiiEventSubscriptionID AddEventHandler(Handler&& handler, Unsubscriber* pUnsubscriber) const;
  void                   TransferSubscription(Unsubscriber& ref_source, Unsubscriber& ref_destination) const;
  void                   RemoveSubscription(Unsubscriber& ref_unsubscriber) const;
  void                   RemoveHandlerAt(xiiUInt32 uiIndex) const;
  void                   RemoveInactiveHandlers();

  xiiUInt16                      m_uiRecursionDepth     = 0;
  mutable bool                   m_bHasInactiveHandlers = false;
  mutable xiiEventSubscriptionID m_uiNextSubscriptionID = 0;

  mutable MutexType m_Mutex;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const void* m_pSelf = nullptr;
#endif

  struct HandlerData
  {
    Handler                m_Handler;
    xiiEventSubscriptionID m_uiSubscriptionID = 0;
    Unsubscriber*          m_pUnsubscriber    = nullptr;
  };

  // A reentrant registration must leave the currently executing delegate at a stable address.
  using HandlerStorage = std::conditional_t<EventType == xiiEventType::Default && !std::is_same_v<MutexType, xiiNoMutex>, xiiDeque<HandlerData>, xiiDynamicArray<HandlerData>>;
  mutable HandlerStorage m_EventHandlers;
};

/// Can be used when xiiEvent is used without any additional data
struct xiiNoEventData
{
};

/// \see xiiEventBase
template <typename EventData, typename MutexType = xiiNoMutex, typename AllocatorWrapper = xiiDefaultAllocatorWrapper, xiiEventType EventType = xiiEventType::Default>
class xiiEvent : public xiiEventBase<EventData, MutexType, EventType>
{
public:
  xiiEvent();
  xiiEvent(xiiAllocator* pAllocator);
};

template <typename EventData, typename MutexType = xiiNoMutex, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
using xiiCopyOnBroadcastEvent = xiiEvent<EventData, MutexType, AllocatorWrapper, xiiEventType::CopyOnBroadcast>;

#include <Foundation/Events/Implementation/Event_inl.h>
