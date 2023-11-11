#include <Core/CorePCH.h>

#include <Core/Utils/IntervalScheduler.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiUpdateRate, 1)
  XII_ENUM_CONSTANTS(xiiUpdateRate::EveryFrame)
  XII_ENUM_CONSTANTS(xiiUpdateRate::Max30fps, xiiUpdateRate::Max20fps, xiiUpdateRate::Max10fps)
  XII_ENUM_CONSTANTS(xiiUpdateRate::Max5fps, xiiUpdateRate::Max2fps, xiiUpdateRate::Max1fps)
  XII_ENUM_CONSTANTS(xiiUpdateRate::Never)
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

xiiIntervalSchedulerBase::xiiIntervalSchedulerBase(xiiTime minInterval, xiiTime maxInterval) :
  m_MinInterval(minInterval), m_MaxInterval(maxInterval)
{
  XII_ASSERT_DEV(m_MinInterval.IsPositive(), "Min interval must be greater than zero.");
  XII_ASSERT_DEV(m_MaxInterval > m_MinInterval, "Max interval must be greater than min interval.");

  m_fInvIntervalRange = 1.0 / (m_MaxInterval - m_MinInterval).GetSeconds();

  for (xiiUInt32 i = 0; i < HistogramSize; ++i)
  {
    m_HistogramSlotValues[i] = GetHistogramSlotValue(i);
  }
}

xiiIntervalSchedulerBase::~xiiIntervalSchedulerBase() = default;

XII_STATICLINK_FILE(Core, Core_Utils_Implementation_IntervalScheduler);
