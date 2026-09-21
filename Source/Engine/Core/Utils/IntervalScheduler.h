/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct XII_CORE_DLL xiiUpdateRate
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    EveryFrame,
    Max30fps,
    Max20fps,
    Max10fps,
    Max5fps,
    Max2fps,
    Max1fps,
    Never,

    Default = Max30fps
  };

  static xiiTime GetInterval(Enum updateRate);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiUpdateRate);

//////////////////////////////////////////////////////////////////////////

/// Helper class to schedule work in intervals typically larger than the duration of one frame
///
/// Tries to maintain an even workload per frame and also keep the given interval for a work as best as possible.
/// A typical use case would be e.g. component update functions that don't need to be called every frame.
class XII_CORE_DLL xiiIntervalSchedulerBase
{
protected:
  xiiIntervalSchedulerBase(xiiTime minInterval, xiiTime maxInterval);
  ~xiiIntervalSchedulerBase();

  xiiUInt32 GetHistogramIndex(xiiTime value);
  xiiTime   GetHistogramSlotValue(xiiUInt32 uiIndex);

  static float   GetRandomZeroToOne(int pos, xiiUInt32& seed);
  static xiiTime GetRandomTimeJitter(int pos, xiiUInt32& seed);

  xiiTime m_MinInterval;
  xiiTime m_MaxInterval;
  double  m_fInvIntervalRange;

  xiiTime m_CurrentTime;

  xiiUInt32 m_uiSeed = 0;

  static constexpr xiiUInt32 HistogramSize                        = 32;
  xiiUInt32                  m_Histogram[HistogramSize]           = {};
  xiiTime                    m_HistogramSlotValues[HistogramSize] = {};
};

//////////////////////////////////////////////////////////////////////////

/// \see xiiIntervalSchedulerBase
template <typename T>
class xiiIntervalScheduler : public xiiIntervalSchedulerBase
{
  using SUPER = xiiIntervalSchedulerBase;

public:
  XII_ALWAYS_INLINE xiiIntervalScheduler(xiiTime minInterval = xiiTime::MakeFromMilliseconds(1), xiiTime maxInterval = xiiTime::MakeFromSeconds(1)) :
    SUPER(minInterval, maxInterval)
  {
  }

  void AddOrUpdateWork(const T& work, xiiTime interval);
  void RemoveWork(const T& work);

  xiiTime GetInterval(const T& work) const;

  // reference to the work that should be run and time passed since this work has been last run.
  using RunWorkCallback = xiiDelegate<void(const T&, xiiTime)>;

  /// Advances the scheduler by deltaTime and triggers runWorkCallback for each work that should be run during this update step.
  /// Since it is not possible to maintain the exact interval all the time the actual delta time for the work is also passed to runWorkCallback.
  void Update(xiiTime deltaTime, RunWorkCallback runWorkCallback);

  void Clear();

private:
  struct Data
  {
    T       m_Work;
    xiiTime m_Interval;
    xiiTime m_DueTime;
    xiiTime m_LastScheduledTime;

    bool IsValid() const;
    void MarkAsInvalid();
  };

  using DataMap = xiiMap<xiiTime, Data>;
  DataMap                                     m_Data;
  xiiHashTable<T, typename DataMap::Iterator> m_WorkIdToData;

  typename DataMap::Iterator                  InsertData(Data& data);
  xiiDynamicArray<typename DataMap::Iterator> m_ScheduledWork;
};

#include <Core/Utils/Implementation/IntervalScheduler_inl.h>
