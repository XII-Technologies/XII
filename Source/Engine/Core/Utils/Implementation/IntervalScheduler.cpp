#include <Core/CorePCH.h>

#include <Core/Utils/IntervalScheduler.h>
#include <Foundation/SimdMath/SimdRandom.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiUpdateRate, 1)
  XII_ENUM_CONSTANTS(xiiUpdateRate::EveryFrame)
  XII_ENUM_CONSTANTS(xiiUpdateRate::Max30fps, xiiUpdateRate::Max20fps, xiiUpdateRate::Max10fps)
  XII_ENUM_CONSTANTS(xiiUpdateRate::Max5fps, xiiUpdateRate::Max2fps, xiiUpdateRate::Max1fps)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

static xiiTime s_Intervals[] = {
  xiiTime::Zero(),              // EveryFrame
  xiiTime::Seconds(1.0 / 30.0), // Max30fps
  xiiTime::Seconds(1.0 / 20.0), // Max20fps
  xiiTime::Seconds(1.0 / 10.0), // Max10fps
  xiiTime::Seconds(1.0 / 5.0),  // Max5fps
  xiiTime::Seconds(1.0 / 2.0),  // Max2fps
  xiiTime::Seconds(1.0 / 1.0),  // Max1fps
};

static_assert(XII_ARRAY_SIZE(s_Intervals) == xiiUpdateRate::Max1fps + 1);

xiiTime xiiUpdateRate::GetInterval(Enum updateRate)
{
  return s_Intervals[updateRate];
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE float GetRandomZeroToOne(int pos, xiiUInt32& seed)
{
  return xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(pos), xiiSimdVec4u(seed++)).x();
}

constexpr xiiTime s_JitterRange = xiiTime::Microseconds(10);

XII_ALWAYS_INLINE xiiTime GetRandomTimeJitter(int pos, xiiUInt32& seed)
{
  const float x = xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(pos), xiiSimdVec4u(seed++)).x();
  return s_JitterRange * (x * 2.0f - 1.0f);
}

xiiIntervalSchedulerBase::xiiIntervalSchedulerBase(xiiTime minInterval, xiiTime maxInterval) :
  m_MinInterval(minInterval), m_MaxInterval(maxInterval)
{
  m_fInvIntervalRange = 1.0 / (m_MaxInterval - m_MinInterval).GetSeconds();

  for (xiiUInt32 i = 0; i < HistogramSize; ++i)
  {
    m_HistogramSlotValues[i] = GetHistogramSlotValue(i);
  }
}

xiiIntervalSchedulerBase::~xiiIntervalSchedulerBase() = default;

void xiiIntervalSchedulerBase::AddOrUpdateWork(xiiUInt64 workId, xiiTime interval)
{
  DataMap::Iterator it;
  if (m_WorkIdToData.TryGetValue(workId, it))
  {
    xiiTime oldInterval = it.Value().m_Interval;
    if (interval == oldInterval)
      return;

    m_Data.Remove(it);

    const xiiUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
    m_Histogram[uiHistogramIndex]--;
  }

  Data data;
  data.m_WorkId            = workId;
  data.m_Interval          = xiiMath::Max(interval, xiiTime::Zero());
  data.m_DueTime           = m_CurrentTime + GetRandomZeroToOne(m_Data.GetCount(), m_uiSeed) * data.m_Interval;
  data.m_LastScheduledTime = m_CurrentTime;

  m_WorkIdToData[workId] = InsertData(data);

  const xiiUInt32 uiHistogramIndex = GetHistogramIndex(data.m_Interval);
  m_Histogram[uiHistogramIndex]++;
}

void xiiIntervalSchedulerBase::RemoveWork(xiiUInt64 workId)
{
  DataMap::Iterator it;
  XII_VERIFY(m_WorkIdToData.Remove(workId, &it), "Entry not found");

  xiiTime oldInterval = it.Value().m_Interval;
  m_Data.Remove(it);

  const xiiUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
  m_Histogram[uiHistogramIndex]--;
}

