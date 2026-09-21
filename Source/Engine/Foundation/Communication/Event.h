/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/Delegate.h>

/// Identifies an event subscription. Zero is always an invalid subscription ID.
using xiiEventSubscriptionID = xiiUInt32;

/// Specifies the type of xiiEvent implementation to use
enum class xiiEventType
{
  Default,        /// Default implementation. Does not support modifying the event while broadcasting.
  CopyOnBroadcast /// CopyOnBroadcast implementation. Supports modifying the event while broadcasting.
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
/// If you need to modify the event while broadcasting, for example inside one of the registered event handlers,
/// set EventType = xiiEventType::CopyOnBroadcast. Each broadcast will then copy the event handler array before signaling them, allowing
/// modifications during broadcasting.
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
    Unsubscriber(Unsubscriber&& other)
    {
      m_pEvent         = other.m_pEvent;
      m_SubscriptionID = other.m_SubscriptionID;
      other.Clear();
    }
    ~Unsubscriber() { Unsubscribe(); }

    void operator=(Unsubscriber&& other)
    {
      Unsubscribe();

      m_pEvent         = other.m_pEvent;
      m_SubscriptionID = other.m_SubscriptionID;
      other.Clear();
    }

    /// If the unsubscriber holds a valid subscription, it will be removed from the target xiiEvent.
    void Unsubscribe()
    {
      if (m_SubscriptionID == 0)
        return;

      m_pEvent->RemoveEventHandler(m_SubscriptionID);
      Clear();
    }

    /// Checks whether this unsubscriber has a valid subscription.
    bool IsSubscribed() const { return m_SubscriptionID != 0; }

    /// Resets the unsubscriber. Use when the target xiiEvent may have been destroyed and automatic unsubscription cannot be executed
    /// anymore.
    void Clear()
    {
      m_pEvent         = nullptr;
      m_SubscriptionID = 0;
    }

  private:
    friend class xiiEventBase<EventData, MutexType, EventType>;

    const xiiEventBase<EventData, MutexType, EventType>* m_pEvent         = nullptr;
    xiiEventSubscriptionID                               m_SubscriptionID = 0;
  };

  /// Implementation specific constants.
  enum
  {
    /// Whether the uiMaxRecursionDepth parameter to Broadcast() is supported in this implementation or not.
    RecursionDepthSupported = (EventType == xiiEventType::Default || xiiConversionTest<MutexType, xiiNoMutex>::sameType == 1) ? 1 : 0,

    /// Default value for the maximum recursion depth of Broadcast.
    /// As limiting the recursion depth is not supported when EventType == xiiEventType::CopyAndBroadcast and MutexType != xiiNoMutex
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

  // it would be a problem if the xiiEvent moves in memory, for instance the Unsubscriber's would point to invalid memory
  XII_DISALLOW_COPY_AND_ASSIGN(xiiEventBase);

private:
  // Used to detect recursive broadcasts and then throw asserts at you.
  xiiUInt8                       m_uiRecursionDepth   = 0;
  mutable xiiEventSubscriptionID m_NextSubscriptionID = 0;

  mutable MutexType m_Mutex;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const void* m_pSelf = nullptr;
#endif

  struct HandlerData
  {
    Handler                m_Handler;
    xiiEventSubscriptionID m_SubscriptionID;
  };

  /// A dynamic array allows to have zero overhead as long as no event handlers are registered.
  mutable xiiDynamicArray<HandlerData> m_EventHandlers;
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

#include <Foundation/Communication/Implementation/Event_inl.h>
