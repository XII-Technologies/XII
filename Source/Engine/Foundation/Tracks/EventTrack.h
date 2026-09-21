/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Time/Time.h>

/// An event track is a time line that contains named events.
///
/// The time line can be sampled to query all events that occurred during a time period.
/// There is no way to sample an event track at a fixed point in time, because events occur at specific time points and thus
/// only range queries make sense.
class XII_FOUNDATION_DLL xiiEventTrack
{
public:
  xiiEventTrack();
  ~xiiEventTrack();

  /// Removes all control points.
  void Clear();

  /// Checks whether there are any control points in the track.
  bool IsEmpty() const;

  /// Adds a named event into the track at the given time.
  void AddControlPoint(xiiTime time, xiiStringView sEvent);

  /// Samples the event track from range [start; end) and adds all events that occured in that time period to the array.
  ///
  /// Note that the range is inclusive for the start time, and exclusive for the end time.
  ///
  /// If rangeStart is larger than rangeEnd, the events are returned in reverse order (backwards traversal).
  void Sample(xiiTime rangeStart, xiiTime rangeEnd, xiiDynamicArray<xiiHashedString>& out_events) const;

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);

private:
  struct ControlPoint
  {
    XII_ALWAYS_INLINE bool operator<(const ControlPoint& rhs) const { return m_Time < rhs.m_Time; }

    xiiTime   m_Time;
    xiiUInt32 m_uiEvent;
  };

  xiiUInt32 FindControlPointAfter(xiiTime x) const;
  xiiInt32  FindControlPointBefore(xiiTime x) const;

  mutable bool                          m_bSort = false;
  mutable xiiDynamicArray<ControlPoint> m_ControlPoints;
  xiiHybridArray<xiiHashedString, 4>    m_Events;
};
