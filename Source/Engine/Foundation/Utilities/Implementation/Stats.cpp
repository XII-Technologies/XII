/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/Stats.h>

xiiMutex                xiiStats::s_Mutex;
xiiStats::MapType       xiiStats::s_Stats;
xiiStats::xiiEventStats xiiStats::s_StatsEvents;

xiiUInt32 xiiStats::Capture(MapType& out_stats, xiiUInt32 uiMaximumEntries)
{
  XII_LOCK(s_Mutex);
  out_stats.Clear();
  for (auto it = s_Stats.GetIterator(); it.IsValid() && out_stats.GetCount() < uiMaximumEntries; ++it)
    out_stats.Insert(it.Key(), it.Value());
  return s_Stats.GetCount() - out_stats.GetCount();
}

void xiiStats::RemoveStat(xiiStringView sStatName)
{
  XII_LOCK(s_Mutex);

  MapType::Iterator it = s_Stats.Find(sStatName);

  if (!it.IsValid())
    return;

  s_Stats.Remove(it);

  StatsEventData e;
  e.m_EventType = StatsEventData::Remove;
  e.m_sStatName = sStatName;

  s_StatsEvents.Broadcast(e);
}

void xiiStats::SetStat(xiiStringView sStatName, const xiiVariant& value)
{
  XII_LOCK(s_Mutex);

  bool bExisted = false;
  auto it       = s_Stats.FindOrAdd(sStatName, &bExisted);

  if (it.Value() == value)
    return;

  it.Value() = value;

  StatsEventData e;
  e.m_EventType    = bExisted ? StatsEventData::Set : StatsEventData::Add;
  e.m_sStatName    = sStatName;
  e.m_NewStatValue = value;

  s_StatsEvents.Broadcast(e);
}
