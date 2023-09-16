
#include <Foundation/SimdMath/SimdRandom.h>

XII_ALWAYS_INLINE xiiUInt32 xiiIntervalSchedulerBase::GetHistogramIndex(xiiTime value)
{
  if (value.IsZero())
    return 0;

  constexpr xiiUInt32 maxSlotIndex = HistogramSize - 1;
  const double        x            = xiiMath::Max((value - m_MinInterval).GetSeconds() * m_fInvIntervalRange, 0.0);
  const double        i            = xiiMath::Sqrt(x) * (maxSlotIndex - 1) + 1;
  return xiiMath::Min(static_cast<xiiUInt32>(i), maxSlotIndex);
}

XII_ALWAYS_INLINE xiiTime xiiIntervalSchedulerBase::GetHistogramSlotValue(xiiUInt32 uiIndex)
{
  if (uiIndex == 0)
    return xiiTime::Zero();

  constexpr double norm = 1.0 / (HistogramSize - 2.0);
  const double     x    = (uiIndex - 1) * norm;
  return (x * x) * (m_MaxInterval - m_MinInterval) + m_MinInterval;
}

// static
XII_ALWAYS_INLINE float xiiIntervalSchedulerBase::GetRandomZeroToOne(int pos, xiiUInt32& seed)
{
  return xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(pos), xiiSimdVec4u(seed++)).x();
}

constexpr xiiTime s_JitterRange = xiiTime::Microseconds(10);

// static
XII_ALWAYS_INLINE xiiTime xiiIntervalSchedulerBase::GetRandomTimeJitter(int pos, xiiUInt32& seed)
{
  const float x = xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(pos), xiiSimdVec4u(seed++)).x();
  return s_JitterRange * (x * 2.0f - 1.0f);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
bool xiiIntervalScheduler<T>::Data::IsValid() const
{
  return m_Interval.IsZeroOrPositive();
}

template <typename T>
void xiiIntervalScheduler<T>::Data::MarkAsInvalid()
{
  m_Interval = xiiTime::Seconds(-1);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
void xiiIntervalScheduler<T>::AddOrUpdateWork(const T& work, xiiTime interval)
{
  typename DataMap::Iterator it;
  if (m_WorkIdToData.TryGetValue(work, it))
  {
    auto&   data        = it.Value();
    xiiTime oldInterval = data.m_Interval;
    if (interval == oldInterval)
      return;

    data.MarkAsInvalid();

    const xiiUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
    m_Histogram[uiHistogramIndex]--;
  }

  Data data;
  data.m_Work              = work;
  data.m_Interval          = xiiMath::Max(interval, xiiTime::Zero());
  data.m_DueTime           = m_CurrentTime + GetRandomZeroToOne(m_Data.GetCount(), m_uiSeed) * data.m_Interval;
  data.m_LastScheduledTime = m_CurrentTime;

  m_WorkIdToData[work] = InsertData(data);

  const xiiUInt32 uiHistogramIndex = GetHistogramIndex(data.m_Interval);
  m_Histogram[uiHistogramIndex]++;
}

template <typename T>
void xiiIntervalScheduler<T>::RemoveWork(const T& work)
{
  typename DataMap::Iterator it;
  if (m_WorkIdToData.Remove(work, &it))
  {
    auto&   data        = it.Value();
    xiiTime oldInterval = data.m_Interval;
    data.MarkAsInvalid();

    const xiiUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
    m_Histogram[uiHistogramIndex]--;
  }
}

template <typename T>
xiiTime xiiIntervalScheduler<T>::GetInterval(const T& work) const
{
  typename DataMap::Iterator it;
  XII_VERIFY(m_WorkIdToData.TryGetValue(work, it), "Entry not found");
  return it.Value().m_Interval;
}

template <typename T>
void xiiIntervalScheduler<T>::Update(xiiTime deltaTime, RunWorkCallback runWorkCallback)
{
  if (deltaTime <= xiiTime::Zero())
    return;

  m_CurrentTime += deltaTime;

  if (m_Data.IsEmpty())
  {
    m_fNumWorkToSchedule = 0.0;
  }
  else
  {
    double fNumWork = 0;
    for (xiiUInt32 i = 1; i < HistogramSize; ++i)
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
    const xiiUInt32 uiScheduleCount = xiiMath::Min(static_cast<xiiUInt32>(m_fNumWorkToSchedule) + extra + m_Histogram[0], m_Data.GetCount());

    // schedule work
    {
      auto RunWork = [&](typename DataMap::Iterator it, xiiUInt32 uiIndex) {
        auto& data = it.Value();
        if (data.IsValid())
        {
          if (runWorkCallback.IsValid())
          {
            runWorkCallback(data.m_Work, m_CurrentTime - data.m_LastScheduledTime);
          }

          // add a little bit of random jitter so we don't end up with perfect timings that might collide with other work
          data.m_DueTime           = m_CurrentTime + data.m_Interval + GetRandomTimeJitter(uiIndex, m_uiSeed);
          data.m_LastScheduledTime = m_CurrentTime;
        }

        m_ScheduledWork.PushBack(it);
      };

      auto it = m_Data.GetIterator();
      for (xiiUInt32 i = 0; i < uiScheduleCount; ++i, ++it)
      {
        RunWork(it, i);
      }

      // check if the next works have a zero interval if so execute them as well to fulfill the every frame guarantee
      xiiUInt32 uiNumExtras = 0;
      while (it.IsValid())
      {
        auto& data = it.Value();
        if (data.m_Interval.IsPositive())
          break;

        RunWork(it, uiNumExtras);

        ++uiNumExtras;
        ++it;
      }
    }

    // re-sort
    for (auto& it : m_ScheduledWork)
    {
      if (it.Value().IsValid())
      {
        // make a copy of data and re-insert at new due time
        Data data                   = it.Value();
        m_WorkIdToData[data.m_Work] = InsertData(data);
      }

      m_Data.Remove(it);
    }
    m_ScheduledWork.Clear();
  }
}

template <typename T>
XII_FORCE_INLINE typename xiiIntervalScheduler<T>::DataMap::Iterator xiiIntervalScheduler<T>::InsertData(Data& data)
{
  // make sure that we have a unique due time since the map can't store multiple keys with the same value
  int pos = 0;
  while (m_Data.Contains(data.m_DueTime))
  {
    data.m_DueTime += GetRandomTimeJitter(pos++, m_uiSeed);
  }

  return m_Data.Insert(data.m_DueTime, data);
}
