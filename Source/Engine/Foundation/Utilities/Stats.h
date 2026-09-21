/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>

/// This class holds a simple map that maps strings (keys) to strings (values), which represent certain stats.
///
/// This can be used by a game to store (and continuously update) information about the internal game state. Other tools can then
/// display this information in a convenient manner. For example the stats can be shown on screen. The data is also transmitted through
/// xiiTelemetry, and the xiiInspector tool will display the information.
class XII_FOUNDATION_DLL xiiStats
{
public:
  using MapType = xiiMap<xiiString, xiiVariant>;

  /// Removes the stat with the given name.
  ///
  /// This will also send a 'remove' message through xiiTelemetry, such that external tools can remove it from their list.
  static void RemoveStat(xiiStringView sStatName);

  /// Sets the value of the given stat, adds it if it did not exist before.
  ///
  /// sStatName may contain slashes (but not backslashes) to define groups and subgroups, which can be used by tools such as xiiInspector to display the stats in a hierarchical way.
  /// This function will also send the name and value of the stat through xiiTelemetry, such that tools like xiiInspector will show the changed value.
  static void SetStat(xiiStringView sStatName, const xiiVariant& value);

  /// Returns the value of the given stat. Returns an invalid xiiVariant, if the stat did not exist before.
  static const xiiVariant& GetStat(xiiStringView sStatName) { return s_Stats[sStatName]; }

  /// Returns the entire map of stats, can be used to display them.
  static const MapType& GetAllStats() { return s_Stats; }

  /// The event data that is broadcast whenever a stat is changed.
  struct StatsEventData
  {
    /// Which type of event this is.
    enum EventType
    {
      Add,   ///< A variable has been set for the first time.
      Set,   ///< A variable has been changed.
      Remove ///< A variable that existed has been removed.
    };

    EventType     m_EventType;
    xiiStringView m_sStatName;
    xiiVariant    m_NewStatValue;
  };

  using xiiEventStats = xiiEvent<const StatsEventData&, xiiMutex>;

  /// Adds an event handler that is called every time a stat is changed.
  static void AddEventHandler(xiiEventStats::Handler handler) { s_StatsEvents.AddEventHandler(handler); }

  /// Removes a previously added event handler.
  static void RemoveEventHandler(xiiEventStats::Handler handler) { s_StatsEvents.RemoveEventHandler(handler); }

private:
  static xiiMutex      s_Mutex;
  static MapType       s_Stats;
  static xiiEventStats s_StatsEvents;
};
