#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/Stats.h>

xiiMutex                xiiStats::s_Mutex;
xiiStats::MapType       xiiStats::s_Stats;
xiiStats::xiiEventStats xiiStats::s_StatsEvents;

void xiiStats::RemoveStat(const char* szStatName)
{
  XII_LOCK(s_Mutex);

  MapType::Iterator it = s_Stats.Find(szStatName);

  if (!it.IsValid())
    return;

  s_Stats.Remove(it);

  StatsEventData e;
  e.m_EventType  = StatsEventData::Remove;
  e.m_szStatName = szStatName;

  s_StatsEvents.Broadcast(e);
}

void xiiStats::SetStat(const char* szStatName, const xiiVariant& value)
{
  XII_LOCK(s_Mutex);

  bool bExisted = false;
  auto it       = s_Stats.FindOrAdd(szStatName, &bExisted);

  if (it.Value() == value)
    return;

  it.Value() = value;

  StatsEventData e;
  e.m_EventType    = bExisted ? StatsEventData::Set : StatsEventData::Add;
  e.m_szStatName   = szStatName;
  e.m_NewStatValue = value;

  s_StatsEvents.Broadcast(e);
}


XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Stats);
