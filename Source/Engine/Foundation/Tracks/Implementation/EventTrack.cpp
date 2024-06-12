#include <Foundation/FoundationPCH.h>

#include <Foundation/Tracks/EventTrack.h>

xiiEventTrack::xiiEventTrack() = default;

xiiEventTrack::~xiiEventTrack() = default;

void xiiEventTrack::Clear()
{
  m_Events.Clear();
  m_ControlPoints.Clear();
}

bool xiiEventTrack::IsEmpty() const
{
  return m_ControlPoints.IsEmpty();
}

void xiiEventTrack::AddControlPoint(xiiTime time, xiiStringView sEvent)
{
  m_bSort = true;

  const xiiUInt32 uiNumEvents = m_Events.GetCount();

  auto& cp  = m_ControlPoints.ExpandAndGetRef();
  cp.m_Time = time;

  // search for existing event
  {
    xiiTempHashedString tmp(sEvent);

    for (xiiUInt32 i = 0; i < uiNumEvents; ++i)
    {
      if (m_Events[i] == tmp)
      {
        cp.m_uiEvent = i;
        return;
      }
    }
  }

  // not found -> add event name
  {
    cp.m_uiEvent = uiNumEvents;

    xiiHashedString hs;
    hs.Assign(sEvent);

    m_Events.PushBack(hs);
  }
}

xiiUInt32 xiiEventTrack::FindControlPointAfter(xiiTime x) const
{
  // searches for a control point after OR AT x

  XII_ASSERT_DEBUG(!m_ControlPoints.IsEmpty(), "");

  xiiUInt32 uiLowIdx  = 0;
  xiiUInt32 uiHighIdx = m_ControlPoints.GetCount() - 1;

  // do a binary search to reduce the search space
  while (uiHighIdx - uiLowIdx > 8)
  {
    const xiiUInt32 uiMidIdx = uiLowIdx + ((uiHighIdx - uiLowIdx) >> 1); // lerp

    if (m_ControlPoints[uiMidIdx].m_Time >= x)
      uiHighIdx = uiMidIdx;
    else
      uiLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (xiiUInt32 idx = uiLowIdx; idx <= uiHighIdx; ++idx)
  {
    if (m_ControlPoints[idx].m_Time >= x)
    {
      return idx;
    }
  }

  XII_ASSERT_DEBUG(uiHighIdx + 1 == m_ControlPoints.GetCount(), "Unexpected event track entry index");
  return m_ControlPoints.GetCount();
}

xiiInt32 xiiEventTrack::FindControlPointBefore(xiiTime x) const
{
  // searches for a control point before OR AT x

  XII_ASSERT_DEBUG(!m_ControlPoints.IsEmpty(), "");

  xiiInt32 iLowIdx  = 0;
  xiiInt32 iHighIdx = (xiiInt32)m_ControlPoints.GetCount() - 1;

  // do a binary search to reduce the search space
  while (iHighIdx - iLowIdx > 8)
  {
    const xiiInt32 uiMidIdx = iLowIdx + ((iHighIdx - iLowIdx) >> 1); // lerp

    if (m_ControlPoints[uiMidIdx].m_Time >= x)
      iHighIdx = uiMidIdx;
    else
      iLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (xiiInt32 idx = iHighIdx; idx >= iLowIdx; --idx)
  {
    if (m_ControlPoints[idx].m_Time <= x)
    {
      return idx;
    }
  }

  XII_ASSERT_DEBUG(iLowIdx == 0, "Unexpected event track entry index");
  return -1;
}

void xiiEventTrack::Sample(xiiTime rangeStart, xiiTime rangeEnd, xiiDynamicArray<xiiHashedString>& out_events) const
{
  if (m_ControlPoints.IsEmpty())
    return;

  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  if (rangeStart <= rangeEnd)
  {
    xiiUInt32 curCpIdx = FindControlPointAfter(rangeStart);

    const xiiUInt32 uiNumCPs = m_ControlPoints.GetCount();
    while (curCpIdx < uiNumCPs && m_ControlPoints[curCpIdx].m_Time < rangeEnd)
    {
      const xiiHashedString& sEvent = m_Events[m_ControlPoints[curCpIdx].m_uiEvent];

      out_events.PushBack(sEvent);

      ++curCpIdx;
    }
  }
  else
  {
    xiiInt32 curCpIdx = FindControlPointBefore(rangeStart);

    while (curCpIdx >= 0 && m_ControlPoints[curCpIdx].m_Time > rangeEnd)
    {
      const xiiHashedString& sEvent = m_Events[m_ControlPoints[curCpIdx].m_uiEvent];

      out_events.PushBack(sEvent);

      --curCpIdx;
    }
  }
}

void xiiEventTrack::Save(xiiStreamWriter& ref_stream) const
{
  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  xiiUInt8 uiVersion = 1;
  ref_stream << uiVersion;

  ref_stream << m_Events.GetCount();
  for (const xiiHashedString& name : m_Events)
  {
    ref_stream << name.GetString();
  }

  ref_stream << m_ControlPoints.GetCount();
  for (const ControlPoint& cp : m_ControlPoints)
  {
    ref_stream << cp.m_Time;
    ref_stream << cp.m_uiEvent;
  }
}

void xiiEventTrack::Load(xiiStreamReader& ref_stream)
{
  // don't rely on the data being sorted
  m_bSort = true;

  xiiUInt8 uiVersion = 0;
  ref_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion == 1, "Invalid event track version {0}", uiVersion);

  xiiUInt32        count = 0;
  xiiStringBuilder tmp;

  ref_stream >> count;
  m_Events.SetCount(count);
  for (xiiHashedString& name : m_Events)
  {
    ref_stream >> tmp;
    name.Assign(tmp);
  }

  ref_stream >> count;
  m_ControlPoints.SetCount(count);
  for (ControlPoint& cp : m_ControlPoints)
  {
    ref_stream >> cp.m_Time;
    ref_stream >> cp.m_uiEvent;
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Tracks_Implementation_EventTrack);