xiiTime xiiIntervalSchedulerBase::GetInterval(xiiUInt64 workId) const
{
  DataMap::Iterator it;
  XII_VERIFY(m_WorkIdToData.TryGetValue(workId, it), "Entry not found");
  return it.Value().m_Interval;
}

void xiiIntervalSchedulerBase::Update(xiiTime deltaTime, xiiDelegate<void(xiiUInt64, xiiTime)> runWorkCallback)
{
  if (deltaTime <= xiiTime::Zero())
    return;

  if (m_Data.IsEmpty())
  {
    m_fNumWorkToSchedule = 0.0;
  }
  else
  {
    double fNumWork = 0;
    for (xiiUInt32 i = 0; i < HistogramSize; ++i)
    {
      fNumWork += (1.0 / xiiMath::Max(m_HistogramSlotValues[i], deltaTime).GetSeconds()) * m_Histogram[i];
    }
    fNumWork *= deltaTime.GetSeconds();

    if (m_fNumWorkToSchedule == 0.0)
    {
      m_fNumWorkToSchedule = fNumWork;
    }
    else
    {
      // running average of num work per update to prevent huge spikes
      m_fNumWorkToSchedule = xiiMath::Lerp<double>(m_fNumWorkToSchedule, fNumWork, 0.05);
    }

    const float     fRemainder      = static_cast<float>(xiiMath::Fraction(m_fNumWorkToSchedule));
    const int       pos             = static_cast<int>(m_CurrentTime.GetNanoseconds());
    const xiiUInt32 extra           = GetRandomZeroToOne(pos, m_uiSeed) < fRemainder ? 1 : 0;
    const xiiUInt32 uiScheduleCount = xiiMath::Min(static_cast<xiiUInt32>(m_fNumWorkToSchedule) + extra, m_Data.GetCount());

    // schedule work
    {
      auto it = m_Data.GetIterator();
      for (xiiUInt32 i = 0; i < uiScheduleCount; ++i, ++it)
      {
        auto& data = it.Value();
        runWorkCallback(data.m_WorkId, m_CurrentTime - data.m_LastScheduledTime);

        // add a little bit of random jitter so we don't end up with perfect timings that might collide with other work
        data.m_DueTime           = m_CurrentTime + xiiMath::Max(data.m_Interval, deltaTime) + GetRandomTimeJitter(i, m_uiSeed);
        data.m_LastScheduledTime = m_CurrentTime;

        m_ScheduledWork.PushBack(it);
      }
    }

    // re-sort
    for (auto& it : m_ScheduledWork)
    {
      Data data                     = it.Value();
      m_WorkIdToData[data.m_WorkId] = InsertData(data);
      m_Data.Remove(it);
    }
    m_ScheduledWork.Clear();
  }

  m_CurrentTime += deltaTime;
}

xiiUInt32 xiiIntervalSchedulerBase::GetHistogramIndex(xiiTime value)
{
  constexpr xiiUInt32 maxSlotIndex = HistogramSize - 1;
  const double        x            = xiiMath::Max((value - m_MinInterval).GetSeconds() * m_fInvIntervalRange, 0.0);
  const double        i            = xiiMath::Sqrt(x) * maxSlotIndex;
  return xiiMath::Min(static_cast<xiiUInt32>(i), maxSlotIndex);
}

xiiTime xiiIntervalSchedulerBase::GetHistogramSlotValue(xiiUInt32 uiIndex)
{
  constexpr double norm = 1.0 / (HistogramSize - 1.0);
  const double     x    = uiIndex * norm;
  return (x * x) * (m_MaxInterval - m_MinInterval) + m_MinInterval;
}

xiiIntervalSchedulerBase::DataMap::Iterator xiiIntervalSchedulerBase::InsertData(Data& data)
{
  // make sure that we have a unique due time since the map can't store multiple keys with the same value
  int pos = 0;
  while (m_Data.Contains(data.m_DueTime))
  {
    data.m_DueTime += GetRandomTimeJitter(pos++, m_uiSeed);
  }

  return m_Data.Insert(data.m_DueTime, data);
}
