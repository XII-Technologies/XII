/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/EnumerableClass.h>

/// A class to broadcast and handle global (system-wide) events.
///
/// A global event is an event that will be sent to all instances of xiiGlobalEvent (or rather their
/// respective handler functions), without the need to first register these event-handlers anywhere.
/// Thus they are very useful to notify sub-systems of certain important events, such as that some kind of
/// initialization will be done shortly, which means they can react by preparing properly.
/// For example the xiiStartup-class will send certain events before doing startup and shutdown steps, which
/// allows code to free resources before a sub-system might be shut down.
/// xiiGlobalEvent's should be used when there is a kind of event that should be propagated throughout the entire
/// engine, without knowledge which systems might want to know about it. These systems can then use an
/// xiiGlobalEvent-instance to hook themselves into the global-event pipeline and react accordingly.
/// Global events should mostly be used for startup / configuration / shutdown procedures.
/// Also one should never assume any specific order of execution, all event handlers should be completely independent
/// from each other.
///
/// To create a global event handler, simply add this code inside a cpp file:
///
/// XII_ON_GLOBAL_EVENT(EventName)
/// {
///   ... do something ...
/// }
///
/// You can also use XII_ON_GLOBAL_EVENT_ONCE, if the handler should only be executed the first time the event is sent.
/// This is more efficient than filtering out duplicate events inside the event handler.
class XII_FOUNDATION_DLL xiiGlobalEvent : public xiiEnumerable<xiiGlobalEvent>
{
  XII_DECLARE_ENUMERABLE_CLASS(xiiGlobalEvent);

public:
  struct XII_FOUNDATION_DLL EventData
  {
    EventData();

    xiiUInt32 m_uiNumTimesFired;
    xiiUInt16 m_uiNumEventHandlersRegular;
    xiiUInt16 m_uiNumEventHandlersOnce;
  };

  using EventMap = xiiMap<xiiString, EventData>;

public:
  /// [internal] Use the macro XII_ON_GLOBAL_EVENT or XII_ON_GLOBAL_EVENT_ONCE to create an event handler.
  using XII_GLOBAL_EVENT_HANDLER = void (*)(const xiiVariant&, const xiiVariant&, const xiiVariant&, const xiiVariant&);

  /// [internal] Use the macro XII_ON_GLOBAL_EVENT or XII_ON_GLOBAL_EVENT_ONCE to create an event handler.
  xiiGlobalEvent(xiiStringView sEventName, XII_GLOBAL_EVENT_HANDLER eventHandler, bool bOnlyOnce); // [tested]

  /// This function will broadcast a system wide event to all event handlers that are registered to handle this specific type of event.
  ///
  /// The string specifies the event type, the parameters are optional and can be used to send additional event specific data.
  static void Broadcast(xiiStringView sEventName, xiiVariant param0 = xiiVariant(), xiiVariant param1 = xiiVariant(), xiiVariant param2 = xiiVariant(), xiiVariant param3 = xiiVariant()); // [tested]

  /// This function will output (via xiiLog) some statistics about which events are used and how often.
  ///
  /// This allows to figure out which events are used throughout the engine and which events might be fired too often.
  static void PrintGlobalEventStatistics(); // [tested]

  /// Updates all global event statistics.
  static void UpdateGlobalEventStatistics();

  /// Returns the map that holds the current statistics about the global events.
  static const EventMap& GetEventStatistics() { return s_KnownEvents; }

private:
  bool                     m_bOnlyOnce;
  bool                     m_bHasBeenFired;
  xiiStringView            m_sEventName;
  XII_GLOBAL_EVENT_HANDLER m_EventHandler;

  static EventMap s_KnownEvents;
};

/// Use this macro to broadcast an event. Pass 0 to 4 parameters of type aeGlobalEventParam to it.
#define XII_BROADCAST_EVENT(name, ...) xiiGlobalEvent::Broadcast(#name, ##__VA_ARGS__);

/// Use this macro to handle an event every time it is broadcast (place function code in curly brackets after it)
#define XII_ON_GLOBAL_EVENT(name)                                                                                                                    \
  static void           EventHandler_##name(const xiiVariant& param0, const xiiVariant& param1, const xiiVariant& param2, const xiiVariant& param3); \
  static xiiGlobalEvent s_EventHandler_##name(#name, EventHandler_##name, false);                                                                    \
  static void           EventHandler_##name(const xiiVariant& param0, const xiiVariant& param1, const xiiVariant& param2, const xiiVariant& param3)

/// Use this macro to handle an event only once (place function code in curly brackets after it)
#define XII_ON_GLOBAL_EVENT_ONCE(name)                                                                                                               \
  static void           EventHandler_##name(const xiiVariant& param0, const xiiVariant& param1, const xiiVariant& param2, const xiiVariant& param3); \
  static xiiGlobalEvent s_EventHandler_##name(#name, EventHandler_##name, true);                                                                     \
  static void           EventHandler_##name(const xiiVariant& param0, const xiiVariant& param1, const xiiVariant& param2, const xiiVariant& param3)
